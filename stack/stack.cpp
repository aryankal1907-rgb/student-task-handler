
#include "stack.h"
#include "../tasks/task.h"
#include "../link list/linkedlist.h"
#include "../queue/queue.h"
#include "../avl/avl.h"
#include "../graphs/graph.h"
#include <iostream>
#include <vector>
using namespace std;

extern Task tasks[100];
extern int taskCount;

void saveTasks();
void displayTasks();
void displayDeletedTasks();
vector<Task> getDeletedTasksSnapshot();
void restoreDeletedTasksSnapshot(const vector<Task>& saved);

struct TaskState {
    Task tasks[100];
    int taskCount;
    vector<Task> deletedTasks;
    string operation;
    vector<int> taskIds;
    vector<int> queueSnapshot;
};

TaskState undoStack[100];
TaskState redoStack[100];

int undoTop = -1;
int redoTop = -1;

TaskState createState(const string& operation,
                      const vector<int>& taskIds = {}) {
    TaskState state{};

    for (int i = 0; i < taskCount; i++)
        state.tasks[i] = tasks[i];

    state.taskCount = taskCount;
    state.deletedTasks = getDeletedTasksSnapshot();
    state.operation = operation;
    state.taskIds = taskIds;
    state.queueSnapshot = getQueueSnapshot();

    return state;
}

void pushState(TaskState stack[], int& top,
               const TaskState& state) {
    if (top == 99) {
        for (int i = 0; i < 99; i++)
            stack[i] = stack[i + 1];

        top = 98;
    }

    stack[++top] = state;
}

void restoreState(const TaskState& state) {
    taskCount = state.taskCount;

    for (int i = 0; i < taskCount; i++)
        tasks[i] = state.tasks[i];

    restoreDeletedTasksSnapshot(state.deletedTasks);
    restoreQueueSnapshot(state.queueSnapshot);
    saveTasks();
}

void printTaskIds(const vector<int>& ids) {
    cout << "Affected Task IDs: ";

    if (ids.empty()) {
        cout << "Not recorded";
    } else {
        for (int id : ids)
            cout << id << " ";
    }

    cout << '\n';
}

void saveState(const string& operation) {
    pushState(undoStack, undoTop, createState(operation));
    redoTop = -1;
}

void saveState(const string& operation,
               const vector<int>& taskIds) {           
    pushState(undoStack, undoTop,
              createState(operation, taskIds));
    redoTop = -1;
}

bool canUndo() {
    return undoTop >= 0;
}

bool canRedo() {
    return redoTop >= 0;
}

void undoOperation() {
    undoMultiple(1);
}

void redoOperation() {
    redoMultiple(1);
}

void undoMultiple(int n) {
    if (n <= 0 || n > undoTop + 1) {
        cout << "Invalid number of operations to undo.\n";
        return;
    }

    for (int i = 0; i < n; i++) {
        TaskState previous = undoStack[undoTop];

        TaskState current = createState(
            previous.operation, previous.taskIds
        );

        pushState(redoStack, redoTop, current);
        undoTop--;

        restoreState(previous);
        syncWorkflowWithTasks();
        synchronizeAVLWithTasks();
        synchronizeGraphWithTasks();

        recordUndoRedoWorkflow(
            "UNDO: " + previous.operation,
            previous.taskIds
        );

        cout << "\nUndone: " << previous.operation << '\n';
        printTaskIds(previous.taskIds);
    }

    cout << "\nUpdated task list after Undo:\n";
    displayTasks();

    cout << "\nUpdated deleted-task history:\n";
    displayDeletedTasks();
}

void redoMultiple(int n) {
    if (n <= 0 || n > redoTop + 1) {
        cout << "Invalid number of operations to redo.\n";
        return;
    }

    for (int i = 0; i < n; i++) {
        TaskState next = redoStack[redoTop];

        TaskState current = createState(
            next.operation, next.taskIds
        );

        pushState(undoStack, undoTop, current);
        redoTop--;

        restoreState(next);
        syncWorkflowWithTasks();
        synchronizeAVLWithTasks();
        synchronizeGraphWithTasks();

        recordUndoRedoWorkflow(
            "REDO: " + next.operation,
            next.taskIds
        );

        cout << "\nRedone: " << next.operation << '\n';
        printTaskIds(next.taskIds);
    }

    cout << "\nUpdated task list after Redo:\n";
    displayTasks();

    cout << "\nUpdated deleted-task history:\n";
    displayDeletedTasks();
}
