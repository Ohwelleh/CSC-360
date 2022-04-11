#ifndef QUEUE_H
#define QUEUE_H

typedef struct NodeData{
    int id;
    int arrivalTime;
    int serviceTime;
    double waitingTime;
    struct NodeData* next;
}NodeData;

typedef struct Queue{
    struct NodeData *start, *end;
}Queue;

Queue* initQueue();
void enQueue(Queue* queueRef, int custID, int custArrival, int custService, double custWait);
NodeData* deQueue(Queue* queueRef);
int isEmpty(Queue* queueRef);
int getQueueLength(Queue* queueRef);
void freeNodeMemory(NodeData* freeNode);

#endif