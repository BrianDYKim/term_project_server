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
    // new_client를 동적할당한다.
    Client* new_client = (Client*)malloc(sizeof(Client));
    new_client->client_socket = client_socket;
    strcpy(new_client->name, name);

    return new_client; // 내용을 다 채운 new_client를 반환해준다
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
    // Linear Search 방법을 통해서 target_name에 해당하는 client를 탐색한다.
    for(int i = 0; i < client_size; i++) {
        if(strcmp(client_list[i]->name, target_name) == 0)
            return client_list[i]->client_socket;
    }

    // 해당 client를 찾기 못하는 경우에는 -1을 반환해준다
    return -1;
}