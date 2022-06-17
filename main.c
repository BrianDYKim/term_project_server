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
#define NAME_SIZE 20
#define MAX_CLNT 255 // 최대 접속 클라이언트 수는 255로 제한
#define DELIM_CHARS "/-: "

void error_handling(char *msg); // 에러 발생시 에러를 핸들링하는 함수
void *handle_client(void *arg); // 자식 thread에 전달할 함수 포인터
void send_message_to_all(char *message, int len); // 메시지를 send하는 함수
void send_message_to_one(char *message, int len, char *target_name, char *src_name); // 메시지를 단 한 명에게 보내는 함수
int is_whisper(char *message, char *dest_name, char *src_name);

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
    char name[BUF_SIZE];
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
        name_len = read(client_socket, name, BUF_SIZE);
        name[name_len] = 0;

        // server가 관리하는 client 접속 정보 세팅 (mutex를 이용해서 race condition issue 해결)
        pthread_mutex_lock(&mutex);
        append_client(&client_list, client_socket, name);
        pthread_mutex_unlock(&mutex);

        // thread 생성 및 thread_detach 등록 -> thread detach는 non-blocking function
        pthread_create(&t_id, NULL, handle_client, (void *) &client_socket);
        pthread_detach(t_id);

        // client가 연결이 되었을때 서버에 출력하는 정보
        printf("Connected client IP: %s, name: %s\n", inet_ntoa(client_adr.sin_addr), name);
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
    char whisper_target_name[NAME_SIZE]; // 귓속말의 대상 이름을 저장하는 변수
    char whisper_src_name[NAME_SIZE]; // 귓속말을 일으킨 사람 이름을 저장하는 변수

    while ((str_len = read(client_socket, message, sizeof(message))) != 0) {
        if (is_whisper(message, whisper_target_name, whisper_src_name)) {
            printf("Whisper on!!\n");
            send_message_to_one(message, str_len, whisper_target_name, whisper_src_name);
        } else {
            send_message_to_all(message, str_len);
        }
    }

    // disconnect the client -> lock을 걸어둔 상태에서 disconnect를 처리한다
    pthread_mutex_lock(&mutex);
    remove_client_by_socket(&client_list, client_socket); // list에서 client를 제거한다
    pthread_mutex_unlock(&mutex);
    close(client_socket);

    return NULL;
}

// 귓속말인지 판단을 하고, name에는 대상 client의 이름을 저장하는 함수
int is_whisper(char *message, char *dest_name, char *src_name) {
    char copied_message[BUF_SIZE];
    strcpy(copied_message, message);
    char *next_buffer; // strtok_s에서 사용할 임시변수 -> thread-safe
    char *token_buffer = strtok_r(copied_message, DELIM_CHARS, &next_buffer); // strtok_s로 분리된 토큰을 저장하는 변수
    int sig = 0;

    while (token_buffer) {
        // 귓속말인 경우 이름을 추출한다. 그리고 sig를 1로 초기화시킨다.
        if (token_buffer[0] == '@') {
            int tok_len = strlen(token_buffer);
            strcpy(dest_name, token_buffer);

            for (int i = 0; i < tok_len; i++)
                dest_name[i] = dest_name[i + 1];

            sig = 1;
        } else if (token_buffer[0] == '[') {
            int tok_len = strlen(token_buffer);
            strcpy(src_name, token_buffer);

            for (int i = 0; i < tok_len; i++)
                src_name[i] = src_name[i + 1];
            src_name[tok_len - 2] = 0;
        }
        token_buffer = strtok_r(NULL, DELIM_CHARS, &next_buffer);
    }

    return sig;
}

// send to all
void send_message_to_all(char *message, int len) {
    int i;
    ClientNode *current_client;

    // 공유자원에 대한 lock
    pthread_mutex_lock(&mutex);
    refer_first_client(&client_list);
    while ((current_client = client_list.current_client) != NULL) {
        write(current_client->client_socket, message, len);
        refer_next_client(&client_list);
    }
    refer_first_client(&client_list);
    pthread_mutex_unlock(&mutex);
}

// send to one
void send_message_to_one(char *message, int len, char *target_name, char *src_name) {
    ClientNode *target_client; // 귓속말의 대상 client
    char error_message[BUF_SIZE];

    pthread_mutex_lock(&mutex);
    target_client = find_client_by_name(&client_list, target_name);

    if (target_client == NULL) {
        sprintf(error_message, "Client [%s] doesn't exist here!!\n", target_name);
        int err_msg_len = strlen(error_message);
        send(find_client_by_name(&client_list, src_name)->client_socket, error_message, err_msg_len, 0);
    } else {
        write(target_client->client_socket, message, len);
    }
    pthread_mutex_unlock(&mutex);
}