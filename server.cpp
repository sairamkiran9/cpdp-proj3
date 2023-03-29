#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <map>
#include <string>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <cstring>

using namespace std;

// const unsigned port = 5100;
const unsigned MAXBUFLEN = 512;
pthread_mutex_t accept_lock = PTHREAD_MUTEX_INITIALIZER;
int serv_sockfd;
// vector<int> sock_vector;
map<string, int> usertofd;
// map<int, string> fdtouser;
fd_set allset, rset;
int sockfd, rec_sock, maxfd;

void parse_message(string str, int cli_fd)
{
	int fd, pos = 0, flag = 0;
	string username = "";
	string login = "login ";
	string chat = "chat [";
	if ((pos = str.find(login)) != string::npos)
	{
		str.erase(0, pos + login.length());
		usertofd.insert(make_pair(str, cli_fd));
		char cstr[str.size() + 1];
		strcpy(cstr, str.c_str());
		write(cli_fd, cstr, strlen(cstr));
	}
	else if ((pos = str.find(chat)) != string::npos)
	{
		str.erase(0, pos + chat.length());
		if ((pos = str.find("@")) == string::npos)
		{
			// cout << "user not specified" << endl;
			flag = 1;
		}
		if ((pos = str.find("]")) != string::npos)
		{
			// cout << flag << pos << endl;
			if (flag == 1)
			{
				username = "";
			}
			else
			{
				username = str.substr(1, pos - 1);
			}
			// cout << "fetching user: " << username << endl;
			if (username != "")
			{
				fd = usertofd[username];
				// cout << "fetched fd: " << fd << endl;
			}
			str.erase(0, pos + 2);
			// cout << "fetching message: " << str << endl;
		}
		char cstr[str.size() + 1];
		strcpy(cstr, str.c_str());
		if (flag == 0)
		{
			// cout << "sending to fd: " << fd << endl;
			write(fd, cstr, strlen(cstr));
		}
		else
		{
			for (const auto &pair : usertofd)
			{
				write(pair.second, cstr, strlen(cstr));
			}
		}
	}
}

int test_func()
{
	int cli_sockfd;
	struct sockaddr_in cli_addr;
	socklen_t sock_len;

	rset = allset;
	sock_len = sizeof(cli_addr);

	select(maxfd + 1, &rset, NULL, NULL, NULL);
	if (FD_ISSET(serv_sockfd, &rset))
	{
		/* somebody tries to connect */
		if (cli_sockfd = accept(serv_sockfd, (struct sockaddr *)&cli_addr, &sock_len) < 0)
		{
			// if (errno == EINTR)
			// 	continue;
			// else
			// {
				perror(":accept error");
				exit(1);
			// }
		}

		// cout << "thread " << tid << ": ";
		cout << "remote client IP == " << inet_ntoa(cli_addr.sin_addr);
		cout << ", port == " << ntohs(cli_addr.sin_port) << endl;

		FD_SET(cli_sockfd, &allset);
		if (cli_sockfd > maxfd)
			maxfd = cli_sockfd;
	}
	return cli_sockfd;
}

void *one_thread(void *arg)
{
	int cli_sockfd;
	struct sockaddr_in cli_addr;
	socklen_t sock_len;
	ssize_t n;
	char buf[MAXBUFLEN];

	int tid = *((int *)arg);
	free(arg);

	cout << "thread " << tid << " created" << endl;
	for (;;)
	{
		sock_len = sizeof(cli_addr);
		pthread_mutex_lock(&accept_lock);
		// cli_sockfd = 
		cli_sockfd = accept(serv_sockfd, (struct sockaddr *)&cli_addr, &sock_len);
		pthread_mutex_unlock(&accept_lock);

		cout << "thread " << tid << ": ";
		cout << "remote client IP == " << inet_ntoa(cli_addr.sin_addr);
		cout << ", port == " << ntohs(cli_addr.sin_port) << endl;

		while ((n = read(cli_sockfd, buf, MAXBUFLEN)) > 0)
		{
			buf[n] = '\0';
			// cout << cli_sockfd<< buf << endl;
			parse_message(buf, cli_sockfd);
		}
		if (n == 0)
		{
			cout << "server: client closed" << endl;
		}
		else
		{
			cout << "server: something wrong" << endl;
		}
		close(cli_sockfd);
	}
	return (NULL);
}

int main(int argc, char *argv[])
{
	struct sockaddr_in serv_addr;
	int port, number_thread, i;
	pthread_t tid;
	int *tid_ptr;

	if (argc != 3)
	{
		fprintf(stderr, "%s port number_thread\n", argv[0]);
		exit(1);
	}

	port = atoi(argv[1]);
	number_thread = atoi(argv[2]);

	cout << "port = " << port << " number of threads == " << number_thread << endl;
	serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero((void *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	listen(serv_sockfd, 5);

	FD_ZERO(&allset);
	FD_SET(serv_sockfd, &allset);
	maxfd = serv_sockfd;

	for (i = 0; i < number_thread; ++i)
	{
		tid_ptr = (int *)malloc(sizeof(int));
		*tid_ptr = i;
		pthread_create(&tid, NULL, &one_thread, (void *)tid_ptr);
	}

	for (;;)
	{
		pause();
	}
}
