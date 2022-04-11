#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Constants
#define SECTORSIZE 512

/*
*   Function: convertToUpper()
*   ________________________________
*   Checks if the input file name is all uppercase, if not, make it all uppercase.
*   
*
*/
void convertToUpper(char* file){

    // Variables
    int fileNameLength = strlen(file);
    int upperCheckLoop;

    // Checking if each letter is upper case, if not, make it upper case.
    for(upperCheckLoop = 0; upperCheckLoop < fileNameLength; upperCheckLoop++){

        if(isupper(file[upperCheckLoop]) == 0){

            file[upperCheckLoop] = toupper(file[upperCheckLoop]);

        }//if

    }//for

}//converToUpper

/*
*   Function: removePadding()
*   ________________________________
*   Removes the trailing white space from a filename.
*   
*
*/
void removePadding(unsigned char *string){

    // Variables
    int endOfStringIndex = -1;
    int stringIndex = 0;

    // Loop until the null terminating character is found.
    while(string[stringIndex] != '\0'){

        // If the character at this index is not a whitespace update the endOfStringIndex variable to this index value.
        if(string[stringIndex] != ' '){

            endOfStringIndex = stringIndex;

        }//if

        stringIndex++;

    }//while

    // Insert a null terminating character in the spot just after the last character.
    string[endOfStringIndex + 1] = '\0';

}//removePadding

/*
*   Function: calculateFreeSpace()
*   ________________________________
*   Scans the FAT12 entires for unused entries.
*
*/
void calculateFreeSpace(FILE* imagePointer, int* freeSpace){

    // Variables
    int entryLoop;              // Integer for for-loop.
    int freeSpaceCounter = 0;   // Integer for tracking free space.
    int fatSector = SECTORSIZE; // Integer of current FAT sector.
    int totalEntries = 0;       // Integer for storing total sector entries.


    // Reading total size of the disk.
    fseek(imagePointer, 19, SEEK_SET);
    fread(&totalEntries, 1, 2, imagePointer);

    // Iterate over the FAT entries
    for(entryLoop = 2; entryLoop < (totalEntries - 31); entryLoop++){

        // Variables to be reset each iteration. (Ensuring no junk is left in variables)
        int value = 0;              // Integer for storing the entry value.
        int fourBitValue = 0;       // Integer for storing the four bits.
        int eightBitValue = 0;      // Integer for storing the eight bits.

        int entryByteLocation = (3 * entryLoop) / 2;    // Integer of the lower index location. (Add 1 to this value to higher index location)


        // Checking if current entry is even or odd.
        if((entryLoop % 2) == 0){

            // Reading the byte of the entry.
            fseek(imagePointer, fatSector + entryByteLocation, SEEK_SET);
            fread(&eightBitValue, 1, 1, imagePointer);

            // Reading the byte that contains the high nibble of the entry.
            fread(&fourBitValue, 1, 1, imagePointer);

            // Isolating the lower nibble of the byte and shifting it eight spots to the left.
            fourBitValue = (fourBitValue & 0x0F) << 8;

            value = fourBitValue + eightBitValue;

        }else{
            
            // Reading the byte contains the low nibble of the byt entry.
            fseek(imagePointer, fatSector + entryByteLocation, SEEK_SET);
            fread(&fourBitValue, 1, 1, imagePointer);

            // Isolationg the high nibble and shifting it four positions to the right.
            fourBitValue = fourBitValue & 0xF0;

            // Reading the byte of the entry.
            fread(&eightBitValue, 1, 1, imagePointer);

            // Shifting the readin values into their correct locations.
            value = (eightBitValue << 4) + (fourBitValue >> 4);

        }// if-else

        // If value is 0, space is free.
        if(value == 0x0000){

            freeSpaceCounter++;

        }//if

    }//for 

    // Setting the point value.
    *freeSpace = freeSpaceCounter;

}//calculateFreeSpace