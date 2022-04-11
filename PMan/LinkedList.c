/*
 **********************************************
 * 
 *          CSC 360 Operating Systems
 *  THIS HEADER WAS CREATED FOR THE FOLLOWING ASSIGNMENT:
 *    Assignment 1: A Process Manager (PMan)
 * 
 *          Created By: Austin Bassett
 *                  V00905244
 * 
 *********************************************
*/

// Libraries
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>     // kill(), SIGTERM, SIGKILL, SIGSTOP, SIGCONT
#include <sys/wait.h>   // waitpid()
#include <unistd.h>

// Linked list data structure to record background processes information.
typedef struct Node{
    int processID;
    int isProcessRunning;
    char processName[64];
    struct Node* next;
}Node;

// Constants.
#define TRUE 1
#define FALSE 0

// Global reference to head.
Node* head;

/*
 * Function initializeLinkedListHead()
 * ---------------------------------------------------------
 *      Initialized head to NULL, thus creating a new linked list. 
 * 
*/
void initializeLinkedListHead(){

    head = NULL;

}//initializeLinkedListHead

/*
 * Function appendNode(char* newProcessName, int newPID)
 * ---------------------------------------------------------
 *      Adds a new node to the end of the linked list.
 *      
 *    newProcessName: pointer to the new process name.
 *    newPID: Interger value of the new process ID.
 * 
*/
void appendNode(char* newProcessName, int newPID){

    // Allocating memory for new node.
    Node* newNode = (Node*) malloc(sizeof(Node));

    // Temp node pointer for traversing the linked list.
    Node* temp = head;

    // Informing the user the process is starting.
    printf("The process: %s with process ID: %d ", newProcessName, newPID);
    printf("will now start in the background.\n");

    // Filling out the data fields of the new node.
    newNode->processID = newPID;
    strcpy(newNode->processName, newProcessName);
    newNode->isProcessRunning = TRUE;
    newNode->next = NULL;

    // Checking if linked list is empty.
    if(head == NULL){

        head = newNode;

    }else{
        
        // Linked list is not empty, traverse until last node.
        while(temp->next != NULL){

            // Set temp to the next node.
            temp = temp->next;

        }//while

        // Add new Node to linked list.
        temp->next = newNode;

    }//if-else block

    return;
}//appendNode

/*
 * Function deleteNode(int pidToDelete)
 * ---------------------------------------------------------
 *      Deletes a Node from the linked list.
 *      
 *    pidToDelete: The processes Integer ID.
 * 
*/
void deleteNode(int pidToDelete){

    // Create two temporary Nodes for traversing the linked list.
    Node* temp = head;
    Node* prev;

    // Check if the process is the head.
    if(temp != NULL && temp->processID == pidToDelete){

        head = temp->next;
        free(temp);
        return;

    }//if

    // Otherwise, search the linked list for the process ID that matches.
    while(temp != NULL && temp->processID != pidToDelete){

        prev = temp;
        temp = temp->next;

    }//while()

    // Process was not found.
    if(temp == NULL){

        printf("Process ID: %d was not found\n", pidToDelete);
        return;

    }//if

    // Delete the node.
    prev->next = temp->next;
    
    // Free the allocated memory.
    free(temp);
    return;

}//deleteNode

/*
 * Function: changeProcessStatus()
 * -------------------------------
 *      Changes the isProcessRunning attribute to either TRUE or FALSE
 * 
*/
void changeProcessStatus(int pid, int newStatus){

    // Temp node pointer for traversal of linked list.
    Node* temp = head;

    // Variables.
    int processFound = FALSE;

    // Searching to see if the passed pid is in the linked list of processes.
    while(temp != NULL){

        if(temp->processID == pid){
            processFound = TRUE;
            break;
        }//if

        // Set temp to the next node.
        temp = temp->next;

    }//while

    // If found change isProcessRunning otherwise print the process was not found.
    if(processFound){

        temp->isProcessRunning = newStatus;
        return;

    }else{

        printf("The process with ID: %d was not found.\n", pid);
        return;

    }//if-else block

}//changeProcessStatus

