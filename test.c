#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <string.h>


int main(int argc, char const *argv[])
{

    printf("first\n");
    char *args[] = {"grep","READ",  "test.c", NULL};
    int rc = fork();
    char temp[64] = "/bin/";
    strcat(temp, args[0]);
    // DIR *dir = opendir
    // closedir(dir);
    // rc == -1 failed.
    
    if(rc == 0) {
        // chdir("/home");
        execv(temp, args);
    }
    wait(NULL);
    printf("done\n");

    return 0;
}

