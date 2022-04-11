/****************************************************
*
*      Assignment 3: A Simple File System (SFS)
*              CSC 360, Prof: Kui Wu
*  
*                    disklist
* 
*          Developed by: Austin Bassett
*                  V00905244
* 
****************************************************/

// Libraries
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "Helpers.h"

// Constants
#define TRUE 1
#define FALSE 0
#define SECTORSIZE 512
#define DIRECTORYENTRIES 16
#define ENDOFROOTDIRECTORY 16896

// Struct for storing the data while traversing directories.
typedef struct Data{
    int count;
    int cluster[256];
    char name[64][13];
}Data;

// Global struct pointers for storing the directory and subdirectory information.
Data* directoryData;
Data* subDirectoryData;

// Global string variable for storing the path string.
char previousDirectory[256];

// Prototypes 
void fatValue(FILE*, int);
void dataParsing(FILE* , int, int, int);

/***************** Helper Functions *******************/
/*
*   Function: diskList()
*   ________________________________
*   Reads all the data from the root directory sectors.
*   
*
*/
void diskList(FILE* filePointer){

    // Variables.
    int rootDirectory = 9728;   // Integer for storing the starting byte location of the root directory.
    int directoryFree = 0;      // Integer for storing the first byte of a directory entry file name.
    int firstCluster = 0;       // Integer for storing the first cluster value of a directory entry.
    int entryLooper;            // Integer for a for-loop.
    int subLooper;              // Integer for a for-loop.

    // Read all directories while root directory value is less than end of root directory value
    while(rootDirectory < ENDOFROOTDIRECTORY){

        // Reading the first byte of the filename.
        fseek(filePointer, rootDirectory, SEEK_SET);
        fread(&directoryFree, 1, 1, filePointer);

        // If the first byte of the file name is 0 means all the entries in this sector are empty.
        if(directoryFree == 0x00){

            // Move to next root directory sector.
            rootDirectory = rootDirectory + SECTORSIZE;

            continue;

        }//if
      
        printf("ROOT\n");
        printf("==================\n");

        // Set the global directory count variable to 0 for each sector.
        directoryData->count = 0;

        // Loop through all 16 directory entries.
        for(entryLooper = 0; entryLooper < DIRECTORYENTRIES; entryLooper++){

            // Variables.
            int dataLocation = rootDirectory + (entryLooper * 32); // Integer for storing the location of each entry. (Each directory entry is 32 bytes long).

            // Reading the first byte of the filename.
            fseek(filePointer, dataLocation, SEEK_SET);
            fread(&directoryFree, 1, 1, filePointer);

            // Reading the value of the first logical cluster.
            fseek(filePointer, dataLocation + 26, SEEK_SET);
            fread(&firstCluster, 1, 2, filePointer);

            // If a directory is unused (229(0xE5)) or first cluster is a 0 or 1, go to next entry of the sector. 
            if(directoryFree == 229 || firstCluster == 0 || firstCluster == 1){

                continue;

            }//if

            // Retrive and output the necessary information about the directory.
            dataParsing(filePointer, dataLocation, firstCluster, FALSE);
            
        }//for

        // Copy the first file name to the global string variable.
        memcpy(previousDirectory, directoryData->name[0], 9);

        // Looping through each directory found.
        for(subLooper = 0; subLooper < directoryData->count; subLooper++){

            // Variables that need to be reset with each iteration.
            subDirectoryData->count = 0;

            printf("/%s \n", directoryData->name[subLooper]);
            printf("==================\n");

            // Read check subdirectories.
            fatValue(filePointer, directoryData->cluster[subLooper]);

            // Completely replace the old global variable string with a new starting path.
            memcpy(previousDirectory, directoryData->name[subLooper], 9);

        }//for

        // Go to next root directory sector.
        rootDirectory = rootDirectory + SECTORSIZE;

    }//while


}//diskList

