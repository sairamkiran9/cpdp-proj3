#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string>
#include <cstring>
#include <pthread.h>
#include <signal.h>

using namespace std;
const unsigned MAXBUFLEN = 512;
int sockfd;

static void sig_int(int signo)
{
	signal(SIGINT, sig_int);
	cout << "client socket closed" << endl;
	write(sockfd, "logout", 6);
	close(sockfd);
	exit(0);
}

void *process_connection(void *arg)
{
	int n;
	char buf[MAXBUFLEN];
	pthread_detach(pthread_self());
	while (1)
	{
		n = read(sockfd, buf, MAXBUFLEN);
		if (n <= 0)
		{
			if (n == 0)
			{
				cout << "server closed" << endl;
			}
			else
			{
				cout << "something wrong" << endl;
			}
			close(sockfd);
			exit(1);
		}
		buf[n] = '\0';
		if (strcmp(buf, "exit") == 0)
		{
			cout << "Client session closed." << endl;
			close(sockfd);
			exit(0);
		}
		cout << buf << endl;
	}
}

// const unsigned serv_port = 5100;

int main(int argc, char **argv)
{
	int rv, flag;
	struct addrinfo hints, *res, *ressave;
	pthread_t tid;

	if (argc != 3)
	{
		cout << "echo_client server_name_or_ip port" << endl;
		exit(1);
	}

	cout << argv[1] << " " << argv[2] << endl;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0)
	{
		cout << "getaddrinfo wrong: " << gai_strerror(rv) << endl;
		exit(1);
	}

	ressave = res;
	flag = 0;
	do
	{
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

		if (sockfd < 0)
			continue;
		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
		{
			flag = 1;
			break;
		}
		else{
			perror("connect: ");
		}
		close(sockfd);
	} while ((res = res->ai_next) != NULL);
	freeaddrinfo(ressave);

	if (flag == 0)
	{
		perror("Error:");
		fprintf(stderr, "cannot connect\n");
		exit(1);
	}

	signal(SIGINT, sig_int);

	pthread_create(&tid, NULL, &process_connection, NULL);

	string oneline;
	while (getline(cin, oneline))
	{
		write(sockfd, oneline.c_str(), oneline.length());
	}
	// exit(0);
}
