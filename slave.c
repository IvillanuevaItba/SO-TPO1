#define _POSIX_C_SOURCE 2

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

int run_file(char * file, char * result);

int main()
{
    char buf[1024];
    int readret;

    do
    {
        readret = read(STDIN_FILENO, buf, 1024);
        if (readret)
        {
            char ret[2048];
            run_file(buf, ret);
            write(STDOUT_FILENO, ret, strlen(ret)+1);
        }
    } while (readret);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);

    return 0;
}

int run_file(char * file, char * result){
    char command[255];
    sprintf(command, "minisat %s | grep -o -e 'Number of.*[0-9]' -e 'CPU time.*' -o -e '.*SATISFIABLE' | xargs | sed 's/ /\t/g'", file);
    FILE * fd = popen(command, "r");

    if(fd == NULL){
        return -1;
        result = "ERROR!";
    }
    
    //strcpy(result, file);

    fgets(result, 1024, fd);

    return 0;
}