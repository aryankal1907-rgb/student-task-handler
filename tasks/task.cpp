
#include "task.h"
#include<iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
#include <algorithm>
#include "../sorting/sorting.h"
#include "../link list/linkedlist.h"
#include "../stack/stack.h"
#include "../queue/queue.h"
#include "../avl/avl.h"
#include "../graphs/graph.h"
using namespace std;

Task tasks[100];
int taskCount = 0;

static int nextTaskId = 1;
vector<Task> deletedTasks;

const string ACTIVE_FILE = "tasks.txt";
const string DELETED_FILE = "deleted_tasks.txt";


bool parseDate(const string& date, tm& result) {
    if (date.size() != 10 ||
        date[2] != '-' || date[5] != '-')
        return false;

    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 5)
            continue;

        if (date[i] < '0' || date[i] > '9')
            return false;
    }

    int day = stoi(date.substr(0, 2));
    int month = stoi(date.substr(3, 2));
    int year = stoi(date.substr(6, 4));

    if (month < 1 || month > 12 ||
        day < 1 || day > 31 ||
        year < 1900)
        return false;

    result = {};
    result.tm_year = year - 1900;
    result.tm_mon = month - 1;
    result.tm_mday = day;
    result.tm_hour = 12;
    result.tm_isdst = -1;

    time_t value = mktime(&result);

    if (value == -1)
        return false;

    tm checked = *localtime(&value);

    return checked.tm_year == year - 1900 &&
           checked.tm_mon == month - 1 &&
           checked.tm_mday == day;
}

int daysRemaining(const string& deadline) {
    tm due = {}, today = {};

    if (!parseDate(deadline, due))
        return 0;

    time_t now = time(nullptr);
    tm* current = localtime(&now);

    if (current == nullptr)
        return 0;

    today = *current;
    today.tm_hour = 12;
    today.tm_min = 0;
    today.tm_sec = 0;
    today.tm_isdst = -1;

    time_t dueTime = mktime(&due);
    time_t todayTime = mktime(&today);

    return static_cast<int>(
        round(difftime(dueTime, todayTime) / 86400.0)
    );
}

int calculatePriority(int days) {
    if (days <= 10)
        return 1;
    if (days <= 20)
        return 2;
    return 3;
}

string priorityName(int priority) {
    if (priority == 1)
        return "High";
    if (priority == 2)
        return "Medium";
    return "Low";
}

string getTaskType(int choice) {
    switch (choice) {
        case 1: return "Assignment";
        case 2: return "Lab Work";
        case 3: return "Tutorial";
        case 4: return "Quiz Preparation";
        case 5: return "Exam Preparation";
        case 6: return "Project";
        case 7: return "Other";
        default: return "";
    }
}

int dateKey(const string& date) {
    int day, month, year;
    sscanf(date.c_str(), "%d-%d-%d", &day, &month, &year);
    return year * 10000 + month * 100 + day;
}

void saveTasks() {
    ofstream file(ACTIVE_FILE);

    for (int i = 0; i < taskCount; i++) {
        file << tasks[i].id << ' '
             << quoted(tasks[i].type) << ' '
             << quoted(tasks[i].subject) << ' '
             << quoted(tasks[i].name) << ' '
             << quoted(tasks[i].deadline) << ' '
             << tasks[i].priority << ' '
             << tasks[i].completed << '\n';
    }
}

void saveDeletedTasks() {
    ofstream file(DELETED_FILE);

    for (const Task& task : deletedTasks) {
        file << task.id << ' '
             << quoted(task.type) << ' '
             << quoted(task.subject) << ' '
             << quoted(task.name) << ' '
             << quoted(task.deadline) << ' '
             << task.priority << ' '
             << task.completed << '\n';
    }
}


void loadTasks() {
    ifstream file(ACTIVE_FILE);
    string line;

    while (getline(file, line)) {
        Task task{};
        istringstream input(line);

        if (!(input >> task.id >> quoted(task.type)))
            continue;

        string subject, name, deadline;
        int priority;
        bool completed;

        istringstream newFormat(line);
        int id;
        string type;

        if (newFormat >> id >> quoted(type)
                      >> quoted(subject) >> quoted(name)
                      >> quoted(deadline) >> priority >> completed) {
            task.subject = subject;
            task.name = name;
            task.deadline = deadline;
            task.priority = priority;
            task.completed = completed;
        } else {
            istringstream oldFormat(line);

            if (!(oldFormat >> task.id >> quoted(task.type)
                           >> quoted(task.name) >> quoted(task.deadline)
                           >> task.priority >> task.completed)) {
                continue;
            }

            task.subject = "Uncategorized";
        }

        tm check = {};

        if (!parseDate(task.deadline, check))
            continue;

        if (taskCount >= 100)
            break;

        tasks[taskCount++] = task;
        nextTaskId = max(nextTaskId, task.id + 1);
    }
}


