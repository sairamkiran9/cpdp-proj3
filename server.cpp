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
#include <algorithm>
#include <pthread.h>
#include <cstring>
#include <signal.h>

using namespace std;

const unsigned MAXBUFLEN = 512;
pthread_mutex_t accept_lock = PTHREAD_MUTEX_INITIALIZER;
int serv_sockfd;
socklen_t len;
pthread_t tid;
struct sockaddr_in addr, recaddr;
vector<int> sock_vector;
map<string, int> usertofd;
map<int, string> fdtouser;
fd_set allset, rset;
int sockfd, rec_sock, maxfd;

void parse_message(string str, int cli_fd)
{
	int fd;
	size_t pos = 0;
	string username = "";
	string login = "login ";
	string chat = "chat ";
	char msg[100];
	if ((pos = str.find(login)) != string::npos)
	{
		str.erase(0, pos + login.length());
		if (fdtouser[cli_fd] != "" && usertofd.find(str) != usertofd.end())
		{
			strcpy(msg, "User already logged in.");
		}
		else if (fdtouser[cli_fd] != "")
		{
			strcpy(msg, "Another user already logged in.");
		}
		else
		{
			usertofd.insert(make_pair(str, cli_fd));
			fdtouser[cli_fd] = str;
			strcpy(msg, "User logged in.");
		}
		write(cli_fd, msg, strlen(msg));
	}
	else if (fdtouser[cli_fd] == "" && str != "exit")
	{
		strcpy(msg, "User not logged in.");
		write(cli_fd, msg, strlen(msg));
	}
	else if ((pos = str.find(chat)) != string::npos)
	{
		str.erase(0, pos + chat.length());
		if ((pos = str.find("@")) != string::npos)
		{
			if ((pos = str.find(" ")) != string::npos)
			{
				username = str.substr(1, pos - 1);
				str.erase(0, pos);
			}
			if (usertofd.find(username) != usertofd.end())
			{
				fd = usertofd[username];
				// cout << "fd:" << fd << endl;
			}
			else
			{
				strcpy(msg, "Error: Mentioned user not on server.");
				write(cli_fd, msg, strlen(msg));
				return;
			}
			str.erase(0, 1);
		}
		str = fdtouser[cli_fd] + " >> " + str;
		strcpy(msg, str.c_str());
		if (username != "")
		{
			write(fd, msg, strlen(msg));
		}
		else
		{
			for (const auto &pair : usertofd)
			{
				if (pair.second != cli_fd)
					write(pair.second, msg, strlen(msg));
			}
		}
	}
	else if (str == "logout")
	{
		strcpy(msg, "User logged out.");
		usertofd.erase(fdtouser[cli_fd]);
		fdtouser[cli_fd] = "";
		write(cli_fd, msg, strlen(msg));
	}
	else if (str == "exit")
	{
		strcpy(msg, "exit");
		if (fdtouser[cli_fd] == "")
		{
			fdtouser.erase(cli_fd);
			write(cli_fd, msg, strlen(msg));
			close(cli_fd);
		}
		else
		{
			strcpy(msg, "User should logout.");
			write(cli_fd, msg, strlen(msg));
		}
	}
}

void *one_thread1(void *arg)
{
	char buf[MAXBUFLEN];
	int tid = *((int *)arg);
	free(arg);

	// cout << "thread " << tid << " created" << endl;
	while (1)
	{
		// pthread_mutex_lock(&accept_lock);
		rset = allset;
		select(maxfd + 1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(serv_sockfd, &rset))
		{
			/* somebody tries to connect */
			if ((rec_sock = accept(serv_sockfd, (struct sockaddr *)(&recaddr), &len)) < 0)
			{
				if (errno == EINTR)
					continue;
				else
				{
					perror(":accept error");
					exit(1);
				}
			}
			// cout << "new connection: " << rec_sock << endl;
			fdtouser[rec_sock] = "";
			sock_vector.push_back(rec_sock);
			FD_SET(rec_sock, &allset);
			if (rec_sock > maxfd)
				maxfd = rec_sock;
		}
		// pthread_mutex_unlock(&accept_lock);

		auto itr = sock_vector.begin();
		while (itr != sock_vector.end())
		{
			int num, fd;
			fd = *itr;
			if (FD_ISSET(fd, &rset))
			{
				num = read(fd, buf, 100);
				if (num == 0)
				{
					/* client exits */
					close(fd);
					FD_CLR(fd, &allset);
					itr = sock_vector.erase(itr);
					continue;
				}
				else
				{
					buf[num] = '\0';
					parse_message(buf, fd);
				}
			}
			++itr;
		}

		maxfd = serv_sockfd;
		if (!sock_vector.empty())
		{
			maxfd = max(maxfd, *max_element(sock_vector.begin(),
											sock_vector.end()));
		}
	}
}

void check_time_wait()
{
	int optval;
	socklen_t optlen = sizeof(optval);
	if (getsockopt(serv_sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) == -1)
	{
		perror("getsockopt");
		exit(EXIT_FAILURE);
	}

	if (optval == EINPROGRESS)
	{
		printf("Socket is in TIME_WAIT state\n");
	}
	else
	{
		printf("Socket is not in TIME_WAIT state\n");
	}

	// Close the socket
	close(serv_sockfd);
}

static void sig_int(int signo)
{
	for (const auto &pair : fdtouser)
	{
		close(pair.first);
	}
	// check_time_wait();
	close(serv_sockfd);
	cout << "Server closed" << endl;
	exit(0);
}

int main(int argc, char *argv[])
{
	struct sockaddr_in serv_addr;
	int port, number_thread, i;
	int *tid_ptr;

	signal(SIGINT, sig_int);

	if (argc != 3)
	{
		fprintf(stderr, "%s port number_thread\n", argv[0]);
		exit(1);
	}

	port = atoi(argv[1]);
	number_thread = atoi(argv[2]);

	// cout << "port = " << port << " number of threads == " << number_thread << endl;
	cout << "server started on port: " << port << endl;
	serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero((void *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	int optval = 1;
	setsockopt(serv_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	listen(serv_sockfd, 5);

	FD_ZERO(&allset);
	FD_SET(serv_sockfd, &allset);
	maxfd = serv_sockfd;

	sock_vector.clear();

	for (i = 0; i < number_thread; ++i)
	{
		tid_ptr = (int *)malloc(sizeof(int));
		*tid_ptr = i;
		pthread_create(&tid, NULL, &one_thread1, (void *)tid_ptr);
		// pthread_join(tid, NULL);
	}

	for (;;)
	{
		pause();
	}
}
