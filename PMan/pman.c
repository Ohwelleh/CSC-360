/*
 **********************************************
 * 
 *          CSC 360 Operating Systems
 *    Assignment 1: A Process Manager (PMan)
 * 
 *          Created By: Austin Bassett
 *                  V00905244
 * 
 *********************************************
*/

// Libraries. (Some of these Libraries were provided in some of the examples on Connex CSC 360, Prof: Kui Wu).
#include <unistd.h>     // fork(), execvp().
#include <stdio.h>      // Input/Output such as printf().
#include <stdlib.h>     // malloc().
#include <string.h>     // String manipulation, strtok().
#include <sys/types.h>  // pid_t.
#include <sys/wait.h>   // waitpid().
#include <signal.h>     // kill(), SIGTERM, SIGKILL, SIGSTOP, SIGCONT.
#include <errno.h>      // errno.
#include <ctype.h>      // isdigit().
#include "LinkedList.h" // Custom header with all the Linked list functions.

// Constants.
#define TRUE 1
#define FALSE 0
#define ARRAY_MAX 128

//************ Main Assignment Functions *************************

/*
 * Function: bg(char* tokenUserData)
 * --------------------------------
 *   Starts the passed program in the background.
 *
 *  tokenUserData: The passed arguments as string tokens.
 * 
*/
void bg(char** tokenUserData){
    
    // Variables.
    pid_t pid = fork();
    
    if(pid == 0){
        
        // This execvp if statement and exit code were provided in the A1_sampleCode.c on connex.
        if(execvp(tokenUserData[0], &tokenUserData[0]) < 0){

            perror("Error on Execvp");

        }//if

        exit(EXIT_FAILURE);

    }else if(pid > 0){

        
        // Add the process to the linked list.
        appendNode(tokenUserData[0], pid);
        usleep(10000);

    }else{

        perror("Fork Failed");
        exit(EXIT_FAILURE);

    }//if-else block
    
}//bg

/*
 * Function: bgList()
 * --------------------------------
 *   Prints out all the processes currently running in the background. 
*/
void bgList(){

    // Iterate through the linked list and print out the Process ID and Name of the process.
    numberOfProcess();

}//bgList

/*
 * Function: pStat(pid_t pid)
 * --------------------------------
 *   List the following information about a process:
 *      Comm
 *      State
 *      Utime
 *      Stime
 *      Rss
 *      Voluntary_ctxt_switches
 *      Nonvoluntary_ctxt_switches
 *
 *  pid: Integer of the process ID passed.
 * 
*/
void pStat(pid_t pid){

    // Variables.
    char statFilePath[ARRAY_MAX];
    char statusFilePath[ARRAY_MAX];
    char statusFileContents[ARRAY_MAX][ARRAY_MAX];            
    char rawStatFile[1024];

    char* statFileContents[ARRAY_MAX];
    char* splitRawString;
    char* ptrForStrtoul;

    int indexLocation = 0;
    int stateLocation = 0;
    int volLocation = 0;
    int nonVolLocation = 0;

    float utime;
    float stime;

    // File pointers.
    FILE *statusFileOperator;
    FILE *statFileOperator;

    // If block is only true if the passed PID is currently in the linked list of processes.
    if(doesProcessExist(pid)){

        // Creating the path string for /proc/[pid]/stat.
        snprintf(statFilePath, sizeof(statFilePath), "/proc/%d/stat", pid);

        // Creating the path string for /proc/[pid]/status.
        snprintf(statusFilePath, sizeof(statusFilePath), "/proc/%d/status", pid);

        // Getting all the contents from /proc/[pid]/status.

        // Open the status file in read mode.
        statusFileOperator = fopen(statusFilePath, "r");

        // Error handling for it file can not be opened.
        if(statusFileOperator == NULL){

            printf("Error: Could not open file %s\n", statusFilePath);
            return;

        }//if

        // Getting each line of the status file.
        while(fgets(statusFileContents[indexLocation], ARRAY_MAX, statusFileOperator) != NULL){

            // Getting the index location of State.
            if(strncmp(statusFileContents[indexLocation], "State:", 6) == 0){

                stateLocation = indexLocation;
                indexLocation++;

                continue;
            }//if

            // Getting the index location of voluntary switches.
            if(strncmp(statusFileContents[indexLocation], "voluntary_ctxt_switches:", 24) == 0){
                
                volLocation = indexLocation;
                indexLocation++;

                continue;
            }//if

            // Getting the index location of nonvoluntary switches.
            if(strncmp(statusFileContents[indexLocation], "nonvoluntary_ctxt_switches:", 27) == 0){
                
                nonVolLocation = indexLocation;
                indexLocation++;

                continue;
            }//if

            indexLocation++;
        }//while

        // Close the opened status file.
        fclose(statusFileOperator);

        // Getting all the contents from /proc/[pid]/stat.

        // Open the stat file in read mode.
        statFileOperator = fopen(statFilePath, "r");

        // Error handling for it file can not be opened.
        if(statFileOperator == NULL){

            printf("Error: Could not open file %s\n", statFilePath);
            return;

        }//if

        // Reset indexLocation for the next array input.
        indexLocation = 0;
        
        while(fgets(rawStatFile, sizeof(rawStatFile)-1, statFileOperator) != NULL){

            // Splitting the stat file into string tokens separated by the white space.
            splitRawString = strtok(rawStatFile, " ");
            statFileContents[indexLocation] = splitRawString;

            while(splitRawString != NULL){

                statFileContents[indexLocation] = splitRawString;
                splitRawString = strtok(NULL, " ");
                indexLocation++;

            }//while(token != NULL)
        }//while(fgets)

        // Close the opened stat file.
        fclose(statFileOperator);

    }else{

        printf("Error: Process: %d does not exist.\n", pid);
        return;

    }//if-else

    // Formatting utime and stime to be floating point values to be output as seconds.
    utime = strtof(statFileContents[14], &ptrForStrtoul) / sysconf(_SC_CLK_TCK);
    stime = strtof(statFileContents[15], &ptrForStrtoul) / sysconf(_SC_CLK_TCK);

    // Outputting the data to the standard out.
    printf("comm: %s\n", statFileContents[1]);
    printf("%s", statusFileContents[stateLocation]);
    printf("utime: %f seconds\n", utime);
    printf("stime: %f seconds\n", stime);
    printf("rss: %s\n", statFileContents[24]);
    printf("%s", statusFileContents[volLocation]);
    printf("%s", statusFileContents[nonVolLocation]);

}//pStat

