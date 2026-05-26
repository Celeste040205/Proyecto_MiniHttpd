#include "http.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int parse_http_request(const char *raw_request, size_t request_len, HttpRequest *req) {
    if (request_len == 0 || !raw_request || !req) return 400; // Bad Request

    // Inicializa la estructura del request
    memset(req, 0, sizeof(HttpRequest));
    req->keep_alive = 0; // Por defecto close

    // Busca el fin de la primera línea (Request Line)
    const char *line_end = strstr(raw_request, "\r\n");
    if (!line_end) {
        return 400; 
    }

    size_t line_len = line_end - raw_request;
    if (line_len >= MAX_REQ_SIZE) return 400;

    // Copia temporal de la primera línea de forma segura
    char request_line[MAX_REQ_SIZE];
    memcpy(request_line, raw_request, line_len);
    request_line[line_len] = '\0';

    // Extrae de manera segura el método, URI y versión
    int scanned = sscanf(request_line, "%15s %1023s %15s", req->method, req->uri, req->version);
    if (scanned != 3) {
        return 400; 
    }

    // Verifica que el método sea estrictamente GET (Requisito obligatorio)
    if (strcmp(req->method, "GET") != 0) {
        return 405; // Method Not Allowed
    }

    // Busca el encabezado Connection en el resto de la petición
    const char *conn_header = strstr(raw_request, "Connection:");
    if (conn_header) {
        const char *val_start = conn_header + 11;
        // Salta espacios en blanco intermedios
        while (*val_start == ' ' || *val_start == '\t') val_start++;
        
        if (strncasecmp(val_start, "keep-alive", 10) == 0) {
            req->keep_alive = 1;
        }
    }

    return 200; // Análisis correcto
}

size_t build_http_response_header(int status_code, const char *status_text, 
                                  const char *content_type, size_t content_len, 
                                  int keep_alive, char *dest_buffer, size_t dest_max) {
    // Uso obligatorio de snprintf para evitar desbordamientos de búfer de salida
    int written = snprintf(dest_buffer, dest_max,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: %s\r\n\r\n",
        status_code, status_text,
        content_type, content_len,
        keep_alive ? "keep-alive" : "close");

    if (written < 0 || (size_t)written >= dest_max) {
        return 0; // Error de truncamiento de buffer
    }

    return (size_t)written;
}