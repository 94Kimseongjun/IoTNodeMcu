#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024

typedef struct MultipleArg
{
	int clnt_sock;
	char buf[BUF_SIZE];
	int str_len;
}Mp;

pthread_mutex_t mtx;

void error_handling(char *message);
void read_childproc(int sig);
void* write_thread(void *arg);

int main(int argc, char *argv[])
{
	Mp *mp;
	mp = (Mp *)malloc(sizeof(Mp));

	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	
	pid_t pid;
	struct sigaction act;
	socklen_t adr_sz;
	int str_len, state;
	char buf[BUF_SIZE];
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	act.sa_handler=read_childproc;
	sigemptyset(&act.sa_mask);
	act.sa_flags=0;
	state=sigaction(SIGCHLD, &act, 0);
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));
	
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");
	
	while(1)
	{
		adr_sz=sizeof(clnt_adr);
		mp->clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
		clnt_sock = mp->clnt_sock;	
		
		if(clnt_sock==-1)
			continue;
		else
			puts("new client connected...");
		pid=fork();
		if(pid==-1)
		{
			close(clnt_sock);
			continue;
		}
		if(pid==0)
		{
			close(serv_sock);

			
			pthread_t t_id;

			while((str_len=read(mp->clnt_sock, mp->buf, BUF_SIZE))!=0)
			{
				mp->str_len = str_len;
				pthread_create(&t_id, NULL, write_thread, (void *)mp);
				pthread_detach(t_id);
			}
			
			close(clnt_sock);
			puts("client disconnected...");
			return 0;
		}
		else
			close(clnt_sock);
	}
	close(serv_sock);
	return 0;
}
void* write_thread(void* arg)
{
	pthread_mutex_lock(&mtx);
	Mp *mp_arg = (Mp *)arg;
	write(mp_arg->clnt_sock, mp_arg->buf, mp_arg->str_len);
	pthread_mutex_unlock(&mtx);
}

void read_childproc(int sig)
{
	pid_t pid;
	int status;
	pid=waitpid(-1, &status, WNOHANG);
	printf("removed proc id: %d \n", pid);
}
void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
} 