/*
 * Function: bgStatusChanger(pid_t pid, int changeStatusTo)
 * --------------------------------
 *   Handles the three options for a process Kill, Stop, or Start the process with PID.
 *
 *  pid: Integer of the process ID passed.
 *  changeStatusTo: Integer flag of which process to run.
 * 
*/
void bgStatusChanger(pid_t pid, int changeStatusTo){

    // Variables.    
    int isProcessRunning = checkRunningStatus(pid);

    if(changeStatusTo == 0){
        //bgKill.
        
        // If isProcessRunning is -1, means that the process was not found.
        if(isProcessRunning == -1){
            return;
        }//if

        printf("Terminating process with ID: %d\n", pid);
        kill(pid, SIGTERM);
        deleteNode(pid);

    }else if(changeStatusTo == 1){
        //bgstop.

        // If isProcessRunning is -1, means that the process was not found.
        if(isProcessRunning == -1){
            return;
        }//if

        // Checking is a process is already running before sending a stop signal.
        if(isProcessRunning){

            printf("Stopping process ID: %d\n", pid);
            kill(pid, SIGSTOP);
            changeProcessStatus(pid, FALSE);

        }else{

            printf("The Process: %d is already stopped.\n", pid);

        }//if-else

    }else{
        //bgStart.

        // If isProcessRunning is -1, means that the process was not found.
        if(isProcessRunning == -1){
            return;
        }//if

        // Checking is a process is already stopped before sending a start signal.
        if(isProcessRunning == FALSE){

            printf("Starting process ID: %d\n", pid);
            kill(pid, SIGCONT);
            changeProcessStatus(pid, TRUE);

        }else{

            printf("The Process: %d is already running.\n", pid);

        }//if-else block

    }//if-else block

}//bgStatusChanger

//************ Helper Assignment Functions *************************

/*
 * Function: processData()
 * -------------------------------
 *      Process the data passed from the command line and split the string into tokens.
 *      Return an integer for the switch-case in main.
 * 
 *  userData: pointer to an array where the string tokens will be stored.
 *  rawData: pointer to a character array of the data from the standard input.
*/
int processData(char** userData, char* rawData){

    // Variables.
    int commandNumber = 8;
    int forLoopCounter;
    int tempStringIndex = 0;

    char* splitRawString;
    char* commandType;
    char* tempStringArray[ARRAY_MAX];

    // Initializing the command 'List.'
    char commandList[3][8] = {"bg", "bglist", "pstat"};
    char statusList[3][8] = {"bgkill", "bgstop", "bgstart"};

    // Splitting the input string by the white space and saving the command passed(e.g., bg, bglist....).
    splitRawString = strtok(rawData, " \n");

    // Checking if the user passed any arguments.
    if(splitRawString == NULL){

        return -3;

    }//if

    // Saving the command type (e.g, bg, bglist, bgkill... etc).
    commandType = splitRawString;

    // Checking for the stop command for exiting the program.
    if(strcmp(commandType, "stop") == 0){

        return 4;

    }//if
    
    // Separate the strings by whitespace and newline character.
    while(splitRawString != NULL){

        tempStringArray[tempStringIndex] = splitRawString;
        splitRawString = strtok(NULL, " \n");
        tempStringIndex++;

    }//while(token != NULL)

    // Transfer the tokenized string over to userData but with out the bg command.
    for(forLoopCounter = 0; forLoopCounter < tempStringIndex - 1; forLoopCounter++){

        userData[forLoopCounter] = tempStringArray[forLoopCounter + 1];

    }//for
    
    // Adding a null terminating character to userData.
    userData[forLoopCounter] = NULL;

    // Comparing the users command to the list of Valid commands.
    for(forLoopCounter = 0; forLoopCounter < 3; forLoopCounter++){

        // This statement checks if the command is bg, bglist, or pstat
        if(strcmp(commandType, commandList[forLoopCounter]) == 0){

            commandNumber = forLoopCounter;
            break;

            // This if statement checks if the command is bgkill, bgstart, or bgstop.
        }else if(strcmp(commandType, statusList[forLoopCounter]) == 0){

            // Creating a unique number to determine if it was bgkill, bgstart, or bgstop.
            commandNumber = 10 + forLoopCounter;
            break;

        }//if-else block

    }//for

    return commandNumber;
}//processData


