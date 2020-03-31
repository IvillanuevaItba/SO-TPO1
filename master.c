#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/wait.h>

#define SLAVE_MAX_AMOUNT 10

typedef struct {
    int pid;
    int fd[2];
    int task;
    int finished;  
} Slave;

int create_slave(Slave * slave);
void stopSlave(Slave * slave, int * active_slaves);



int main(int argc, char *argv[])
{
    int tasks_total = argc - 1;
    int slave_total = SLAVE_MAX_AMOUNT < tasks_total ? SLAVE_MAX_AMOUNT : tasks_total;
    int active_slaves = 0;
    printf("Slaves initialized: %d\n", slave_total);
    Slave slaves[slave_total];

    fd_set rfds;
    char buf[1024];

    FD_ZERO(&rfds);

    int tasks_sent = 0;
    int tasks_received = 0;

    for (int i = 0; i < slave_total; i++)
    {
        create_slave(slaves+i);
        FD_SET(slaves[i].fd[0], &rfds);
        write(slaves[i].fd[1], &i, 5);
        active_slaves++;
        tasks_sent++;
    }

    
    while (tasks_received < tasks_total)
    {
        FD_ZERO(&rfds);
        int maxfd = 0;
        for (int i = 0; i < slave_total; i++)
        {
            if (!slaves[i].finished)
            {
                FD_SET(slaves[i].fd[0], &rfds);
                if (maxfd < slaves[i].fd[0] + 1)
                {
                    maxfd = slaves[i].fd[0] + 1;
                }
            }
        }

        printf("Wating to receive tasks\n");
        select(maxfd, &rfds, NULL, NULL, NULL);

        for (int i = 0; i < slave_total; i++)
        {
            if (!slaves[i].finished)
            {
                if (FD_ISSET(slaves[i].fd[0], &rfds))
                {
                    read(slaves[i].fd[0], buf, 1024);
                    tasks_received++;
                    printf("%d - S%d - R%d - Slave says: %s\n", slaves[i].pid, tasks_sent, tasks_received, buf);

                    if (tasks_sent < tasks_total )
                    {
                        printf("Sending new task to: %d\n", i);
                        write(slaves[i].fd[1], &i, 5);
                        tasks_sent++;
                    }
                    else
                    {
                        stopSlave(slaves+i, &active_slaves);
                    }
                }
            }
        }
    }

    printf("Stopping\n");

    int idontgiveashit;
    for (size_t i = 0; i < slave_total; i++)
    {
        wait(&idontgiveashit);
    }

    return 0;
}

int create_slave(Slave *slave)
{
    slave->finished = 0;

    int Sl2Ma_Pipe[2];
    int Ma2Sl_Pipe[2];

    // Initialize Pipes
    if (pipe(Sl2Ma_Pipe) == -1)
    {
        perror("pipe");
        exit(1);
    }
    if (pipe(Ma2Sl_Pipe) == -1)
    {
        close(Sl2Ma_Pipe[0]);
        close(Sl2Ma_Pipe[1]);
        perror("pipe");
        exit(1);
    }

    // Fork process
    int pid = fork();
    if (pid == -1)
    {
        close(Sl2Ma_Pipe[0]);
        close(Sl2Ma_Pipe[1]);
        close(Ma2Sl_Pipe[0]);
        close(Ma2Sl_Pipe[1]);
        perror("fork");
        exit(2);
    }

    if (pid > 0) // Master Process
    {
        close(Sl2Ma_Pipe[1]);
        close(Ma2Sl_Pipe[0]);
        slave->fd[0] = Sl2Ma_Pipe[0];
        slave->fd[1] = Ma2Sl_Pipe[1];
        slave->pid=pid;
    }
    else // Slave Process
    {
        close(Sl2Ma_Pipe[0]);
        close(Ma2Sl_Pipe[1]);

        close(STDOUT_FILENO);
        dup(Sl2Ma_Pipe[1]);
        close(STDIN_FILENO);
        dup(Ma2Sl_Pipe[0]);

        execl("./slave.o", "");
    }
    return 0;
}

void stopSlave(Slave * slave, int *active_slaves)
{
    slave ->finished =1;

    close(slave->fd[0]);
    close(slave->fd[1]);

    (*active_slaves)--;
    printf("Stopped slave: %d - Slaves Remaining %d\n", slave->pid, *active_slaves);
}
