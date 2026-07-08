#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUF_SIZE 512
#define SOCK_PATH "./socket_default.sock"

// Biến toàn cục để kiểm soát vòng lặp chính của server
static volatile sig_atomic_t keep_running = 1;

static void sigint_handler(int sig)
{
    (void)sig;
    keep_running = 0;
}

int main()
{
    int server_fd;
    struct sockaddr_un server_addr;
    char buf[BUF_SIZE];

    /**/
    /* 1. Cấu hình bắt tín hiệu SIGINT (Ctrl+C) để dọn dẹp file socket khi tắt */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    /**/
    /* 2. Tạo socket file descriptor */
    server_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /**/
    /* 3. Khởi tạo cấu trúc địa chỉ socket */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_LOCAL;
    strncpy(server_addr.sun_path, SOCK_PATH, sizeof(server_addr.sun_path) - 1);

    /**/
    /* 4. Xóa socket cũ nếu còn tồn tại trên disk */
    unlink(SOCK_PATH);

    /**/
    /* 5. Gán địa chỉ (bind) vào socket file descriptor */
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /**/
    /* 6. Lắng nghe (listen) kết nối từ client */
    if (listen(server_fd, 5) == -1)
    {
        perror("listen");
        close(server_fd);
        unlink(SOCK_PATH);
        exit(EXIT_FAILURE);
    }

    printf("Server dang lang nghe tren: %s (Nhan Ctrl+C de thoat)\n", SOCK_PATH);

    /**/
    /* 7. Vòng lặp chấp nhận nhiều client kết nối nối tiếp nhau */
    while (keep_running)
    {
        int client_fd;
        struct sockaddr_un client_addr;
        socklen_t client_len = sizeof(client_addr);

        // Sử dụng accept4 kết hợp SOCK_CLOEXEC để tự động đóng FD khi exec
        client_fd = accept4(server_fd, (struct sockaddr *)&client_addr, &client_len, SOCK_CLOEXEC);
        if (client_fd == -1)
        {
            if (errno == EINTR)
            {
                // Bị ngắt bởi tín hiệu Ctrl+C
                break;
            }
            perror("accept4");
            continue;
        }

        /**/
        /* 8. Lấy thông tin định danh (Credentials) của client từ kernel */
        struct ucred cred;
        socklen_t cred_len = sizeof(cred);
        if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len) == -1)
        {
            perror("getsockopt SO_PEERCRED");
            close(client_fd);
            continue;
        }

        printf("\n[CONNECT] Client moi ket noi: PID=%d, UID=%d, GID=%d\n", cred.pid, cred.uid, cred.gid);

        /**/
        /* 9. Lọc client: Chỉ cho phép cùng UID với Server hoặc Root (UID == 0) */
        uid_t server_uid = getuid();
        if (cred.uid != 0 && cred.uid != server_uid)
        {
            fprintf(stderr, "[DENY] Tu choi ket noi tu UID=%d. Chi cho phep cung user (UID=%d) hoac root.\n", cred.uid, server_uid);
            const char *err_msg = "Error: Access Denied. You are not authorized.\n";
            send(client_fd, err_msg, strlen(err_msg), MSG_NOSIGNAL);
            close(client_fd);
            continue;
        }

        printf("[AUTH] Client hop le. Bat dau phuc vu...\n");

        /**/
        /* 10. Doc du lieu tu client va gui phan hoi */
        ssize_t bytes_read;
        while ((bytes_read = read(client_fd, buf, sizeof(buf) - 1)) > 0)
        {
            buf[bytes_read] = '\0';
            printf("Client gui: %s", buf);

            const char *ok_msg = "Message received successfully.\n";
            send(client_fd, ok_msg, strlen(ok_msg), MSG_NOSIGNAL);
        }

        /**/
        /* 11. Dong ket noi voi client hien tai */
        printf("[DISCONNECT] Dong ket noi client.\n");
        close(client_fd);
    }

    /**/
    /* 12. Don dep tai nguyen khi dung server */
    printf("\nServer dang tat. Dang don dep file socket...\n");
    close(server_fd);
    unlink(SOCK_PATH);
    printf("Hoan thanh. Goodbye!\n");

    return 0;
}