
#include "graph.h"
#include "../tasks/task.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <limits>
using namespace std;

static map<string, set<int>> graph;
static const string GRAPH_FILE = "graph.txt";

void saveGraph() {
    ofstream file(GRAPH_FILE);

    for (const auto& subject : graph) {
        file << quoted(subject.first);

        for (int id : subject.second)
            file << ' ' << id;

        file << '\n';
    }
}

void initializeGraph() {
    graph.clear();

    ifstream file(GRAPH_FILE);
    string line;

    while (getline(file, line)) {
        istringstream input(line);
        string subject;
        input >> quoted(subject);

        if (subject.empty())
            continue;

        int id;

        while (input >> id) {
            for (int i = 0; i < taskCount; i++) {
                if (tasks[i].id == id) {
                    graph[subject].insert(id);
                    break;
                }
            }
        }

        graph[subject];
    }
}

static bool validTaskId(int id) {
    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].id == id)
            return true;
    }

    return false;
}

static string taskDescription(int id) {
    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].id == id)
            return tasks[i].name;
    }

    return "Task not found";
}

void addSubject() {
    string subject;

    cout << "Enter subject name: ";
    getline(cin >> ws, subject);

    if (subject.empty()) {
        cout << "Subject name cannot be empty.\n";
        return;
    }

    if (graph.count(subject)) {
        cout << "Subject already exists.\n";
        return;
    }

    graph[subject];
    saveGraph();

    cout << "Subject added successfully.\n";
}

void connectTasksToSubject() {
    if (graph.empty()) {
        cout << "Add a subject first.\n";
        return;
    }

    cout << "\nSubjects:\n";

    int index = 1;
    vector<string> subjects;

    for (const auto& item : graph) {
        cout << index++ << ". " << item.first << '\n';
        subjects.push_back(item.first);
    }

    int choice;
    cout << "Select subject: ";

    if (!(cin >> choice)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input.\n";
        return;
    }

    if (choice < 1 || choice > static_cast<int>(subjects.size())) {
        cout << "Invalid subject choice.\n";
        return;
    }

    string subject = subjects[choice - 1];

    int count;
    cout << "How many Task IDs do you want to connect? ";

    if (!(cin >> count)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input.\n";
        return;
    }

    if (count < 1 || count > taskCount) {
        cout << "Invalid number of tasks.\n";
        return;
    }

    vector<int> ids;

    cout << "Enter Task IDs: ";

    for (int i = 0; i < count; i++) {
        int id;

        if (!(cin >> id)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid Task ID.\n";
            return;
        }

        if (!validTaskId(id)) {
            cout << "Task ID " << id << " does not exist.\n";
            return;
        }

        if (find(ids.begin(), ids.end(), id) != ids.end()) {
            cout << "Duplicate Task ID entered.\n";
            return;
        }

        ids.push_back(id);
    }

    for (int id : ids)
        graph[subject].insert(id);

    saveGraph();
    cout << "Task connections added successfully.\n";
}

void removeTaskFromSubject() {
    if (graph.empty()) {
        cout << "No subjects available.\n";
        return;
    }

    string subject;

    cout << "Enter subject name: ";
    getline(cin >> ws, subject);

    auto it = graph.find(subject);

    if (it == graph.end()) {
        cout << "Subject not found.\n";
        return;
    }

    int count;
    cout << "How many connections do you want to remove? ";

    if (!(cin >> count)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input.\n";
        return;
    }

    if (count < 1 || count > static_cast<int>(it->second.size())) {
        cout << "Invalid number of connections.\n";
        return;
    }

    vector<int> ids;

    cout << "Enter Task IDs: ";

    for (int i = 0; i < count; i++) {
        int id;

        if (!(cin >> id)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input.\n";
            return;
        }

        if (!it->second.count(id)) {
            cout << "Task ID " << id
                 << " is not connected to this subject.\n";
            return;
        }

        if (find(ids.begin(), ids.end(), id) != ids.end()) {
            cout << "Duplicate Task ID entered.\n";
            return;
        }

        ids.push_back(id);
    }

    for (int id : ids)
        it->second.erase(id);

    saveGraph();
    cout << "Connections removed successfully.\n";
}

