/**
 * @author Steve Tolvaj
 * CIS 3207 001
 * Project 2: Creating a Linux Type Shell Program
 * 
 * The myshell.c file includes redirection and logic for the user prompts. This includes
 * the calls to the built in functions located in utility.c which is linked by the header file
 * myshell.h.
**/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<dirent.h>
#include<sys/wait.h>
#include<string.h>
#include<fcntl.h>
#include"myshell.h"

#define MAX_ARGS 100

extern char **environ;

void parser(char *input, char **args);
void checkParsedArgs(char **args);
int redirect(char **args, int position);
int redirectAppend(char **args, int position);
int redirectInput(char **args, int *position);
int execute(char **args, int *position);
int findWait(char **args, int position);
void removeArgs(char **args, int position);
int runBuiltIns(char **args);
int checkRedirectBuiltIn(char **args);

const char error_message[30]="An error has occurred\n";


int main(int argc, char const *argv[])
{
    
    // Ovewrite path variable to only contain /bin at start of program.
    setenv("PATH","/bin", 1);
    // The shell environment should contain shell=<pathname>/myshell where <pathname>/myshell
    char direct[PATH_MAX];
    // Get current directory that the program was started from.
    getcwd(direct,PATH_MAX);
    // Make and set new shell variable.
    setenv("shell", direct, 1);
 
    // printHelp();
    FILE *input;
    char *args[MAX_ARGS];    // The arguments extracted from stdin or batch file after parsing.
    char *buff = NULL;     // The buffer used for getline().
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
        printf("%s: myshell>", getenv("PWD"));
    }
    
    // Read input from stdin.
    while(getline(&buff, &size, input) != -1) {
        // Replace newline with \0 if exists.
        if(strchr(buff, '\n') != NULL) {
            buff[strlen(buff) - 1] = '\0';
        }
        
        // If quit is typed exit with 0.
        if (strcmp(buff, "quit") == 0 || strcmp(buff, "exit") == 0) {
            exit(0);
        }

        // Send buff to parser and stored parsed char arrays in args.
        parser(buff, args);

        // Send the parsed args to check parsed args to check for redirection, piping, or execute command.
        checkParsedArgs(args);

        // Only print prompt if no batch file entered.
        if (!fileSpecifiedFlag) {
             printf("%s: myshell>", getenv("PWD"));
        } 
       
    }
    
    free(buff);
    fclose(input);
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

