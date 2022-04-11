/****************************************************
 *
 *      Assignment 2: Airline Check-in System
 *              CSC 360, Prof: Kui Wu
 *  
 * 
 *          Developed by: Austin Bassett
 *                  V00905244
 * 
 ****************************************************/

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "Queue.h"

// Constaints
#define TRUE 1
#define FALSE 0
#define MAXCLERKS 4
#define USLEEPCONVERTER 100000
#define TIMEMICROCONVERTER 1000000
#define MAXMUTEX 4

// Prototype
void clerkServer();

// Global variables.
int freeClerks = MAXCLERKS;     // Integer for tracking the number of available clerks.
int busiWaitIndex = 0;          // Integer for tracking the next free position in the busiWaitTime array.
int econWaitIndex = 0;          // Integer for tracking the next free position in the econWaitTime array.
int allWaitIndex = 0;           // Integer for tracking the next free position in the allWaitTime array.

char clerksStatus[MAXCLERKS] = {'A', 'A', 'A', 'A'};  // Char array for clerks available. A = Available, B = Busy helping.

double busiWaitTimes[32];   // Double array for tracking the wait times of the Business customers.
double econWaitTimes[32];   // Double array for tracking the wait times of the Economy customers.
double allWaitTimes[64];    // Double array for tracking the wait times of all the customers.
double simStartTime = 0.0;  // Double for storing the calculated start time fo the simulation.

struct timeval startTime;   // Struct pointer for the start time of the simulation.

// Global variables for mutex and conditional variables.
pthread_mutex_t clerkMutex;
pthread_mutex_t clerkStatusMutex;
pthread_mutex_t busiQueue;
pthread_mutex_t econQueue;

pthread_cond_t freeClerkConditional;

// Global pointers to the Queues.
Queue *custEconomy;
Queue *custBusiness;

// Struct for storing the customers raw data. (This struct was derived from the A2_hint_detailed.c provided on connex)
typedef struct customerRaw{

    int id;
    int class;
    int arrival;
    int service;

}customerRaw;

//************** Assignment Functions ****************

/*
    Function: customerThread
    --------------------------------
    This function is used for all the created cutomer threads.
    Contains the critical sections for the thread.

    Credit: Parts of this code were provided in the A2-hint-detailed.c on Connex.
*/
void *customerThread(void *customerDetails){

    // Variables
    double startWait = 0.0; // Double for storing the calculated start time the customer will be waiting.

    // Initialize a struct pointer for start time.
    struct timeval waitTime;

    // Initializing a struct pointer to the passed data.
    customerRaw *customerInfo = (customerRaw*) customerDetails;
    
    // Sleep for the as long as the customer's arrival time.
    usleep(customerInfo->arrival * USLEEPCONVERTER);

    // Print out that the customer has arrived.
    printf("A customer arrives: their customer ID is %d. \n", customerInfo->id);

    // Enqueue the customer to either economy class (0) or business class (1).
    if(customerInfo->class == 1){

        // Lock the following section of code from other business threads.
        pthread_mutex_lock(&busiQueue);

        // Get current time.
        gettimeofday(&waitTime, NULL);
        startWait = (waitTime.tv_sec + (double) waitTime.tv_usec / TIMEMICROCONVERTER) - simStartTime; // Formula is derived from same_gettimeofday.c provided on connex

        // Place a customer in the business queue.
        enQueue(custBusiness, customerInfo->id, customerInfo->arrival, customerInfo->service, startWait);

        // Print out the Queue and length of queue.
        printf("A customer enters a queue: the queue ID %d, and length of the queue is %d\n", customerInfo->class, getQueueLength(custBusiness));

        // Unlock this section of code for another thread to access.
        pthread_mutex_unlock(&busiQueue);
        

    }else{

        // Lock the following section of code from other economy threads.
        pthread_mutex_lock(&econQueue);

        // Get current time.
        gettimeofday(&waitTime, NULL);
        startWait = (waitTime.tv_sec + (double) waitTime.tv_usec / TIMEMICROCONVERTER) - simStartTime;  // Formula is derived from same_gettimeofday.c provided on connex

        // Place a customer in the economy queue.
        enQueue(custEconomy, customerInfo->id, customerInfo->arrival, customerInfo->service, startWait);

        // Print out the Queue and length of queue.
        printf("A customer enters a queue: the queue ID %d, and length of the queue is %d\n", customerInfo->class, getQueueLength(custEconomy));

        // Unlock this section of code for another thread to access.
        pthread_mutex_unlock(&econQueue);

    }//if-else

    // Call to the clerk handling function.
    clerkServer(customerInfo);

    // Exit thread process.
    pthread_exit(0);
    
}//customerThread