void loadDeletedTasks() {
    ifstream file(DELETED_FILE);
    string line;

    while (getline(file, line)) {
        istringstream input(line);
        Task task;

        if (!(input >> task.id >> quoted(task.type)))
            continue;

        string remaining;
        getline(input >> ws, remaining);

        istringstream oldInput(remaining);
        string name, deadline;
        int priority;
        bool completed;

        if (!(oldInput >> quoted(name) >> quoted(deadline)
                       >> priority >> completed))
            continue;

        task.name = name;
        task.deadline = deadline;
        task.priority = priority;
        task.completed = completed;

        istringstream newInput(remaining);
        string subject, newName, newDeadline;
        int newPriority;
        bool newCompleted;

        if (newInput >> quoted(subject) >> quoted(newName)
                     >> quoted(newDeadline)
                     >> newPriority >> newCompleted) {
            task.subject = subject;
            task.name = newName;
            task.deadline = newDeadline;
            task.priority = newPriority;
            task.completed = newCompleted;
        } else {
            task.subject = "Uncategorized";
        }

        tm check = {};

        if (!parseDate(task.deadline, check))
            continue;

        deletedTasks.push_back(task);
        nextTaskId = max(nextTaskId, task.id + 1);
    }
}

void removeTaskAt(int index) {
    for (int i = index; i < taskCount - 1; i++)
        tasks[i] = tasks[i + 1];

    taskCount--;
}


void cleanupOverdueTasks() {
    bool changed = false;

    for (int i = 0; i < taskCount; ) {
        int days = daysRemaining(tasks[i].deadline);

        if (!tasks[i].completed && days < 0) {
            cout << "\nAutomatically deleted overdue task: "
                 << tasks[i].name << " (ID "
                 << tasks[i].id << ")\n";

            recordTaskDeleted(tasks[i]);
            deletedTasks.push_back(tasks[i]);
            removeTaskFromQueue(tasks[i].id);
            removeTaskAt(i);
            changed = true;
        } else {
            if (!tasks[i].completed) {
                int oldPriority = tasks[i].priority;
                tasks[i].priority = calculatePriority(days);

                if (tasks[i].priority != oldPriority) {
                    recordTaskUpdated(tasks[i]);
                    changed = true;
                }
            }
            i++;
        }
    }

    saveTasks();

    if (changed) {
        saveDeletedTasks();
    }
    synchronizeAVLWithTasks();
}

vector<Task> getDeletedTasksSnapshot() {
    return deletedTasks;
}

void restoreDeletedTasksSnapshot(const vector<Task>& saved) {
    deletedTasks = saved;
    saveDeletedTasks();
}


void initializeTasks() {
    taskCount = 0;
    nextTaskId = 1;
    deletedTasks.clear();

    loadTasks();
    loadDeletedTasks();
    cleanupOverdueTasks();
    initializeQueueFromTasks();
    synchronizeAVLWithTasks();

    cout << "Tasks loaded successfully.\n";
}

void showTaskTypes() {
    cout << "\n1. Assignment\n";
    cout << "2. Lab Work\n";
    cout << "3. Tutorial\n";
    cout << "4. Quiz Preparation\n";
    cout << "5. Exam Preparation\n";
    cout << "6. Project\n";
    cout << "7. Other\n";
    cout << "Enter task type: ";
}

