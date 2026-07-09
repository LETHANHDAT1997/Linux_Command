#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define FIFO_PATH "/tmp/my_fifo"

int main(void) 
{
    /* open() block cho đến khi server mở O_RDONLY */
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) 
    {
        perror("open");
        return 1;
    }
    
    int loop_cnt = 10;
    char *message;
    while(loop_cnt-- > 0)
    {
        message = "LeThanhDat";
        int bytes_write = strlen(message);
        ssize_t bytes_written = write(fd, message, bytes_write);
        
        if (bytes_written != bytes_write) 
        {
            perror("write");
            close(fd);
            return 1;
        }

        printf("[WRITER]: %s\n", message);
        sleep(1);
    }

    close(fd);  /* Đóng → server thấy EOF */
    return 0;
}