/**
 * @author Steve Tolvaj
 * CIS 3207 001
 * Project 2: Creating a Linux Type Shell Program
 * 
 * The myshell.c file includes redirection, piping, background execution 
 * and logic for the user prompts. This includes the calls to the built 
 * in functions located in utility.c which is linked by the header file
 * myshell.h. The check_parsed function keeps the positions of any 
 * redirection or piping characters found in args. There are 3 functions
 * starting with underscores that may update this position integer when 
 * passed in as an argument: _redirect_input, _execute, _run_pipe, to 
 * update position of args being checked in check_parsed().
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
void check_parsed(char **args);
int redirect(char **args, int position);
int redirect_append(char **args, int position);
int _redirect_input(char **args, int *position);
int _execute(char **args, int *position);
void remove_args(char **args, int position);
int run_builtins(char **args);
int builtin_redirect(char **args);
int _run_pipe(char **args, int *position);

const char error_message[30]="An error has occurred\n";

int main(int argc, char const *argv[])
{
    // The shell environment should contain 'shell=<pathname>/myshell' where path is the directory
    // of the myshell exe as per instructions.
    char direct[PATH_MAX];
    // Get current directory that the program was started from.
    getcwd(direct,PATH_MAX);
    // Make and set new shell variable.
    setenv("shell", direct, 1);
   
    // Your initial shell path should contain one directory '/bin' as per instructions.
    setenv("PATH","/bin", 1);

    FILE *input;
    char *args[MAX_ARGS];    // The arguments extracted from stdin or batch file after parsing.
    char *buff = NULL;     // The buffer used for getline().
    size_t size = 0;// Size used for getline().
    int file_specified_flag = 0;  // Flag used to print prompt if batch file was not used.
    
    // If myshell is invoked with more than 1 file, print error and exit.
    if(argc > 2) {
        write(STDERR_FILENO,error_message,strlen(error_message));
        exit(1);
    }
    // If filename is specified in arguments set the file descriptor to stdin descriptor.
    if (argc == 2) { 
        input = fopen(argv[1], "r");
        file_specified_flag = 1;
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
        check_parsed(args);
        // printf("%s: myshell>", getenv("PWD"));
        // Only print prompt if no batch file entered.
        if (!file_specified_flag) {
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
 * The check_parsed function will find and redirection, pipe, or execute if neither
 * and set the special char argument to NULL. Then it will send the arguments and 
 * position of the special char to the apropriate function below. If nothing is 
 * found it will send to _execute as normal child process.
 * 
 * @param args The arguments parsed by spaces.
**/
void check_parsed(char **args) {
    // Start checking for redirection and piping after first command.
    int i = 1;
    // Flag to indicate if function should run without piping or redirection.
    int found_special = 0;
    // Check if first arg is NULL if null return and get next line from input.
    if(args[i - 1] == NULL) {
        return;
    }
    // Loop through until a pipe or redirection command is found.
    while(args[i] != NULL) {
        if(strcmp(args[i], ">") == 0) {
            // Set terminating position of args so exec knows when to stop.
            args[i] = NULL;
            if (redirect(args, i) == -1) {
                write(STDERR_FILENO,error_message,strlen(error_message));
            }
            found_special = 1;
        }
        else if(strcmp(args[i], ">>") == 0) {
            // Set terminating position of args so exec knows when to stop.
            args[i] = NULL;
            if (redirect_append(args, i) == -1) {
                write(STDERR_FILENO,error_message,strlen(error_message));
            }
            found_special = 1;
        } else if(strcmp(args[i], "<") == 0) {
            args[i] = NULL;
            // update the position of i passed in by reference so same args are not searched and ran again.
            if (_redirect_input(args, &i) == -1) { 
                write(STDERR_FILENO,error_message,strlen(error_message));
            }
            found_special = 1;
        }  else if(strcmp(args[i], "|") == 0) {
            args[i] = NULL;
            // update the position of i passed in by reference so same args are not searched and ran again.
            if (_run_pipe(args, &i) == -1) {  
                write(STDERR_FILENO,error_message,strlen(error_message));
            }
            found_special = 1;
        }  
        i++; // Increment while checking each arg until end of args is reached.
    }

    // If no special argument is found run as normal command as child process in _execute function.
    if (!found_special)
    {
        // update the position of i passed in by reference so same args are not searched and ran again.
        if (_execute(args, &i) == -1) {
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
    if(builtin_redirect(args) == 1) {    // If built in cannot be redirected return -1 for error.
        return -1;
    }

    // If built in is ok open file.
    int fd = open(args[position + 1], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
    if(fd == -1) {
        return -1;
    }

    // Check if built in should run or fork with exec.
    int stdO;
    if(builtin_redirect(args) == 2) {
        stdO = dup(STDOUT_FILENO);
        if(stdO == -1) {    // If failed return -1 for error checking.
            return -1;
        }
        
        // Replace stdout with filename.
        if(dup2(fd, STDOUT_FILENO) == -1) {
            return -1;       // If failed return -1 for error checking.
        }
        close(fd);
        run_builtins(args);
        // Change back to original stdout.
        fflush(stdout);
        if (dup2(stdO, STDOUT_FILENO) == -1) {
            return -1;
        }
        close(stdO);
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
            waitpid(rc, NULL, 0);
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
 * @return Will return -1 if failure occurred in parent process or 0 if successful.
**/
int redirect_append(char **args, int position) {

    if(builtin_redirect(args) == 1) {    // If built in cannot be redirected return -1 for error.
        return -1;
    }
    
    // Open descriptor with filename located after '>>'
    int fd = open(args[position + 1], O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);

    if(fd == -1) {
        return -1;
    }

    // Check if it is a built in command that can be redirected or fork and exec if not.
    int stdO;
    if(builtin_redirect(args) == 2) {
        stdO = dup(STDOUT_FILENO);
        if(stdO == -1) {    // If failed return -1 for error checking.
            return -1;
        }
        
        // Replace stdout with filename.
        if(dup2(fd, STDOUT_FILENO) == -1) {
            return -1;       // If failed return -1 for error checking.
        }
        close(fd);
        run_builtins(args);
        // Change back to original stdout.
        fflush(stdout);
        if (dup2(stdO, STDOUT_FILENO) == -1) {
            return -1;
        }
        close(stdO);
    }  else { // If not built in fork() and exec().
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
            waitpid(rc, NULL, 0);
        }
    }
    return 0;
}

/**
 * The _redirect_input function will direct the executed command args up to the first NULL in the arguments.
 * This will cause a fork and run the arguments in a child function and direct input from the filename
 * located at the argument after the first NULL(Position + 1). If a '>' or '>>' is found after the filename
 * it will perform the output redirection required.
 * 
 * @param args The line of arguments from user input.
 * @param position The position of the first '<' found, this position may be updated in function.
 * 
 * @return Will return -1 if failure occurred in parent process or 0 if successful.
**/
int _redirect_input(char **args, int *position) {

    int out_fd; // Output file descriptor if needed to redirect out.
    int redirect_out = 0; // Flag for checking if output is needed.

    // Check if next two args are null. If they are do not check for output redirection.
    if(args[*position+2] != NULL && args[*position+3] != NULL) {
        // Check if truncation or appending is needed for file descriptors if '>' or '>>' is found.
        if(strcmp(args[*position + 2], ">") == 0) {
            out_fd = open(args[*position + 3], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);  
            redirect_out = 1;
        }
        if(strcmp(args[*position + 2], ">>") == 0) {
            out_fd = open(args[*position + 3], O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);  
            redirect_out = 1;     
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
        if(redirect_out == 1) {
            if(dup2(out_fd, STDOUT_FILENO) == -1) {
                write(STDERR_FILENO,error_message,strlen(error_message));
                exit(1);
            }
            close(out_fd);
        }
        
        close(fd);
        if (execvp(args[0], args) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
        exit(0);
    } else {
      
        waitpid(rc, NULL, 0);

        // If output redirection was found in args increase position in while loop to the output
        // filename location.
        if(redirect_out) {
             (*position) += 3;
        }
       
    }
    
    return 0;
}

/**
 * The _execute function will fork and create a child process for the
 * execution of arguments. Only used when redirection or piping is not needed.
 * 
 * @param args The line of arguments from user input.
 * @param position The position of the first null located in args,
 * this position may be updated in function.
 * 
 * @return Will return -1 if failure occurred or 0 if successful.
**/
int _execute(char **args, int *position) {
  
    // Testing for running in background.
    int should_wait = 1;
    // If last arg is '&' set should wait to 0 and '&' to null.
    if (args[*position - 1] != NULL && strcmp(args[*position - 1], "&") == 0) {
        should_wait = 0;
        args[*position - 1] = NULL;
    }
    // Check if it is a built in command. Does not matter if it is redirectable by the time
    // it reaches this function.
    if(builtin_redirect(args) == 1 || builtin_redirect(args) == 2) {
        run_builtins(args);
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
    } else {    // No '&' found so wait.
         if (should_wait == 1){
              waitpid(rc, NULL, 0);
         } else {   // '&' found, do not wait.
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { }
            // If enter is pressed print return message and kill child process.
            printf("Returned from background process %d\n", rc);
            // Terminate child process
            kill(rc, SIGKILL);
            }
         } 
    }
    return 0;
}

/**
 * The run_builtins function will run the built in command if the first argument
 * corresponds to a built in function. If it does it is called and 
 * checked for errors.
 * 
 * @param args The arguments that contain input commands.
 * 
 * @return 0 if no built in functions were called, 1 if built in.
**/
int run_builtins(char **args) {
    char *first_arg = args[0];
    
    if(strcmp("cd", first_arg) == 0) {
        if (change_dir(args) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
        }
    } else if(strcmp("clr", first_arg) == 0) {
        clear_prompt();
        // no return if succesful
    } else if(strcmp("dir", first_arg) == 0) {
        if (print_directory(args) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
        }
    }else if(strcmp("environ", first_arg) == 0) {
        if (print_envp(environ) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
        }
    }else if(strcmp("echo", first_arg) == 0) {
        echo_print(args);
        // No return value if succesfull.
    }else if(strcmp("help", first_arg) == 0) {
        if (print_help() == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
        }
    }else if(strcmp("pause", first_arg) == 0) {
        pause_prompt();
        // no return if succesful
    }else if(strcmp("path", first_arg) == 0) {
        if (add_path(args) == -1) {
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
 * The builtin_redirect function will check if it is a built in function
 * call, a redirectable built in, or not.
 * 
 * @param args The arguments to check.
 * 
 * @return 0 if not built in function. 1 if it is a built in function. 
 * 2 if the built in function is redirectable.
**/
int builtin_redirect(char **args) {
    int i = 0;
    char *first_arg;
   
    first_arg = args[i++];
    // Check for redirectable built in first.

    if((strcmp("dir", first_arg) == 0) || (strcmp("environ", first_arg) == 0)|| (strcmp("echo", first_arg) == 0)|| (strcmp("help", first_arg) == 0)) {
        return 2;
    } else if((strcmp("cd", first_arg) == 0) || (strcmp("clr", first_arg) == 0)|| (strcmp("pause", first_arg) == 0)|| (strcmp("path", first_arg) == 0)) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * The pipe function will pipe the arguments between the pipe character found in args.
 * 
 * @param args The arguments to pipe.
 * 
 * @return 0 if succesfull or -1 if failure occured in parent process.
**/
int _run_pipe(char **args, int *position) {
    // Split args between the | character into two seperate arrays.
    char *args1[MAX_ARGS];
    char *args2[MAX_ARGS];
    int i = 0;

    // Split first part before pipe character.
    while (i <= *position) {
        args1[i] = args[i];
        i++;
    }

    // Split second part after pipe character.
    i = 0;
    *position = *position + 1;
    while(args[*position] != NULL) {
        args2[i] = args[*position];
        i++;
        *position = *position + 1;
    }

    args2[i] = NULL; // Set last arg to null.

    int pipe_descriptor[2];

    if(pipe(pipe_descriptor) == -1) {
        return -1;
    }

    // Fork and exec first set of args to stdout.
    int rc1 = fork();

    if (rc1 < 0) {
        return -1;
    }
    if (rc1 == 0) {
        if(dup2(pipe_descriptor[1], STDOUT_FILENO) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
        close(pipe_descriptor[0]);
        close(pipe_descriptor[1]);
        if(execvp(args1[0], args1) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
        exit(0);
    }

    // Fork and exec second set of args to STDIN
    int rc2 = fork();

    if (rc2 < 0) {
        return -1;
    }
    if (rc2 == 0) {
        if(dup2(pipe_descriptor[0], STDIN_FILENO) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
        close(pipe_descriptor[0]);
        close(pipe_descriptor[1]);
        if(execvp(args2[0], args2) == -1) {
            write(STDERR_FILENO,error_message,strlen(error_message));
            exit(1);
        }
        exit(0);
    }
    
    // Close open file descriptors in parent process.
    close(pipe_descriptor[0]);
    close(pipe_descriptor[1]);

    // Wait for both children to finish.
    waitpid(rc1, NULL, 0);
    waitpid(rc2, NULL, 0);

    return 0;
}