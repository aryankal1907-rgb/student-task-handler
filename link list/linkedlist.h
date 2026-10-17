
#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include "../tasks/task.h"
#include <string>
#include <vector>

void LinkedListMenu();
void initializeWorkflow();
void addTaskToWorkflow(int id);
void removeTaskFromWorkflow(int id);
void displayWorkflow();

void recordTaskAdded(const Task& task);
void recordTaskDeleted(const Task& task);
void recordTaskRestored(const Task& task);
void recordTaskCompleted(const Task& task);
void recordTaskUpdated(const Task& task);

void syncWorkflowWithTasks();

void recordUndoRedoWorkflow(
    const std::string& action,
    const std::vector<int>& ids
);

#endif
