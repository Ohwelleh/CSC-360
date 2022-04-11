/****************************************************
*
*      Assignment 3: A Simple File System (SFS)
*              CSC 360, Prof: Kui Wu
*  
*                    diskput
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

// Prototypes
void placeBytes(FILE*);
void calculateFreeSpace(FILE*, int*);
void searchSubDirectory(FILE*, int);
void dataParsing(FILE*, int, int, int);
int getFATEntry(FILE*, int);
void updateFAT(FILE*, int, int);
void updateDirectory(FILE*, int);

// Two separate structs are being used as not every execution of this program will require the DirectoryData to be run. Thus, saving on memory allocation.
// Struct for storing the data while traversing directories.
typedef struct DirectoryData{
    int count;
    int cluster[256];
    char name[64][13];
    char path[256];
    int pathFound;
}DirectoryData;

// Struct for storing the passed file to be stored in the image.
typedef struct FileData{
    char* filename;
    char* fileOutputname;
    char* fileExtension;
    int size;
    int firstCluster;
}FileData;

// Global reference to file data and directory search.
FileData* fileInformation;
DirectoryData* directoryData;
DirectoryData* subDirectoryData;

// Global string variable for storing the path string.
char previousDirectory[256];


/***************** Helper Functions *******************/
/*
*   Function: findSubDirectory()
*   ________________________________
*   Searches the root directory first and then the subdirectories.
*   
*
*/
void findSubDirectory(FILE* imagePointer){

    // Variables.
    int rootDirectory = 9728;   // Integer for storing the starting byte location of the root directory.
    int directoryFree = 0;      // Integer for storing the first byte of a directory entry file name.
    int firstCluster = 0;       // Integer for storing the first cluster value of a directory entry.
    int entryLooper;            // Integer for a for-loop.
    int subLooper;              // Integer for a for-loop.

    // Read all directories while root directory value is less than end of root directory value
    while(rootDirectory < ENDOFROOTDIRECTORY){

        // Reading the first byte of the filename.
        fseek(imagePointer, rootDirectory, SEEK_SET);
        fread(&directoryFree, 1, 1, imagePointer);

        // If the first byte of the file name is 0 means all the entries in this sector are empty.
        if(directoryFree == 0x00){

            // Move to next root directory sector.
            rootDirectory = rootDirectory + SECTORSIZE;

            continue;

        }//if

        // Set the global directory count variable to 0 for each sector.
        directoryData->count = 0;

        // Loop through all 16 directory entries.
        for(entryLooper = 0; entryLooper < DIRECTORYENTRIES; entryLooper++){

            // Variables.
            int dataLocation = rootDirectory + (entryLooper * 32); // Integer for storing the location of each entry. (Each directory entry is 32 bytes long).

            // Reading the first byte of the filename.
            fseek(imagePointer, dataLocation, SEEK_SET);
            fread(&directoryFree, 1, 1, imagePointer);

            // Reading the value of the first logical cluster.
            fseek(imagePointer, dataLocation + 26, SEEK_SET);
            fread(&firstCluster, 1, 2, imagePointer);

            // If a directory is unused (229(0xE5)) or first cluster is a 0 or 1, go to next entry of the sector. 
            if(directoryFree == 229 || firstCluster == 0 || firstCluster == 1){

                continue;

            }//if

            // Retrive and output the necessary information about the directory.
            dataParsing(imagePointer, dataLocation, firstCluster, FALSE);

            
        }//for

        // Copy the first file name to the global string variable.
        strcat(previousDirectory, "/");
        strcat(previousDirectory, directoryData->name[0]);


        // Looping through each directory found.
        for(subLooper = 0; subLooper < directoryData->count; subLooper++){
            
            // Checking if directory matches the path.
            if(strcmp(previousDirectory, directoryData->path) == 0){

                directoryData->pathFound = TRUE;
                updateDirectory(imagePointer, (31 + directoryData->cluster[subLooper]) * SECTORSIZE);
                return;

            }//if

            // Variables that need to be reset with each iteration.
            subDirectoryData->count = 0;

            // Read check subdirectories.
            searchSubDirectory(imagePointer, directoryData->cluster[subLooper]);

            // Check if the path was found in the subdirectories.
            if(directoryData->pathFound == TRUE){
                
                return;

            }//if

            // Changing the global directory string to the next directory in the root.
            strcpy(previousDirectory, "/");
            strcat(previousDirectory, directoryData->name[subLooper + 1]);


        }//for

        // Go to next root directory sector.
        rootDirectory = rootDirectory + SECTORSIZE;

    }//while

}//findSubDirectory

