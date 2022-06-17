//
// Created by 김도엽 on 2022/06/17.
//

#include "client_list.h"

#include <stdlib.h>
#include <string.h>

// 클라이언트 리스트를 초기화시키는 함수 -> 더미노드 기반의 연결리스트
void client_list_init(ClientList *list) {
    ClientNode* dummy_node = (ClientNode*)malloc(sizeof(ClientNode));

    list->client_number = 0;
    list->head_client = dummy_node; // head client를 dummy로 채운다
    list->tail_client = dummy_node;
    list->current_client = NULL;
    list -> previous_client = NULL;
}

// 클라이언트를 생성하는 함수
ClientNode *create_client(int client_socket, char *name) {
    ClientNode *new_client = (ClientNode *) malloc(sizeof(ClientNode));
    new_client->client_socket = client_socket;
    new_client->name = name;
    new_client->next_client = NULL;

    return new_client;
}

// 클라이언트 하나를 리스트의 맨 뒤에 붙이는 함수
void append_client(ClientList *list, int client_socket, char *name) {
    ClientNode *new_client = create_client(client_socket, name);

    // list에 아무 클라이언트도 존재하지 않는 경우
    if (get_list_length(list) == 0) {
        list->head_client->next_client = new_client; // dummy node의 다음으로 채운다
        list->tail_client = new_client;
    } else {
        list->tail_client->next_client = new_client; // tail_client 뒤에다가 붙이기
        list->tail_client = list->tail_client->next_client; // tail_client를 새로운 클라이언트로 등록
    }

    (list->client_number)++;
}

// list의 cur을 첫번째 클라이언트로 수정하는 함수
int refer_first_client(ClientList *list) {
    // 리스트에 아무도 존재하지 않는 경우
    if (get_list_length(list) == 0)
        return 0;

    list->previous_client = list->head_client; // previous client는 더미노드를 가르키게 만든다
    list->current_client = list->head_client->next_client; // 더미 노드의 다음 노드가 첫번째 클라이언트이기 때문이다.
    return 1;
}

int refer_next_client(ClientList *list) {
    if (get_list_length(list) == 0)
        return 0;

    list -> previous_client = list->current_client; // previous client는 현재 클라이언트를 가르키게 만든다
    list->current_client = list->current_client->next_client; // current client는 다음 클라이언트를 가르키게 만든다
    return 1;
}

// TODO client_socket에 해당하는 클라이언트를 없앤뒤 소켓 디스크립터를 반환. 존재하지 않는 사용자인 경우 -1을 반환
int remove_client_by_socket(ClientList *list, int client_socket) {
    ClientNode* tmp_client;
    int tmp_socket;

    if (get_list_length(list) == 0)
        return -1;

    if((tmp_client = find_client_by_socket(list, client_socket)) == NULL)
        return -1;

    tmp_socket = tmp_client->client_socket;

    list->previous_client->next_client = list->current_client->next_client;
    (list->client_number)--; // client가 하나 삭제됐으므로 카운트를 하나 깎는다

    // 만약에 tmp_client가 tail_client인 경우 tail_client를 previous_client로 수정해줘야함
    if(list->current_client->next_client == NULL)
        list->tail_client = list->previous_client;

    free(tmp_client);
    refer_first_client(list);

    return tmp_socket;
}

// 현재 접속된 클라이언트의 수를 반환하는 함수
int get_list_length(ClientList *list) {
    return list->client_number;
}

// TODO name을 이용해서 client를 검색하는 함수. 존재하지 않으면 NULL을 반환.
ClientNode *find_client_by_name(ClientList *list, char *name) {
    // 클라이언트가 존재하지 않는 경우 NULL을 반환
    if (get_list_length(list) == 0)
        return NULL;

    refer_first_client(list); // 첫번째 클라이언트를 참고

    while(list -> current_client != NULL) {
        // 이름이 일치하는 경우에는 바로 clientNode를 반환
        if (strcmp(list -> current_client->name, name) == 0)
            return list -> current_client;

        // 찾지못하면 다음으로 넘어간다
        refer_next_client(list);
    }

    return NULL;
}

// socket descriptor를 이용해서 client를 검색하는 함수. 존재하지 않으면 NULL을 반환한다.
ClientNode *find_client_by_socket(ClientList *list, int socket) {
    // 리스트가 비어있는 경우
    if(get_list_length(list) == 0)
        return NULL;

    refer_first_client(list);

    while(list ->current_client != NULL) {
        // 소켓 디스크립터가 일치하는 경우에는 바로 clientNode를 반환
        if (list -> current_client->client_socket == socket)
            return list->current_client;

        // 찾지못하는 경우 다음으로 넘어간다
        refer_next_client(list);
    }

    return NULL;
}
