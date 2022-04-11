.phony all:
all: acs
acs: acs.c Queue.c
	#gcc -Wall -lpthread acs.c Queue.c -o ACS # Uncomment this line if pthread error, Comment out the line below.
	gcc -Wall -pthread acs.c Queue.c -o ACS

.PHONY clean:
clean:
	-rm -rf *.o *.exe