void addTask() {
    cleanupOverdueTasks();

    if (taskCount >= 100) {
        cout << "Task list is full.\n";
        return;
    }

    int choice;
    showTaskTypes();

    if (!(cin >> choice)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input.\n";
        return;
    }

    string type = getTaskType(choice);

    if (type.empty()) {
        cout << "Invalid task type.\n";
        return;
    }

    Task task;
    task.type = type;

    cout << "Enter subject name:";
    getline(cin >> ws, task.subject);

    if (task.subject.empty()) {
        cout << "Subject cannot be empty.\n";
        return;
    }

    cout << "Enter task description: ";
    getline(cin >> ws, task.name);

    if (task.name.empty()) {
        cout << "Description cannot be empty.\n";
        return;
    }

    cout << "Enter deadline (DD-MM-YYYY): ";
    cin >> task.deadline;

    tm check = {};

    if (!parseDate(task.deadline, check)) {
        cout << "Invalid date. Use DD-MM-YYYY.\n";
        return;
    }

    int days = daysRemaining(task.deadline);

    if (days < 0) {
        cout << "Deadline is in the past. Task not added.\n";
        return;
    }

    task.id = nextTaskId++;
    task.priority = calculatePriority(days);
    task.completed = false;

    saveState("Added Task: " + task.name + " (ID " + to_string(task.id) + ")");
    tasks[taskCount++] = task;
    recordTaskAdded(task);
    enqueueTaskAutomatically(task.id);
    saveTasks();
    synchronizeAVLWithTasks();
    synchronizeGraphWithTasks();

    cout << "Task added successfully. ID: "
         << task.id << '\n';
}

void updateTask() {
    cleanupOverdueTasks();

    if (taskCount == 0) {
        cout << "No tasks available to update.\n";
        return;
    }

    int id;
    cout << "Enter task ID to update: ";

    if (!(cin >> id)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input.\n";
        return;
    }

    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].id != id)
            continue;

        cout << "\nCurrent description: " << tasks[i].name << '\n';
        cout<< "Current subject: " << tasks[i].subject << '\n';
        cout << "Current type: " << tasks[i].type << '\n';
        cout << "Current deadline: " << tasks[i].deadline << '\n';
        cout << "1. Update type\n";
        cout<< "2. Update subject\n";
        cout << "3. Update description\n";
        cout << "4. Update deadline\n";
        cout << "5. Update all details\n";
        cout << "Enter choice: ";

        int choice;

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input.\n";
            return;
        }

        Task updated = tasks[i];

        if (choice == 1 || choice == 5) {
            showTaskTypes();
            int typeChoice;

            if (!(cin >> typeChoice)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input.\n";
                return;
            }

            string type = getTaskType(typeChoice);

            if (type.empty()) {
                cout << "Invalid task type.\n";
                return;
            }

            updated.type = type;
        }

        if (choice == 3|| choice == 5){
            cout << "Enter new description: ";
            getline(cin >> ws, updated.name);

            if (updated.name.empty()) {
                cout << "Description cannot be empty.\n";
                return;
            }
        }

        if (choice == 4 || choice == 5) {
            cout << "Enter new deadline (DD-MM-YYYY): ";
            cin >> updated.deadline;

            tm check = {};

            if (!parseDate(updated.deadline, check)) {
                cout << "Invalid date.\n";
                return;
            }

            if (daysRemaining(updated.deadline) < 0) {
                cout << "Deadline cannot be in the past.\n";
                return;
            }

            updated.priority = calculatePriority(
                daysRemaining(updated.deadline)
            );
        }

        if(choice == 2 || choice == 5){
            cout << "Enter new subject: ";
            getline(cin >> ws, updated.subject);

            if (updated.subject.empty()) {
                cout << "Subject cannot be empty.\n";
                return;
            }
        }

        if (choice < 1 || choice > 5){
            cout << "Invalid choice.\n";
            return;
        }

        saveState("Updated Task: " + updated.name + " (ID " + to_string(updated.id) + ")");
        tasks[i] = updated;
        recordTaskUpdated(updated);
        refreshQueue();
        sortTasks();
        saveTasks();
        synchronizeAVLWithTasks();
        synchronizeGraphWithTasks();
        cout << "Task updated and saved successfully.\n";
        return;
    }

    cout << "Task ID not found.\n";
}