/*
*   Function: fatValue()
*   ________________________________
*   Reads all the data about subdirectories from the FAT and then the data sectors.
*   
*
*/
void fatValue(FILE* filePointer, int entryLocation){

    // Variables to be reset each iteration. (Ensuring no junk is left in variables)
    int value = 0;              // Integer for storing the entry value.
    int fourBitValue = 0;       // Integer for storing the four bits.
    int eightBitValue = 0;      // Integer for storing the eight bits.
    int fatSector = SECTORSIZE; // Integer for storing the FAT starting sector location.
    int firstCluster = 0;       // Integer for storing the first cluster value.
    int running = TRUE;         // Integer for while-loop condition.
    int clusterLoop;            // Integer for a for-loop.
    int subLoop;                // Integer for a for-loop.

    int lastCluster[9] = {0xFF8, 0xFF9, 0xFFA, 0xFFB, 0xFFC, 0xFFD, 0xFFE, 0xFFF};              // Integer array holding all the values which indicate it is the last cluster of the file.
    int noDataChecker[10] = {0x000, 0xFF0, 0xFF1, 0xFF2, 0xFF3, 0xFF4, 0xFF5, 0xFF6, 0xFF7};    // Integer array holding all the values which indicate no data their. (Reserved clusters 0xFF0 - 0xFF6, Bad clusters 0xFF7, Unused 0x000)

    // While loop for reading the file data stored in the FAT.
    while(running){

        // Variables.
        int entryByteLocation = (3 * entryLocation) / 2;    // Integer of the lower index location. (Add 1 to this value to higher index location)
        int physicalAddress = 31 + entryLocation;           // Integer for storing the data sector location.
        char output[20] = "/";                              // Character array for holding the subdirectory path.

        // Checking if current entry is even or odd.
        if((entryLocation % 2) == 0){

            // Reading the byte of the entry.
            fseek(filePointer, fatSector + entryByteLocation, SEEK_SET);
            fread(&eightBitValue, 1, 1, filePointer);

            // Reading the byte that contains the high nibble of the entry.
            fread(&fourBitValue, 1, 1, filePointer);

            // Isolating the lower nibble of the byte and shifting it eight spots to the left.
            fourBitValue = (fourBitValue & 0x0F) << 8;

            // Combined the bits.
            value = fourBitValue + eightBitValue;

        }else{
            
            // Reading the byte contains the low nibble of the byt entry.
            fseek(filePointer, fatSector + entryByteLocation, SEEK_SET);
            fread(&fourBitValue, 1, 1, filePointer);

            // Isolationg the high nibble and shifting it four positions to the right.
            fourBitValue = fourBitValue & 0xF0;

            // Reading the byte of the entry.
            fread(&eightBitValue, 1, 1, filePointer);

            // Shifting the readin values into their correct locations.
            value = (eightBitValue << 4) + (fourBitValue >> 4);

        }// if-else

        // Comparing the value with each value in the noDataChecker array.
        for(clusterLoop = 0; clusterLoop < 10; clusterLoop++){

            if(value == noDataChecker[clusterLoop]){
                
                // If value is in the array, return, as no data is in this FAT entry.
                return;

            }//if

        }//for

        // Looping through each subdirectory which is stored in the data sectors. (These are exactly like the root sectors, as in, each sector is 16 entries that are 32 bytes long)
        for(clusterLoop = 0; clusterLoop < DIRECTORYENTRIES; clusterLoop++){

            // Variables.
            int dataLocation = (physicalAddress * SECTORSIZE) + (clusterLoop * 32); // Integer for storing data location in the data sector area.
            int entryCheck = 0;                                                     // Integer for storing the first byte of the file name.

            // Reading the first byte of the fileName.
            fseek(filePointer, dataLocation, SEEK_SET);
            fread(&entryCheck, 1, 1, filePointer);

            // Reading the value of the first logical cluster.
            fseek(filePointer, dataLocation+ 26, SEEK_SET);
            fread(&firstCluster, 1, 2, filePointer);

            // If first byte of file name is 0, break out of this for loop, as the following entries in this sector are empty.
            if(entryCheck == 0){

                break;

            }//if

            // If the first byte of the file name is 0xE5 (229), then the entry is free.
            if(entryCheck == 229){
                
                // Go to next entry.
                continue;

            }//if
            
            // Retrive and output the necessary information about the directory.
            dataParsing(filePointer, dataLocation, firstCluster, TRUE);

        }//for

        // For loop for checking if value is the last cluster of the file.
        for(clusterLoop = 0; clusterLoop < 9; clusterLoop++){

            if(value == lastCluster[clusterLoop]){
                
                running = FALSE;
                break;

            }//if

        }//for
        
        // For loop for getting the subdirectories information.
        for(subLoop = 0; subLoop < subDirectoryData->count; subLoop++){
            
            // Adding the subdirectory name to the global variable string.
            strcat(output, subDirectoryData->name[subLoop]);
            strcat(previousDirectory, output);
        
            printf("/%s \n", previousDirectory);
            printf("==================\n");

            subDirectoryData->count--;
            fatValue(filePointer, subDirectoryData->cluster[subLoop]);

        }//for

        // Setting entryLocation to the next data sector location.
        entryLocation = value;

    }//while

}//fatValue

