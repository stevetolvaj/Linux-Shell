#ifndef MyShell_H
#define MyShell_H

int changeDir(char **args);
void clearPrompt();
int countArgs(char **args);
int printDirectory(char **args);
int printEnvp(const char **args);


#endif