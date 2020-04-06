#define _POSIX_C_SOURCE 200112L

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

#define SLAVE_MAX_AMOUNT 10
#define FD_READ 0
#define FD_WRITE 1

#define SHM_NAME "/tpso_shmview"
#define SEM_NAME "/tpso_semview"

typedef struct
{
    int pid;
    int fd[2];
    char *task;
    int finished;
} Slave;

int create_slave(Slave *slave);
void send_task(Slave *slave, char *task);
void kill_slave(Slave *slave, int *active_slaves);
void safe_exit(int error_code);

int main(int argc, char *argv[])
{
    int tasks_total = argc - 1;
    int tasks_sent = 0;
    int tasks_received = 0;

    char task_out[10];
    sprintf(task_out, "%d\n", tasks_total);
    write(STDOUT_FILENO, task_out, 10);

    int slaves_total = SLAVE_MAX_AMOUNT < tasks_total ? SLAVE_MAX_AMOUNT : tasks_total;
    int slaves_active = 0;
    Slave slaves[slaves_total];

    fd_set rfds;

    int shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (shm_fd < 0)
    {
        perror("shm_open()");
        exit(EXIT_FAILURE);
    }

    ftruncate(shm_fd, MAX_BUF_SIZE * tasks_total);
    
    char *shm_buf = mmap(NULL, MAX_BUF_SIZE * tasks_total, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_buf < 0)
    {
        perror("mmap()");
        safe_exit(EXIT_FAILURE);
    }

    sem_t *sem_newTask = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 0);
    if (sem_newTask == SEM_FAILED)
    {
        perror("sem_open()");
        safe_exit(EXIT_FAILURE);
    }

    // Initialiaze Slaves
    for (int i = 0; i < slaves_total; i++)
    {
        create_slave(slaves + i);
        FD_SET(slaves[i].fd[FD_READ], &rfds);
        send_task(slaves + i, argv[tasks_sent + 1]);
        slaves_active++;
        tasks_sent++;
    }


    // Main Loop.
    while (tasks_received < tasks_total)
    {

        // Initiliaze FD SET for select.
        FD_ZERO(&rfds);
        int maxfd = 0;
        for (int i = 0; i < slaves_total; i++)
        {
            if (!slaves[i].finished)
            {
                FD_SET(slaves[i].fd[FD_READ], &rfds);
                if (maxfd < slaves[i].fd[FD_READ] + 1)
                {
                    maxfd = slaves[i].fd[FD_READ] + 1;
                }
            }
        }

        
        int selerr = select(maxfd, &rfds, NULL, NULL, NULL);    // Wait to receive new task.
        if (selerr <= 0)
        {
            perror("select()");
            safe_exit(EXIT_FAILURE);
        }

        for (int i = 0; i < slaves_total; i++)
        {
            if (!slaves[i].finished)
            {
                if (FD_ISSET(slaves[i].fd[FD_READ], &rfds))
                {
                    int bytes_read = read(slaves[i].fd[FD_READ], shm_buf, MAX_BUF_SIZE);

                    if (bytes_read > 0)
                    {
                        tasks_received++;
                        sem_post(sem_newTask);
                        shm_buf += bytes_read;
                        
                        if (tasks_sent < tasks_total)
                        {
                            send_task(slaves + i, argv[tasks_sent + 1]);
                            tasks_sent++;
                        }
                        else
                        {
                            kill_slave(slaves + i, &slaves_active);
                        }
                    }
                    else
                    {
                        // Manejar error del slave
                    }
                    
                }
            }
        }
    }

    // Clean up memory
    munmap(shm_buf, MAX_BUF_SIZE * tasks_total);
    shm_unlink(SHM_NAME);
    close(shm_fd);
    sem_unlink(SEM_NAME);
    sem_close(sem_newTask);

    int retval;
    for (size_t i = 0; i < slaves_total; i++)
    {
        wait(&retval);
    }

    return 0;
}

/* Initialize a slave */
int create_slave(Slave *slave)
{
    slave->finished = 0;

    int Sl2Ma_Pipe[2];
    int Ma2Sl_Pipe[2];

    // Initialize Pipes
    if (pipe(Sl2Ma_Pipe) == -1)
    {
        perror("pipe");
        safe_exit(1);
    }
    if (pipe(Ma2Sl_Pipe) == -1)
    {
        close(Sl2Ma_Pipe[FD_READ]);
        close(Sl2Ma_Pipe[FD_WRITE]);
        perror("pipe");
        safe_exit(1);
    }

    // Fork process
    int pid = fork();
    if (pid == -1)
    {
        close(Sl2Ma_Pipe[FD_READ]);
        close(Sl2Ma_Pipe[FD_WRITE]);
        close(Ma2Sl_Pipe[FD_READ]);
        close(Ma2Sl_Pipe[FD_WRITE]);
        perror("fork");
        safe_exit(2);
    }

    if (pid > 0) // Master Process
    {
        close(Sl2Ma_Pipe[FD_WRITE]);
        close(Ma2Sl_Pipe[FD_READ]);
        slave->fd[FD_READ] = Sl2Ma_Pipe[FD_READ];
        slave->fd[FD_WRITE] = Ma2Sl_Pipe[FD_WRITE];
        slave->pid = pid;
    }
    else // Slave Process
    {
        close(Sl2Ma_Pipe[FD_READ]);
        close(Ma2Sl_Pipe[FD_WRITE]);

        close(STDOUT_FILENO);
        dup(Sl2Ma_Pipe[FD_WRITE]);
        close(STDIN_FILENO);
        dup(Ma2Sl_Pipe[FD_READ]);

        int err = execl("./slave.o", "Slave", NULL);
        if (err == -1)
        {
            perror("exec()");
            safe_exit(EXIT_FAILURE);
        }
    }
    return 0;
}

/* Kill slave. Cleans up all memory in use. */
void kill_slave(Slave *slave, int *active_slaves)
{
    slave->finished = 1;

    close(slave->fd[FD_READ]);
    close(slave->fd[FD_WRITE]);

    (*active_slaves)--;
    //printf("Stopped slave: %d - Slaves Remaining %d\n", slave->pid, *active_slaves);
}

/* Sends a task path (Null Terminated) to the slave */
void send_task(Slave *slave, char *task)
{
    int len = strlen(task);
    char send[len + 1];
    strcpy(send, task);
    send[len] = 0;

    int wrierr = write(slave->fd[FD_WRITE], send, len + 1);
    if (wrierr < 0)
    {
        perror("write()");
        safe_exit(EXIT_FAILURE);
    }

    slave->task = task;
}

/* Same as exit, but does a minimum of clean up. */
void safe_exit(int error_code)
{
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);

    exit(error_code);
}