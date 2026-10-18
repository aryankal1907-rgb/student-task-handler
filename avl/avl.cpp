
#include "avl.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <cstdio>
#include <limits>

using namespace std;

extern Task tasks[100];
extern int taskCount;

struct AVLNode {
    int deadlineKey;
    int taskId;
    int height;
    AVLNode* left;
    AVLNode* right;

    AVLNode(int date, int id) {
        deadlineKey = date;
        taskId = id;
        height = 1;
        left = nullptr;
        right = nullptr;
    }
};

static AVLNode* root = nullptr;

static int getHeight(AVLNode* node) {
    return node ? node->height : 0;
}

static int getBalance(AVLNode* node) {
    return node
        ? getHeight(node->left) - getHeight(node->right)
        : 0;
}

static void updateHeight(AVLNode* node) {
    if (node) {
        int leftHeight = getHeight(node->left);
        int rightHeight = getHeight(node->right);

        node->height = 1 +
            (leftHeight > rightHeight ? leftHeight : rightHeight);
    }
}

static bool smallerKey(int date1, int id1, int date2, int id2) {
    if (date1 != date2)
        return date1 < date2;

    return id1 < id2;
}

static AVLNode* rotateRight(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* temp = x->right;

    x->right = y;
    y->left = temp;

    updateHeight(y);
    updateHeight(x);

    return x;
}

static AVLNode* rotateLeft(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* temp = y->left;

    y->left = x;
    x->right = temp;

    updateHeight(x);
    updateHeight(y);

    return y;
}

static AVLNode* insertNode(AVLNode* node, int date, int id) {
    if (!node)
        return new AVLNode(date, id);

    if (smallerKey(date, id, node->deadlineKey, node->taskId)) {
        node->left = insertNode(node->left, date, id);
    } else if (smallerKey(
                   node->deadlineKey, node->taskId, date, id)) {
        node->right = insertNode(node->right, date, id);
    } else {
        return node;
    }

    updateHeight(node);

    int balance = getBalance(node);

    if (balance > 1 &&
        smallerKey(date, id,
                   node->left->deadlineKey, node->left->taskId))
        return rotateRight(node);

    if (balance < -1 &&
        smallerKey(node->right->deadlineKey, node->right->taskId,
                   date, id))
        return rotateLeft(node);

    if (balance > 1 &&
        smallerKey(node->left->deadlineKey, node->left->taskId,
                   date, id)) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }

    if (balance < -1 &&
        smallerKey(date, id,
                   node->right->deadlineKey, node->right->taskId)) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node;
}

static void clearTree(AVLNode* node) {
    if (!node)
        return;

    clearTree(node->left);
    clearTree(node->right);
    delete node;
}

static int findTaskIndex(int id) {
    for (int i = 0; i < taskCount; i++) {
        if (tasks[i].id == id)
            return i;
    }

    return -1;
}

static bool deadlineToKey(const string& date, int& key) {
    if (date.size() != 10 ||
        date[2] != '-' || date[5] != '-')
        return false;

    int day, month, year;

    if (sscanf(date.c_str(), "%d-%d-%d",
               &day, &month, &year) != 3)
        return false;

    if (month < 1 || month > 12 ||
        day < 1 || day > 31 || year < 1900)
        return false;

    tm value = {};
    value.tm_mday = day;
    value.tm_mon = month - 1;
    value.tm_year = year - 1900;
    value.tm_hour = 12;
    value.tm_isdst = -1;

    time_t timestamp = mktime(&value);

    if (timestamp == -1)
        return false;

    tm* checked = localtime(&timestamp);

    if (!checked ||
        checked->tm_mday != day ||
        checked->tm_mon != month - 1 ||
        checked->tm_year != year - 1900)
        return false;

    key = year * 10000 + month * 100 + day;
    return true;
}

static void displayRange(
    AVLNode* node, int startKey, int endKey, int& count
) {
    if (!node)
        return;

    if (node->deadlineKey >= startKey)
        displayRange(node->left, startKey, endKey, count);

    if (node->deadlineKey >= startKey &&
        node->deadlineKey <= endKey) {
        int index = findTaskIndex(node->taskId);

        if (index != -1) {
            const Task& task = tasks[index];

            cout << left
                 << setw(7) << task.id
                 << setw(18) << task.subject
                 << setw(22) << task.name
                 << setw(15) << task.deadline
                 << setw(12)
                 << (task.priority == 1 ? "High" :
                     task.priority == 2 ? "Medium" : "Low")
                 << (task.completed ? "Completed" : "Pending")
                 << '\n';

            count++;
        }
    }

    if (node->deadlineKey <= endKey)
        displayRange(node->right, startKey, endKey, count);
}

static int countRange(
    AVLNode* node, int startKey, int endKey
) {
    if (!node)
        return 0;

    if (node->deadlineKey < startKey)
        return countRange(node->right, startKey, endKey);

    if (node->deadlineKey > endKey)
        return countRange(node->left, startKey, endKey);

    return 1 +
        countRange(node->left, startKey, endKey) +
        countRange(node->right, startKey, endKey);
}