/*
 * Function: numberOfProcess()
 * -------------------------------
 *      Iterates over the linked list and prints out the process ID, name and total background processes that are running.
 * 
*/
void numberOfProcess(){

    // Creating a temp Node pointer for traversing the linked list.
    Node* temp = head;

    // Variable.
    int totalJobs = 0;

    // Traverse the linked list output the process name and process ID.
    while(temp != NULL){

        // Only increase the total jobs running in background, if the process is running.
        if(temp->isProcessRunning == TRUE){

            printf("%d: %s\n", temp->processID, temp->processName);
            totalJobs++;

        }else{

            // Indicating a process is temporarily stopped, incase the user would liked to start it again.
            printf("%d: %s (stopped)\n", temp->processID, temp->processName);

        }//if-else
        
        // Set temp to the next node.
        temp = temp->next;
    }//while

    printf("Total running background jobs: %d\n", totalJobs);
}//numberOfProcess

/*
 * Function: zombieHunter()
 * -------------------------------
 *      Checks for zombie processes and kills them.
 * 
 *     Note: this function was provided on connex CSC 360 prof: Kui Wu. Under A1_sampleCode.c
*/
void zombieHunter(){

    // Variables.
    int status;
    int returnValue = 0;

    while(TRUE){

        usleep(1000);

        if(head == NULL){
            return;
        }//if

        returnValue = waitpid(-1, &status, WNOHANG);

        if(returnValue > 0){

            // Remove from linked list.
            deleteNode(returnValue);

        }else if(returnValue == 0){

            break;

        }else{

            perror("Waitpid Failed");
            exit(EXIT_FAILURE);

        }//if-else block
        
    }//while

}//zombieHunter

/*
 * Function: killAllProcesses()
 * -------------------------------
 *      Terminates all running processes and frees all allocated memory.
 * 
*/
void killAllProcesses(){

    // Temporary node pointer for list traversal.
    Node* temp;

    // Iterate over the linked list and kill the process and free the allocated memory.
    while(head != NULL){

        temp = head;
        head = head->next;

        kill(temp->processID, SIGTERM);
        free(temp);

    }//while
    
    sleep(1);

    printf("All processes have been terminated.\n");
    printf("All allocated memory has been freed.\n");

}//killAllProcesses

/*
 * Function: doesProcessExist(int pid)
 * -------------------------------
 *      Checks for if a process exitst.
 *      This functions sole purpose is for Pstat as to save time
 *      if the process does not exist.
 * 
 *     pid: process ID.
*/
int doesProcessExist(int pid){
    
    // Temp node pointer for traveral of the linked list.
    Node* temp = head;

    // Searching to see if the passed PID is currently in the linked list.
    while(temp != NULL){

        if(temp->processID == pid){

            return TRUE;

        }//if

        // Set temp to the next node.
        temp = temp->next;

    }//while

    return FALSE;
}//doesProcessExist

/*
 * Function: checkRunningStatus(int pid)
 * -------------------------------
 *      Checks for if a process is running or not.
 * 
 *     pid: process ID.
*/
int checkRunningStatus(int pid){

    // Variables.
    int processFound = FALSE;

    // Temp node pointer for traversing the linked list.
    Node* temp = head;

    // Searching for the process.
    while(temp != NULL){
        
        // If PID is found break out of while loop.
        if(temp->processID == pid){

            processFound = TRUE;
            break;

        }//if

        // Set temp to the next node.
        temp = temp->next;

    }//wihle

    // Error handling for if the passed pid is not in the linked list.
    if(processFound == FALSE){

        printf("Error: Process: %d does not exist.\n", pid);
        return -1;

    }//if

    // Return the current status of the process.
    return temp->isProcessRunning;

}//checkRunningStatus