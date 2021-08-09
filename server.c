#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>

#define BUF_SIZE 100
#define MAX_CLIENT 100 //최대 접속 인원 수

void * client_handling(void *arg); //클라이언트 메세지를 수신 및 믈라이언트 관리
void send_message(char *message, int len);//수신한 메세지 클라이언트들에게 송신 및 메세지 출력
void error_handling(char *message); //에러 발생시 에러문 발생

int client_cnt=0; //접속자 인원 수
int client_socks[MAX_CLIENT]; // 접속자를 각 소켓에 저장하기 위한 정수형 배열
pthread_mutex_t mutx; //뮤텍스 객체 선언

int main(int argc, char *argv[])
{
    int server_sock, client_sock, option;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_size;
    pthread_t id;

    //사용법 설명
    if (argc != 2)
    {
        printf(" Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutx, NULL); //뮤텍스 객체 초기화
    server_sock=socket(PF_INET, SOCK_STREAM, 0);
    option = 1; //SO_REUSEADDR의 옵션 값을 TRUE로 설정
    //같은 포트에 대해 다른 소켓이 bind()되는 것을 허락해 주기 위한 코드
    setsockopt( server_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option) );

    memset(&server_addr, 0, sizeof(server_addr)); //주소 초기화

    // 구조체를 이용하여 서버의 아이피 주소와 포트, 프로토콜 설정 및 관리
    server_addr.sin_family=AF_INET; // 통신 프로토콜 설정
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY); //ip 주소 를 컴퓨터가  인식 >가능하도록 처리
    server_addr.sin_port=htons(atoi(argv[1])); // 입력된 포트 번호 컴퓨터가 인식
 가능하도록 처리

    //바인드 함수, 오류시 오류 출력
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr))==-1)
        error_handling("bind() error");
    if (listen(server_sock, 5)==-1)
        error_handling("listen() error");

    while(1)
    {
        client_addr_size=sizeof(client_addr);
        // 클라이언트 접속 요철 들어올 시 accept함수로 연결 수락함
        client_sock=accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);

        pthread_mutex_lock(&mutx); //잠금을 생성
        client_socks[client_cnt++]=client_sock; //새로운 사용자가 입장할 때마다 소켓번호 부여
        pthread_mutex_unlock(&mutx); //잠금을 해제

        pthread_create(&id, NULL, client_handling, (void*)&client_sock); //쓰레>드 생성
        pthread_detach(id);//식별번호 id를 가지는 쓰레드를 분리시킴
    }
    close(server_sock);//소켓을 닫음
    return 0;
}

void *client_handling(void *arg)
{
    int client_sock=*((int*)arg);
    int str_len=0, i;
    char message[BUF_SIZE];

    while((str_len=read(client_sock, message, sizeof(message)))!=0)//클라이언트 메세지 수신
        send_message(message, str_len);

    //연결 해제된 클라이언트 삭제
    pthread_mutex_lock(&mutx);
    for (i=0; i<client_cnt; i++)
    {
        if (client_sock==client_socks[i])
        {
            while(i++<client_cnt-1)
                client_socks[i]=client_socks[i+1];
            break;
        }
    }
    client_cnt--;
    pthread_mutex_unlock(&mutx);
    close(client_sock);//클라이언트 소켓 닫음
    return NULL;
}

void send_message(char* message, int len)
{
    int i;
    pthread_mutex_lock(&mutx);
    for (i=0; i<client_cnt; i++)
        write(client_socks[i], message, len); //클라이언트로 송신
    for (i=0; i<len; i++)
        printf("%c", message[i]); //클라이언트들의 메세지 출력
    pthread_mutex_unlock(&mutx);
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
