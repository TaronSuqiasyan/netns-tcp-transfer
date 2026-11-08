/*
 * receiver.c - TCP file receiver (server)
 *
 * Usage: ./receiver <bind_ip> <port> <output_file>
 * Example: ./receiver 10.0.0.2 5000 received.txt
 *
 * Flow: socket() -> bind() -> listen() -> accept() -> recv() -> close()
 * The received bytes are written to <output_file> inside the receiver/ directory.
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

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <bind_ip> <port> <output_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    const char *out_name = argv[3];

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    /* Always write inside receiver/ (create path "receiver/<name>" unless
     * the program is already being run from within receiver/). */
    char path[512];
    snprintf(path, sizeof(path), "receiver/%s", out_name);
    FILE *probe = fopen(path, "wb");
    if (!probe) {
        /* Fallback: running from inside receiver/ itself */
        snprintf(path, sizeof(path), "%s", out_name);
    } else {
        fclose(probe);
    }

    /* 1. socket() */
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. bind() */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        close(listen_fd);
        return EXIT_FAILURE;
    }
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    /* 3. listen() */
    if (listen(listen_fd, 1) < 0) {
        perror("listen");
        close(listen_fd);
        return EXIT_FAILURE;
    }
    printf("[receiver] Listening on %s:%d ...\n", ip, port);

    /* 4. accept() */
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int conn_fd = accept(listen_fd, (struct sockaddr *)&client, &client_len);
    if (conn_fd < 0) {
        perror("accept");
        close(listen_fd);
        return EXIT_FAILURE;
    }
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, client_ip, sizeof(client_ip));
    printf("[receiver] Connection from %s:%d\n", client_ip, ntohs(client.sin_port));

    FILE *out = fopen(path, "wb");
    if (!out) {
        perror("fopen");
        close(conn_fd);
        close(listen_fd);
        return EXIT_FAILURE;
    }

    /* 5. recv() until the sender closes the connection (recv returns 0) */
    char buf[BUF_SIZE];
    size_t total = 0;
    ssize_t n;
    while ((n = recv(conn_fd, buf, sizeof(buf), 0)) > 0) {
        if (fwrite(buf, 1, (size_t)n, out) != (size_t)n) {
            perror("fwrite");
            fclose(out);
            close(conn_fd);
            close(listen_fd);
            return EXIT_FAILURE;
        }
        total += (size_t)n;
    }
    if (n < 0) {
        perror("recv");
    }

    printf("[receiver] Received %zu bytes -> %s\n", total, path);

    /* 6. close() */
    fclose(out);
    close(conn_fd);
    close(listen_fd);
    return n < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
