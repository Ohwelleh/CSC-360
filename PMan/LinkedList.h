#ifndef LINKEDLIST_H
#define LINKEDLIST_H

// Linked list data structure to record background processes information.
typedef struct Node{
    int processID;
    int isProcessRunning;
    char processName[64];
    struct Node* next;
}Node;

void initializeLinkedListHead();
void appendNode(char* newProcessName, int pid);
void deleteNode(int pidToDelete);
void changeProcessStatus(int pid, int newStatus);
int doesProcessExist(int pid);
void numberOfProcess();
void zombieHunter();
void killAllProcesses();
int checkRunningStatus(int pid);

#endif
