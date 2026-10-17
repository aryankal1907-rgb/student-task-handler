#ifndef QUEUE_H
#define QUEUE_H

#include <vector>

void enqueueMultiple();
void enqueueTaskAutomatically(int id);
void processMultiple();
void displayQueue();
void clearQueue();
void queueMenu();
void displayNextTask();

void removeTaskFromQueue(int id);
void refreshQueue();
void initializeQueueFromTasks();

std::vector<int> getQueueSnapshot();
void restoreQueueSnapshot(const std::vector<int>& saved);

#endif