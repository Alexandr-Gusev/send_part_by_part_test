//------------------------------------------------------------------------------
#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <sys\stat.h>
#include <string>
#include <error.h>
//------------------------------------------------------------------------------
//#define IS_SERVER

#define IP "172.20.44.45"
#define PORT 555

#define TO_SERVER_FILE_NAME "to_server.txt"
#define TO_CLIENT_FILE_NAME "to_client.txt"
#define FROM_SERVER_FILE_NAME "from_server.txt"
#define FROM_CLIENT_FILE_NAME "from_client.txt"
//------------------------------------------------------------------------------
bool read_file(const char *file_name, char ** const data, int * const data_size)
{
	struct stat si;
	if (stat(file_name, &si))
	{
		printf("stat error\n");
		return false;
	}

	*data_size = si.st_size;

	FILE *f = fopen(file_name, "rb");
	if (!f)
	{
		printf("fopen error\n");
		return false;
	}

	*data = new char[*data_size];

	if (fread(*data, 1, *data_size, f) != *data_size)
	{
		printf("fread error\n");
		fclose(f);
		delete [] *data;
		return false;
	}

	fclose(f);
	return true;
}
int s2i(const std::string &s)
{
	int res = 0;
	sscanf(s.c_str(), "%d", &res);
	return res;
}
bool receive_file(const char *file_name, SOCKET s)
{
	FILE *f = fopen(file_name, "wb");
	if (!f)
	{
		printf("fopen error\n");
		return false;
	}

	char buffer[4096];
	int buffer_size = sizeof(buffer);

	int size = -1;

	while (size)
	{
		fd_set s_set_in;
		FD_ZERO(&s_set_in);
		FD_SET(s, &s_set_in);

		timeval timeout = {0, 0};

		int select_res = select(s + 1, &s_set_in, 0, 0, &timeout);
		if (select_res == SOCKET_ERROR)
		{
			printf("select error\n");
			return false;
		}
		if (!select_res) continue;

		int n = recv(s, buffer, buffer_size, 0);
		if (n == SOCKET_ERROR)
		{
			if (socket_subsys_error() == WSAEAGAIN || socket_subsys_error() == WSAEWOULDBLOCK)
			{
				Sleep(250);
				continue;
			}
			else
			{
				printf("recv error\n");
				return false;
			}
		}
		if (!n)
		{
			printf("disconnected\n");
			break;
		}
		if (fwrite(buffer, 1, n, f) != n)
		{
			printf("fwrite error\n");
			return false;
		}
		if (fflush(f))
		{
			printf("fflush error\n");
			return false;
		}

		if (size == -1)
		{
			std::string len_tag = "Content-Length: ";

			std::string header(buffer, n);

			int sep_pos = header.find("\r\n\r\n");
			if (sep_pos == std::string::npos)
			{
				printf("no header in first packet\n");
				return false;
			}

			int len_pos = header.find(len_tag);
			if (len_pos == std::string::npos)
			{
				printf("no header in first packet\n");
				return false;
			}

			int pos = header.find("\r\n", len_pos);
			if (pos == std::string::npos)
			{
				printf("no header in first packet\n");
				return false;
			}

			std::string len_str = header.substr(len_pos + len_tag.size(), pos - len_pos - len_tag.size());
			size = s2i(len_str) + sep_pos + 4 /* \r\n\r\n */;
		}

		size -= n;
		if (size < 0)
		{
			printf("Content-Length error\n");
			return false;
		}
	}

	fclose(f);
	return true;
}
//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	if (!socket_subsys_init())
	{
		printf("socket_subsys_init error\n");
		return -1;
	}

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		printf("socket error\n");
		return -1;
	}

	SOCKADDR_IN sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(argc == 3 ? atoi(argv[2]) : PORT);
	sa.sin_addr.S_un.S_addr = inet_addr(argc == 3 ? argv[1] : IP);

	#ifndef IS_SERVER

	if (!enable_nonblocking_mode(s, true))
	{
		printf("enable_nonblocking_mode error\n");
		return -1;
	}

	printf("wait for server...");
	connect(s, (SOCKADDR *)&sa, sizeof(sa));
	while (true)
	{
		fd_set s_set_out;
		FD_ZERO(&s_set_out);
		FD_SET(s, &s_set_out);

		fd_set s_set_err;
		FD_ZERO(&s_set_err);
		FD_SET(s, &s_set_err);

		timeval timeout = {0, 250000};

		int select_res = select(s + 1, 0, &s_set_out, &s_set_err, &timeout);
		if (select_res == SOCKET_ERROR)
		{
			printf("select error\n");
			return -1;
		}
		if (!select_res) continue;
		if (FD_ISSET(s, &s_set_err))
		{
			printf("select socket error\n");
			return -1;
		}
		break;
	}
	printf("connected\n");

	char *to_server = 0;
	int to_server_size = 0;
	if (!read_file(TO_SERVER_FILE_NAME, &to_server, &to_server_size))
	{
		printf("%s read_file error\n", TO_SERVER_FILE_NAME);
		return -1;
	}