//************** Helper Functions ********************

/*
    Function: clerkServer
    --------------------------------
    This function contains the code to simulate customer service of a clerk.

*/
void clerkServer(customerRaw* customerThreadData){

    // Variables.
    int clerkID = 0;                    // Integer for storing the clerk's ID.
    int clerkFinder = 0;                // Integer for finding a clerk.

    double customerWaitingTime = 0.0;   // Double for storing how long the customer has been waiting.
    double startServiceTime = 0.0;      // Double for storing the starting service time.
    double endServiceTime = 0.0;        // Double for storing the end service time.

    // Initializing a struct pointer to a customer node from the queue.
    NodeData *customerNode = NULL;

    // Initializing a struct pointer for getting the current time.
    struct timeval currentTime;

    // Lock the clerkMutex while checking if there are any clerks free.
    pthread_mutex_lock(&clerkMutex);

    // Check if there are any free clerks.
    while(freeClerks == 0){
        
        // No clerks are currently free, wait for a clerk to become free before proceeding.
        if(pthread_cond_wait(&freeClerkConditional, &clerkMutex) != 0){

            // Error handling for if wait fails.
            printf("Error: freeConditional variable wait function failed.\n");
            exit(0);

        }//if

    }//while

    // A clerk was avaiable to help.
    freeClerks--;

    // Unlock the clerkMutex once a clerk becomes free.
    pthread_mutex_unlock(&clerkMutex);

    // Sleep to allow the other threads to reach wait condition.
    usleep(20);

    while(TRUE){

        // If clerk is available, break loop
        if(clerksStatus[clerkFinder] == 'A'){

            pthread_mutex_lock(&clerkStatusMutex);

            // Only want clerk ID's to range from [1, 4].
            clerkID = clerkFinder + 1;

            // Set the clerk's status to busy (B).
            clerksStatus[clerkFinder] = 'B';
            
            pthread_mutex_unlock(&clerkStatusMutex);

            break;

        }else{

            // Increment clerkFinder, meaning a clerk was not available at this index.
            clerkFinder++;

            if(clerkFinder == 4){

                // Reset clerkFinder
                clerkFinder = 0;

            }//if

        }//if-else
    
    }//while
    
    // First check if the business queue is empty, if not dequeue from there.
    if(!isEmpty(custBusiness)){

        // Lock business Queue mutex.
        pthread_mutex_lock(&busiQueue);

        // Get the current time of day.
        gettimeofday(&currentTime, NULL);
        startServiceTime = (currentTime.tv_sec + (double) currentTime.tv_usec / TIMEMICROCONVERTER) - simStartTime; // Formula is derived from same_gettimeofday.c provided on connex

        // Grab a customer fro the business queue.
        customerNode = deQueue(custBusiness);
        
        // Calculate how long the customer has been waiting.
        customerWaitingTime = startServiceTime - customerNode->waitingTime;

        // Store the customers wait time in the respective arrays.
        busiWaitTimes[busiWaitIndex] = customerWaitingTime;
        allWaitTimes[allWaitIndex] = customerWaitingTime;

        busiWaitIndex++;
        allWaitIndex++;
        
        // Unlock business Queue mutex.
        pthread_mutex_unlock(&busiQueue);

    }else if(!isEmpty(custEconomy)){

        // Lock economy Queue mutex.
        pthread_mutex_lock(&econQueue);

        // Get the current time of day.
        gettimeofday(&currentTime, NULL);
        startServiceTime = (currentTime.tv_sec + (double) currentTime.tv_usec / TIMEMICROCONVERTER) - simStartTime; // Formula is derived from same_gettimeofday.c provided on connex

        // Grab a customer from the economy queue.
        customerNode = deQueue(custEconomy);

        // Calculate how long the customer has been waiting.
        customerWaitingTime = startServiceTime - customerNode->waitingTime;

        // Store the customers wait time in the respective arrays.
        econWaitTimes[econWaitIndex] = customerWaitingTime;
        allWaitTimes[allWaitIndex] = customerWaitingTime;
        
        econWaitIndex++;
        allWaitIndex++;

        // Unlock ecnonomy Queue mutex.
        pthread_mutex_unlock(&econQueue);

    }else{

        // Fail safe, shouldn't reach here.
        return;

    }//if-else block

    // Serve the customer.
    printf("A clerk starts serving a customer: start time %.2f seconds, the customer ID %d, the clerk ID %d.\n", startServiceTime, customerNode->id, clerkID);

    // Simulating service time.
    usleep(customerNode->serviceTime * USLEEPCONVERTER);
    
    // Get the current time of day and calculate the endServiceTime.
    gettimeofday(&currentTime, NULL);
    endServiceTime = (currentTime.tv_sec + (double) currentTime.tv_usec / TIMEMICROCONVERTER) - simStartTime; // Formula is derived from same_gettimeofday.c provided on connex

    printf("A clerk finishes serving a customer: end time %.2f seconds, the customer ID %d, the clerk ID %d.\n", endServiceTime, customerNode->id, clerkID);

    // Lock the clerkStatusMutex to up date the clerk from Busy (B) to available (A).
    pthread_mutex_lock(&clerkStatusMutex);

    // Minus 1 from clerkID to place A in the correct index locations.
    clerksStatus[clerkID - 1] = 'A';

    // Unlock the clerkStatusMutex.
    pthread_mutex_unlock(&clerkStatusMutex);


    // Update the number of free clerks.
    pthread_mutex_lock(&clerkMutex);

    // A clerk just became free.
    freeClerks++;

    // Broadcast to wake waiting threads, informing them a clerk has become available.
    if(pthread_cond_signal(&freeClerkConditional) != 0){
        printf("FAILED\n");
    }

    // Unlock the clerkMutex now that the clerk status has been updated.
    pthread_mutex_unlock(&clerkMutex);

    // Free the allocated memory of the customer.
    freeNodeMemory(customerNode);

}//clerkServer