static void findHighestPriority(
    AVLNode* node, int startKey, int endKey,
    int& bestId, int& bestPriority
) {
    if (!node)
        return;

    if (node->deadlineKey >= startKey)
        findHighestPriority(
            node->left, startKey, endKey, bestId, bestPriority
        );

    if (node->deadlineKey >= startKey &&
        node->deadlineKey <= endKey) {
        int index = findTaskIndex(node->taskId);

        if (index != -1 &&
            (bestId == -1 ||
             tasks[index].priority < bestPriority)) {
            bestId = tasks[index].id;
            bestPriority = tasks[index].priority;
        }
    }

    if (node->deadlineKey <= endKey)
        findHighestPriority(
            node->right, startKey, endKey, bestId, bestPriority
        );
}

static bool findIdInRange(
    AVLNode* node, int startKey, int endKey, int id
) {
    if (!node)
        return false;

    if (node->deadlineKey >= startKey &&
        findIdInRange(node->left, startKey, endKey, id))
        return true;

    if (node->deadlineKey >= startKey &&
        node->deadlineKey <= endKey &&
        node->taskId == id)
        return true;

    if (node->deadlineKey <= endKey &&
        findIdInRange(node->right, startKey, endKey, id))
        return true;

    return false;
}

void synchronizeAVLWithTasks() {
    clearTree(root);
    root = nullptr;

    for (int i = 0; i < taskCount; i++) {
        int key;

        if (deadlineToKey(tasks[i].deadline, key))
            root = insertNode(root, key, tasks[i].id);
    }
}

void initializeAVL() {
    synchronizeAVLWithTasks();
}

void avlMenu() {
    synchronizeAVLWithTasks();

    int choice;

    do {
        cout << "\n===== AVL DEADLINE RANGE OPERATIONS =====\n";
        cout << "1. Display tasks within a deadline range\n";
        cout << "2. Count tasks within a deadline range\n";
        cout << "3. Find highest-priority task within a range\n";
        cout << "4. Search Task ID within a deadline range\n";
        cout << "5. Return to main menu\n";
        cout << "Enter choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input.\n";
            continue;
        }

        if (choice == 5)
            break;

        if (choice < 1 || choice > 5) {
            cout << "Invalid choice.\n";
            continue;
        }

        string startDate, endDate;
        int startKey, endKey;

        cout << "Enter start deadline (DD-MM-YYYY): ";
        cin >> startDate;

        cout << "Enter end deadline (DD-MM-YYYY): ";
        cin >> endDate;

        if (!deadlineToKey(startDate, startKey) ||
            !deadlineToKey(endDate, endKey)) {
            cout << "Invalid date. Use DD-MM-YYYY.\n";
            continue;
        }

        if (startKey > endKey) {
            cout << "Start deadline must not be after end deadline.\n";
            continue;
        }

        cout << "\n===== SELECTED DEADLINE RANGE =====\n";
        cout << "From: " << startDate
             << " To: " << endDate << "\n\n";

        int rangeCount = 0;

        cout << left
             << setw(7) << "ID"
             << setw(18) << "Subject"
             << setw(22) << "Description"
             << setw(15) << "Deadline"
             << setw(12) << "Priority"
             << "Status\n";

        cout << string(96, '-') << '\n';

        displayRange(root, startKey, endKey, rangeCount);

        if (rangeCount == 0)
            cout << "No tasks found in this range.\n";

        cout << "\nTotal tasks in range: " << rangeCount << '\n';

        if (choice == 1) {
            cout << "Displayed all tasks in the selected range.\n";
        } else if (choice == 2) {
            cout << "Count of tasks in this range: "
                 << countRange(root, startKey, endKey) << '\n';
        } else if (choice == 3) {
            int bestId = -1;
            int bestPriority = 4;

            findHighestPriority(
                root, startKey, endKey, bestId, bestPriority
            );

            if (bestId == -1) {
                cout << "No tasks found in this range.\n";
            } else {
                int index = findTaskIndex(bestId);

                cout << "\nHighest-priority task:\n";
                cout << "ID: " << tasks[index].id << '\n';
                cout << "Subject: " << tasks[index].subject << '\n';
                cout << "Description: " << tasks[index].name << '\n';
                cout << "Deadline: " << tasks[index].deadline << '\n';
                cout << "Priority: "
                     << (bestPriority == 1 ? "High" :
                         bestPriority == 2 ? "Medium" : "Low")
                     << '\n';
                cout << "Status: "
                     << (tasks[index].completed
                         ? "Completed" : "Pending") << '\n';
            }
        } else if (choice == 4) {
            int id;

            cout << "\nSearch Task ID within "
                 << startDate << " to " << endDate << '\n';
            cout << "Enter Task ID to search: ";

            if (!(cin >> id)) {
                cin.clear();
                cin.ignore(
                    numeric_limits<streamsize>::max(), '\n'
                );
                cout << "Invalid Task ID.\n";
                continue;
            }

            if (findIdInRange(root, startKey, endKey, id)) {
                int index = findTaskIndex(id);

                if (index != -1) {
                    cout << "\nTask found in the selected range:\n";
                    cout << "ID: " << tasks[index].id << '\n';
                    cout << "Subject: " << tasks[index].subject << '\n';
                    cout << "Description: "
                         << tasks[index].name << '\n';
                    cout << "Deadline: "
                         << tasks[index].deadline << '\n';
                    cout << "Priority: "
                         << (tasks[index].priority == 1 ? "High" :
                             tasks[index].priority == 2 ? "Medium" : "Low")
                         << '\n';
                    cout << "Status: "
                         << (tasks[index].completed
                             ? "Completed" : "Pending") << '\n';
                }
            } else {
                cout << "Task ID not found within this deadline range.\n";
            }
        }

    } while (true);
}
