#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define MAX_ARGS 100

void parser(char *input, char **args);
int checkParsedArgs(char **args);
int redirect(char **args, int position);
int redirectAppend(char **args, int position);
void redirectInput(char *arg1, char *arg2);

const char error_message[30]="An error has occurred\n";

int main(int argc, char const *argv[], char const *envp[])
{

    char *args[MAX_ARGS];    // The arguments extracted from stdin or batch file after parsing.
    char *buff;     // The buffer used for getline().
    size_t size = 0;// Size used for getline().
    int fileSpecifiedFlag = 0;  // Flag used to print prompt if batch file was not used.
    
    // If myshell is invoked with more than 1 file, print error and exit.
    if(argc > 2) {
        write(STDERR_FILENO,error_message,strlen(error_message));
        exit(1);
    }
    // If filename is specified in arguments set the file descriptor to stdin descriptor.
    if (argc == 2) { 
        int fd = open(argv[1], O_RDONLY);
        dup2(fd, STDIN_FILENO);
        close(fd);
        fileSpecifiedFlag = 1;

        // Exit with code 1 and specified error message if bad batch file.
        if (fd == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
    } else {    // Only print prompt if no batch file entered.
        printf("%s/myshell>", getenv("PWD"));
    }

    // Read input from stdin.
    while(getline(&buff, &size, stdin) != -1) {
        // Replace newline with \0 if exists.
        if(strchr(buff, '\n') != NULL) {
            buff[strlen(buff) - 1] = '\0';
        }
        
        // If quit is typed exit with 0.
        if (strcmp(buff, "quit") == 0) {
            exit(0);
        }
        parser(buff, args);
        checkParsedArgs(args);
        printf("%s\n", buff);
        // Only print prompt if no batch file entered.
        if (!fileSpecifiedFlag) {
             printf("%s/myshell>", getenv("PWD"));
        }
       
    }
    
    free(buff);
    
    return 0;
}



/**
 * The parser function will seperate input by spaces into
 * the supplied **args parameter.
 * 
 * @param input The input to be parsed by space delimeter.
 * @param args The array of strings to store the parsed input into.
**/
void parser(char *input, char **args) {

    int i = 0;
    // split at spaces and tabs.
    while((args[i++] = strtok_r(input, "\t, ", &input))){}
}

int checkParsedArgs(char **args) {
    int i = 0;
    
    while(args[i] != NULL) {
        if(strcmp(args[i], ">>") == 0) {
            // Set terminating position of args so exec knows when to stop.
            args[i] = NULL;
            if (redirect(args, i) == -1) {
                write(STDERR_FILENO,error_message,strlen(error_message));
            }
        }
        else if(strcmp(args[i], ">") == 0) {
            // Set terminating position of args so exec knows when to stop.
            args[i] = NULL;
            if (redirectAppend(args, i) == -1) {
                write(STDERR_FILENO,error_message,strlen(error_message));
            }
        }

        i++;
    }
    return 0;
}

/**
 * The redirect function will direct the executed command args up to the first NULL in the arguments.
 * This will cause a fork and run the arguments in a child function and redirect output to the filename
 * located at the argument after the first NULL(Position + 1).
 * 
 * @param args The line of arguments from user input.
 * @param position The position of the first null located in args.
 * 
 * @return Will return -1 if failure occurred or 0 if successfull.
**/
int redirect(char **args, int position) {
    
    int fd = open(args[position + 1], O_CREAT | O_TRUNC | O_WRONLY);

    if(fd == -1) {
        return -1;
    }
 
    int rc = fork();

    if (rc < 0)
    {
        return -1;
    }

    if (rc == 0) {
        if (dup2(fd, STDOUT_FILENO) == -1) {
            return -1;
        }
        close(fd);
        execvp(args[0], args);
    }
    wait(NULL);
    return 0;
}

/**
 * The redirect function will direct the executed command args up to the first NULL in the arguments.
 * This will cause a fork and run the arguments in a child function and redirect output to the filename
 * located at the argument after the first NULL(Position + 1). If the file exists it will append to 
 * the filename specified.
 * 
 * @param args The line of arguments from user input.
 * @param position The position of the first null located in args.
 * 
 * @return Will return -1 if failure occurred or 0 if successfull.
**/
int redirectAppend(char **args, int position) {
    printf("%s\n", args[position + 1]);
    int fd = open(args[position + 1], O_CREAT | O_WRONLY | O_APPEND);

    if(fd == -1) {
        return -1;
    }
 
    int rc = fork();

    if (rc < 0)
    {
        return -1;
    }

    if (rc == 0) {
        if (dup2(fd, STDOUT_FILENO) == -1) {
            return -1;
        }
        close(fd);
        execvp(args[0], args);
    }
    wait(NULL);
    return 0;
}

void redirectInput(char *arg1, char *arg2) {
    
}
