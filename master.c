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
#define FD_READ 0
#define FD_WRITE 1

typedef struct {
    int pid;
    int fd[2];
    char * task;
    int finished;  
} Slave;

int create_slave(Slave * slave);
void send_task(Slave * slave,char * task);
void kill_slave(Slave * slave, int * active_slaves);



int main(int argc, char *argv[])
{
    int tasks_total = argc - 1;
    int tasks_sent = 0;
    int tasks_received = 0;
    
    int slaves_total = SLAVE_MAX_AMOUNT < tasks_total ? SLAVE_MAX_AMOUNT : tasks_total;
    int slaves_active = 0;
    Slave slaves[slaves_total];
    printf("Slaves initialized: %d\n", slaves_total);
    
    fd_set rfds;
    char buf[1024];

    for (int i = 0; i < slaves_total; i++)
    {
        create_slave(slaves+i);
        FD_SET(slaves[i].fd[FD_READ], &rfds);
        send_task(slaves + i, argv[tasks_sent+1]);
        slaves_active++;
        tasks_sent++;
    }

    
    while (tasks_received < tasks_total)
    {
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

        printf("Wating to receive tasks\n");
        select(maxfd, &rfds, NULL, NULL, NULL);         // TODO: Select puede tirar error. Timeout?

        for (int i = 0; i < slaves_total; i++)
        {
            if (!slaves[i].finished)
            {
                if (FD_ISSET(slaves[i].fd[FD_READ], &rfds))
                {
                    read(slaves[i].fd[FD_READ], buf, 1024);
                    tasks_received++;
                    printf("Task: %s\n", slaves[i].task);
                    printf("%d - S%d - R%d - Slave says: %s\n", slaves[i].pid, tasks_sent, tasks_received, buf);
                    

                    if ( tasks_sent < tasks_total )
                    {
                        printf("Sending new task to: %d\n", i);
                        send_task(slaves + i, argv[tasks_sent+1]);      // TODO: Write tiene que mandar un char * NULL TERMINATED
                        tasks_sent++;
                    }
                    else
                    {
                        kill_slave(slaves+i, &slaves_active);
                    }
                }
            }
        }
    }

    printf("Stopping\n");

    int idontgiveashit;
    for (size_t i = 0; i < slaves_total; i++)
    {
        wait(&idontgiveashit);
    }

    return 0;
}

int create_slave(Slave * slave)
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
        close(Sl2Ma_Pipe[FD_READ]);
        close(Sl2Ma_Pipe[FD_WRITE]);
        perror("pipe");
        exit(1);
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
        exit(2);
    }

    if (pid > 0) // Master Process
    {
        close(Sl2Ma_Pipe[FD_WRITE]);
        close(Ma2Sl_Pipe[FD_READ]);
        slave->fd[FD_READ] = Sl2Ma_Pipe[FD_READ];
        slave->fd[FD_WRITE] = Ma2Sl_Pipe[FD_WRITE];
        slave->pid=pid;
    }
    else // Slave Process
    {
        close(Sl2Ma_Pipe[FD_READ]);
        close(Ma2Sl_Pipe[FD_WRITE]);

        close(STDOUT_FILENO);
        dup(Sl2Ma_Pipe[FD_WRITE]);
        close(STDIN_FILENO);
        dup(Ma2Sl_Pipe[FD_READ]);

        execl("./slave.o", "");
    }
    return 0;
}

void kill_slave(Slave * slave, int *active_slaves)
{
    slave->finished = 1;

    close(slave->fd[FD_READ]);
    close(slave->fd[FD_WRITE]);

    (*active_slaves)--;
    printf("Stopped slave: %d - Slaves Remaining %d\n", slave->pid, *active_slaves);
}

void send_task(Slave * slave, char * task){
    int len = strlen(task);
    char send[len+1];
    strcpy(send, task);
    send[len] = 0; 
    write(slave -> fd[FD_WRITE], send, len+1);
    slave->task = task;
}

void read_task(Slave * slave, char * result){

}