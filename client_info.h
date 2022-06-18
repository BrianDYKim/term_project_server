//
// Created by 김도엽 on 2022/06/18.
//

#ifndef TERM_PROJECT_SERVER_CLIENT_INFO_H
#define TERM_PROJECT_SERVER_CLIENT_INFO_H

// client를 관리할 구조체 선언
typedef struct client_info {
    int client_socket; // client에 해당하는 socket descriptor
    char* name; // client의 이름
}Client;

// 새로운 client를 생성하는 함수
Client* create_client(int client_socket, char* name);

// 해당 이름을 가진 client가 존재하는지 검사하는 함수
// 존재하는 경우 해당 이름을 가진 클라이언트 소켓을 반환하고, 존재하지 않는 경우 -1을 반환한다.
int find_client_by_name(Client* client_list[], int client_size, char* target_name);

#endif //TERM_PROJECT_SERVER_CLIENT_INFO_H
