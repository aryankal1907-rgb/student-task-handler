#ifndef TASK_H
#define TASK_H

#include <string>

struct Task {
    int id;
    std::string type;
    std::string name;
    std::string deadline;
    int priority;
    bool completed;
};

extern Task tasks[100];
extern int taskCount;

void initializeTasks();
void addTask();
void displayTasks();
void displayDeletedTasks();
void markTaskCompleted();
void deleteTask();
void updateTask();
int daysRemaining(const std::string& deadline);
void restoreDeletedTask();
void sortTasks();

#endif