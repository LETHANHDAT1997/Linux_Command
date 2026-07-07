#include <unistd.h>    /* pipe(), read(), write(), fork(), close() */
#include <stdio.h>     /* printf(), perror() */
#include <stdlib.h>    /* exit() */
#include <string.h>    /* strlen() */
#include <sys/wait.h>  /* wait() */

int main()
{
    int pipefd[2];
    pid_t pid;
    char buffer[1024];
    int n;

    /* Create the pipe */
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    /* Nhân đôi tiến trình */
    pid = fork();

    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        /* Child process */
        close(pipefd[1]);
        while (1)
        {
            /* Read from the pipe */
            n = read(pipefd[0], buffer, sizeof(buffer) - 1);
            if (n == -1)
            {
                perror("read");
                exit(EXIT_FAILURE);
            }
            if (n == 0)
            {
                /* Pipe closed by parent */
                printf("[CHILD]: Connection closed by parent.\n");
                break;
            }
            buffer[n] = '\0';
            printf("[CHILD]: %s\n", buffer);
        }
        close(pipefd[0]);
    }
    else
    {
        /* Parent process */
        close(pipefd[0]);
        int count = 1;
        while (1)
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Hello from parent! Message #%d", count++);
            printf("[PARENT]: %s\n", msg);
            if (write(pipefd[1], msg, strlen(msg)) == -1)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }
            sleep(3);
        }
        close(pipefd[1]);
        waitpid(pid, NULL, 0);
    }

    return 0;
}