To compile the programs:
    Run make or Make on the terminal command line.

To execute the programs:
    ./diskinfo <Pass IMA file>
    ./disklist <Pass .IMA file>
    ./diskget <Pass .IMA file> <A file to find on the .IMA>
    ./diskput <Pass .IMA file> <(Opitional: Directory Location) A file to store on .IMA>

These set of programs were developed for a simple file system assignment which used the FAT12 system.
*Note* This program does not add the creation time or date to the file.

Diskinfo:
    Takes in an .IMA file and finds and outputs the following information:
        - OS Name
        - Label of the disk
        - Total size of the disk
        - Free size of the disk
        - Number of files in the disk
        - Number of FAT copies
        - Sectors per FAT

Disklist:
    Takes in an .IMA file and starts from the root directory and lists all the contents of the root directory and all subdirectories with the output formated as follows:

        Direcotry Name
        ==================
        File/Directory   Size   File/Directory Name    Creation Date   Creation Time

        Example Output:
            ROOT
            ==================
            D         0          SUB1 1980-00-00 00:00 
            D         0          SUB2 1980-00-00 00:00 
            F     14657   FIGURE1.JPG 2020-09-12 11:24 
            /SUB1 
            ==================
            D         0       SUBSUB1 1980-00-00 00:00 
            /SUB1/SUBSUB1 
            ==================
            D         0      SUBSUB11 1980-00-00 00:00 
            F     11052      2F11.JPG 2008-11-06 20:55 

Diskget:
    Takes in an .IMA file and a file name to search for. If the file exsits then it will be copied to the same directory that diskget is run in.
    Otherwise, the message 'File not found' will be returned and the program exits.

Diskput:
    Takes in an .IMA file and a file name with an optional path to a specific directory. The file will then tried to be stored on the .IMA if spaces allows it.
    If not path is specified, then the file will be stored in the root directory.