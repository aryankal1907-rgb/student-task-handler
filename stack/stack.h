
#ifndef STACK_H
#define STACK_H

#include <string>
#include <vector>
using namespace std;

void saveState(const string& operation);
void saveState(const string& operation, const vector<int>& taskIds);

void undoOperation();
void redoOperation();
void undoMultiple(int n);
void redoMultiple(int n);

bool canUndo();
bool canRedo();

#endif
