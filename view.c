#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MAX_BUF_SIZE 2048
#define SHM_NAME "/tpso_shmview"
#define SEM_NAME "/tpso_semview"

int println(char *s);

int main(int argc, char *argv[])
{

    int tasks_total;
    int task_count = 0;

    if (argc > 2)
    {
        printf("Incorrect amount of arguments. Expected entry: view <info>\n");
        return -1;
    }
    else if (argc == 2)
    {
        tasks_total = atoi(argv[1]);
    }
    else
    {
        char input[10];
        read(STDIN_FILENO, input, 10);
        tasks_total = atoi(input);
    }

    sleep(2);   // Wait for master to create Shared Memory

    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);

    if (shm_fd < 0)
    {
        perror("shm_open()");
        return EXIT_FAILURE;
    }

    char *shm_buf = mmap(NULL, MAX_BUF_SIZE * tasks_total, PROT_READ, MAP_SHARED, shm_fd, 0);

    sem_t *sem_newResult = sem_open(SEM_NAME, O_RDWR);
    if (sem_newResult == SEM_FAILED)
    {
        perror("sem_open()");
        exit(EXIT_FAILURE);
    }

    int offset = 0;

    while (task_count < tasks_total)
    {
        sem_wait(sem_newResult);
        offset += println(shm_buf + offset);
        task_count++;
    }

    munmap(shm_buf, offset);
    close(shm_fd);
    sem_close(sem_newResult);
    return 0;
}

int println(char *s)
{
    int i = 0;
    while (*(s + i) != '\n')
    {
        i++;
    }
    i++;
    write(STDOUT_FILENO, s, i);
    return i;
}