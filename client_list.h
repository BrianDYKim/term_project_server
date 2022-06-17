//
// Created by 김도엽 on 2022/06/17.
//

#ifndef TERM_PROJECT_SERVER_CLIENT_LIST_H
#define TERM_PROJECT_SERVER_CLIENT_LIST_H

// client 개개인의 정보 프레임을 정의한 구조체
typedef struct client_node {
    int client_socket; // socket descriptor of client
    char *name; // name of client
    struct client_node *next_client;
} ClientNode;

// 연결리스트 본체
typedef struct client_list {
    ClientNode *head_client; // 연결리스트의 첫번째 위치에 해당하는 클라이언트
    ClientNode *tail_client; // 연결리스트의 마지막 위치에 해당하는 클라이언트
    ClientNode *current_client; // 연결리스트에서 현재 참조중인 클라이언트
    ClientNode *previous_client; // current_client 이전의 client
    int client_number; // 현재 클라이언트 접속 개수
} ClientList;

/* ============================== [functions] ============================== */
// 클라이언트 리스트를 초기화시키는 함수
void client_list_init(ClientList *list);

// 클라이언트를 생성하는 함수
ClientNode *create_client(int client_socket, char *name);

// 클라이언트 하나를 리스트의 맨 뒤에 붙이는 함수
void append_client(ClientList *list, int client_socket, char *name);

// 첫번째 클라이언트를 참조하는 함수. 빈 리스트인 경우 0을 반환
int refer_first_client(ClientList *list);

// cur 다음의 클라이언트를 참조하는 함수. 빈 리스트인 경우 0을 반환
int refer_next_client(ClientList *list);

// client_socket에 해당하는 클라이언트를 없앤뒤 소켓 디스크립터를 반환. 존재하지 않는 사용자인 경우 -1을 반환
int remove_client_by_socket(ClientList *list, int client_socket);

// 접속한 클라이언트의 숫자를 반환하는 함수.
int get_list_length(ClientList *list);

// name을 이용해서 client를 검색하는 함수. 존재하지 않으면 NULL을 반환.
ClientNode *find_client_by_name(ClientList *list, char *name);

// socket descriptor를 이용해서 client를 검색하는 함수. 존재하지 않으면 NULL을 반환한다.
ClientNode *find_client_by_socket(ClientList *list, int socket);

#endif //TERM_PROJECT_SERVER_CLIENT_LIST_H