/*
*   Function: searchSubDirectory()
*   ________________________________
*   Reads all the data about subdirectories from the FAT and then the data sectors.
*   
*
*/
void searchSubDirectory(FILE* imagePointer, int entryLocation){

    // Variables to be reset each iteration. (Ensuring no junk is left in variables)
    int value = 0;              // Integer for storing the entry value.
    int fourBitValue = 0;       // Integer for storing the four bits.
    int eightBitValue = 0;      // Integer for storing the eight bits.
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
            fseek(imagePointer, SECTORSIZE + entryByteLocation, SEEK_SET);
            fread(&eightBitValue, 1, 1, imagePointer);

            // Reading the byte that contains the high nibble of the entry.
            fread(&fourBitValue, 1, 1, imagePointer);

            // Isolating the lower nibble of the byte and shifting it eight spots to the left.
            fourBitValue = (fourBitValue & 0x0F) << 8;

            // Combined the bits.
            value = fourBitValue + eightBitValue;

        }else{
            
            // Reading the byte contains the low nibble of the byt entry.
            fseek(imagePointer, SECTORSIZE + entryByteLocation, SEEK_SET);
            fread(&fourBitValue, 1, 1, imagePointer);

            // Isolationg the high nibble and shifting it four positions to the right.
            fourBitValue = fourBitValue & 0xF0;

            // Reading the byte of the entry.
            fread(&eightBitValue, 1, 1, imagePointer);

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
            fseek(imagePointer, dataLocation, SEEK_SET);
            fread(&entryCheck, 1, 1, imagePointer);

            // Reading the value of the first logical cluster.
            fseek(imagePointer, dataLocation+ 26, SEEK_SET);
            fread(&firstCluster, 1, 2, imagePointer);

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
            dataParsing(imagePointer, dataLocation, firstCluster, TRUE);
            
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

            // Checking if directory matches path.
            if(strcmp(previousDirectory, subDirectoryData->path) == 0){
                
                directoryData->pathFound = TRUE;
                updateDirectory(imagePointer, (31 + subDirectoryData->cluster[subLoop]) * SECTORSIZE);
                return;

            }//if
            
            subDirectoryData->count--;
            searchSubDirectory(imagePointer, subDirectoryData->cluster[subLoop]);

            // Return when path location is found.
            if(directoryData->pathFound == TRUE){

                return;

            }//if

        }//for

        // Setting entryLocation to the next data sector location.
        entryLocation = value;

    }//while

}//searchSubDirectory

