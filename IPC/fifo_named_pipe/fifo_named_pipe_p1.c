#include <sys/stat.h>  /* mkfifo() */
#include <fcntl.h>     /* open(), O_RDONLY */
#include <unistd.h>    /* read(), close(), unlink() */
#include <stdio.h>
#include <errno.h>     /* errno, EEXIST */

#define FIFO_PATH "/tmp/my_fifo"
#define BUFFER_SIZE 256

int main() {
    int fd;
    char buffer[BUFFER_SIZE];

    // Create the FIFO if it doesn't exist
    // S_IRUSR | S_IWUSR gives read and write permissions to the owner
    if (mkfifo(FIFO_PATH, S_IRUSR | S_IWUSR) == -1) 
    {
        if (errno != EEXIST) 
        {
            perror("mkfifo");
            return 1;
        }
    }

    printf("Waiting for writer to open FIFO...\n");

    // Open the FIFO for reading
    // This call will block until a writer opens the FIFO for writing
    fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) 
    {
        perror("open");
        return 1;
    }

    printf("[READER]: Open FIFO. Reading messages...\n");

    // Read from the FIFO
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) 
    {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        printf("[Reader]: %s\n", buffer);
    }

    if (bytes_read == -1) 
    {
        perror("read");
        close(fd);
        return 1;
    }

    printf("[Reader]: Writer closed the FIFO. Closing reader...\n");

    // Close the FIFO
    close(fd);

    // Clean up: remove the FIFO from the filesystem
    if (unlink(FIFO_PATH) == -1) 
    {
        perror("unlink");
        return 1;
    }

    printf("[Reader]: FIFO removed. Exiting.\n");
    return 0;
}
