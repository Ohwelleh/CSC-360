.phony all:
all: disk
disk: diskinfo.c disklist.c diskget.c diskput.c Helpers.c
	gcc -Wall diskinfo.c Helpers.c -o diskinfo
	gcc -Wall disklist.c Helpers.c -o disklist
	gcc -Wall diskget.c Helpers.c -o diskget
	gcc -Wall diskput.c Helpers.c -o diskput

.PHONY clean:
clean:
	-rm -rf *.o *.exe