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

const char error_message[30]="An error has occurred\n";

int main(int argc, char const *argv[], char const *envp[])
{

    char *args[MAX_ARGS];    // The arguments extracted from stdin or batch file after parsing.
    char *buff;     // The buffer used for getline().
    size_t size = 0;// Size used for getline().
    int fileSpecifiedFlag = 0;  // Flag used to print prompt if batch file was not used.
    

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

    // Test by printing out delimited parsed string.
    i = 0;
    while(args[i] != NULL) {
        printf("%s\n", args[i++]);
    }
}
