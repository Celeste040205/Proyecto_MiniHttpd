#include "mime.h"
#include <string.h>

typedef struct {
    const char *extension;
    const char *mime_type;
} MimeEntry;

// Estructura estática con los tipos MIME
static MimeEntry mime_table[] = {
    {".html", "text/html"},
    {".css",  "text/css"},
    {".js",   "application/javascript"},
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
    {NULL,    NULL}
};

const char *get_mime_type(const char *filename) {
    if (!filename) return "application/octet-stream";

    const char *dot = strrchr(filename, '.');
    if (!dot) {
        return "application/octet-stream"; //por defecto si no hay extensión
    }

    // Recorre la tabla buscando coincidencias
    for (int i = 0; mime_table[i].extension != NULL; i++) {
        if (strcasecmp(dot, mime_table[i].extension) == 0) {
            return mime_table[i].mime_type;
        }
    }

    return "application/octet-stream";
}