/*
    Function: destroyMutexCond
    --------------------------------
    This function destroys all the mutex and conditional variables created for this program.

*/
void destroyMutexCond(){

    // Destroy all mutex variables.
    pthread_mutex_destroy(&busiQueue);
    pthread_mutex_destroy(&econQueue);
    pthread_mutex_destroy(&clerkMutex);
    pthread_mutex_destroy(&clerkStatusMutex);

    // Destroy all conditional variables.
    pthread_cond_destroy(&freeClerkConditional);

}//destroyMutex

/*
    Function: threadProcessing
    --------------------------------
    This function initalizes all the mutex's, conditional variables and threads need for the program.
    
*/
void threadProcessing(int totalCustomers, customerRaw* passedData){

    // Variables
    int loopCounter;            // Integer for a for-loop.
    int errorThread;            // Interger for checking if thread creation worked.
    int errorMutex[MAXMUTEX];   // Integer array for checking if any of the init functions failed.
    int errorCond;              // Integer array for checking if conditional variable init failed.

    // Initialize mutex and conditional variables.
    errorMutex[0] = pthread_mutex_init(&clerkMutex, NULL);
    errorMutex[1] = pthread_mutex_init(&clerkStatusMutex, NULL);
    errorMutex[2] = pthread_mutex_init(&econQueue, NULL);
    errorMutex[3] = pthread_mutex_init(&busiQueue, NULL);

    errorCond = pthread_cond_init(&freeClerkConditional, NULL);

    // Error handling if cond_init fails.
    if(errorCond != 0){

        printf("Creation of the conditional variable has failed.\n");
        exit(0);

    }//if

    // Error handling if any mutex_init fails.
    for(loopCounter = 0; loopCounter < MAXMUTEX; loopCounter++){

        if(errorMutex[loopCounter] != 0){

            printf("Creation of Mutex %d has failed.\n", loopCounter);
            exit(0);

        }//if

    }//for

    // Threading Variables.
    pthread_t customerThreadID[totalCustomers];

    // Create threads for each customer.
    for(loopCounter = 0; loopCounter < totalCustomers; loopCounter++){
        
        errorThread = pthread_create(&customerThreadID[loopCounter], NULL, customerThread, (void *)&passedData[loopCounter]);

        // Error handling if a thread fails to be created.
        if(errorThread != 0){

            printf("Creation of thread %d has failed.\n", loopCounter);
            exit(0);

        }//if

    }//for

    // Wait for all the customer threads to finish.
    for(loopCounter = 0; loopCounter < totalCustomers; loopCounter++){

        pthread_join(customerThreadID[loopCounter], NULL);

    }//for

    // Destroy mutex and conditional variables.
    destroyMutexCond();
    
}//threadProcessing

