#ifndef SERVER_H
#define SERVER_H

// Crea un socket de escucha en el puerto indicado y lo configura como No Bloqueante
int create_server_socket(int port);

// Cambia cualquier descriptor de socket al modo de E/S No Bloqueante (Non-blocking)
int make_socket_non_blocking(int fd);

#endif 