/*
*   Function: searchRoot()
*   ________________________________
*   Finds the first free entry in the root directory.
*
*/
void searchRoot(FILE* imagePointer){

    // Variables.
    int rootDirectory = 9728;   // Integer for storing the starting byte location of the root directory.
    int directoryFree = 0;      // Integer for storing the first byte of a directory entry file name.
    int firstCluster = 0;       // Integer for storing the first cluster value of a directory entry.
    int entryLooper;            // Integer for a for-loop.
    int freeEntryFound = FALSE; // Integer flag for checking if free entry was found.
    int freeEntryLocation = 0;  // Integer for tracking the free location.

    // Read all directories while root directory value is less than end of root directory value
    while(rootDirectory < ENDOFROOTDIRECTORY){

        // Reading the first byte of the filename.
        fseek(imagePointer, rootDirectory, SEEK_SET);
        fread(&directoryFree, 1, 1, imagePointer);

        // If the first byte of the file name is 0 means all the entries in this sector are empty.
        if(directoryFree == 0x00){

            freeEntryLocation = rootDirectory;

            break;

        }//if

        // Loop through all 16 directory entries.
        for(entryLooper = 0; entryLooper < DIRECTORYENTRIES; entryLooper++){

            // Variables.
            int dataLocation = rootDirectory + (entryLooper * 32); // Integer for storing the location of each entry. (Each directory entry is 32 bytes long).

            // Reading the first byte of the filename.
            fseek(imagePointer, dataLocation, SEEK_SET);
            fread(&directoryFree, 1, 1, imagePointer);

            // Reading the value of the first logical cluster.
            fseek(imagePointer, dataLocation + 26, SEEK_SET);
            fread(&firstCluster, 1, 2, imagePointer);

            // Checking if entry is unused.
            if(directoryFree == 0){

                freeEntryLocation = dataLocation;
                freeEntryFound = TRUE;
                break;

            }//if

            // If a directory is unused (229(0xE5)) or first cluster is a 0 or 1, go to next entry of the sector. 
            if(firstCluster == 0 || firstCluster == 1){

                continue;

            }//if

            
        }//for

        // Check if free entry was found, if so, break out of while loop.
        if(freeEntryFound == TRUE){

            break;

        }//if

        // Go to next root directory sector.
        rootDirectory = rootDirectory + SECTORSIZE;

    }//while

    updateDirectory(imagePointer, freeEntryLocation);

}//searchRoot

/*
*   Function: updateDirectory()
*   ________________________________
*   Updates the directory location with the file location.
*
*/
void updateDirectory(FILE* imagePointer, int location){

    // Write the file name out.    
    fseek(imagePointer, location, SEEK_SET);
    fwrite(fileInformation->fileOutputname, 1, sizeof(fileInformation->fileOutputname), imagePointer);

    // Write the file extension.
    fseek(imagePointer, location + 8, SEEK_SET);
    fwrite(fileInformation->fileExtension, 1, sizeof(fileInformation->fileExtension), imagePointer);

    // Write the file size.
    fseek(imagePointer, location + 28, SEEK_SET);
    fwrite(&fileInformation->size, 1, sizeof(fileInformation->size), imagePointer);

    // Write the first cluster out.
    fseek(imagePointer, location + 26, SEEK_SET);
    fwrite(&fileInformation->firstCluster, 1, sizeof(fileInformation->firstCluster), imagePointer);


}//updateDirectory

/*
*   Function: placedBytes()
*   ________________________________
*   Places the bytes of the file into the image file.
*
*/
void placeBytes(FILE* imagePointer){
    
    // Variables.
    FILE* filePointer;
    int entryLocation = 0;      // Integer for tracking the FAT entry location.
    int oldEntryLocation = 0;   // Integer for tracking the old FAT entry location.
    int printedBytes = 0;       // Integer for tracking the number of bytes that have been printed.
    int outputByteCap = SECTORSIZE; // Integer for the for-loop condition.
    int writeByte = 0;              // Integer for storing the byte to be written.
    int loopCounter;                // Integer for a for-loop.
    int fileSize = fileInformation->size;   // Integer for storing the file size.

    // No need for error handling on this file as it was already opened earlier in the programs execution.
    filePointer = fopen(fileInformation->filename, "rb");

    // Get the startting location for the file.
    entryLocation = getFATEntry(imagePointer, 0);

    // Storing the first logical cluster value.
    fileInformation->firstCluster = entryLocation;

    // Loop until the entire file has been copied over.
    while(printedBytes < fileInformation->size){

        // Variables.
        int physicalAddress = (31 + entryLocation) * SECTORSIZE;


        // Set the for-loop condition to be less than a sector when remaining bytes are under 512.
        if((fileSize - printedBytes) < SECTORSIZE){

            outputByteCap = fileSize - printedBytes;

        }//if

        // Move file pointer to the start of the physicalAddress location.
        fseek(imagePointer, physicalAddress, SEEK_SET);

        // Loop for writing bytes to the image.
        for(loopCounter = 0; loopCounter < outputByteCap; loopCounter++){
            
            // If the number of printed bytes is greater than the file size, break out of this for loop.
            if(printedBytes > fileInformation->size){

                break;

            }//if

            // Read a byte from the file.
            fread(&writeByte, 1, 1, filePointer);

            // Place a byte in the image file.
            fputc(writeByte, imagePointer);

            printedBytes++;

        }//for
        
        // Store old FAT entry.
        oldEntryLocation = entryLocation;

        // Get the next free FAT entry location.
        entryLocation = getFATEntry(imagePointer, entryLocation);

        if(printedBytes == fileInformation->size){

            updateFAT(imagePointer, oldEntryLocation, 0xFFF);
            return;

        }else{

            // Update old FAT entry to point to the new location.
            updateFAT(imagePointer, oldEntryLocation, entryLocation);
        }

    }//while

}//placeBytes