void displayTasks() {
    cleanupOverdueTasks();
    sortTasks();

    if (taskCount == 0) {
        cout << "\nNo active tasks available.\n";
        return;
    }

    const int idW = 6;
    const int typeW = 20;
    const int subjectW = 20;
    const int nameW = 28;
    const int dateW = 14;
    const int daysW = 12;
    const int priorityW = 16;
    const int statusW = 12;

    int totalW = idW + typeW + subjectW + nameW + dateW
               + daysW + priorityW + statusW;

    cout << "\n" << string(totalW, '=') << '\n';
    cout << "STUDENT TASKS\n";
    cout << string(totalW, '=') << '\n';

    cout << left
         << setw(idW) << "ID"
         << setw(typeW) << "Type"
         << setw(subjectW) << "Subject"
         << setw(nameW) << "Description"
         << setw(dateW) << "Deadline"
         << setw(daysW) << "Days Left"
         << setw(priorityW) << "Priority"
         << setw(statusW) << "Status" << '\n';

    cout << string(totalW, '-') << '\n';

    for (int i = 0; i < taskCount; i++) {
        int days = daysRemaining(tasks[i].deadline);

        if (!tasks[i].completed)
            tasks[i].priority = calculatePriority(days);

        string remaining = days < 0
                         ? "Overdue"
                         : to_string(days);

        string description = tasks[i].name;
        if (description.length() >= nameW)
            description = description.substr(0, nameW - 1);

        string subject = tasks[i].subject;
        if (subject.length() >= subjectW)
            subject = subject.substr(0, subjectW - 1);  

        cout << left
             << setw(idW) << tasks[i].id
             << setw(typeW) << tasks[i].type
             << setw(subjectW) << subject
             << setw(nameW) << description
             << setw(dateW) << tasks[i].deadline
             << setw(daysW) << remaining
             << setw(priorityW)
             << (to_string(tasks[i].priority) + " - " +
                 priorityName(tasks[i].priority))
             << setw(statusW)
             << (tasks[i].completed ? "Completed" : "Pending")
             << '\n';
    }

    cout << string(totalW, '=') << '\n';

    saveTasks();
}


void displayDeletedTasks() {
    if (deletedTasks.empty()) {
        cout << "No deleted tasks in history.\n";
        return;
    }

    cout << "\n========== DELETED TASK HISTORY ==========\n";

    for (const Task& task : deletedTasks) {
        cout << "\nID: " << task.id
             << "\nType: " << task.type
            << "\nSubject: " << task.subject
             << "\nDescription: " << task.name
             << "\nDeadline: " << task.deadline
             << "\nStatus: "
             << (task.completed ? "Completed" : "Deleted")
             << '\n';
    }
}




void restoreDeletedTask() {
    if (deletedTasks.empty()) {
        cout << "No deleted tasks available to restore.\n";
        return;
    }

    cout << "\n========== DELETED TASKS ==========\n";

    for (const Task& task : deletedTasks) {
        cout << "ID: " << task.id
             << " | Description: " << task.name
             << " | Subject: " << task.subject
             << " | Deadline: " << task.deadline << '\n';
    }

    int count;
    cout << "How many tasks do you want to restore? ";

    if (!(cin >> count)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input.\n";
        return;
    }

    if (count < 1 || count > static_cast<int>(deletedTasks.size())) {
        cout << "Enter a number between 1 and "
             << deletedTasks.size() << ".\n";
        return;
    }

    if (taskCount + count > 100) {
        cout << "Not enough space in the active task list.\n";
        return;
    }

    vector<int> ids;

    cout << "Enter " << count << " Task IDs: ";

    for (int i = 0; i < count; i++) {
        int id;

        if (!(cin >> id)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid Task ID.\n";
            return;
        }

        bool found = false;

        for (const Task& task : deletedTasks) {
            if (task.id == id) {
                found = true;
                break;
            }
        }

        if (!found) {
            cout << "Deleted Task ID " << id << " not found.\n";
            return;
        }

        for (int previous : ids) {
            if (previous == id) {
                cout << "Duplicate Task ID entered.\n";
                return;
            }
        }

        ids.push_back(id);
    }

    char confirm;
    cout << "Confirm restoration of " << count
         << " tasks? (Y/N): ";
    cin >> confirm;

    if (confirm != 'Y' && confirm != 'y') {
        cout << "Restoration cancelled.\n";
        return;
    }

    saveState("Restored " + to_string(count) + " Task(s)", ids);
    for (int id : ids) {
        for (size_t i = 0; i < deletedTasks.size(); i++) {
            if (deletedTasks[i].id == id) {
                Task restoredTask = deletedTasks[i];
                tasks[taskCount++] = deletedTasks[i];
                recordTaskRestored(restoredTask);
                enqueueTaskAutomatically(restoredTask.id);
                deletedTasks.erase(deletedTasks.begin() + i);
                break;
            }
        }
    }

    sortTasks();
    saveTasks();
    saveDeletedTasks();
    synchronizeAVLWithTasks();
    synchronizeGraphWithTasks();

    cout << count << " tasks restored successfully.\n";
}



