/**
 * @author Steve Tolvaj
 * CIS 3207 001
 * Project 2: Creating a Linux Type Shell Program
 * 
 * The myshell.h file is used to link the myshell.c(contians the myshell program logic) 
 * and utility.c file(contains built in and utility functions).
**/

#ifndef MyShell_H
#define MyShell_H

int changeDir(char **args);
void clearPrompt();
int countArgs(char **args);
int printDirectory(char **args);
int printEnvp(char **args);
void echoPrint(char **args);
int printHelp();
void pausePrompt();
int addPath (char **args);

#endif