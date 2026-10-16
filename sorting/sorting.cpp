
#include "sorting.h"
#include "../tasks/task.h"
#include <vector>

using namespace std;

bool comesBefore(const Task& a, const Task& b) {
    if (a.priority != b.priority)
        return a.priority < b.priority;

    int daysA = daysRemaining(a.deadline);
    int daysB = daysRemaining(b.deadline);

    if (daysA != daysB)
        return daysA < daysB;

    return false;
}


void mergeTasks(int left, int mid, int right, Task temp[]) {
    int i = left;
    int j = mid + 1;
    int k = left;

    while (i <= mid && j <= right) {
        if (comesBefore(tasks[i], tasks[j]))
            temp[k++] = tasks[i++];
        else
            temp[k++] = tasks[j++];
    }

    while (i <= mid)
        temp[k++] = tasks[i++];

    while (j <= right)
        temp[k++] = tasks[j++];

    for (int p = left; p <= right; p++)
        tasks[p] = temp[p];
}

void mergeSortTasks(int left, int right, Task temp[]) {
    if (left >= right)
        return;

    int mid = left + (right - left) / 2;

    mergeSortTasks(left, mid, temp);
    mergeSortTasks(mid + 1, right, temp);
    mergeTasks(left, mid, right, temp);
}

void sortTasks() {
    if (taskCount <= 1)
        return;

    Task temp[100];
    mergeSortTasks(0, taskCount - 1, temp);
}
