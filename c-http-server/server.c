#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

#define PORT 3000
#define BUFFER_SIZE 1024
#define WEB_ROOT "./www" // Directory to serve static files from

void *handle_client(void *socket_desc) {
    int client_sock = *(int*)socket_desc;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // Read the client's request
    bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
        perror("recv failed");
        close(client_sock);
        free(socket_desc);
        return NULL;
    }
    buffer[bytes_received] = '\0';

    printf("Received request:\n%s\n", buffer);

    char method[16];
    char path[256];
    sscanf(buffer, "%s %s", method, path);
    printf("Method: %s, Path: %s\n", method, path);

    if (strcmp(method, "GET") == 0) {
        char full_path[512];
        if (strcmp(path, "/") == 0) {
            snprintf(full_path, sizeof(full_path), "%s/index.html", WEB_ROOT);
        } else {
            snprintf(full_path, sizeof(full_path), "%s%s", WEB_ROOT, path);
        }
        FILE *file = fopen(full_path, "rb");
        if (file == NULL) {
            char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 23\r\n\r\n<h1>404 Not Found</h1>";
            send(client_sock, response, strlen(response), 0);
        } else {
            struct stat st;
            stat(full_path, &st);
            off_t file_size = st.st_size;

            char header[BUFFER_SIZE];
            snprintf(header, sizeof(header),
                     "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n",
                     strstr(full_path, ".html") ? "text/html" :
                     strstr(full_path, ".css") ? "text/css" :
                     strstr(full_path, ".js") ? "application/javascript" :
                     strstr(full_path, ".jpg") ? "image/jpeg" :
                     strstr(full_path, ".png") ? "image/png" :
                     "application/octet-stream",
                     file_size);
            send(client_sock, header, strlen(header), 0);
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                send(client_sock, buffer, bytes_read, 0);
            }
            fclose(file);
        }
    } else {
        char *response = "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/html\r\nContent-Length: 26\r\n\r\n<h1>501 Not Implemented</h1>";
        send(client_sock, response, strlen(response), 0);
    }
    
    close(client_sock);
    free(socket_desc);
    return NULL;
}

int main() {
    int socket_desc, client_sock, c;
    struct sockaddr_in server, client;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0); // socket creation
    if (socket_desc == -1) {
        perror("Could not create socket");
        return 1; // return if we can't assign socket
    }
    printf("Socket created\n");

    int yes = 1;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed. Error");
        return 1;
    }
    printf("Bind done\n");

    // Listen for incoming connections
    listen(socket_desc, 3); // 3 => queue size

    printf("Waiting for incoming connections on port %d...\n", PORT);
    c = sizeof(struct sockaddr_in);

    system("mkdir -p www");
    system("echo '<h1>Hello from C Server!</h1><p>This is index.html</p>' > www/index.html");
    system("echo 'body { background-color: #f0f0f0; }' > www/style.css");

    while ((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))) {
        printf("Connection accepted\n");

        pthread_t thread_id;
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;

        if (pthread_create(&thread_id, NULL, handle_client, (void*) new_sock) < 0) {
            perror("could not create thread");
            return 1;
        }
        pthread_detach(thread_id);
    }

    if (client_sock < 0) {
        perror("accept failed");
        return 1;
    }

    close(socket_desc); // closing the socker when done
    return 0;
}