/*
*   Function: dataParsing()
*   ________________________________
*   Extractes the information needed for the assignment and outputs it.
*   
*   If reading a subdirectory pass TRUE to subDirectory.  
*
*/
void dataParsing(FILE* filePointer, int dataLocation, int firstCluster, int subDirectory){
    
    // Variables.
    int rawDate = 0;        // Integer for storing the raw date value.
    int rawTime = 0;        // Integer for storing the raw time value.
    int parsedDate[3];      // Integer array for storing the formated date values. (Year = parsedDate[0], Month = parsedDate[1], Day = parsedDate[2])
    int parsedTime[2];      // Integer array for storing the formated time values. (Hour = parsedTime[0], Minutes = parsedTime[1])
    int combineIndex = 0;   // Integer for tracking where to add the '.' in the file name and to combine the file name with its file extension.
    int combineLoop;        // Integer for a for-loop.
    int fileAttributes = 0; // Integer for storing the file's attribute.
    int fileSize = 0;       // Integer for storing the file size.

    unsigned char fileName[13];     // Unsigned character array for storing the entire file name, including it's exetension.
    unsigned char fileExtension[4]; // Unsigned character array for stoing the file's extension.

    // Reading the file attribute
    fseek(filePointer, dataLocation + 11, SEEK_SET);
    fread(&fileAttributes, 1, 1, filePointer);

    // Reading the file name
    fseek(filePointer, dataLocation, SEEK_SET);
    fread(fileName, 1, 8, filePointer);

    // Adding a null terminating character and removing the trailing whitespace.
    fileName[8] = '\0';
    removePadding(fileName);

    // Checking if the entry is a file or a directory.
    if((fileAttributes & 0x10) == 16){
        
        // Checking if function is reading a directory or a subdirectory.
        if(subDirectory == FALSE){
            
            printf("D ");

            // Add the first cluster value into the directory global integer array.
            directoryData->cluster[directoryData->count] = firstCluster;

            // Copy the file name into the directory global character array.
            memcpy(directoryData->name[directoryData->count], fileName, 9);

            // Increment the global directory count variable.
            directoryData->count++;

        }else{

            // Checking if the file name is '.' or '..', if so, return
            if(memcmp(fileName, ".", 1) == 0 || memcmp(fileName, "..", 2) == 0){

                return;

            }//if

            printf("D ");

            // Add the first cluster value into the subdirectory global integer array.
            subDirectoryData->cluster[subDirectoryData->count] = firstCluster;

            // Copy the file name into the subdirectory global character array.
            memcpy(subDirectoryData->name[subDirectoryData->count], fileName, 9);

            // Increment the global subdirectory count variable.
            subDirectoryData->count++;

        }//if-else

    }else{

        printf("F ");

        // Reading the file extenstion.
        fseek(filePointer, dataLocation + 8, SEEK_SET);
        fread(fileExtension, 1, 3, filePointer);
        fileExtension[3] = '\0';

        // Counter the number of characters.
        combineIndex = 0;
        while(fileName[combineIndex] != '\0'){

            combineIndex++;

        }//while

        // Adding a '.' to the end of the file name.
        fileName[combineIndex] = '.';
        combineIndex++;

        // Adding the file extension to the name.
        for(combineLoop = 0; combineLoop < 3; combineLoop++){
                    
            fileName[combineIndex] = fileExtension[combineLoop];
            combineIndex++;

        }//for

    }//if-else

    // Reading the size of the file.
    fseek(filePointer, dataLocation + 28, SEEK_SET);
    fread(&fileSize, 1, 4, filePointer);

    // Reading the raw date of creation the file.
    fseek(filePointer, dataLocation + 16, SEEK_SET);
    fread(&rawDate, 1, 2, filePointer);

    // Reading the raw time of creation of the file.
    fseek(filePointer, dataLocation + 14, SEEK_SET);
    fread(&rawTime, 1, 2, filePointer);

    // Formating the creation date.
    parsedDate[0] = parsedDate[1] = parsedDate[2] = rawDate;

    // Isolating Year.
    parsedDate[0] = ((parsedDate[0] & 0XF800) >> 9) + 1980;

    // Isolating Month.
    parsedDate[1] = (parsedDate[1] & 0x1E0) >> 5;

    // Isolating Day.
    parsedDate[2] = parsedDate[2] & 0x1F;

    // Formating the creation time.
    parsedTime[0] = parsedTime[1] = rawTime;

    // Isolating Hour.
    parsedTime[0] = (parsedTime[0] & 0xF800) >> 11;

    // Isolating Minutes.
    parsedTime[1] = (parsedTime[1] & 0x7E0) >> 5;

    // Output the data.
    printf("%9d ", fileSize);
    printf("%13s ", fileName);
    printf("%d-%02d-%02d ", parsedDate[0], parsedDate[1], parsedDate[2]);
    printf("%02d:%02d \n", parsedTime[0], parsedTime[1]);
        

}//dataParsing