void deleteTask() {
    cleanupOverdueTasks();

    if (taskCount == 0) {
        cout << "No active tasks to delete.\n";
        return;
    }

    int count;
    cout << "How many tasks do you want to delete? ";

    if (!(cin >> count)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input.\n";
        return;
    }

    if (count < 1 || count > taskCount) {
        cout << "Enter a number between 1 and "
             << taskCount << ".\n";
        return;
    }

    vector<int> ids;

    cout << "Enter " << count << " Task IDs: ";

    for (int i = 0; i < count; i++) {
        int id;

        if (!(cin >> id)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid Task ID.\n";
            return;
        }

        bool found = false;

        for (int j = 0; j < taskCount; j++) {
            if (tasks[j].id == id) {
                found = true;
                break;
            }
        }

        if (!found || id <= 0) {
            cout << "Invalid Task ID: " << id << '\n';
            return;
        }

        for (int previous : ids) {
            if (previous == id) {
                cout << "Duplicate Task ID entered.\n";
                return;
            }
        }

        ids.push_back(id);
    }

    char confirm;
    cout << "Confirm deletion of " << count
         << " tasks? (Y/N): ";
    cin >> confirm;

    if (confirm != 'Y' && confirm != 'y') {
        cout << "Deletion cancelled.\n";
        return;
    }
    saveState("Deleted " + to_string(count) + " Task(s)", ids);

    for (int id : ids) {
        for (int i = 0; i < taskCount; i++) {
            if (tasks[i].id == id) {
                recordTaskDeleted(tasks[i]);
                deletedTasks.push_back(tasks[i]);
                removeTaskFromQueue(id);
                removeTaskAt(i);
                break;
            }
        }
    }

    sortTasks();
    saveTasks();
    saveDeletedTasks();
    synchronizeAVLWithTasks();
    synchronizeGraphWithTasks();

    cout << count << " tasks deleted successfully.\n";
}


void markTaskCompleted() {
    cleanupOverdueTasks();

    if (taskCount == 0) {
        cout << "No active tasks available.\n";
        return;
    }

    int id;
    cout << "Enter task ID to mark completed: ";

    if (!(cin >> id)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input.\n";
        return;
    }
   saveState("Marked Task ID " + to_string(id) + " as Completed");

    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].id == id) {
            tasks[i].completed = true;
            recordTaskCompleted(tasks[i]);
            removeTaskFromQueue(id);
            sortTasks();
            saveTasks();
            synchronizeAVLWithTasks();
            cout << "Task marked as completed.\n";
            return;
        }
    }

    cout << "Task ID not found.\n";
}

