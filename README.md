# MiniHTTPd - Servidor Web Linux HTTP/1.1 Asíncrono en C

MiniHTTPd es un servidor web básico para entornos Linux desarrollado desde cero en lenguaje C, sin utilizar bibliotecas HTTP externas. El diseño está basado en una arquitectura guiada por eventos (*event-driven*) utilizando la llamada al sistema `epoll`, lo que le permite gestionar múltiples conexiones simultáneas de forma eficiente con un solo hilo de ejecución.

## Características del Servidor

* **Concurrencia Eficiente:** Utiliza `epoll` con notificaciones *Edge-Triggered* (`EPOLLET`) y sockets no bloqueantes para manejar múltiples clientes concurrentes de manera asíncrona.
* **Conexiones Persistentes:** Soporta el encabezado `Connection: keep-alive` de HTTP/1.1, permitiendo procesar múltiples peticiones sobre un mismo canal antes de cerrar el socket.
* **Servicio de Contenido Estático:** Capaz de servir archivos de texto y binarios (páginas web, estilos y fotos) desde el directorio raíz configurado (`www/`).
* **Mapeo de Tipos MIME:** Soporte integrado para determinar el tipo de contenido basándose en la extensión del archivo (`.html`, `.css`, `.js`, `.png`, `.jpg`).
* **Códigos de Estado HTTP:** Implementación de respuestas estándar para flujos de éxito y manejo de errores (`200 OK`, `400 Bad Request`, `403 Forbidden`, `404 Not Found`, `405 Method Not Allowed`, y `500 Internal Server Error`).

## Mitigación de Vulnerabilidades y Seguridad

El diseño de este servidor prioriza la seguridad del sistema operativo frente a ataques web comunes:

1.  **Directory Traversal (`403 Forbidden`):** Se utiliza la función analítica `realpath()` del kernel para resolver las rutas absolutas de los archivos solicitados. El servidor verifica estrictamente que la ruta resultante comience con el prefijo del directorio raíz (`www/`), bloqueando cualquier intento de acceder a archivos sensibles del sistema (ej. `/../../etc/passwd`).
2.  **Buffer Overflows:** Se prohíbe terminantemente el uso de funciones propensas a desbordamientos como `strcpy` o `sprintf`[cite: 49]. En su lugar, todas las manipulaciones de cadenas y buffers de red utilizan de manera segura `strncpy`, `memcpy` controlados y `snprintf`.
3.  **Control de Métodos (`405 Method Not Allowed`):** El analizador sintáctico restringe y rechaza cualquier método HTTP que no sea estrictamente `GET`.
4.  **Límites de Solicitud (`400 Bad Request`):** Se imponen límites estrictos al tamaño máximo de la línea de petición y los encabezados entrantes para frustrar ataques de denegación de servicio (DoS) por agotamiento de memoria o buffers.

---

## 📂 Estructura del Repositorio

El proyecto se organiza siguiendo la estructura modular obligatoria:

```text
minihttpd/
├── Makefile            # Automatización de la compilación con flags estrictos (-Wall -Wextra -Werror)
├── README.md           # Documentación técnica del proyecto
├── include/            # Cabeceras (.h) de los módulos del sistema
│   ├── files.h         # Validaciones de rutas y lecturas del Sistema de Archivos
│   ├── http.h          # Constantes, estructuras y firmas de parsing/generación HTTP
│   ├── mime.h          # Definición de estructuras y mapeo de tipos MIME
│   └── server.h        # Abstracción de sockets y configuración de red
├── src/                # Código fuente de implementación (.c)
│   ├── files.c         # Lógica de seguridad física y control de Directory Traversal
│   ├── http.c          # Analizador de sintaxis de peticiones y construcción de respuestas
│   ├── main.c          # Bucle de eventos principal (epoll_wait) y reactor asíncrono
│   ├── mime.c          # Tabla estática y búsquedas de extensiones MIME
│   └── server.c        # Configuración de sockets e inicialización no bloqueante
└── www/                # Directorio raíz público de datos estáticos
    ├── image.png       # Recurso de imagen binaria para pruebas
    ├── index.html      # Página de inicio del servidor
    └── style.css       # Hoja de estilos del sitio de pruebas
