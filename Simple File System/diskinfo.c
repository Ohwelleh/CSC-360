/****************************************************
*
*      Assignment 3: A Simple File System (SFS)
*              CSC 360, Prof: Kui Wu
*  
*                   diskinfo
* 
*          Developed by: Austin Bassett
*                  V00905244
* 
****************************************************/

// Libraries
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Helpers.h"

#define SECTORSIZE 512

/***************** Helper Functions *******************/

/*
*   Function: searchDirectories()
*   ________________________________
*   Searches the directories in sector 19-32 (inclusive) and finds the number of files and
*   the label of the disk.
*
*/
void searchDirectories(FILE* filePointer, int* numberOfFiles, char* labelName){

    // Variables
    int rootDirectory = 9728;   // Integer indicating where the start of root directory.
    unsigned char fileAttributes;     // Integer for storing file attribute.

    int fileCounter = 0;        // Integer for tracking files.
    int firstCluster = 0;       // Integer for storing first cluster value.
    int directoryFree = 0;      // Integer for storing first byte of file name.

    int labelOffset = 0;        // Integer for storing location of label.
    
    int directoryCount = 0;     // Integer for current directory sector.
    int entryLooper;            // Integer for for-loop.
    int entryOffset = 0;        // Integer to track each 32 byte entry of each sector.


    // While loop for counting the number of files in the root directory.
    while(directoryCount < 14){

        // Setting offset to the start of directory.
        entryOffset = rootDirectory;

        for(entryLooper = 0; entryLooper < 16; entryLooper++){   

            // Reading the first byte of the filename.
            fseek(filePointer, entryOffset, SEEK_SET);
            fread(&directoryFree, 1, 1, filePointer);

            // If the first byte is 0 then all directories in this entry are free.
            if(directoryFree == 0){
                
                // Go to next sector.
                break;

            }//if

            // If the first byte is 0xE5 (229) then the entry is unused. 
            if(directoryFree == 229){            

                // Go to next directory entry.
                entryOffset = entryOffset + 32;

                continue;

            }//if

            // Reading the file attributes.
            fseek(filePointer, (entryOffset + 11), SEEK_SET);
            fread(&fileAttributes, 1, 1, filePointer);

            // Reading the value of the first logical cluster.
            fseek(filePointer, (entryOffset + 26), SEEK_SET);
            fread(&firstCluster, 1, 2, filePointer);

            // Checking if the volume label bit was set.
            if(fileAttributes == 8){

                // Save the directory location.
                labelOffset = entryOffset;

            }//if

            // Checking if attribute is 0x0F (15) OR first logical cluster is a 1 or 0.
            if(fileAttributes == 15 || firstCluster == 1 || firstCluster == 0){

                // Go to next directory.
                entryOffset = entryOffset + 32;
            
                continue;

            }//if

            // Increment the number of files counter, and go to next directory.
            entryOffset = entryOffset + 32;
            fileCounter++;

        }//for

        directoryCount++;

        // Moving to the next sector.
        rootDirectory = rootDirectory + SECTORSIZE;

    }//while

    // If labelOffset is not set, make label name blank.
    if(labelOffset == 0){

        strcpy(labelName, "");

    }else{

        // Reading the label name.
        fseek(filePointer, labelOffset, SEEK_SET);
        fread(labelName, 1, 8, filePointer);

    }//if-else

    // Setting the pointer value to fileCounter value.
    *numberOfFiles = fileCounter;


}//searchDirectories

/***************** Main Function *******************/
int main(int argc, char *argv[]){

    // Variables
    char *inputFile = argv[1];        // String for stoing the passed argument name.
    char checkFileUpper[5] = ".IMA";  // Character array for checking upper case file extension.
    char *isValidFileUpperCase;       // Character pointer for storing the results from checking if the file extension is upper case.
    char checkFileLower[5] = ".ima";  // Character array for checking upper case file extension.
    char *isValidFileLowerCase;       // Character pointer for storing the results from checking if the file extension is upper case.
    char osName[9];                   // Character array for storing the OS Name.
    char labelOfDisk[11];             // Character array storing the label of the disk.

    int freeSpace = 0;          // Integer for storing the number of free sectors.
    int sectorCount = 0;        // Integer for storing the total sectors on the disk.
    int sectorsPerFat = 0;      // Integer for storing the number of sectors per FAT.
    int numberOfFat = 0;        // Integer for storing total number of FAT.
    int numberOfFiles = 0;      // Integer for stroing the total number of files.

    // Error handling.
    if(argc != 2){
        
        printf("Error: Missing or Extra arguments.\n");
        printf("Intended execution: ./diskinfo test.ima\n");
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


    // Reading the OS Name at starting byte 3, length of 8.
    fseek(filePointer, 3, SEEK_SET);
    fread(osName, 1, 8, filePointer);

    // Searching the directories for number of files and the label of disk.
    searchDirectories(filePointer, &numberOfFiles, labelOfDisk);

    // Reading total size of the disk.
    fseek(filePointer, 19, SEEK_SET);
    fread(&sectorCount, 1, 2, filePointer);

    // Finding the number of unused entries in the FAT12 tables.
    calculateFreeSpace(filePointer, &freeSpace);

    // Reading number of FAT copies.
    fseek(filePointer, 16, SEEK_SET);
    fread(&numberOfFat, 1, 1, filePointer);

    // Reading sectors per FAT number.
    fseek(filePointer, 22, SEEK_SET);
    fread(&sectorsPerFat, 1, 2, filePointer);

    // Close the passed file.
    fclose(filePointer);

    // Output
    printf("OS Name: %s\n", osName);
    printf("Label of the disk: %s\n", labelOfDisk);
    printf("Total size of the disk: %d bytes\n", sectorCount * SECTORSIZE);
    printf("Free size of the disk: %d bytes\n", freeSpace * SECTORSIZE);

    printf("==============\n");
    printf("The number of files in the disk: %d\n", numberOfFiles);
    printf("==============\n");

    printf("Number of FAT copies: %d\n", numberOfFat);
    printf("Sectors per FAT: %d\n", sectorsPerFat);

    return 0;

}//main