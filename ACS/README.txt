To compile my ACS program:
	Run make or Make on the terminal command line.

	*The make file is set to compile with the following*
	gcc -Wall -pthread acs.c Queue.c -o ACS

	*If a pthread compiler error happens, comment out that gcc and uncomment the one above it using # *
	gcc -Wall -lpthread acs.c Queue.c -o ACS

To execute ACS:
	Type ./ACS <text file> on the terminal command line.

NOTE: Using customers over 50 will cause my pthread_cond_signal(&freeClerksConditional) to fail.