#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "client_info.h"

#define BUF_SIZE 100
#define NAME_SIZE 20
#define MAX_CLNT 255 // 최대 접속 클라이언트 수는 255로 제한

#define DELIM_CHARS "/-: "

void error_handling(char *msg); // 에러 발생시 에러를 핸들링하는 함수
void *handle_client(void *arg); // 자식 thread에 전달할 함수 포인터
void send_message(char *message, int len, char *dest_name, char *src_name); // 메시지를 send하는 함수
void whisper_init(char *message, char *dest_name, char *src_name); // 메세지의 대상 이름, 주체 이름을 저장하는 함수
void print_client_list();

// critical sections
int client_cnt = 0; // 접속한 사용자의 수
Client *client_list[MAX_CLNT]; // 접속한 사용자의 소켓 정보
pthread_mutex_t mutex;

int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_adr, client_adr;
    int client_adr_size;
    int client_name_len; // 접속을 요청하는 client의 이름 길이
    char client_name[BUF_SIZE]; // 접속을 요청하는 client의 이름 정보
    pthread_t t_id;

    // 잘못된 input이 들어오는 경우에 에러 처리
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutex, NULL); // mutex 변수의 초기화
    server_socket = socket(PF_INET, SOCK_STREAM, 0); // server의 socket descriptor 생성

    // server socket을 바인드하기 위해서 server_adr 구조체의 내용을 채워넣는다
    memset(&server_adr, 0, sizeof(server_adr));
    server_adr.sin_family = AF_INET;
    server_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_adr.sin_port = htons(atoi(argv[1]));

    // bind 함수 실행
    if (bind(server_socket, (struct sockaddr *) &server_adr, sizeof(server_adr)) == -1)
        error_handling("bind() error!");

    // listen 함수 실행 -> listening queue의 사이즈는 10으로 설정한다
    if (listen(server_socket, 10) == -1)
        error_handling("listen() error!");

    while (1) {
        client_adr_size = sizeof(client_adr);
        // accept 함수를 통해 client의 접속을 감지한다. 기본적으로 non-blocking 함수이기 때문에 client의 접속이 이뤄질 때까지 멈춰있다.
        client_socket = accept(server_socket, (struct sockaddr *) &client_adr, &client_adr_size);

        // connect가 이뤄지자마자 client의 이름 정보를 가져와서 new client 포인터를 생성해낸다
        client_name_len = read(client_socket, client_name, BUF_SIZE);
        client_name[client_name_len] = 0;
        Client *new_client = create_client(client_socket, client_name);

        // server가 관리하는 client 접속 정보 세팅
        // client_list, client_cnt의 경우 여러 쓰레드가 공유하는 자원이기 때문에 쓰레드 간의 Race-condition을 방지하기 위해 mutex를 이용해서 lock을 걸어야한다.
        pthread_mutex_lock(&mutex);
        client_list[client_cnt++] = new_client;
        print_client_list();
        pthread_mutex_unlock(&mutex);

        // thread 생성 및 thread_detach 등록 -> thread detach는 non-blocking function
        // thread 생성시 handle_client 함수 포인터를 넘기고, handle_client의 매개변수로 client_socket을 넘겨준다.
        pthread_create(&t_id, NULL, handle_client, (void *) &client_socket);
        pthread_detach(t_id); // non-blocking으로 t_id에 해당하는 쓰레드가 삭제되었는지 감지하는 역할을 수행하는 함수

        // client가 연결이 되었을때 서버에 출력하는 정보
        printf("Connected client IP: %s, name: %s, socket: %d\n", inet_ntoa(client_adr.sin_addr), client_name, client_socket);
    }

    close(server_socket);
    return 0;
}

/**
 * Function that handles the error of process
 * @param msg message
 */
