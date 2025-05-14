#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT "3490"
#define DEST "127.0.0.1"
#define BUFFERSIZE 100

// getaddrinfo() -> socket() -> connect() -> send(), recv() -> close()

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{ // IPv4
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	// IPv6
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int recv_msg(int fd, char *buffer, int buffersize)
{
	memset(buffer, 0, buffersize);
	int numbytes;
	if ((numbytes = recv(fd, buffer, buffersize - 1, 0)) == -1)
		return -1;
	buffer[numbytes] = '\0';
	printf("%s", buffer);

	return numbytes;
}

int send_input(int fd, char *buffer, int buffersize)
{
	memset(buffer, 0, buffersize);
	if (fgets(buffer, buffersize, stdin) == NULL)
		return -1;
	int len = strlen(buffer);
	if (send(fd, buffer, len, 0) == -1)
		return -1;

	return len;
}

int main()
{
	struct addrinfo hints, *res;
	int status;
	int fd;
	char buffer[BUFFERSIZE];
	char s[INET6_ADDRSTRLEN];
	int numbytes = 0;

	memset(&hints, 0, sizeof(hints)); // clear the structure
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	status = getaddrinfo(DEST, PORT, &hints, &res);

	if (status != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}
	struct addrinfo *p = res; // pointer to the linked lists of results

	for (; p != NULL; p = p->ai_next)
	{
		if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("Client Socket");
			continue;
		}
		if (connect(fd, p->ai_addr, p->ai_addrlen) == -1)
		{
			perror("Client Connect");
			close(fd);
			continue;
		}
		break;
	}

	if (p == NULL) // check p reached the end of LL of results unable to find an address
	{
		fprintf(stderr, "Client: failed to connect\n");
		return 2;
	}

	// Convert IP address from binary form to human-readable form

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));
	freeaddrinfo(res); // free memory for pointer to linked list of results
	printf("Connected to %s:%s \n", s, PORT);

	if (recv_msg(fd, buffer, BUFFERSIZE) == -1)
	{
		close(fd);
		return 1;
	}

	buffer[numbytes] = '\0';
	printf("%s", buffer); // Print welcome message
	memset(buffer, 0, BUFFERSIZE);

	while (1)
	{
		// Send user input
		if (send_input(fd, buffer, BUFFERSIZE) == -1)
			break;

		if (strcmp("quit", buffer) == 0 || strcmp("exit", buffer) == 0 ||
			strcmp("No", buffer) == 0 || strcmp("NO", buffer) == 0 ||
			strcmp("no", buffer) == 0 || strcmp("n", buffer) == 0 || strcmp("N", buffer) == 0)
		{
			// Receive goodbye message
			recv_msg(fd, buffer, BUFFERSIZE);
			break;
		}

		// Receive server response
		if (recv_msg(fd, buffer, BUFFERSIZE) == -1)
		{
			break;
		}
	}

	close(fd);
	return 0;
}
