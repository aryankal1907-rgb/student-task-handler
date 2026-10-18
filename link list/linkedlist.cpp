
#include "linkedlist.h"
#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <fstream>

using namespace std;

struct Node {
    int taskId;
    Node* next;
};

struct WorkflowEvent {
    string action;
    Task task;
};

static Node* head = nullptr;
static vector<WorkflowEvent> history;
static const string HISTORY_FILE = "workflow_history.txt";

static bool inWorkflow(int id) {
    Node* current = head;

    while (current != nullptr) {
        if (current->taskId == id)
            return true;

        current = current->next;
    }

    return false;
}

static void addToEnd(int id) {
    if (inWorkflow(id))
        return;

    Node* newNode = new Node{id, nullptr};

    if (head == nullptr) {
        head = newNode;
        return;
    }

    Node* current = head;

    while (current->next != nullptr)
        current = current->next;

    current->next = newNode;
}

static void saveHistory() {
    ofstream file(HISTORY_FILE);

    if (!file) {
        cout << "Could not save workflow history.\n";
        return;
    }

    for (const WorkflowEvent& event : history) {
        file << event.action << '\n';
        file << event.task.id << '\n';
        file << event.task.type << '\n';
        file << event.task.subject << '\n';
        file << event.task.name << '\n';
        file << event.task.deadline << '\n';
        file << event.task.priority << '\n';
        file << event.task.completed << '\n';
    }
}

static void loadHistory() {
    history.clear();

    ifstream file(HISTORY_FILE);

    if (!file)
        return;

    WorkflowEvent event;

    while (getline(file, event.action)) {
        string idLine, priorityLine, completedLine;

        if (!getline(file, idLine) ||
            !getline(file, event.task.type) ||
            !getline(file, event.task.subject) ||
            !getline(file, event.task.name) ||
            !getline(file, event.task.deadline) ||
            !getline(file, priorityLine) ||
            !getline(file, completedLine))
            break;

        try {
            event.task.id = stoi(idLine);
            event.task.priority = stoi(priorityLine);
            event.task.completed = (stoi(completedLine) != 0);
        } catch (...) {
            break;
        }

        history.push_back(event);
    }
}

void addTaskToWorkflow(int id) {
    addToEnd(id);
}

void removeTaskFromWorkflow(int id) {
    Node* current = head;
    Node* previous = nullptr;

    while (current != nullptr) {
        if (current->taskId == id) {
            if (previous == nullptr)
                head = current->next;
            else
                previous->next = current->next;

            delete current;
            return;
        }

        previous = current;
        current = current->next;
    }
}

void syncWorkflowWithTasks() {
    while (head != nullptr) {
        Node* temp = head;
        head = head->next;
        delete temp;
    }

    for (int i = 0; i < taskCount; i++)
        addToEnd(tasks[i].id);
}

void initializeWorkflow() {
    while (head != nullptr) {
        Node* temp = head;
        head = head->next;
        delete temp;
    }

    loadHistory();

    for (int i = 0; i < taskCount; i++)
        addToEnd(tasks[i].id);
}

void recordTaskAdded(const Task& task) {
    history.push_back({"ADDED", task});
    addToEnd(task.id);
    saveHistory();
}

void recordTaskDeleted(const Task& task) {
    history.push_back({"DELETED", task});
    removeTaskFromWorkflow(task.id);
    saveHistory();
}

void recordTaskRestored(const Task& task) {
    history.push_back({"RESTORED", task});
    addToEnd(task.id);
    saveHistory();
}

void recordTaskCompleted(const Task& task) {
    history.push_back({"COMPLETED", task});
    saveHistory();
}

void recordTaskUpdated(const Task& task) {
    history.push_back({"UPDATED", task});
    saveHistory();
}

void recordUndoRedoWorkflow(const string& action,
                            const vector<int>& ids) {
    for (int id : ids) {
        Task recordedTask{};
        recordedTask.id = id;
        recordedTask.name = "Task state changed";

        for (int i = 0; i < taskCount; i++) {
            if (tasks[i].id == id) {
                recordedTask = tasks[i];
                break;
            }
        }

        for (const WorkflowEvent& event : history) {
            if (event.task.id == id &&
                recordedTask.name == "Task state changed") {
                recordedTask = event.task;
            }
        }

        history.push_back({action, recordedTask});
    }

    saveHistory();
}

static string priorityName(int priority) {
    if (priority == 1)
        return "High";
    if (priority == 2)
        return "Medium";
    return "Low";
}

static void displayHistory() {
    cout << "\n========== TASK HISTORY ==========\n";

    if (history.empty()) {
        cout << "No task operations recorded yet.\n";
        return;
    }

    for (int i = 0; i < (int)history.size(); i++) {
        cout << i + 1 << ". " << history[i].action
             << " | ID: " << history[i].task.id
             << " | Type: " << history[i].task.type
             << " | Subject: " << history[i].task.subject
             << " | Task: " << history[i].task.name
             << " | Priority: " << priorityName(history[i].task.priority)
             << " | Days remaining: "
             << daysRemaining(history[i].task.deadline)
             << " | Deadline: " << history[i].task.deadline
             << " | Status: "
             << (history[i].task.completed ? "Completed" : "Pending")
             << '\n';
    }
}

static void displayCurrentWorkflow() {
    cout << "\n========== CURRENT TASK WORKFLOW ==========\n";

    if (head == nullptr) {
        cout << "No tasks in the workflow.\n";
        return;
    }

    Node* current = head;

    while (current != nullptr) {
        int id = current->taskId;

        for (int i = 0; i < taskCount; i++) {
            if (tasks[i].id == id) {
                cout << "ID: " << tasks[i].id
                     << " | Type: " << tasks[i].type
                     << " | Subject: " << tasks[i].subject
                     << " | Task: " << tasks[i].name
                     << " | Priority: " << priorityName(tasks[i].priority)
                     << " | Days remaining: "
                     << daysRemaining(tasks[i].deadline)
                     << " | Deadline: " << tasks[i].deadline
                     << " | Status: "
                     << (tasks[i].completed ? "Completed" : "Pending")
                     << '\n';
                break;
            }
        }

        current = current->next;
    }
}

void displayWorkflow() {
    displayHistory();
    displayCurrentWorkflow();
}

void LinkedListMenu() {
    int choice;

    do {
        cout << "\n========== TASK WORKFLOW ==========\n";
        cout << "1. Display Workflow and History\n";
        cout << "2. Return to Main Menu\n";
        cout << "Enter choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input.\n";
            continue;
        }

        switch (choice) {
            case 1:
                displayWorkflow();
                break;

            case 2:
                return;

            default:
                cout << "Invalid choice.\n";
        }

    } while (true);
}
