
#include "hashing.h"
#include "../tasks/task.h"
#include <iostream>
#include <limits>
#include <vector>
using namespace std;

const int TABLE_SIZE = 101;

void searchTaskById() {
    if (taskCount == 0) {
        cout << "No tasks available to search.\n";
        return;
    }

    vector<int> hashTable[TABLE_SIZE];

    for (int i = 0; i < taskCount; i++) {
        int index = tasks[i].id % TABLE_SIZE;
        hashTable[index].push_back(i);
    }

    int id;
    cout << "Enter Task ID to search: ";

    if (!(cin >> id)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid Task ID.\n";
        return;
    }

    if (id <= 0) {
        cout << "Task ID must be positive.\n";
        return;
    }

    int index = id % TABLE_SIZE;

    for (int position : hashTable[index]) {
        if (tasks[position].id == id) {
            cout << "\n========== TASK FOUND ==========\n";
            cout << "Task ID: " << tasks[position].id << '\n';
            cout << "Type: " << tasks[position].type << '\n';
            cout << "Subject: " << tasks[position].subject << '\n';
            cout << "Task: " << tasks[position].name << '\n';
            cout << "Description: " << tasks[position].name<<'\n';
            cout << "Deadline: " << tasks[position].deadline << '\n';
            cout << "Priority: " << tasks[position].priority << '\n';
            cout << "Status: "
                 << (tasks[position].completed ? "Completed" : "Pending")
                 << '\n';
            return;
        }
    }

    cout << "Task ID not found.\n";
}
