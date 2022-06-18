#include "client_info.h"

#include <stdlib.h>
#include <string.h>

/**
 * function that returns new client with socket descriptor, name sent by parameter
 * @param client_socket socket descriptor of client
 * @param name name of client
 * @return new_client
 */
Client* create_client(int client_socket, char* name) {
    Client* new_client = (Client*)malloc(sizeof(Client));
    new_client->client_socket = client_socket;
    strcpy(new_client->name, name);

    return new_client;
}

/**
 * function that find client socket descriptor where client's name is target_name.
 * To guarantee thread-safety, use mutex when you use this function.
 * @param client_list  list of client
 * @param client_size  size of list
 * @param target_name  name you wanna find
 * @return client socket descriptor if client exists whose name is "target_name", else -1
 */
int find_client_by_name(Client* client_list[], int client_size, char* target_name) {
    for(int i = 0; i < client_size; i++) {
        if(strcmp(client_list[i]->name, target_name) == 0)
            return client_list[i]->client_socket;
    }

    return -1;
}