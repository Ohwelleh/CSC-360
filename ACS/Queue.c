#include <stdlib.h>
#include <stdio.h>

// Struct for storing the data of each node.
typedef struct NodeData{

    int id;
    int arrivalTime;
    int serviceTime;
    double waitingTime;
    struct NodeData* next;

}NodeData;

// Struct for queue storing a start and end pointer.
typedef struct Queue{

    struct NodeData *start, *end;
    int length;

}Queue;

// Constants
#define TRUE 1
#define FALSE 0

// Prototype.
int isEmpty(Queue* queueRef);


/*
    Function: initQueue
    --------------------------------
    Initializes the queue with start and end pointers pointing to NULL.

    return: A pointer to the Queue.

*/
Queue* initQueue(){

    // Allocate space in memory for the Queue.
    Queue* newQueue = (Queue*)malloc(sizeof(Queue));

    // Initializing start and end to NULL.
    newQueue->start = newQueue->end = NULL;
    newQueue->length = 0;

    // Return a pointer to the Queue.
    return newQueue;

}//initQueue

/*
    Function: enQueue
    --------------------------------
    Allocates memory for a new node and fills the node with the passed data and enQueues it into the passed queue pointer.

*/
void enQueue(Queue* queueRef, int custID, int custArrival, int custService, double custWait){

    // Allocate space in memory for a new node.
    NodeData *newNode = (NodeData*)malloc(sizeof(NodeData));

    // Add data to the new node.
    newNode->id = custID;
    newNode->arrivalTime = custArrival;
    newNode->serviceTime = custService;
    newNode->waitingTime = custWait;
    newNode->next = NULL;

    // Check if Queue is empty.
    if(isEmpty(queueRef)){

        // Set start pointer to newNode and end pointer to start.
        queueRef->start = newNode;
        queueRef->end = queueRef->start;
        queueRef->length++;

        return;
    }//if

    // Add new node to the end of queue.
    queueRef->end->next = newNode;
    queueRef->end = newNode;
    queueRef->length++;

}//enQueue

/*
    Function: deQueue
    --------------------------------
    Removes the node at the front of the queue and frees the memory the node was allocated.

    return: Node

*/
NodeData* deQueue(Queue* queueRef){

    // Check if queue is empty before trying to remove something from NULL.
    if(isEmpty(queueRef)){

        return NULL;

    }//if

    // Temporary NodeData pointer for storing the current start node.
    NodeData* temp = queueRef->start;

    // Move the start pointer to the next node.
    queueRef->start = queueRef->start->next;
    queueRef->length--;

    // Check for if this deQueue operation will make the queue empty, if so adjust the end pointer and set the length to 0.
    if(queueRef->start == NULL){

        queueRef->end = queueRef->start;
        queueRef->length = 0;

    }//if

    return temp;

}//dequeue

/*
    Function: freeNode
    --------------------------------
    Frees the memory allocated to a node.
    
*/
void freeNodeMemory(NodeData* freeNode){
    
    // Free the memory that was allocated to freeNode.
    free(freeNode);

}//freeNodeMemeory

/*
    Function: isEmpty
    --------------------------------
    Checks if the queue is empty.
    
    return: TRUE or FALSE.
*/
int isEmpty(Queue* queueRef){

    if(queueRef->start == NULL || queueRef->end == NULL){

        return TRUE;

    }//if

    return FALSE;

}//isEmpty

/*
    Function: getQueueLength
    --------------------------------
    Returns the length of the queue.
    
    return: length
*/
int getQueueLength(Queue* queueRef){

    return queueRef->length;

}//getQueueLength