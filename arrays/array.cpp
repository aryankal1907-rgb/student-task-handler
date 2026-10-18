
#include "array.h"
#include "../tasks/task.h"
#include <iostream>
#include <limits>
using namespace std;

void arrayMenu() {
    int choice;

    do {
        cout << "\n===== Array-Based Task Management =====\n";
        cout << "1. Display All Tasks\n";
        cout << "2. Add Task\n";
        cout << "3. Delete Task\n";
        cout << "4. Update Task\n";
        cout << "5. Display Pending Tasks\n";
        cout << "6. Display Completed Tasks\n";
        cout << "7. Find Earliest Deadline\n";
        cout << "8. Count Tasks by Priority\n";
        cout << "9. Return to Main Menu\n";
        cout << "Enter choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input.\n";
            continue;
        }

        switch (choice) {
            case 1:
                displayTasks();
                break;

            case 2:
                addTask();
                break;

            case 3:
                deleteTask();
                break;

            case 4:
                displayTasks();
                updateTask();
                break;

            case 5: {
                bool found = false;

                for (int i = 0; i < taskCount; i++) {
                    if (!tasks[i].completed) {
                        cout << "ID: " << tasks[i].id
                            <<"Subject: " << tasks[i].subject
                             << " | " << tasks[i].name
                             << " | Deadline: " << tasks[i].deadline
                             << " | Priority: " << tasks[i].priority
                             << '\n';
                        found = true;
                    }
                }

                if (!found)
                    cout << "No pending tasks.\n";

                break;
            }

            case 6: {
                bool found = false;

                for (int i = 0; i < taskCount; i++) {
                    if (tasks[i].completed) {
                        cout << "ID: " << tasks[i].id
                            <<"Subject: " << tasks[i].subject
                             << " | " << tasks[i].name
                             << " | Deadline: " << tasks[i].deadline
                             << '\n';
                        found = true;
                    }
                }

                if (!found)
                    cout << "No completed tasks.\n";

                break;
            }

            case 7: {
                if (taskCount == 0) {
                    cout << "No tasks available.\n";
                    break;
                }

                int earliest = 0;

                for (int i = 1; i < taskCount; i++) {
                    int d1, m1, y1, d2, m2, y2;

                    sscanf(tasks[i].deadline.c_str(),
                           "%d-%d-%d", &d1, &m1, &y1);

                    sscanf(tasks[earliest].deadline.c_str(),
                           "%d-%d-%d", &d2, &m2, &y2);

                    if (y1 < y2 ||
                        (y1 == y2 && m1 < m2) ||
                        (y1 == y2 && m1 == m2 && d1 < d2))
                        earliest = i;
                }

                cout << "Task with earliest deadline:\n";
                cout << "ID: " << tasks[earliest].id
                     << "\nSubject: " << tasks[earliest].subject
                     << "\nTask: " << tasks[earliest].name
                     << "\nDescription: " << tasks[earliest].name
                     << "\nDeadline: " << tasks[earliest].deadline
                     << '\n';
                break;
            }

            case 8: {
                int high = 0, medium = 0, low = 0;

                for (int i = 0; i < taskCount; i++) {
                    if (tasks[i].priority == 1)
                        high++;
                    else if (tasks[i].priority == 2)
                        medium++;
                    else if (tasks[i].priority == 3)
                        low++;
                }

                cout << "High priority: " << high << '\n';
                cout << "Medium priority: " << medium << '\n';
                cout << "Low priority: " << low << '\n';
                break;
            }

            case 9:
                cout << "Returning to main menu.\n";
                break;

            default:
                cout << "Invalid choice.\n";
        }

    } while (choice != 9);
}