/*
    Function: retrieveFileData
    --------------------------------
    This function retrieves all the data from the passed file.

    returns TRUE if all values are positive, FALSE if a negative value is found
    
*/
int retrieveFileData(FILE* filePointer, customerRaw* dataContainer, char* inputFile){

    // Variables
    char ignoreColon;               // Char for grabbing the ":" from the file.
    char ignoreComma1;              // Char for grabbing the first "," from the file.
    char ignoreComma2;              // Char for grabbing the second "," from the file.
    
    int customerID = 0;             // Integer for storing customer's ID while reading the file.
    int customerClass = 0;          // Integer for storing customer's Class while reading the file.
    int customerArrivalTime = 0;    // Integer for storing customer's Arrival time while reading the file.
    int customerServiceTime = 0;    // Integer for storing customer's Service time while reading the file.
    int rawIndex = 0;               // Integer index for placing raw data into 2D array.

    int noDataErrors = TRUE;        // Integer flag for if any negative values appear.
    int correctValues;              // Integer for checking if the proper amount of data was read in.

    // Scan the passed file, store only the integers that pertain to this assignment. Ignore the rest.
    while((correctValues = fscanf(filePointer, "%d %c %d %c %d %c %d", &customerID, &ignoreColon, &customerClass, &ignoreComma1, &customerArrivalTime, &ignoreComma2, &customerServiceTime)) != EOF){
        
        // Check if fscanf read in 7 correct values.
        if(correctValues != 7){

            // Error handling for if the file is of incorrect input.
            printf("Error: Input file is either incorrect format, or contains a string in an integer spot.\n");
            exit(0);

        }//if

        // If any value is a negative integer, set allPositiveValues to FALSE and return.
        if(customerID < 0 || customerClass < 0 || customerArrivalTime < 0 || customerServiceTime < 0){

            // Error message for when a negative value is found.
            printf("ACS requires all positive integer. Please change the negative value in %s to be a positive integer.\n", inputFile);
            noDataErrors = FALSE;

            return noDataErrors;

        }//if

        // Checking if the customer class is either 1 or 0.
        if(customerClass != 1 && customerClass != 0){

            // Error message for when the customer class is anything besides 1 or 0.
            // Adding 2 to raw index because I removed the first line before enter this function and to compensate for arrays starting at 0.
            printf("ACS requires customers to be of class 1 or 0. Please change the number %d on line %d to be either a 1 or 0 in %s.\n", customerClass, rawIndex + 2, inputFile);
            noDataErrors = FALSE;

            return noDataErrors;

        }//if

        // Storing the customers data into an array of structs.
        dataContainer[rawIndex].id = customerID;
        dataContainer[rawIndex].class = customerClass;
        dataContainer[rawIndex].arrival = customerArrivalTime;
        dataContainer[rawIndex].service = customerServiceTime;

        rawIndex++;

    }//while

    return noDataErrors;

}//retrieveFileData

