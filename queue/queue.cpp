#include "queue.h"
#include "../tasks/task.h"
#include "../link list/linkedlist.h"
#include "../stack/stack.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>

using namespace std;

static vector<int> taskQueue;

Task* findTask(int id) {
    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].id == id)
            return &tasks[i];
    }

    return nullptr;
}

bool alreadyQueued(int id) {
    return find(taskQueue.begin(), taskQueue.end(), id)
           != taskQueue.end();
}

int currentPriority(int id) {
    Task* task = findTask(id);

    if (task == nullptr)
        return 4;

    return calculatePriority(daysRemaining(task->deadline));
}

void sortQueue() {
    taskQueue.erase(
        remove_if(taskQueue.begin(), taskQueue.end(),
            [](int id) {
                Task* task = findTask(id);

                return task == nullptr ||
                       task->completed ||
                       daysRemaining(task->deadline) < 0;
            }),
        taskQueue.end()
    );

    stable_sort(taskQueue.begin(), taskQueue.end(),
        [](int a, int b) {
            Task* taskA = findTask(a);
            Task* taskB = findTask(b);

            int priorityA = currentPriority(a);
            int priorityB = currentPriority(b);

            if (priorityA != priorityB) {
                return priorityA < priorityB;
            }

            int daysA = daysRemaining(taskA->deadline);
            int daysB = daysRemaining(taskB->deadline);

            if (daysA != daysB) {
                return daysA < daysB;
            }

            return a < b;
        });
}

vector<int> getQueueSnapshot() {
    return taskQueue;
}

void initializeQueueFromTasks() {
    taskQueue.clear();

    for (int i = 0; i < taskCount; i++) {
        if (!tasks[i].completed &&
            daysRemaining(tasks[i].deadline) >= 0) {
            taskQueue.push_back(tasks[i].id);
        }
    }

    sortQueue();
}

void restoreQueueSnapshot(const vector<int>& saved) {
    taskQueue.clear();

    for (int id : saved) {
        Task* task = findTask(id);

        if (task != nullptr &&
            !task->completed &&
            daysRemaining(task->deadline) >= 0 &&
            !alreadyQueued(id)) {
            taskQueue.push_back(id);
        }
    }

    sortQueue();
}

void refreshQueue() {
    sortQueue();
}

void enqueueTaskAutomatically(int id) {
    Task* task = findTask(id);

    if (task == nullptr ||
        task->completed ||
        daysRemaining(task->deadline) < 0 ||
        alreadyQueued(id)) {
        return;
    }

    taskQueue.push_back(id);
    sortQueue();

    recordUndoRedoWorkflow(
        "QUEUE AUTO ENQUEUE", vector<int>{id}
    );

    cout << "Task ID " << id
         << " automatically added to the queue.\n";
}

void removeTaskFromQueue(int id) {
    auto it = find(taskQueue.begin(), taskQueue.end(), id);

    if (it == taskQueue.end())
        return;

    taskQueue.erase(it);

    recordUndoRedoWorkflow(
        "QUEUE REMOVE", vector<int>{id}
    );
}

void enqueueMultiple() {
    int count;

    cout << "How many tasks do you want to enqueue? ";

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

    cout << "Enter " << count << " Task IDs: ";

    for (int i = 0; i < count; i++) {
        int id;

        if (!(cin >> id)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid Task ID.\n";
            return;
        }

        Task* task = findTask(id);

        if (task == nullptr ||
            task->completed ||
            daysRemaining(task->deadline) < 0 ||
            alreadyQueued(id)) {
            cout << "Task ID " << id
                 << " cannot be enqueued.\n";
            return;
        }

        if (find(ids.begin(), ids.end(), id) != ids.end()) {
            cout << "Duplicate Task ID entered.\n";
            return;
        }

        ids.push_back(id);
    }

    saveState("Enqueued multiple tasks", ids);

    for (int id : ids)
        taskQueue.push_back(id);

    sortQueue();

    recordUndoRedoWorkflow("QUEUE ENQUEUE", ids);

    cout << ids.size()
         << " task(s) added to the queue.\n";
}

