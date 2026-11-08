/*
 * sender.c - TCP file sender (client)
 *
 * Usage: ./sender <server_ip> <port> <file_in_sender_dir>
 * Example: ./sender 10.0.0.2 5000 sample.txt
 *
 * Flow: socket() -> connect() -> read file + send() -> close()
 * No bind(): the kernel picks an ephemeral source port automatically.
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 4096

/* send() may write fewer bytes than requested; loop until all are sent. */
static int send_all(int fd, const char *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, data + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    const char *file_name = argv[3];

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    /* Open the file from sender/ (fallback: current directory). */
    char path[512];
    snprintf(path, sizeof(path), "sender/%s", file_name);
    FILE *in = fopen(path, "rb");
    if (!in) {
        snprintf(path, sizeof(path), "%s", file_name);
        in = fopen(path, "rb");
    }
    if (!in) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    /* 1. socket() */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        fclose(in);
        return EXIT_FAILURE;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, ip, &server.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        close(fd);
        fclose(in);
        return EXIT_FAILURE;
    }

    /* 2. connect() */
    printf("[sender] Connecting to %s:%d ...\n", ip, port);
    if (connect(fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect");
        close(fd);
        fclose(in);
        return EXIT_FAILURE;
    }

    /* 3. read file and send() */
    char buf[BUF_SIZE];
    size_t total = 0, n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (send_all(fd, buf, n) < 0) {
            perror("send");
            close(fd);
            fclose(in);
            return EXIT_FAILURE;
        }
        total += n;
    }

    printf("[sender] Sent %zu bytes from %s\n", total, path);

    /* 4. close() - signals end of file to the receiver */
    fclose(in);
    close(fd);
    return EXIT_SUCCESS;
}
