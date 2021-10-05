#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

const char error_message[30]="An error has occurred\n";

int main(int argc, char const *argv[])
{
    char *buff;
    size_t size = 0;
    int fileSpecifiedFlag = 0;
    
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
    } else {
        // TODO Change the var name from Home to shell
        printf("%s/myshell>", getenv("HOME"));
    }

       
    


    while(getline(&buff, &size, stdin) != -1) {
        // Replace newline with \0 if exists.
        if(strchr(buff, '\n') != NULL) {
            buff[strlen(buff) - 1] = '\0';
        }
        
        // If quit is typed exit with 0.
        if (strcmp(buff, "quit") == 0) {
            exit(0);
        }
        printf("%s\n", buff);

        // TODO change the path from "Home to shell"
        if (!fileSpecifiedFlag) {
             printf("%s/myshell>", getenv("HOME"));
        }
       
    }
    
  

    
    return 0;
}