// ===== WEB API LAYER =====
void saveTasks();
static string trimStr(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
static bool validType(const string& t) {
    for (int i = 1; i <= 7; i++) if (getTaskType(i) == t) return true;
    return false;
}
static int findIndexById(int id) {
    for (int i = 0; i < taskCount; i++) if (tasks[i].id == id) return i;
    return -1;
}
void apiRefresh() { cleanupOverdueTasks(); sortTasks(); }

bool apiAddTask(const string& type, const string& subjectIn, const string& nameIn,
                const string& deadline, int& newId, string& err) {
    cleanupOverdueTasks();
    if (taskCount >= 100) { err = "Task list is full (100 tasks max)."; return false; }
    if (!validType(type)) { err = "Choose a valid task type."; return false; }
    string subject = trimStr(subjectIn), name = trimStr(nameIn);
    if (subject.empty()) { err = "Subject cannot be empty."; return false; }
    if (name.empty()) { err = "Description cannot be empty."; return false; }
    tm check = {};
    if (!parseDate(deadline, check)) { err = "Invalid date. Use DD-MM-YYYY."; return false; }
    int days = daysRemaining(deadline);
    if (days < 0) { err = "Deadline is in the past."; return false; }

    Task task;
    task.id = nextTaskId++;
    task.type = type; task.subject = subject; task.name = name; task.deadline = deadline;
    task.priority = calculatePriority(days);
    task.completed = false;

    saveState("Added Task: " + task.name + " (ID " + to_string(task.id) + ")");
    tasks[taskCount++] = task;
    recordTaskAdded(task);
    enqueueTaskAutomatically(task.id);
    sortTasks();
    saveTasks();
    synchronizeAVLWithTasks();
    synchronizeGraphWithTasks();
    newId = task.id;
    return true;
}

bool apiUpdateTask(int id, const string& type, const string& subject, const string& name,
                   const string& deadline, string& err) {
    cleanupOverdueTasks();
    int i = findIndexById(id);
    if (i < 0) { err = "Task ID not found."; return false; }
    Task updated = tasks[i];
    if (!type.empty()) {
        if (!validType(type)) { err = "Choose a valid task type."; return false; }
        updated.type = type;
    }
    if (!subject.empty()) updated.subject = trimStr(subject);
    if (!name.empty()) updated.name = trimStr(name);
    if (!deadline.empty()) {
        tm check = {};
        if (!parseDate(deadline, check)) { err = "Invalid date. Use DD-MM-YYYY."; return false; }
        if (daysRemaining(deadline) < 0) { err = "Deadline cannot be in the past."; return false; }
        updated.deadline = deadline;
        updated.priority = calculatePriority(daysRemaining(deadline));
    }
    if (updated.subject.empty() || updated.name.empty()) { err = "Subject and description cannot be empty."; return false; }

    saveState("Updated Task: " + updated.name + " (ID " + to_string(updated.id) + ")");
    tasks[i] = updated;
    recordTaskUpdated(updated);
    refreshQueue();
    sortTasks();
    saveTasks();
    synchronizeAVLWithTasks();
    synchronizeGraphWithTasks();
    return true;
}

bool apiCompleteTask(int id, string& err) {
    cleanupOverdueTasks();
    int i = findIndexById(id);
    if (i < 0) { err = "Task ID not found."; return false; }
    if (tasks[i].completed) { err = "Task is already completed."; return false; }
    saveState("Marked Task ID " + to_string(id) + " as Completed");
    tasks[i].completed = true;
    recordTaskCompleted(tasks[i]);
    removeTaskFromQueue(id);
    sortTasks();
    saveTasks();
    synchronizeAVLWithTasks();
    return true;
}

bool apiDeleteTasks(const vector<int>& ids, string& err) {
    cleanupOverdueTasks();
    if (ids.empty()) { err = "No tasks selected."; return false; }
    for (size_t a = 0; a < ids.size(); a++) {
        if (findIndexById(ids[a]) < 0) { err = "Invalid Task ID: " + to_string(ids[a]); return false; }
        for (size_t b = a + 1; b < ids.size(); b++)
            if (ids[a] == ids[b]) { err = "Duplicate Task ID."; return false; }
    }
    saveState("Deleted " + to_string(ids.size()) + " Task(s)", ids);
    for (int id : ids) {
        int i = findIndexById(id);
        recordTaskDeleted(tasks[i]);
        deletedTasks.push_back(tasks[i]);
        removeTaskFromQueue(id);
        removeTaskAt(i);
    }
    sortTasks();
    saveTasks();
    saveDeletedTasks();
    synchronizeAVLWithTasks();
    synchronizeGraphWithTasks();
    return true;
}

bool apiRestoreTasks(const vector<int>& ids, string& err) {
    if (ids.empty()) { err = "No tasks selected."; return false; }
    if (taskCount + (int)ids.size() > 100) { err = "Not enough space in the active task list."; return false; }
    for (int id : ids) {
        bool found = false;
        for (const Task& t : deletedTasks) {
            if (t.id != id) continue;
            found = true;
            if (!t.completed && daysRemaining(t.deadline) < 0) {
                err = "\"" + t.name + "\" is past its deadline and cannot be restored.";
                return false;
            }
        }
        if (!found) { err = "Deleted task ID " + to_string(id) + " not found."; return false; }
    }
    saveState("Restored " + to_string(ids.size()) + " Task(s)", ids);
    for (int id : ids) {
        for (size_t i = 0; i < deletedTasks.size(); i++) {
            if (deletedTasks[i].id != id) continue;
            Task t = deletedTasks[i];
            if (!t.completed) t.priority = calculatePriority(daysRemaining(t.deadline));
            tasks[taskCount++] = t;
            recordTaskRestored(t);
            enqueueTaskAutomatically(t.id);
            deletedTasks.erase(deletedTasks.begin() + i);
            break;
        }
    }
    sortTasks();
    saveTasks();
    saveDeletedTasks();
    synchronizeAVLWithTasks();
    synchronizeGraphWithTasks();
    return true;
}