/***************** Main Function *******************/
int main(int argc, char *argv[]){

    // Variables
    char *inputFile = argv[1];        // String for stoing the passed argument name.
    char checkFileUpper[5] = ".IMA";  // Character array for checking upper case file extension.
    char *isValidFileUpperCase;       // Character pointer for storing the results from checking if the file extension is upper case.
    char checkFileLower[5] = ".ima";  // Character array for checking upper case file extension.
    char *isValidFileLowerCase;       // Character pointer for storing the results from checking if the file extension is upper case.

    // Error handling.
    if(argc != 2){
        
        printf("Error: Missing or Extra arguments.\n");
        printf("Intended execution: ./disklist test.ima\n");
        exit(0);

    }//if

    // File pointer.
    FILE *filePointer;

    // Open file in read binary mode.
    filePointer = fopen(inputFile, "rb");

    // File passed does not exist.
    if(filePointer == NULL){

        printf("Error: The passed file %s does not exist.", inputFile);
        exit(0);

    }//if

    // Checking the passed file.
    isValidFileUpperCase = strstr(inputFile, checkFileUpper);
    isValidFileLowerCase = strstr(inputFile, checkFileLower);
    if(isValidFileUpperCase == isValidFileLowerCase){

        printf("Error: %s is not a .IMA file.\n", inputFile);
        exit(0);

    }//if

    // Initalize the memory space for the global struct pointers.
    directoryData = malloc(sizeof(Data));
    subDirectoryData = malloc(sizeof(Data));

    diskList(filePointer);

    // Freeing the allocated memory.
    free(directoryData);
    free(subDirectoryData);

    fclose(filePointer);

    return 0;
}//main