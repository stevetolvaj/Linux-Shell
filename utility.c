/**
 * @author Steve Tolvaj
 * CIS 3207 001
 * Project 2: Creating a Linux Type Shell Program
 * 
 * The utility.c file is used to support the built in functions needed for the myshell program.
**/

#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<dirent.h>
#include<string.h>
#include"myshell.h"

/**
 * The changeDir function will change the directory with the supplied
 * arguments the first argument should be cd which is checked beforehand.
 * The second argument will either be NULL and will print the current working 
 * directory or will contain the directory to change to.
 * 
 * @param args The arguments supplied to change or print the directory.
 * 
 * @return Will return 0 if succesful or -1 if error occured.
**/
int changeDir(char **args) {
    int count = countArgs(args);
    int rc;
    char direct[PATH_MAX];

    // Only run if correct amount or args are present.
    if (count != 2) {
        if(count == 1) {
            // Print current directory if no directory was specified.
            getcwd(direct, PATH_MAX);
            printf("%s\n", direct);
            return 0;
        } else {
            return -1;
        }
        
    }
    
    rc = chdir(args[1]);

    if (rc < 0) {
        return rc;
    } else {
        setenv("PWD", getcwd(direct, PATH_MAX), 1);
        return 0;
    }

}

/**
 * The clearPrompt function will clear and remaining prompts and output shown in command line output.
**/
void clearPrompt() {
    printf("\e[1;1H\e[2J");
}

/**
 * The printDirectory function will print the current directory contents if no arguments other than 
 * the first command is supplied. If there is a directory location after the command it will then print
 * that locations directory contents.
 * 
 * @param args The arguments that may include directory path and flags.
 * 
 * @return Will return 0 if succesful or -1 if failure occured.
**/
int printDirectory(char **args) {
    struct dirent *entry;
    DIR *openDirectory; 
    
    // If only built in command is supplied print current directory.
    if (countArgs(args) == 1) {
        openDirectory = opendir(".");
    } else if (args[1] != NULL) {   // If there is an arg present after try to run on that directory.
        openDirectory = opendir(args[1]);
    }
    

    if(openDirectory == NULL) {
        return -1;
    }

    while((entry = readdir(openDirectory)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    closedir(openDirectory);
    return 0;
}

/**
 * The printEnvp will print a const NULL terminated array of character arrays. Useful for printing
 * enviroment variables from main function parameter envp and test printing const **char types.
 * 
 * @param args Any null terminated array of char arrays.
 * 
 * @return If succesful, will return 0, otherwise returns 0.
**/
int printEnvp(char **args) {
    int i = 0;

    while(args[i] != NULL) {
        printf("%s\n", args[i++]);
    }

    // If no args are found return -1 for error checking.
    if (i == 0) {
        return -1;
    } else {
        return 0;
    }
}

/**
 * The echo function will print any commands found after the first command.
 * 
 *@param args The arguments to print after index 0. 
 * 
**/
void echoPrint(char **args) {
    int i = 1;

    while(args[i] != NULL) {
        printf("%s ", args[i++]);
    }
    printf("\n");
}

/**
 * The printHelp function will print the myshell manual located in help.txt
 * located in the original exe directory.
 * 
 * @return 0 if successful or -1 if failure.
**/
int printHelp() {

    // Save original shell path set when program started.
    char *shellPath = getenv("shell");

    if(shellPath == NULL){
        return -1;
    }

    char helpLocation[PATH_MAX];
    // Copy path to new char array to avoid changing environment variable.
    strcpy(helpLocation, shellPath);
    // Concat on the name of the help file.
    strcat(helpLocation, "/help.txt");
    FILE *file = fopen(helpLocation, "r");
    if (file == NULL) {
        return -1;
    } 

    char *buff = NULL;
    size_t size = 0;

    while (getline(&buff, &size, file) != -1) {
        printf("%s", buff);
    }
    printf("\n");

    free(buff);
    return 0;
}

/**
 * The pausePrompt function will pause the program until the enter key is pressed.
**/
void pausePrompt() {
    while(getchar() != '\n') {}
}

/**
 * The addPath function will add any new path enviroment variables specified
 * by the user input starting at argument 1. All extra path args greater than 1
 * will be seperated by ';'.
 * 
 * @param args The arguments specified by the user.
 * 
 * @return -1 if failure, 0 if successful.
**/
int addPath (char **args) {
    int count = countArgs(args);
    int i = 2;

    // No argumnets supplied after command.
    if (count == 1) {
        if(setenv("PATH", "", 1) == -1) {
            return -1;
        }
    } 
    // Exactly one argument supplied after command.
    if (count == 2) {
        if(setenv("PATH", args[1], 1) == -1) {
            return -1;
        }
    }
    char temp[PATH_MAX];
    // More than one argument after command needs ':' seperation.
    if(count > 2) {
        strcpy(temp, args[1]);
        while(args[i] != NULL) {
            strcat(temp, ":");
            strcat(temp, args[i++]);
        }
        if (setenv("PATH", temp, 1) == -1) {
            return -1;
        }
    }

    return 0;
}

/**
 * The countArgs function will count any arguments that occur before the NULL 
 * terminator character array.
 * 
 * @param args The array of args to count.
 * 
 * @return The number of args found before NULL occured.
**/
int countArgs(char **args) {
    int count = 0;
    while(args[count] != NULL) {
        count++;
    }
    return count;
}