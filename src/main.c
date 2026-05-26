#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "server.h"
#include "http.h"
#include "files.h"
#include "mime.h"

#define PORT 8080
#define MAX_EVENTS 64
#define BASE_DIR "./www"

// Procesa el envío controlado de los códigos de error HTTP requeridos
void send_error_response(int client_fd, int status_code, const char *status_text, int keep_alive) {
    char header[512];
    const char *html_err_template = "<html><body><h1>Error</h1></body></html>";
    size_t header_len = build_http_response_header(status_code, status_text, "text/html", 
                                                   strlen(html_err_template), keep_alive, 
                                                   header, sizeof(header));
    if (header_len > 0) {
        send(client_fd, header, header_len, 0);
        send(client_fd, html_err_template, strlen(html_err_template), 0);
    }
}

// Lógica para atender y despachar solicitudes sobre un cliente asíncrono
void handle_client_event(int client_fd, int epoll_fd) {
    char buffer[MAX_REQ_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return; // No hay más datos por leer en el buffer interno del kernel
        }
        // Desconexión del cliente o error grave de red
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';

    HttpRequest req;
    int parse_status = parse_http_request(buffer, (size_t)bytes_read, &req);

    // Mapeo exhaustivo de códigos de error según especificaciones
    if (parse_status == 400) {
        send_error_response(client_fd, 400, "Bad Request", 0);
        goto close_connection;
    } else if (parse_status == 405) {
        send_error_response(client_fd, 405, "Method Not Allowed", 0);
        goto close_connection;
    }

    // Traduce la ruta virtual URI a la ruta física local en la carpeta www/
    char target_path[MAX_PATH_SIZE * 2];
    if (strcmp(req.uri, "/") == 0) {
        snprintf(target_path, sizeof(target_path), "%s/index.html", BASE_DIR);
    } else {
        snprintf(target_path, sizeof(target_path), "%s%s", BASE_DIR, req.uri);
    }

    char safe_resolved_path[PATH_MAX];
    if (!is_safe_path(BASE_DIR, target_path, safe_resolved_path)) {
        // Acceso no autorizado o Directory Traversal evitado con éxito
        send_error_response(client_fd, 403, "Forbidden", req.keep_alive);
        if (!req.keep_alive) goto close_connection;
        return;
    }

    // Intenta leer el archivo solicitado
    size_t file_size = 0;
    unsigned char *file_content = read_file_content(safe_resolved_path, &file_size);
    if (!file_content) {
        send_error_response(client_fd, 404, "Not Found", req.keep_alive);
        if (!req.keep_alive) goto close_connection;
        return;
    }

    // Resuelve el tipo MIME dinámicamente según extensión
    const char *mime_type = get_mime_type(safe_resolved_path);

    // Construye y despacha las cabeceras de éxito
    char response_header[MAX_REQ_SIZE];
    size_t header_len = build_http_response_header(200, "OK", mime_type, file_size, 
                                                   req.keep_alive, response_header, 
                                                   sizeof(response_header));

    if (header_len > 0) {
        send(client_fd, response_header, header_len, 0);
        send(client_fd, file_content, file_size, 0);
    } else {
        send_error_response(client_fd, 500, "Internal Server Error", 0);
        free(file_content);
        goto close_connection;
    }

    free(file_content);

    // Si el cliente no soporta persistencia, cerramos inmediatamente
    if (!req.keep_alive) {
    close_connection:
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        close(client_fd);
    }
}

int main() {
    int server_fd = create_server_socket(PORT);
    if (server_fd == -1) {
        return EXIT_FAILURE;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Error al instanciar epoll");
        close(server_fd);
        return EXIT_FAILURE;
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN; // Monitorea eventos de lectura entrantes
    ev.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("Error en epoll_ctl para el socket del servidor");
        close(server_fd);
        close(epoll_fd);
        return EXIT_FAILURE;
    }

    printf("Servidor MiniHTTPd corriendo asíncronamente en el puerto %d...\n", PORT);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue; // Maneja interrupciones de señales del sistema
            perror("Error en epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {
                // Hay nuevas conexiones pendientes en la cola del socket del servidor
                while (1) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                    
                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break; // Se procesaron todos los clientes pendientes de la cola
                        }
                        perror("Error en accept");
                        break;
                    }

                    if (make_socket_non_blocking(client_fd) < 0) {
                        close(client_fd);
                        continue;
                    }

                    // Registramos el nuevo descriptor de cliente en epoll mediante Edge-Triggered
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                        perror("Error al agregar el cliente a epoll");
                        close(client_fd);
                    }
                }
            } else {
                // Un cliente ya conectado envió datos a la red
                handle_client_event(events[i].data.fd, epoll_fd);
            }
        }
    }

    close(server_fd);
    close(epoll_fd);
    return EXIT_SUCCESS;
}