#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUF_SIZE 512
#define SOCK_PATH "./socket_default.sock"

int main()
{
    int client_fd;
    struct sockaddr_un server_addr;
    char buf[BUF_SIZE];

    /**/
    /* 1. Tạo socket file descriptor */
    client_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (client_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /**/
    /* 2. Khởi tạo cấu trúc địa chỉ của server socket */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_LOCAL;
    strncpy(server_addr.sun_path, SOCK_PATH, sizeof(server_addr.sun_path) - 1);

    /**/
    /* 3. Thực hiện kết nối tới server */
    printf("Dang ket noi den server tai: %s...\n", SOCK_PATH);
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("Ket noi thanh cong!\n");

    /**/
    /* 4. Chẩn bị và gửi tin nhắn đến server */
    snprintf(buf, sizeof(buf), "Hello from Client (PID=%d)!\n", getpid());
    printf("Gui den server: %s", buf);
    if (send(client_fd, buf, strlen(buf), 0) == -1)
    {
        perror("send");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    /**/
    /* 5. Cho phan hoi tu server */
    ssize_t bytes_received = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (bytes_received > 0)
    {
        buf[bytes_received] = '\0';
        printf("Phan hoi tu server: %s", buf);
    }
    else if (bytes_received == 0)
    {
        printf("Server da dong ket noi.\n");
    }
    else
    {
        perror("recv");
    }

    /**/
    /* 6. Dong socket va ket thuc */
    printf("Dong ket noi client.\n");
    close(client_fd);

    return 0;
}
