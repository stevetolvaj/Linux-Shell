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

int change_dir(char **args);
void clear_prompt();
int count_args(char **args);
int print_directory(char **args);
int print_envp(char **args);
void echo_print(char **args);
int print_help();
void pause_prompt();
int add_path (char **args);

#endif