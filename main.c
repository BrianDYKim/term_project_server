#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 100
#define MAX_CLNT 255 // 최대 접속 클라이언트 수는 255로 제한

void error_handling(char *msg); // 에러 발생시 에러를 핸들링하는 함수

int main(int argc, char *argv[]) {
    for(int i = 0; i < argc; i++)
        printf("%s\n", *(argv + i));

    return 0;
}

void error_handling(char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}