/*
*   Function: updateFAT()
*   ________________________________
*   Updates the old FAT location to point to the new location.
*
*/
void updateFAT(FILE* imagePointer, int oldLocation, int newLocation){

    // Variables.
    unsigned char fourBits; // Unsigned character for placing the value into the FAT.
    unsigned char eightBits; // Unsigned character for placing the value into the FAT.
    int entryLocation = (3 * oldLocation) / 2; // Integer for finding the FAT entry to update.

    // Check if the oldLocation is even.
    if((oldLocation % 2) == 0){

        // Extracting the 4 bit value from the new location.
        fourBits = (newLocation >> 8) & 0x0F;
        eightBits = newLocation;

        // Move pointer to the correct location and insert the values.
        fseek(imagePointer, entryLocation + SECTORSIZE, SEEK_SET);
        fputc(eightBits, imagePointer);
        fputc(fourBits, imagePointer);

    }else{

        // Extracting the 4 bit value from the new location.
        fourBits = (newLocation << 4) & 0xF0;
        eightBits = (newLocation >> 4);

        // Move pointer to the correct location and insert the values.
        fseek(imagePointer, entryLocation + SECTORSIZE, SEEK_SET);
        fputc(fourBits, imagePointer);
        fputc(eightBits, imagePointer);

    }//if-else


}//updateFAT

/*
*   Function: getFATEntry()
*   ________________________________
*   Finds the next free location in the FAT. Based on the entryLocation value.
*   If 0 is passed, then functions returns the first free FAT entry.
*
*/
int getFATEntry(FILE* imagePointer, int entryLocation){

    // Variables.
    int entryIndex = entryLocation; // Integer for tracking the FAT entry index.
    int freeEntry = FALSE;  // Integer flag for checking if a FAT entry is unused.
    int entryValue = 0;     // Integer for tracking the FAT entry value.
    int fourBitValue = 0;   // Integer for stoing the 4 bits of the FAT entry.
    int eightBitValue = 0;  // Integer for storing the 8 bits of the FAT entry.
    
    // If entryLocation is 0, start at the first index of the FAT.
    if(entryLocation == 0){

        entryIndex = 2;

    }//if

    // Loop through the FAT entries, looking for a free index.
    while(freeEntry == FALSE){

        int entryByteLocation = (3 * entryIndex) / 2;    // Integer of the lower index location. (Add 1 to this value to higher index location)

        if((entryIndex % 2) == 0){

            // Reading the byte of the entry.
            fseek(imagePointer, SECTORSIZE + entryByteLocation, SEEK_SET);
            fread(&eightBitValue, 1, 1, imagePointer);

            // Reading the byte that contains the high nibble of the entry.
            fread(&fourBitValue, 1, 1, imagePointer);

            // Isolating the lower nibble of the byte and shifting it eight spots to the left.
            fourBitValue = (fourBitValue & 0x0F) << 8;

            // Combined the bits.
            entryValue = fourBitValue + eightBitValue;

        }else{
            
            // Reading the byte contains the low nibble of the byt entry.
            fseek(imagePointer, SECTORSIZE + entryByteLocation, SEEK_SET);
            fread(&fourBitValue, 1, 1, imagePointer);

            // Isolationg the high nibble and shifting it four positions to the right.
            fourBitValue = fourBitValue & 0xF0;

            // Reading the byte of the entry.
            fread(&eightBitValue, 1, 1, imagePointer);

            // Shifting the readin values into their correct locations.
            entryValue = (eightBitValue << 4) + (fourBitValue >> 4);

        }// if-else

        // Check if entry value is 0.
        if(entryValue == 0x000){

            return entryIndex;

        }//if

            entryIndex++;

        }//while


    return 0;

}//getFATEntry