//////////////////////////////////////////////////////////
	char *data = to_server;
	int size = to_server_size;
	int part_size = 200000;
	while (size)
	{
		int c = size < part_size? size : part_size;
		int send_res = send(s, data, c, 0);
		if (send_res == SOCKET_ERROR)
		{
			if (socket_subsys_error() == WSAEAGAIN || socket_subsys_error() == WSAEWOULDBLOCK || socket_subsys_error() == WSAENOBUFS)
			{
				Sleep(250);
				continue;
			}
			else
			{
				printf("send error\n");
				return -1;
			}
		}
		data += send_res;
		size -= send_res;
	}
//////////////////////////////////////////////////////////
	delete [] to_server;

	if (!receive_file(FROM_SERVER_FILE_NAME, s))
	{
		return -1;
	}

	#else

	if (bind(s, (SOCKADDR *)&sa, sizeof(sa)) == SOCKET_ERROR)
	{
		printf("bind error\n");
		return -1;
	}
	if (listen(s, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("listen error\n");
		return -1;
	}

	printf("wait for client...");
	while (true)
	{
		fd_set s_set_in;
		FD_ZERO(&s_set_in);
		FD_SET(s, &s_set_in);

		fd_set s_set_err;
		FD_ZERO(&s_set_err);
		FD_SET(s, &s_set_err);

		timeval timeout = {0, 250000};

		int select_res = select(s + 1, &s_set_in, 0, &s_set_err, &timeout);
		if (select_res == SOCKET_ERROR)
		{
			printf("select error\n");
			return -1;
		}
		if (!select_res) continue;
		if (FD_ISSET(s, &s_set_err))
		{
			printf("select socket error\n");
			return -1;
		}
		break;
	}
	printf("connected\n");

	SOCKET ns;

	SOCKADDR_IN nsa;
	int sizeof_nsa = sizeof(nsa);

	ns = accept(s, (SOCKADDR *)&nsa, &sizeof_nsa);
	if (ns == INVALID_SOCKET)
	{
		printf("accept error\n");
		return -1;
	}

	if (!enable_nonblocking_mode(ns, true))
	{
		printf("enable_nonblocking_mode error\n");
		return -1;
	}

	if (!receive_file(FROM_CLIENT_FILE_NAME, ns))
	{
		return -1;
	}

	char *to_client = 0;
	int to_client_size = 0;
	if (!read_file(TO_CLIENT_FILE_NAME, &to_client, &to_client_size))
	{
		printf("%s read_file error\n", TO_CLIENT_FILE_NAME);
		return -1;
	}

	if (send(ns, to_client, to_client_size, 0) != to_client_size)
	{
		printf("send error\n");
		return -1;
	}

	delete [] to_client;

	shutdown(ns, SD_BOTH);
	closesocket(ns);

	#endif

	closesocket(s);

	socket_subsys_uninit();

	printf("done\n");

	return 0;
}
