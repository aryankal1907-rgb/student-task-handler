// FocusMate web server: exposes the existing C++ data-structure backend
// over HTTP and serves the frontend/ folder. Run from the project folder.
#include "httplib.h"
#include <iostream>
#include <sstream>
#include <mutex>
#include <set>
#include <queue>
#include <ctime>
#include "../tasks/task.h"
#include "../link list/linkedlist.h"
#include "../stack/stack.h"
#include "../queue/queue.h"
#include "../avl/avl.h"
#include "../graphs/graph.h"
using namespace std;

void apiRefresh();
bool apiAddTask(const string&, const string&, const string&, const string&, int&, string&);
bool apiUpdateTask(int, const string&, const string&, const string&, const string&, string&);
bool apiCompleteTask(int, string&);
bool apiDeleteTasks(const vector<int>&, string&);
bool apiRestoreTasks(const vector<int>&, string&);
int hashFindTaskIndex(int id);
bool avlRangeIds(const string&, const string&, vector<int>&);
const map<string, set<int>>& graphData();
vector<pair<string, Task>> workflowHistory();
vector<int> workflowOrder();
int undoCount(); int redoCount(); string undoLabel(); string redoLabel();
string priorityName(int);

static mutex gLock;

static string esc(const string& s) {
    string o;
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': break;
            case '\t': o += "\\t"; break;
            default: if (c < 0x20) o += ' '; else o += (char)c;
        }
    }
    return o;
}
static string q(const string& s) { return "\"" + esc(s) + "\""; }

static string taskJson(const Task& t) {
    int days = daysRemaining(t.deadline);
    ostringstream o;
    o << "{\"id\":" << t.id << ",\"type\":" << q(t.type) << ",\"subject\":" << q(t.subject)
      << ",\"name\":" << q(t.name) << ",\"deadline\":" << q(t.deadline)
      << ",\"priority\":" << t.priority << ",\"priorityName\":" << q(priorityName(t.priority))
      << ",\"completed\":" << (t.completed ? "true" : "false") << ",\"daysLeft\":" << days << "}";
    return o.str();
}
static string idsJson(const vector<int>& v) {
    string s = "[";
    for (size_t i = 0; i < v.size(); i++) s += (i ? "," : "") + to_string(v[i]);
    return s + "]";
}
static vector<int> parseIds(const string& s) {
    vector<int> ids; stringstream ss(s); string part;
    while (getline(ss, part, ',')) { try { if (!part.empty()) ids.push_back(stoi(part)); } catch (...) {} }
    return ids;
}
static int toInt(const string& s, int def = -1) { try { return stoi(s); } catch (...) { return def; } }
static const Task* findActive(int id) {
    for (int i = 0; i < taskCount; i++) if (tasks[i].id == id) return &tasks[i];
    return nullptr;
}

static string stateJson() {
    apiRefresh();
    ostringstream o;
    time_t now = time(nullptr); char buf[16]; strftime(buf, sizeof buf, "%d-%m-%Y", localtime(&now));
    int pending = 0, done = 0, high = 0, week = 0;
    o << "{\"today\":" << q(buf) << ",\"tasks\":[";
    for (int i = 0; i < taskCount; i++) {
        o << (i ? "," : "") << taskJson(tasks[i]);
        if (tasks[i].completed) done++; else {
            pending++;
            if (tasks[i].priority == 1) high++;
            int d = daysRemaining(tasks[i].deadline); if (d >= 0 && d <= 7) week++;
        }
    }
    o << "],\"queue\":" << idsJson(getQueueSnapshot()) << ",\"deleted\":[";
    for (size_t i = 0; i < deletedTasks.size(); i++) o << (i ? "," : "") << taskJson(deletedTasks[i]);
    o << "],\"stats\":{\"total\":" << taskCount << ",\"pending\":" << pending << ",\"completed\":" << done
      << ",\"high\":" << high << ",\"dueThisWeek\":" << week << ",\"deleted\":" << deletedTasks.size() << "}";
    o << ",\"undo\":{\"count\":" << undoCount() << ",\"label\":" << q(undoLabel()) << "}"
      << ",\"redo\":{\"count\":" << redoCount() << ",\"label\":" << q(redoLabel()) << "}";
    o << ",\"subjects\":{";
    bool first = true;
    for (const auto& s : graphData()) {
        if (s.second.empty()) continue;
        o << (first ? "" : ",") << q(s.first) << ":" << idsJson(vector<int>(s.second.begin(), s.second.end()));
        first = false;
    }
    o << "},\"workflow\":" << idsJson(workflowOrder()) << ",\"history\":[";
    auto h = workflowHistory();
    size_t from = h.size() > 60 ? h.size() - 60 : 0;
    for (size_t i = from; i < h.size(); i++)
        o << (i > from ? "," : "") << "{\"action\":" << q(h[i].first) << ",\"id\":" << h[i].second.id
          << ",\"name\":" << q(h[i].second.name) << ",\"subject\":" << q(h[i].second.subject) << "}";
    o << "]}";
    return o.str();
}

