#include "files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>

int is_safe_path(const char *base_dir, const char *requested_path, char *resolved_path) {
    char actual_base[PATH_MAX];
    char temp_path[PATH_MAX * 2];
    
    // Obtiene la ruta absoluta real del directorio raíz (www/) 
    if (!realpath(base_dir, actual_base)) {
        return 0;
    }

    // Copiamos la ruta solicitada para no modificar el string original
    strncpy(temp_path, requested_path, sizeof(temp_path) - 1);
    temp_path[sizeof(temp_path) - 1] = '\0';

    // Separamos en directorio y nombre base para poder validar rutas de archivos que no existen
    char *dir_name = dirname(temp_path);
    char resolved_dir[PATH_MAX];

    // Intentamos resolver la ruta absoluta del directorio que contiene al archivo
    if (!realpath(dir_name, resolved_dir)) {
        return 0; 
    }

    // Re-ensamblamos la ruta absoluta teórica que tendría el archivo
    // Aunque el archivo no exista, su directorio contenedor sí, lo que nos permite validar la ruta de forma segura 
    char *base_name = basename((char*)requested_path);
    snprintf(resolved_path, PATH_MAX, "%s/%s", resolved_dir, base_name);

    // Verifica estrictamente si la ruta del archivo empieza con la ruta raíz www/ 
    if (strncmp(resolved_path, actual_base, strlen(actual_base)) == 0) {
        return 1; // Ruta segura dentro de los límites permitidos 
    }

    return 0; // Intento de Directory Traversal detectado 
}

unsigned char *read_file_content(const char *filepath, size_t *file_size) {
    FILE *file = fopen(filepath, "rb"); // Modo binario obligatorio para imágenes 
    if (!file) {
        return NULL;
    }

    // Calcula el tamaño exacto del archivo en bytes
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size < 0) {
        fclose(file);
        return NULL;
    }

    // Reserva memoria dinámica para alojar el contenido [cite: 49]
    unsigned char *buffer = malloc(size);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, size, file);
    fclose(file);

    if (read_bytes != (size_t)size) {
        free(buffer);
        return NULL;
    }

    *file_size = (size_t)size;
    return buffer;
}