/*
*   Function: dataParsing()
*   ________________________________
*   Reads all the files and directories in the directory entry.
*   
*   If reading a subdirectory pass TRUE to subDirectory.  
*
*/
void dataParsing(FILE* filePointer, int dataLocation, int firstCluster, int subDirectory){
    
    // Variables.
    int fileAttributes = 0; // Integer for storing the file's attribute.

    unsigned char fileName[13];     // Unsigned character array for storing the entire file name, including it's exetension.

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

            // Add the first cluster value into the subdirectory global integer array.
            subDirectoryData->cluster[subDirectoryData->count] = firstCluster;

            // Copy the file name into the subdirectory global character array.
            memcpy(subDirectoryData->name[subDirectoryData->count], fileName, 9);

            // Increment the global subdirectory count variable.
            subDirectoryData->count++;

        }//if-else

    }//if

}//dataParsing

/*
*   Function: isEnoughSpace()
*   ________________________________
*   Checks if the image has enough space to store the passed file.
*
*/
int isEnoughSpace(FILE* imagePointer){

    // Variables.
    int diskFreeSpace = 0;  // Integer for tracking the free space on the image.
    int fileSize = 0;       // Integer for tracking the file size of the input file.
    FILE* pathFilePointer;  // File pointer for doing operations on the passed file.

    // Calcualting the free space on the disk image
    calculateFreeSpace(imagePointer, &diskFreeSpace);

    // Open the passed file.
    pathFilePointer = fopen(fileInformation->filename, "rb");
    
    // Error handling if file does not exist.
    if(pathFilePointer == NULL){

        printf("Error: %s does not exist\n", fileInformation->filename);
        fclose(imagePointer);
        exit(0);

    }//if

    // Getting disk space into bytes.
    diskFreeSpace = diskFreeSpace * SECTORSIZE;

    // Get the size of the passed file.
    fseek(pathFilePointer, 0, SEEK_END);
    fileSize = ftell(pathFilePointer);

    // Move the file pointer back to the start of the file and then close the file.
    rewind(pathFilePointer);
    fclose(pathFilePointer);

    // Checking if their is enough space.
    if(fileSize <= diskFreeSpace){
        
        // Adding the file size to file information struct to be stored.
        fileInformation->size = fileSize;
        return TRUE;

    }//if

    return FALSE;

}//isEnoughSpace

