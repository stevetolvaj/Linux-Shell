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
int redirectInput(char **args, int *position);
int execute(char **args, int position);

const char error_message[30]="An error has occurred\n";

int main(int argc, char const *argv[], char const *envp[])
{
    FILE *input;
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
        input = fopen(argv[1], "r");
       
        fileSpecifiedFlag = 1;

        // Exit with code 1 and specified error message if bad batch file.
        if (input == NULL) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
    } else {    // Only print prompt if no batch file entered.
        input = stdin;
        printf("%s/myshell>", getenv("PWD"));
    }

    // Read input from stdin.
    while(getline(&buff, &size, input) != -1) {
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
    // File descriptor to save prev output.
    int foundSpecial = 0;

    while(args[i] != NULL) {
        if(strcmp(args[i], ">") == 0) {
            // Set terminating position of args so exec knows when to stop.
            args[i] = NULL;
            if (redirect(args, i) == -1) {
                write(STDERR_FILENO,error_message,strlen(error_message));
            }
            foundSpecial = 1;
        }
        else if(strcmp(args[i], ">>") == 0) {
            // Set terminating position of args so exec knows when to stop.
            args[i] = NULL;
            if (redirectAppend(args, i) == -1) {
                write(STDERR_FILENO,error_message,strlen(error_message));
            }
            foundSpecial = 1;
        } else if(strcmp(args[i], "<") == 0) {
            args[i] = NULL;
            if (redirectInput(args, &i) == -1) {
                write(STDERR_FILENO,error_message,strlen(error_message));
            }
            foundSpecial = 1;
        } 
        i++;
    }

    if (!foundSpecial)
    {
        if (execute(args, i) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
        }
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
 * @return Will return -1 if failure occurred or 0 if successful.
**/
int redirect(char **args, int position) {
    
    int fd = open(args[position + 1], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);

    if(fd == -1) {
        return -1;
    }
 
    int rc = fork();

    if (rc < 0)
    {
        return -1;
    } else if (rc == 0) {
        if (dup2(fd, STDOUT_FILENO) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
        close(fd);
        if (execvp(args[0], args) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
        exit(0);
    } else {
        wait(NULL);
    }
   
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
 * @return Will return -1 if failure occurred or 0 if successful.
**/
int redirectAppend(char **args, int position) {
    // Open descriptor with filename located after '>>'
    int fd = open(args[position + 1], O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);

    if(fd == -1) {
        return -1;
    }
 
    int rc = fork();

    // Errors are handled within the child.
    if (rc < 0)
    {
        return -1;
    } else if (rc == 0) {
        if (dup2(fd, STDOUT_FILENO) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
        close(fd);
        if (execvp(args[0], args) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
        exit(0);
    } else {
        wait(NULL);
    }
    
    return 0;
}

/**
 * The redirectInput function will direct the executed command args up to the first NULL in the arguments.
 * This will cause a fork and run the arguments in a child function and direct input from the filename
 * located at the argument after the first NULL(Position + 1). If a '>' or '>>' is found after the filename
 * it will perform the output redirection required.
 * 
 * @param args The line of arguments from user input.
 * @param position The position of the first null located in args.
 * 
 * @return Will return -1 if failure occurred or 0 if successful.
**/
int redirectInput(char **args, int *position) {

    int outFd; // Output file descriptor if needed.
    int redirectOut = 0; // Flag for checking if output is needed.

    // Check if next two args are null. If they are do not check for output redirection.
    if(args[*position+2] != NULL && args[*position+3] != NULL) {
        // Check if truncation or appending is needed for file descriptors if '>' or '>>' is found.
        if(strcmp(args[*position + 2], ">") == 0) {
            outFd = open(args[*position + 3], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);  
            redirectOut = 1;
        }
        if(strcmp(args[*position + 2], ">>") == 0) {
            outFd = open(args[*position + 3], O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);  
            redirectOut = 1;     
        }
    }

    // Open input redirection file with filename after the '<' in args.
    int fd = open(args[*position + 1], O_RDONLY);

    if(fd == -1) {
        return -1;
    }
 
    int rc = fork();

    if (rc < 0)
    {
        return -1;
    } else if (rc == 0) {

        if (dup2(fd, STDIN_FILENO) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
        
        // Redirect to output if output redirect is found.
        if(redirectOut == 1) {
            if(dup2(outFd, STDOUT_FILENO) == -1) {
                write(STDERR_FILENO,error_message,strlen(error_message));
                exit(1);
            }
            close(outFd);
        }
        
        close(fd);
        if (execvp(args[0], args) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
        exit(0);
    } else {
      
        wait(NULL);

        // If output redirection was found in args increase position in while loop to the output
        // filename location.
        if(redirectOut) {
             (*position) += 3;
        }
       
    }
    
    return 0;
}

/**
 * The execute function will fork and create a child process for the
 * execution of arguments. Only used when redirection or piping is not needed.
 * 
 * @param args The line of arguments from user input.
 * @param position The position of the first null located in args.
 * 
 * @return Will return -1 if failure occurred or 0 if successful.
**/
int execute(char **args, int position) {
    
    int rc = fork();

    if (rc < 0)
    {
        return -1;
    } else if (rc == 0) { 
        if (execvp(args[0], args) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
    } else {
        wait(NULL);
    }
    
    return 0;
    
}