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
    int exit_value = EXIT_SUCCESS;

    do
    {
        readret = read(STDIN_FILENO, buf, 1024);
        if (readret)
        {
            char ret[2048];
            
            int comerr = run_file(buf, ret);
            if (comerr < 0){
                exit_value = EXIT_FAILURE;
                break;
            }


            int wrierr = write(STDOUT_FILENO, ret, strlen(ret)+1);
            if(wrierr < 0)
            {
                perror("slave: write()");
                exit_value = EXIT_FAILURE;
                break;
            }
        }

    } while (readret);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);

    return exit_value;
}

int run_file(char * file, char * result){
    char command[255];
    sprintf(command, "minisat %s | grep -o -e 'Number of.*[0-9]' -e 'CPU time.*' -o -e '.*SATISFIABLE' | xargs | sed 's/ /\t/g'", file);
    FILE * fd = popen(command, "r");

    if(fd == NULL){
        return -1;
    }

    fgets(result, 1024, fd);

    return 0;
}