void processMultiple() {
    if (taskQueue.empty()) {
        cout << "Queue is empty.\n";
        return;
    }

    sortQueue();

    int count;

    cout << "How many tasks do you want to process? ";

    if (!(cin >> count)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input.\n";
        return;
    }

    if (count < 1 || count > static_cast<int>(taskQueue.size())) {
        cout << "Enter a number between 1 and "
             << taskQueue.size() << ".\n";
        return;
    }

    vector<int> processed;

    for (int i = 0; i < count; i++)
        processed.push_back(taskQueue[i]);

    saveState("Processed multiple tasks", processed);

    for (int id : processed) {
        cout << "Processed Task ID: " << id << '\n';
    }

    taskQueue.erase(taskQueue.begin(),
                    taskQueue.begin() + count);

    recordUndoRedoWorkflow("QUEUE PROCESS", processed);

    cout << count << " task(s) processed.\n";
}

void displayQueue() {
    sortQueue();

    if (taskQueue.empty()) {
        cout << "Queue is empty.\n";
        return;
    }

    cout << "\n========== TASK QUEUE ==========\n";

    for (int id : taskQueue) {
        Task* task = findTask(id);

        if (task == nullptr)
            continue;

        int days = daysRemaining(task->deadline);
        int priority = calculatePriority(days);

        cout << "ID: " << task->id
             << " | Type: " << task->type
             << " | Subject: " << task->subject
             << " | Task: " << task->name
             << " | Description: " << task->name
             << " | Deadline: " << task->deadline
             << " | Priority: ";

        if (priority == 1)
            cout << "High";
        else if (priority == 2)
            cout << "Medium";
        else
            cout << "Low";

        cout << " | Days Left: " << days << '\n';
    }

    cout << "================================\n";
}

void displayNextTask() {
    sortQueue();

    while (!taskQueue.empty()) {
        Task* task = findTask(taskQueue.front());

        if (task != nullptr &&
            !task->completed &&
            daysRemaining(task->deadline) >= 0) {
            break;
        }

        taskQueue.erase(taskQueue.begin());
    }

    if (taskQueue.empty()) {
        cout << "\nNo pending tasks in the queue.\n";
        return;
    }

    Task* task = findTask(taskQueue.front());
    int days = daysRemaining(task->deadline);
    int priority = calculatePriority(days);

    cout << "\n========== NEXT TASK TO OPERATE ==========\n";
    cout << "Task ID: " << task->id << '\n';
    cout << "Task: " << task->name << '\n';
    cout << "Type: " << task->type << '\n';
    cout << "Subject: " << task->subject << '\n';
    cout << "Description: " << task->name<<'\n';
    cout << "Priority: ";

    if (priority == 1)
        cout << "High";
    else if (priority == 2)
        cout << "Medium";
    else
        cout << "Low";

    cout << "\nDeadline: " << task->deadline;
    cout << "\nDays Remaining: " << days << '\n';
    cout << "==========================================\n";
}

void clearQueue() {
    if (taskQueue.empty()) {
        cout << "Queue is already empty.\n";
        return;
    }

    vector<int> removed = taskQueue;

    saveState("Cleared queue", removed);

    taskQueue.clear();

    recordUndoRedoWorkflow("QUEUE CLEAR", removed);

    cout << "Queue cleared successfully.\n";
}

void queueMenu() {
    int choice;

    do {
        cout << "\n========== QUEUE MENU ==========\n";
        cout << "1. Enqueue Multiple Tasks\n";
        cout << "2. Process Multiple Tasks\n";
        cout << "3. Display Queue\n";
        cout << "4. Clear Queue\n";
        cout << "0. Return\n";
        cout << "Enter choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input.\n";
            continue;
        }

        switch (choice) {
            case 1:
                enqueueMultiple();
                break;
            case 2:
                processMultiple();
                break;
            case 3:
                displayQueue();
                break;
            case 4:
                clearQueue();
                break;
            case 0:
                break;
            default:
                cout << "Invalid choice.\n";
        }
    } while (choice != 0);
}