#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

int main()
{
    char buf[1024];
    int readret;

    readret = read(STDIN_FILENO,buf,1024);
    int i = 1;

    while(readret){
    char ret[1024];
    sprintf(ret, "Result = %d", i++);
    sleep(1);
    write(STDOUT_FILENO, ret, strlen(ret));
    readret = read(STDIN_FILENO,buf,1024);
    }
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    return 0;
}