#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <strings.h>
#include <pthread.h>
#include <algorithm>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

const unsigned MAXBUFLEN = 512;
// pthread_mutex_t accept_lock = PTHREAD_MUTEX_INITIALIZER;
socklen_t len;
pthread_t tid;
fd_set allset, rset;
vector<int> sock_vector;
map<string, int> usertofd;
map<int, string> fdtouser;
struct sockaddr_in addr, recaddr;
int serv_sockfd, port, rec_sock, maxfd, number_thread = 1;

static void sig_int(int signo)
{
	for (const auto &pair : fdtouser)
	{
		close(pair.first);
	}
	close(serv_sockfd);
	cout << "Server closed" << endl;
	exit(0);
}

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
			strcpy(msg, "ERROR: User already logged in.");
		}
		else if (fdtouser[cli_fd] != "")
		{
			strcpy(msg, "ERROR: Another user already logged in.");
		}
		else if (usertofd.find(str) == usertofd.end())
		{
			usertofd.insert(make_pair(str, cli_fd));
			fdtouser[cli_fd] = str;
			strcpy(msg, "User logged in.");
		}
		else
		{
			strcpy(msg, "ERROR: User already logged in.");
		}
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
	else if (str == "close")
	{
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
			FD_CLR(cli_fd, &allset);
			close(cli_fd);
		}
		else
		{
			strcpy(msg, "ERROR: User should logout.");
			write(cli_fd, msg, strlen(msg));
		}
	}
	else
	{
		strcpy(msg, "ERROR: Invalid command");
		write(cli_fd, msg, strlen(msg));
	}
}

void *init_connection(void *arg)
{
	char buf[MAXBUFLEN];
	free(arg);

	while (1)
	{
		rset = allset;
		select(maxfd + 1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(serv_sockfd, &rset))
		{
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
			fdtouser[rec_sock] = "";
			sock_vector.push_back(rec_sock);
			FD_SET(rec_sock, &allset);
			if (rec_sock > maxfd)
				maxfd = rec_sock;
		}

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

void load_config(char *filename)
{
	map<string, string> config;
	ifstream infile(filename);
	string line;
	while (getline(infile, line))
	{
		size_t pos = line.find(':');
		if (pos != string::npos)
		{
			string key = line.substr(0, pos);
			string value = line.substr(pos + 1);
			config[key] = value;
		}
	}
	infile.close();

	for (auto it = config.begin(); it != config.end(); ++it)
	{
		if (it->first == "port")
		{
			port = stoi(it->second);
		}
		else if (it->first == "threads")
		{
			number_thread = stoi(it->second);
		}
	}
}

int main(int argc, char *argv[])
{
	int *tid_ptr;
	char ipstr[INET_ADDRSTRLEN];
	struct sockaddr_in serv_addr, addr_info;

	socklen_t addr_info_len = sizeof(addr_info);

	signal(SIGINT, sig_int);

	if (argc != 2)
	{
		fprintf(stderr, "%s chat_server configration_file\n", argv[0]);
		exit(1);
	}

	load_config(argv[1]);

	if (port != 0 && (port < 25100 || port > 25299))
	{
		fprintf(stderr, "port should be 0 in the range 25100 - 25299\n");
		exit(1);
	}

	serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero((void *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	int optval = 1;
	setsockopt(serv_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	listen(serv_sockfd, 5);

	getsockname(serv_sockfd, (sockaddr *)&addr_info, &addr_info_len);

	// convert the IP address from network byte order to a string

	inet_ntop(AF_INET, &(addr_info.sin_addr), ipstr, INET_ADDRSTRLEN);

	// print the IP address and port
	cout << "Server started on " << ipstr << " on port " << ntohs(addr_info.sin_port) << endl;

	FD_ZERO(&allset);
	FD_SET(serv_sockfd, &allset);
	maxfd = serv_sockfd;
	sock_vector.clear();

	for (int i = 0; i < number_thread; ++i)
	{
		tid_ptr = (int *)malloc(sizeof(int));
		*tid_ptr = i;
		pthread_create(&tid, NULL, &init_connection, (void *)tid_ptr);
	}

	for (;;)
	{
		pause();
	}
}