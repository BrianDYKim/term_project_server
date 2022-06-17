#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "client_list.h"

#define BUF_SIZE 100
#define MAX_CLNT 255 // 최대 접속 클라이언트 수는 255로 제한

void error_handling(char *msg); // 에러 발생시 에러를 핸들링하는 함수
void *handle_client(void *arg); // 자식 thread에 전달할 함수 포인터
void send_message(char *message, int len); // 메시지를 send하는 함수

// critical sections
int client_cnt = 0; // 접속한 사용자의 수
int client_socks[MAX_CLNT]; // 접속한 사용자의 소켓 정보
ClientList client_list; // 클라이언트의 목록을 저장하는 자료구조
pthread_mutex_t mutex;

// TODO 적절한 자료구조를 이용해서 client를 관리하여 귓속말 기능을 구현한다. -> Hash Table이 적절할 것으로 보인다.
int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_adr, client_adr;
    int client_adr_size;
    int name_len; //
    char name_info[BUF_SIZE];
    pthread_t t_id;

    // 잘못된 main에 대한 input
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutex, NULL); // mutex 변수의 초기화
    client_list_init(&client_list); // client_list의 초기화
    server_socket = socket(PF_INET, SOCK_STREAM, 0); // server의 socket descriptor 생성

    memset(&server_adr, 0, sizeof(server_adr));
    server_adr.sin_family = AF_INET;
    server_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_adr.sin_port = htons(atoi(argv[1]));

    // bind 함수 실행
    if (bind(server_socket, (struct sockaddr *) &server_adr, sizeof(server_adr)) == -1)
        error_handling("bind() error!");

    // listen 함수 실행
    if (listen(server_socket, 10) == -1)
        error_handling("listen() error!");

    while (1) {
        client_adr_size = sizeof(client_adr);

        // accept 함수를 통해 client의 접속을 스니핑
        client_socket = accept(server_socket, (struct sockaddr *) &client_adr, &client_adr_size);

        // 연결이 이뤄지자마자 이름 정보를 가져온다
        name_len = read(client_socket, name_info, BUF_SIZE);
        name_info[name_len] = 0;

        // server가 관리하는 client 접속 정보 세팅 (mutex를 이용해서 race condition issue 해결)
        pthread_mutex_lock(&mutex);
        append_client(&client_list, client_socket, name_info);
        pthread_mutex_unlock(&mutex);

        // thread 생성 및 thread_detach 등록 -> thread detach는 non-blocking function
        pthread_create(&t_id, NULL, handle_client, (void *) &client_socket);
        pthread_detach(t_id);

        // client가 연결이 되었을때 서버에 출력하는 정보
        printf("Connected client IP: %s, name: %s\n", inet_ntoa(client_adr.sin_addr), name_info);
    }

    close(server_socket);
    return 0;
}

void error_handling(char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void *handle_client(void *arg) {
    int client_socket = *((int *) arg); // void pointer를 int pointer로 타입 캐스팅 한 후 역참조
    int str_len = 0;
    int i;
    char message[BUF_SIZE]; // 메시지를 저장하는 string 변수

    while ((str_len = read(client_socket, message, sizeof(message))) != 0) {
        send_message(message, str_len);
    }

    // disconnect the client -> lock을 걸어둔 상태에서 disconnect를 처리한다
    pthread_mutex_lock(&mutex);
    remove_client_by_socket(&client_list, client_socket); // list에서 client를 제거한다
    pthread_mutex_unlock(&mutex);
    close(client_socket);

    return NULL;
}

// send to all
void send_message(char *message, int len) {
    int i;
    ClientNode* current_client;

    // 공유자원에 대한 lock
    pthread_mutex_lock(&mutex);
    refer_first_client(&client_list);
    while((current_client = client_list.current_client) != NULL) {
        write(current_client->client_socket, message, len);
        refer_next_client(&client_list);
    }
    refer_first_client(&client_list);
    pthread_mutex_unlock(&mutex);
}