void displaySubjectTasks() {
    string subject;

    cout << "Enter subject name: ";
    getline(cin >> ws, subject);

    auto it = graph.find(subject);

    if (it == graph.end()) {
        cout << "Subject not found.\n";
        return;
    }

    cout << "\nSubject: " << subject << '\n';

    if (it->second.empty()) {
        cout << "No tasks connected to this subject.\n";
        return;
    }

    for (int id : it->second)
        cout << "Task ID: " << id
             << " | " << taskDescription(id) << '\n';
}

void displayGraph() {
    if (graph.empty()) {
        cout << "Graph is empty. Add subjects first.\n";
        return;
    }

    cout << "\n========== SUBJECT-TASK GRAPH ==========\n";

    for (const auto& subject : graph) {
        cout << "\n" << subject.first << " -> ";

        if (subject.second.empty()) {
            cout << "No tasks";
        } else {
            for (int id : subject.second)
                cout << "[ID " << id << ": "
                     << taskDescription(id) << "] ";
        }

        cout << '\n';
    }
}

void bfsTraversal() {
    string start;

    cout << "Enter starting subject: ";
    getline(cin >> ws, start);

    if (!graph.count(start)) {
        cout << "Subject not found.\n";
        return;
    }

    queue<string> q;
    set<string> visited;

    q.push("S:" + start);
    visited.insert("S:" + start);

    cout << "BFS Traversal: ";

    while (!q.empty()) {
        string vertex = q.front();
        q.pop();

        if (vertex.substr(0, 2) == "S:") {
            string subject = vertex.substr(2);
            cout << "[Subject: " << subject << "] ";

            for (int id : graph[subject]) {
                string task = "T:" + to_string(id);

                if (visited.insert(task).second)
                    q.push(task);
            }
        } else {
            int id = stoi(vertex.substr(2));
            cout << "[Task ID: " << id << "] ";

            for (const auto& subject : graph) {
                if (subject.second.count(id)) {
                    string next = "S:" + subject.first;

                    if (visited.insert(next).second)
                        q.push(next);
                }
            }
        }
    }

    cout << '\n';
}

void dfsTraversal() {
    string start;

    cout << "Enter starting subject: ";
    getline(cin >> ws, start);

    if (!graph.count(start)) {
        cout << "Subject not found.\n";
        return;
    }

    set<string> visited;

    auto dfs = [&](auto&& self, string vertex) -> void {
        if (!visited.insert(vertex).second)
            return;

        if (vertex.substr(0, 2) == "S:") {
            string subject = vertex.substr(2);
            cout << "[Subject: " << subject << "] ";

            for (int id : graph[subject])
                self(self, "T:" + to_string(id));
        } else {
            int id = stoi(vertex.substr(2));
            cout << "[Task ID: " << id << "] ";

            for (const auto& subject : graph) {
                if (subject.second.count(id))
                    self(self, "S:" + subject.first);
            }
        }
    };

    cout << "DFS Traversal: ";
    dfs(dfs, "S:" + start);
    cout << '\n';
}

void graphMenu() {
    int choice;

    do {
        cout << "\n========== GRAPH MENU ==========\n";
        cout << "1. Add Subject\n";
        cout << "2. Connect Tasks to Subject\n";
        cout << "3. Remove Task Connections\n";
        cout << "4. Display Tasks by Subject\n";
        cout << "5. Display Complete Graph\n";
        cout << "6. BFS Traversal\n";
        cout << "7. DFS Traversal\n";
        cout << "8. Back to Main Menu\n";
        cout << "Enter choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input.\n";
            continue;
        }

        switch (choice) {
            case 1: addSubject(); break;
            case 2: connectTasksToSubject(); break;
            case 3: removeTaskFromSubject(); break;
            case 4: displaySubjectTasks(); break;
            case 5: displayGraph(); break;
            case 6: bfsTraversal(); break;
            case 7: dfsTraversal(); break;
            case 8: break;
            default: cout << "Invalid choice.\n";
        }

    } while (choice != 8);
}
