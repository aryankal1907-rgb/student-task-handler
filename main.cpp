
#include <iostream>
#include <limits>
#include "tasks/task.h"
#include "arrays/array.h"
#include "hashing/hashing.h"
using namespace std;

int main() {
    initializeTasks();

    int choice = 0;

    do {
        cout << "\n===== Student Task Management System =====\n";
        cout << "1. Add Task\n";
        cout << "2. Display All Tasks\n";
        cout << "3. Mark Task Completed\n";
        cout << "4. Delete Task\n";
        cout << "5. Display Deleted Tasks\n";
        cout << "6. Task Operations\n";
        cout << "7. Search Task by ID \n";
        cout << "8. Restore Deleted Task\n";
        cout << "9. Exit\n";
        cout << "Enter choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Enter a number.\n";
            continue;
        }

        switch (choice) {
            case 1:
                addTask();
                break;

            case 2:
                displayTasks();
                break;

            case 3:
                markTaskCompleted();
                break;

            case 4:
                deleteTask();
                break;

            case 5:
                displayDeletedTasks();
                break;

            case 6:
                arrayMenu();
                break;

            case 7:
                searchTaskById();
                break;

            case 8:
                restoreDeletedTask();
                break;

            case 9:
                cout << "Tasks saved. Exiting application.\n";
                break;

            default:
                cout << "Invalid choice.\n";
        }

    } while (choice != 9);

    return 0;
}