/*
 * Function: isValidNumber(char* stingInput)
 * -------------------------------
 *      Checks if the string is a valid number.
 *      Return the string as an integer.
 * 
 *  stringInput: String to be checked.
*/
int isValidNumber(char* stringInput){

    // Variables.
    int i;
    int notNumber = 0;

    // Checking if the passed argument is all numbers.
    for(i = 0; i < strlen(stringInput); i++){

        // Error handling for if passed argument is not a number.
        if(isdigit(stringInput[i]) == 0){

            printf("The passed PID: %s is not a valid number.\n", stringInput);
            return notNumber;

        }//if

    }//for

    return atoi(stringInput);
}//isValidNumber

//************ Main function *************************

int main(int argc, char *argv[]){

    // Initializing the linked list data structure.
    initializeLinkedListHead();
    
    // Main infinite loop of the program.
    while(TRUE){
        
        // Variables that need to be reset every iteration.
        char* userInput[ARRAY_MAX];
        char rawInputData[ARRAY_MAX];
        char* invalidCommand;

        int commandFound = -1;
        int isInteger;
        int statusUpdateID = 0;

        // Getting user input.
        printf("PMan:> ");
        fgets(rawInputData, ARRAY_MAX, stdin);

        // Processing user input and finding if a valid command was passed.
        commandFound = processData(userInput, rawInputData);

        // No arguments were passed, throw error and goto next iteration.
        if(commandFound < 0){

            printf("Error: No commands were passed.\n");
            continue;

        }//if

        // This if statement takes the unique number that was created in processData and splits it into
        // a statusUpdateID used to tell if the user wants bgkill(0), bgstop(1) or bgstart(2).
        if(commandFound > 9){

            statusUpdateID = commandFound - 10;
            commandFound = 3;

        }//if

        // This if statement saves the comman the user input just to output back informing them that command is invalid.
        if(commandFound == 8){
            invalidCommand = strtok(rawInputData, " \n");
        }
        
        // Jump to the proper function.
        switch(commandFound){

            case 0: // bg Command case.

                // Error handling for if the user just types bg.
                if(userInput[0] == NULL){

                    printf("The command  \"bg\" requires a file after it.\n");
                    printf("For example: bg ./Foo\n");
                    
                }else{
                    
                    bg(userInput);
                    
                }//if-else block
                break;

            case 1: // bglist Command case.

                bgList();
                break;

            case 2: // pStat Command case.

                // Error handling if a second argument is not passed.
                if(userInput[0] == NULL){

                    printf("The command  \"pstat\" requires a valid PID after it.\n");
                    printf("For example: pstat 123\n");

                }else{
                    // Checking if argument is a valid number.
                    isInteger = isValidNumber(userInput[0]);

                    if(isInteger != 0){

                        pStat(isInteger);

                    }//if

                }//if-else block   
                break;
            
            case 3: // bgkill, bgstart, bgstop Command case.

                // Error handling if user does not provide an number after the command.
                if(userInput[0] == NULL){

                    printf("Missing valid Process ID after command\n");

                }else{
                    
                    // Checking if argument is a valid number.
                    isInteger = isValidNumber(userInput[0]);

                    if(isInteger != 0){

                        bgStatusChanger(isInteger, statusUpdateID);

                    }//if
                }//if-else block
                break;

            case 4: // Exit PMan program and free all allocated memory and kill all processes.

                printf("Stopping All PMan background processes....\n");
                killAllProcesses();
                
                exit(0);
                break;

            default: // User passed a command that doesn't exist.

                printf("PMan:> %s: command not found\n", invalidCommand);
                break;

        }//switch(command)

        // Check for Zombie processes before the next iteration.
        zombieHunter();
        
    }//while

    return 0;
}//main()