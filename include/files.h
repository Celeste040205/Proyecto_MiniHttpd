#ifndef FILES_H
#define FILES_H

#include <stddef.h>

int is_safe_path(
    const char *base_dir,       // Directorio raíz permitido
    const char *requested_path, // Ruta solicitada por el cliente
    char *resolved_path);       // Buffer para almacenar la ruta resuelta (debe ser lo suficientemente grande)

unsigned char *read_file_content(
    const char *filepath, // Ruta del archivo a leer
    size_t *file_size);   // Salida: tamaño del archivo leído

#endif

