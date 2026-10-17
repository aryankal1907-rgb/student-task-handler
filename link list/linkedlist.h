#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include "../tasks/task.h"

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

#endif