void error_handling(char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

/**
 * Function that is enterance point of the thread.
 * @param arg parameter that this function uses.
 */
void *handle_client(void *arg) {
    int client_socket = *((int *) arg); // void pointer를 int pointer로 타입 캐스팅 한 후 역참조
    int str_len = 0;
    int i;
    char message[BUF_SIZE]; // 메시지를 저장하는 string 변수
    char whisper_dest_name[NAME_SIZE]; // 귓속말의 대상의 이름을 저장하는 변수
    char whisper_src_name[NAME_SIZE]; // 귓속말을 던지는 주체의 이름을 저장하는 변수
    Client *removed_client; // Client 포인터를 메모리 해제시키기 위해서 임시 저장해둘 변수

    while ((str_len = read(client_socket, message, sizeof(message))) != 0) {
        whisper_init(message, whisper_dest_name, whisper_src_name);
        send_message(message, str_len, whisper_dest_name, whisper_src_name);
    }

    // disconnect the client -> lock을 걸어둔 상태에서 disconnect를 처리한다
    pthread_mutex_lock(&mutex);
    for (i = 0; i < client_cnt; i++) {
        // 자신에 해당하는 변수가 찾아지면 그 뒤로부터 한 칸씩 땡긴다.
        if (client_socket == client_list[i]->client_socket) {
            removed_client = client_list[i];
            while (i < client_cnt - 1) {
                client_list[i] = client_list[i + 1];
                i++;
            }

            // 자기가 사용하던 client의 정보를 메모리 상에서 삭제한다
            free(removed_client);
            break;
        }
    }
    client_cnt--;
    print_client_list(); // 현재 접속 클라이언트 정보 출력
    pthread_mutex_unlock(&mutex); // mutex lock을 해제한다.
    close(client_socket); // client의 접속을 끊는다.

    return NULL;
}

/**
 * Function that snipping whether the message is to go all or someone.
 * @param message
 * @param dest_name name variable who sends the message.
 * @param src_name name variable who receives the message.
 */
void whisper_init(char *message, char *dest_name, char *src_name) {
    char copied_message[BUF_SIZE]; // message의 내용을 복사해서 가지고있을 변수
    strcpy(copied_message, message);
    char *next_buffer; // strtok_s에서 사용할 임시변수 -> thread-safe
    char *token_buffer = strtok_r(copied_message, DELIM_CHARS, &next_buffer); // strtok_s로 분리된 토큰을 저장하는 변수

    // message를 tokenize하면서 dest_name, src_name을 초기화시킨다.
    while (token_buffer) {
        int tok_len;

        // '['로 시작하는 경우 src_name이 담겨있기 때문에 src_name은 여기서 초기화시킨다.
        if (token_buffer[0] == '[') {
            tok_len = strlen(token_buffer);
            strcpy(src_name, token_buffer);

            for (int i = 0; i < tok_len; i++)
                src_name[i] = src_name[i + 1];
            src_name[tok_len - 2] = 0;
        }
        // '@'로 시작하는 경우 dest_name이 담겨있기 때문에 dest_name은 여기서 초기화시킨다.
        else if (token_buffer[0] == '@') {
            tok_len = strlen(token_buffer);
            strcpy(dest_name, token_buffer);

            for (int i = 0; i < tok_len; i++)
                dest_name[i] = dest_name[i + 1];
        }

        // 다음 token을 읽어낸다.
        token_buffer = strtok_r(NULL, DELIM_CHARS, &next_buffer);
    }
}

// send to all
void send_message(char *message, int len, char *dest_name, char *src_name) {
    int i;
    int target_socket, src_socket;
    char error_message[BUF_SIZE];

    // 모든 클라이언트에게 전송하는 메세지인 경우
    if (strcmp(dest_name, "all") == 0) {
        // 공유자원에 대한 lock
        pthread_mutex_lock(&mutex);
        for (i = 0; i < client_cnt; i++)
            write(client_list[i]->client_socket, message, len);
        pthread_mutex_unlock(&mutex);
    }
    // 특정 사용자에게 메세지를 보내는 것이거나, 아니면 존재하지 않는 클라이언트에게 전송하는 경우이다.
    else {
        // client의 정보를 다루기 때문에 mutex lock을 이용해서 race-condition을 해소시켜줘야한다.
        pthread_mutex_lock(&mutex);
        // dest_name에 해당하는 client를 find_client_by_name 함수를 이용해서 탐색한다. -> Complexity equals O(N)
        target_socket = find_client_by_name(client_list, client_cnt, dest_name);

        // target이 현재 채팅방에 존재하는 경우
        if (target_socket != -1) {
            write(target_socket, message, len);
        }
        // target이 현재 채팅방에 존재하지도 않는 경우
        else {
            // src_name을 이용해서 메세지를 보냈던 client를 탐색한다
            int src_socket = find_client_by_name(client_list, client_cnt, src_name);
            // 에러 메시지 작성
            sprintf(error_message, "Client [%s] doesn't exist here!!\n", dest_name);
            int error_message_len = strlen(error_message);
            // 에러 메시지 전달
            send(src_socket, error_message, error_message_len, 0);
        }
        pthread_mutex_unlock(&mutex);
    }
}

void print_client_list() {
    printf("==========[Current Clients]==========\n");
    for(int i = 0; i < client_cnt; i++)
        printf("user_name: %s, socket: %d\n", client_list[i]->name, client_list[i]->client_socket);
    printf("==========[END]==========\n");
}