/*
    Function: outputAverages
    --------------------------------
    This function outputs the aveerage wait times.
    
*/
void outputAverages(){

    // Variables.
    double totalAll = 0.0;          // Double for storing the total number of customers.
    double totalBusiness = 0.0;     // Double for storing the total number of business class customers.
    double totalEconomy = 0.0;      // Double for storing the total number of economy class customers.

    double averageAll = 0.0;        // Double for storing the average wait time of all customers.
    double averageBusiness = 0.0;   // Double for storing the average wait time of al business class customers.
    double averageEconomy = 0.0;    // Double for storing the average wait time of all economy class customers.

    double sum = 0.0;               // Double for stoing the sum.

    int loopCounter;                // Integer for for-loop.

    // Only calculate the average waiting time if their were customers.
    if(allWaitIndex != 0){

        // We must minus one from the index because the index indicates the next free location in the array.
        totalAll = (double)(allWaitIndex);


        // Sum all the values stored in allWaitTimes.
        for(loopCounter = 0; loopCounter < allWaitIndex; loopCounter++){
        
            sum = sum + allWaitTimes[loopCounter];

        }//for

        // Calculate the average wait time for all the customers.
        averageAll = sum / totalAll;

    }//if

    if(busiWaitIndex != 0){

        // We must minus one from the index because the index indicates the next free location in the array.
        totalBusiness = (double)(busiWaitIndex);

        // Reset sum to zero
        sum = 0.0;

        // Sum all the values stored in busiWaitTimes.
        for(loopCounter = 0; loopCounter < busiWaitIndex; loopCounter++){

            sum = sum + busiWaitTimes[loopCounter];

        }//for

        // Calculate the average wait time for all the business class customers.
        averageBusiness = sum / totalBusiness;

    }//if

    if(econWaitIndex != 0){

        // We must minus one from the index because the index indicates the next free location in the array.
        totalEconomy = (double)(econWaitIndex );

        // Reset sum to zero.
        sum = 0.0;

        // Sum all the values in the econWaitTimes.
        for(loopCounter = 0; loopCounter < econWaitIndex; loopCounter++){

            sum = sum + econWaitTimes[loopCounter];

        }//for

        // Calculating the average wait time for all the economy class customers.
        averageEconomy = sum / totalEconomy;

    }//if

    // Output all the average waiting times
    printf("\n");
    printf("The average waiting time for all customers in the system is: %.2f seconds.\n", averageAll);
    printf("The average waiting time for all business-class customers is: %.2f seconds.\n", averageBusiness);
    printf("The average waiting time for all economy-class customers is: %.2f seconds.\n", averageEconomy);

}//outputAverages

//************* Main Function ************************

int main(int argc, char* argv[]){

    // Variables.
    char checkFile[5] = ".txt";  // Char array for file extension.
    char *inputFile = argv[1];   // Char pointer for storing file string.
    char *isValidFile;           // Char pointer for storing result from checking file extension.

    int totalCustomers = 0;      // Integer for storing total number of customers.
    int noFileDataError = TRUE;  // Integer flag for checking if the file contained any incorrect data for the program.

    double timeToComplete = 0.0; // Double for storing the overall time to complete the simulation.

    // Initializing a struct pointer for getting the current time.
    struct timeval endOfSimulation;

    // Initalize Queues
    custBusiness = initQueue();
    custEconomy = initQueue();

    // Initalize start time of simulation.
    gettimeofday(&startTime, NULL);

    // Initalizing sim start time. (Formula is derived from same_gettimeofday.c provided on connex).
    simStartTime = startTime.tv_sec + (double) startTime.tv_usec / TIMEMICROCONVERTER;

    // File pointer for operating on the passed file.
    FILE *fileOperator;    

    // Error handling for improper number of arguments.
    if(argc != 2){

        printf("Error: ACS requires one input (ACS <text file>).\n");
        exit(0);

    }//if

    // Check if the extension ".txt" is in the file that was passed.
    isValidFile = strstr(inputFile, checkFile);
    if(isValidFile == NULL){

        // Error handling for if passed file is not a ".txt" file.
        printf("Error: %s is not a text file.\n", inputFile);
        exit(0);

    }//if

    // Open input file into read mode.
    fileOperator = fopen(inputFile, "r");
    if(fileOperator == NULL){

        // Error handling for if the passed file does not exist.
        printf("Error: %s does not exist.\n", inputFile);
        exit(0);

    }//if

    // Grab the total number of customers from the passed file.
    fscanf(fileOperator, "%d", &totalCustomers);

    // Initialzie a struct array to store customer infomation.
    customerRaw rawInfo[totalCustomers];
    
    // Retrieve the data from the file 
    noFileDataError = retrieveFileData(fileOperator, rawInfo, inputFile);

    // Close the opened file.
    fclose(fileOperator);

    // Error was found during while reading in the data, exit program after closing the file.
    // An error message was sent in retrieveFileData.
    if(!noFileDataError){

        exit(0);

    }//if

    // Program starting message.
    printf("Welcome to the Airline Check-in System simulation.\n");
    printf("\n");

    // Handling all the thread initialization, creation, deletion.
    threadProcessing(totalCustomers, rawInfo);

    // Output the average waiting times.
    outputAverages();
    
    // Get current time
    gettimeofday(&endOfSimulation, NULL);

    // Calculate the over simulation time.
    timeToComplete = (endOfSimulation.tv_sec + (double) endOfSimulation.tv_usec / TIMEMICROCONVERTER) - simStartTime;

    // Completion message.
    printf("\n");
    printf("The Airline Check-in System simulation took %.2f seconds to be compleleted.\n", timeToComplete);
    printf("\n");

    return 0;

}//main