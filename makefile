# Steve Tolvaj, CIS 3207-001, Operating Systems Project 2 - Implement a Linux Type Shell.
myshell: myshell.c utility.c myshell.h
	gcc -Wall -Werror myshell.c utility.c -o myshell