#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define MAX_REQ_SIZE 8192
#define MAX_PATH_SIZE 1024

typedef struct {
    char method[16]; 
    char uri[MAX_PATH_SIZE];
    char version[16];
    int keep_alive;
} HttpRequest;

// Analiza de manera segura la cadena HTTP entrante del cliente
int parse_http_request(const char *raw_request, size_t request_len, HttpRequest *req);

// Construye la cabecera HTTP de respuesta
size_t build_http_response_header(int status_code, const char *status_text, 
                                  const char *content_type, size_t content_len, 
                                  int keep_alive, char *dest_buffer, size_t dest_max);

#endif