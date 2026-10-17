
#include <iostream>
#include <limits>
#include "tasks/task.h"
#include "arrays/array.h"
#include "hashing/hashing.h"
#include "link list/linkedlist.h"
#include "stack/stack.h"
#include "queue/queue.h"
#include "avl/avl.h"
#include "graphs/graph.h"
using namespace std;

int main() {
    initializeTasks();
    initializeWorkflow();
    initializeAVL();
    initializeGraph();

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
        cout << "9. Task Workflow\n";
        cout<< "10.Undo operation\n";
        cout<< "11.Redo operation\n";
        cout<<"12. Display Next Task\n";
        cout<<"13. Display Tasks by Deadline Range\n";
        cout<<"14. Display Graph\n";
        cout << "0. Exit\n";
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
                displayTasks();
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
                LinkedListMenu();
                break;
            
            case 10:
            if(!canUndo()){
                cout << "No operations to undo.\n";
                break;
            }
            int n;
            cout << "Enter number of operations to undo: ";
                if (!(cin >> n)) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Invalid input. Enter a number.\n";
                    break;
                }
                undoMultiple(n);
                break;

            case 11:
            if(!canRedo()){
                cout << "No operations to redo.\n";
                break;
            }
            int m;
                cout << "Enter number of operations to redo: ";
                if (!(cin >> m)) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Invalid input. Enter a number.\n";
                    break;
                }
                redoMultiple(m);
                break;
            
            case 12:
                displayNextTask();
                displayQueue();
                break;
            
            case 13:
                avlMenu();
                break;

            case 14:
                graphMenu();
                break;

            case 0:
                cout << "Tasks saved. Exiting program.\n";
                break;   

            default:
                cout << "Invalid choice.\n";
        }

    } while (choice != 0);

    return 0;
}
