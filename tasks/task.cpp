
#include "task.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
#include <algorithm>
#include "../sorting/sorting.h"
using namespace std;

Task tasks[100];
int taskCount = 0;

static int nextTaskId = 1;
static vector<Task> deletedTasks;

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
             << quoted(task.name) << ' '
             << quoted(task.deadline) << ' '
             << task.priority << ' '
             << task.completed << '\n';
    }
}

void loadTasks() {
    ifstream file(ACTIVE_FILE);
    Task task;

    while (file >> task.id
                >> quoted(task.type)
                >> quoted(task.name)
                >> quoted(task.deadline)
                >> task.priority
                >> task.completed) {
        tm check = {};

        if (taskCount >= 100)
            break;

        if (!parseDate(task.deadline, check))
            continue;

        tasks[taskCount++] = task;
        nextTaskId = max(nextTaskId, task.id + 1);
    }
}

void loadDeletedTasks() {
    ifstream file(DELETED_FILE);
    Task task;

    while (file >> task.id
                >> quoted(task.type)
                >> quoted(task.name)
                >> quoted(task.deadline)
                >> task.priority
                >> task.completed) {
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

    for (int i = 0; i < taskCount;) {
        int days = daysRemaining(tasks[i].deadline);

        if (!tasks[i].completed && days < 0) {
            cout << "\nAutomatically deleted overdue task: "
                 << tasks[i].name << " (ID "
                 << tasks[i].id << ")\n";

            deletedTasks.push_back(tasks[i]);
            removeTaskAt(i);
            changed = true;
        } else {
            if (!tasks[i].completed)
                tasks[i].priority = calculatePriority(days);
            i++;
        }
    }

    if (changed) {
        saveTasks();
        saveDeletedTasks();
    } else {
        saveTasks();
    }
}

void initializeTasks() {
    taskCount = 0;
    nextTaskId = 1;
    deletedTasks.clear();

    loadTasks();
    loadDeletedTasks();
    cleanupOverdueTasks();

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

    tasks[taskCount++] = task;
    saveTasks();

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
        cout << "Current deadline: " << tasks[i].deadline << '\n';
        cout << "1. Update type\n";
        cout << "2. Update description\n";
        cout << "3. Update deadline\n";
        cout << "4. Update all details\n";
        cout << "Enter choice: ";

        int choice;

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input.\n";
            return;
        }

        Task updated = tasks[i];

        if (choice == 1 || choice == 4) {
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

        if (choice == 2 || choice == 4) {
            cout << "Enter new description: ";
            getline(cin >> ws, updated.name);

            if (updated.name.empty()) {
                cout << "Description cannot be empty.\n";
                return;
            }
        }

        if (choice == 3 || choice == 4) {
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

        if (choice < 1 || choice > 4) {
            cout << "Invalid choice.\n";
            return;
        }

        tasks[i] = updated;
        saveTasks();
        cout << "Task updated and saved successfully.\n";
        return;
    }

    cout << "Task ID not found.\n";
}

void displayTasks() {
    cleanupOverdueTasks();

    if (taskCount == 0) {
        cout << "\nNo active tasks available.\n";
        return;
    }

    const int idW = 6;
    const int typeW = 20;
    const int nameW = 28;
    const int dateW = 14;
    const int daysW = 12;
    const int priorityW = 16;
    const int statusW = 12;

    int totalW = idW + typeW + nameW + dateW
               + daysW + priorityW + statusW;

    cout << "\n" << string(totalW, '=') << '\n';
    cout << "STUDENT TASKS\n";
    cout << string(totalW, '=') << '\n';

    cout << left
         << setw(idW) << "ID"
         << setw(typeW) << "Type"
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

        cout << left
             << setw(idW) << tasks[i].id
             << setw(typeW) << tasks[i].type
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
             << "\nDescription: " << task.name
             << "\nDeadline: " << task.deadline
             << "\nStatus: "
             << (task.completed ? "Completed" : "Deleted")
             << '\n';
    }
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

    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].id == id) {
            tasks[i].completed = true;
            saveTasks();
            cout << "Task completed and saved.\n";
            return;
        }
    }

    cout << "Task ID not found.\n";
}


void deleteTask() {
    cleanupOverdueTasks();

    if (taskCount == 0) {
        cout << "No active tasks to delete.\n";
        return;
    }

    int id;
    cout << "Enter task ID to delete: ";

    if (!(cin >> id)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input.\n";
        return;
    }

    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].id == id) {
            deletedTasks.push_back(tasks[i]);

            removeTaskAt(i);
            saveTasks();
            saveDeletedTasks();

            cout << "Task deleted and saved in history.\n";
            return;
        }
    }

    cout << "Task ID not found.\n";
}