/***************** Main Function *******************/
int main(int argc, char *argv[]){

    // Variables
    char *imageFile = argv[1];        // String for storing the passed image file.
    char *pathToFile = argv[2];       // String for storing the passed path name.
    char directoryPath[256];          // String for getting the directory path without the file name.
    char checkFileUpper[5] = ".IMA";  // Character array for checking upper case file extension.
    char *isValidFileUpperCase;       // Character pointer for storing the results from checking if the file extension is upper case.
    char checkFileLower[5] = ".ima";  // Character array for checking upper case file extension.
    char *isValidFileLowerCase;       // Character pointer for storing the results from checking if the file extension is upper case.
    char *stringToken;                // Character pointer for separating the passed string by the '/'.
    char *parsedString[256];          // Character array used for storing the parsed path name.
    char *fileSplitDot[32];           // Character array used for splitting the dot from the filename.
    char rawFile[256];

    int parsedStringIndex = 0;        // Integer for indexing the parsedString array.
    int splitDotIndex = 0;            // Integer for indexing the parsedString array.
    int loopCounter;                  // Integer for a for for-loop.

    // Error handling if not enough arguments are passed.
    if(argc != 3){
        
        printf("Error: Missing or Extra arguments.\n");
        printf("Intended execution: ./diskput test.ima <(Optional subdirectory path) A file to store>\n");
        exit(0);

    }//if

    // File pointer.
    FILE *imagePointer;

    // Open file in read binary mode.
    imagePointer = fopen(imageFile, "r+");

    // File passed does not exist.
    if(imagePointer == NULL){

        printf("Error: The passed image file %s does not exist.", imageFile);
        exit(0);

    }//if

    // Checking the passed file.
    isValidFileUpperCase = strstr(imageFile, checkFileUpper);
    isValidFileLowerCase = strstr(imageFile, checkFileLower);
    if(isValidFileUpperCase == isValidFileLowerCase){

        printf("Error: %s is not a .IMA file.\n", imageFile);
        exit(0);

    }//if

    // Splitting the pass path string by '/'.
    stringToken = strtok(pathToFile, "/");

    while(stringToken != NULL){

        parsedString[parsedStringIndex] = stringToken;
        stringToken = strtok(NULL, "/");
        parsedStringIndex++;

    }//while

    // Allocate space for the file struct.
    fileInformation = malloc(sizeof(FileData));

    // Storing the file name to be opened later.
    strcpy(rawFile, parsedString[parsedStringIndex - 1]);
    fileInformation->filename = rawFile;

    // Splitting the file name from the '.';
    stringToken = strtok(parsedString[parsedStringIndex - 1], ".");
    while(stringToken != NULL){

        fileSplitDot[splitDotIndex] = stringToken;
        stringToken = strtok(NULL, ".");
        splitDotIndex++;

    }//while


    // Add the file name to the fileInformation struct.
    fileInformation->fileOutputname = fileSplitDot[0];
    fileInformation->fileExtension = fileSplitDot[1];

    // Converting the file name and extension to all upper case.
    convertToUpper(fileInformation->fileOutputname);
    convertToUpper(fileInformation->fileExtension);

    // Check the size of the file to be stored.
    if(isEnoughSpace(imagePointer) == FALSE){

        printf("Error: Not enough free space in the disk image.\n");
        fclose(imagePointer);
        exit(0);

    }//if

    // Store the file on the image.
    placeBytes(imagePointer);

    // If parsed string index is greater than 1 it means a path to a subdirectory was provided.
    if(parsedStringIndex > 1){ 

        // Allocating memory space for the directory data structs.
        directoryData = malloc(sizeof(DirectoryData));
        subDirectoryData = malloc(sizeof(DirectoryData)); 
        
        // Build the subdirectory path which excludes the file that is to be stored.
        for(loopCounter = 0; loopCounter < parsedStringIndex - 1; loopCounter++){

            strcat(directoryPath, "/");
            strcat(directoryPath, parsedString[loopCounter]);

        }//for

        // Convert the path to all upper case.
        convertToUpper(directoryPath);

        // Store the path string in the structs.
        strcpy(directoryData->path, directoryPath);
        directoryData->pathFound = FALSE;

        strcpy(subDirectoryData->path, directoryPath);

        findSubDirectory(imagePointer);

        // The path does not exist in the image.
        if(directoryData->pathFound == FALSE){

            printf("The directory was not found.\n");
            exit(0);

        }//if

        // Store the file in the image.
        placeBytes(imagePointer);

        // Freeing the memory allocated to the directory structs.
        free(directoryData);
        free(subDirectoryData);

    }else{

        // Update the root.
        searchRoot(imagePointer);

    }//if-else

    // Closed the opened input file.
    fclose(imagePointer);

    // Free the allocated memory.
    free(fileInformation);

    return 0;
}//main