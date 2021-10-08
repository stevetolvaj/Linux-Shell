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
 * enviroment variables from main function parameter envp and test printing.
 * 
 * @param args Any null terminated array of char arrays.
 * 
 * @return If succesful, will return 0, otherwise returns 0.
**/
int printEnvp(const char **args) {
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

int countArgs(char **args) {
    int count = 0;
    while(args[count] != NULL) {
        count++;
    }
    return count;
}