// Runs fn with console output captured (the original modules print to cout).
template <class F>
static void handle(httplib::Response& res, F fn) {
    lock_guard<mutex> g(gLock);
    ostringstream captured; auto* old = cout.rdbuf(captured.rdbuf());
    string err; bool ok = true;
    try { ok = fn(err); } catch (const exception& e) { ok = false; err = e.what(); }
    cout.rdbuf(old);
    if (!ok) { res.status = 400; res.set_content("{\"ok\":false,\"error\":" + q(err) + "}", "application/json"); return; }
    res.set_content("{\"ok\":true,\"state\":" + stateJson() + "}", "application/json");
}

int main() {
    initializeTasks(); initializeWorkflow(); initializeAVL(); initializeGraph();
    httplib::Server svr;
    svr.set_mount_point("/", "./frontend");

    svr.Get("/api/state", [](const httplib::Request&, httplib::Response& res) {
        lock_guard<mutex> g(gLock);
        ostringstream cap; auto* old = cout.rdbuf(cap.rdbuf());
        string s = stateJson(); cout.rdbuf(old);
        res.set_content("{\"ok\":true,\"state\":" + s + "}", "application/json");
    });
    svr.Post("/api/tasks", [](const httplib::Request& r, httplib::Response& res) {
        handle(res, [&](string& err) { int id;
            return apiAddTask(r.get_param_value("type"), r.get_param_value("subject"),
                              r.get_param_value("name"), r.get_param_value("deadline"), id, err); });
    });
    svr.Post("/api/tasks/update", [](const httplib::Request& r, httplib::Response& res) {
        handle(res, [&](string& err) {
            return apiUpdateTask(toInt(r.get_param_value("id")), r.get_param_value("type"), r.get_param_value("subject"),
                                 r.get_param_value("name"), r.get_param_value("deadline"), err); });
    });
    svr.Post("/api/tasks/complete", [](const httplib::Request& r, httplib::Response& res) {
        handle(res, [&](string& err) { return apiCompleteTask(toInt(r.get_param_value("id")), err); });
    });
    svr.Post("/api/tasks/delete", [](const httplib::Request& r, httplib::Response& res) {
        handle(res, [&](string& err) { return apiDeleteTasks(parseIds(r.get_param_value("ids")), err); });
    });
    svr.Post("/api/tasks/restore", [](const httplib::Request& r, httplib::Response& res) {
        handle(res, [&](string& err) { return apiRestoreTasks(parseIds(r.get_param_value("ids")), err); });
    });
    svr.Post("/api/undo", [](const httplib::Request& r, httplib::Response& res) {
        handle(res, [&](string& err) {
            int n = toInt(r.get_param_value("n"), 1);
            if (!canUndo()) { err = "Nothing to undo."; return false; }
            if (n < 1 || n > undoCount()) { err = "Invalid number of steps."; return false; }
            undoMultiple(n); return true; });
    });
    svr.Post("/api/redo", [](const httplib::Request& r, httplib::Response& res) {
        handle(res, [&](string& err) {
            int n = toInt(r.get_param_value("n"), 1);
            if (!canRedo()) { err = "Nothing to redo."; return false; }
            if (n < 1 || n > redoCount()) { err = "Invalid number of steps."; return false; }
            redoMultiple(n); return true; });
    });
    // Hash-table lookup by ID
    svr.Get("/api/search", [](const httplib::Request& r, httplib::Response& res) {
        lock_guard<mutex> g(gLock);
        int i = hashFindTaskIndex(toInt(r.get_param_value("id"), 0));
        if (i < 0) { res.status = 404; res.set_content("{\"ok\":false,\"error\":\"Task ID not found.\"}", "application/json"); return; }
        res.set_content("{\"ok\":true,\"task\":" + taskJson(tasks[i]) + "}", "application/json");
    });
    // AVL-tree deadline range query
    svr.Get("/api/range", [](const httplib::Request& r, httplib::Response& res) {
        lock_guard<mutex> g(gLock);
        vector<int> ids;
        if (!avlRangeIds(r.get_param_value("start"), r.get_param_value("end"), ids)) {
            res.status = 400; res.set_content("{\"ok\":false,\"error\":\"Enter a valid start and end date (start must not be after end).\"}", "application/json"); return; }
        string s = "{\"ok\":true,\"tasks\":[";
        int best = -1;
        for (size_t k = 0; k < ids.size(); k++) {
            const Task* t = findActive(ids[k]); if (!t) continue;
            s += (k ? "," : "") + taskJson(*t);
            if (!t->completed && (best < 0 || t->priority < findActive(best)->priority)) best = t->id;
        }
        res.set_content(s + "],\"highestPriorityId\":" + to_string(best) + "}", "application/json");
    });
    // Graph BFS / DFS starting from a subject
    svr.Get("/api/graph", [](const httplib::Request& r, httplib::Response& res) {
        lock_guard<mutex> g(gLock);
        string start = r.get_param_value("start"), mode = r.get_param_value("mode");
        auto graph = graphData();
        if (!graph.count(start)) { res.status = 404; res.set_content("{\"ok\":false,\"error\":\"Subject not found.\"}", "application/json"); return; }
        vector<string> order; set<string> seen;
        auto neighbours = [&](const string& v) {
            vector<string> n;
            if (v.compare(0, 2, "S:") == 0) { for (int id : graph[v.substr(2)]) n.push_back("T:" + to_string(id)); }
            else { int id = stoi(v.substr(2)); for (auto& s : graph) if (s.second.count(id)) n.push_back("S:" + s.first); }
            return n; };
        if (mode == "dfs") {
            auto dfs = [&](auto&& self, const string& v) -> void {
                if (!seen.insert(v).second) return; order.push_back(v);
                for (auto& n : neighbours(v)) self(self, n); };
            dfs(dfs, "S:" + start);
        } else {
            queue<string> qq; qq.push("S:" + start); seen.insert("S:" + start);
            while (!qq.empty()) { string v = qq.front(); qq.pop(); order.push_back(v);
                for (auto& n : neighbours(v)) if (seen.insert(n).second) qq.push(n); }
        }
        string s = "{\"ok\":true,\"order\":[";
        for (size_t i = 0; i < order.size(); i++) {
            bool sub = order[i][0] == 'S';
            s += string(i ? "," : "") + "{\"kind\":" + (sub ? "\"subject\"" : "\"task\"") + ",\"value\":" + q(order[i].substr(2));
            if (!sub) { const Task* t = findActive(stoi(order[i].substr(2))); s += ",\"name\":" + q(t ? t->name : ""); }
            s += "}";
        }
        res.set_content(s + "]}", "application/json");
    });

    cout << "\n  FocusMate is running ->  http://localhost:8080\n  (press Ctrl+C to stop)\n" << endl;
    if (!svr.listen("127.0.0.1", 8080)) { cerr << "Could not start server on port 8080 (already in use?)\n"; return 1; }
    return 0;
}