/**
 * The checkParsedArgs function will find and redirection, pipe, and run in background chars
 * and set the special char argument to NULL. Then it will send the arguments and position of
 * the special char to the apropriate function below. If nothing is found it will send to execute
 * as normal child process.
 * 
 * @param args The arguments parsed by spaces.
**/
void checkParsedArgs(char **args) {
    int i = 1;
    // Flag to indicate if function should run without piping or redirection.
    int foundSpecial = 0;

    // Loop through until a pipe or redirection command is found.
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

    // If no special argument is found run as normal command as child process in execute function.
    if (!foundSpecial)
    {
        if (execute(args, &i) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
        }
    }
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
    
    // Check if it is a built in command that can be redirected.
    int stdO;
    if(checkRedirectBuiltIn(args) == 2) {
        stdO = dup(STDOUT_FILENO);
        if(stdO == -1) {    // If failed return -1 for error checking.
            return -1;
        }
        
        // Replace stdout with filename.
        if(dup2(fd, STDOUT_FILENO) == -1) {
            return -1;       // If failed return -1 for error checking.
        }
        close(fd);
        runBuiltIns(args);
        // Change back to original stdout.
        fflush(stdout);
        if (dup2(stdO, STDOUT_FILENO) == -1) {
            return -1;
        }
        close(stdO);
    } else if(checkRedirectBuiltIn(args) == 1) {    // If built in cannot be redirected print error message.
        write(STDERR_FILENO,error_message,strlen(error_message));
    } else { // If not built in fork() and exec().
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

    // Check if it is a built in command that can be redirected.
    int stdO;
    if(checkRedirectBuiltIn(args) == 2) {
        stdO = dup(STDOUT_FILENO);
        if(stdO == -1) {    // If failed return -1 for error checking.
            return -1;
        }
        
        // Replace stdout with filename.
        if(dup2(fd, STDOUT_FILENO) == -1) {
            return -1;       // If failed return -1 for error checking.
        }
        close(fd);
        runBuiltIns(args);
        // Change back to original stdout.
        fflush(stdout);
        if (dup2(stdO, STDOUT_FILENO) == -1) {
            return -1;
        }
        close(stdO);
    } else if(checkRedirectBuiltIn(args) == 1) {    // If built in cannot be redirected print error message.
        write(STDERR_FILENO,error_message,strlen(error_message));
    } else { // If not built in fork() and exec().
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

    int outFd; // Output file descriptor if needed to redirect out.
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
int execute(char **args, int *position) {
  
    // Testing for running in background.
    // int shouldWait = findWait(args, *position);

    // Check if it is a built in command. Does not matter if it is redirectable by the time
    // it reaches this function.
    if(checkRedirectBuiltIn(args) == 1 || checkRedirectBuiltIn(args) == 2) {
        runBuiltIns(args);
    } else {
         int rc = fork();

        if (rc < 0)
        {
            return -1;
        } else if (rc == 0) { 
            if (execvp(args[0], args) == -1) {
                write(STDERR_FILENO,error_message,strlen(error_message));
                exit(1);
            }
        exit(0);
    } else {
        wait(NULL);

        // Testing for running in background.
        // if (shouldWait == 0) {
        //     wait(NULL);
        // } else {
        //     printf("RUNNING in background, Position: %d\n", shouldWait);
        // }
    }
    }
   
    // Testing for running in background.
    // removeArgs(args, *position);
    //  int j = 0;
    // while (args[j] != NULL) {
    //     printf("%s\n", args[j++]);
    // }
    return 0;
}

/**
 * The runBuiltIns function will run the built in command if the first argument
 * corresponds to a built in function. If it does it is called and 
 * checked for errors.
 * 
 * @param args The arguments that contain input commands.
 * 
 * @return 0 if no built in functions were called, 1 if built in.
**/
int runBuiltIns(char **args) {
    char *firstArg = args[0];
    
    if(strcmp("cd", firstArg) == 0) {
        if (changeDir(args) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
        }
    } else if(strcmp("clr", firstArg) == 0) {
        clearPrompt();
        // no return if succesful
    } else if(strcmp("dir", firstArg) == 0) {
        if (printDirectory(args) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
        }
    }else if(strcmp("environ", firstArg) == 0) {
        if (printEnvp(environ) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
        }
    }else if(strcmp("echo", firstArg) == 0) {
        echoPrint(args);
    }else if(strcmp("help", firstArg) == 0) {
        if (printHelp() == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
        }
        return 2; // returns 2 for redirection capability.
    }else if(strcmp("pause", firstArg) == 0) {
        pausePrompt();
        // no return if succesful
    }else if(strcmp("path", firstArg) == 0) {
        if (addPath(args) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
        }
    }else {
        // If no built in found return 0;
        return 0;
    }
    // Return 1 if it was a built in
    return 1;
}

/**
 * The checkRedirectBuiltIn function will check if it is a built in function
 * call, a redirectable built in, or not a built in function.
 * 
 * @param args The arguments to check.
 * 
 * @return 0 if not built in function. 1 if it is a built in function. 2 if the built in function is redirectable.
**/
int checkRedirectBuiltIn(char **args) {
    int i = 0;
    char *firstArg;
   
    firstArg = args[i++];
    // Check for redirectable built in first.

    if((strcmp("dir", firstArg) == 0) || (strcmp("environ", firstArg) == 0)|| (strcmp("echo", firstArg) == 0)|| (strcmp("help", firstArg) == 0)) {
        return 2;
    } else if((strcmp("cd", firstArg) == 0) || (strcmp("clr", firstArg) == 0)|| (strcmp("pause", firstArg) == 0)|| (strcmp("path", firstArg) == 0)) {
        return 1;
    } else {
        return 0;
    }
    
}

// *****************************************************
// The following are not used yet. Still working out how to 
// run multiple processes in background from one line of 
// command arguments.
// ******************************************************

/**
 * The findWait function will find any ampersand in the arguments to check if
 * process should run in the background. It will also set the ampersand char array to 
 * NULL in the supplied args argument.
 * 
 * @param args The arguments to search through.
 * @param args The position to start the search at.
 * 
 * @return The position of the ampersand that was found.
**/
int findWait(char **args, int position) {
    // Start loop at position of special char.
    // Start at end - 1 if position was already at NULL terminator.
    int i = 0;

    while(args[i] != NULL) {
        if (strcmp(args[i], "&") == 0) {
            args[i] = NULL;
            return i;
        }
        i++;
    }
    return 0;
}

void removeArgs(char **args, int position) {
    int i;
    int j;
    for (j = 0; j < position; j++)
    {
        for(i = 0; i < 100; i++) {
            args[i] = args[i+1];
        }
    }
}