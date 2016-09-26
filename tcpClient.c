// tcp.c

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>

int main() {

	void (*old_handler)(int); //interrupt hanlder
	if ((old_handler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) exit(1); // Error

	int fd, n, nbytes, nleft, nwritten, nread;
	struct sockaddr_in addr;
	struct hostent *hostptr;
	char *ptr, buffer[128];

	fd = socket(AF_INET, SOCK_STREAM, 0); // TCP Socket
	if (fd == -1) perror("Failed initializing socket");

	hostptr = gethostbyname("elementary.ist.utl.pt");

	memset((void*)&addr, (int)'\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ((struct in_addr *)(hostptr -> h_addr_list[0])) -> s_addr;
	addr.sin_port = htons(58000);

	n = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
	if (n == -1) perror("Failed to connect");

	ptr = strcpy(buffer, "Hello!\n");
	nbytes = 7;

	nleft = nbytes;
	while(nleft > 0) {
		nwritten = write(fd, ptr, nleft);
		if (nwritten <= 0) perror("Falha a enviar mensagem");
		nleft -= nwritten;
		ptr += nwritten;
	}
	
	nleft = nbytes;
	ptr = &buffer[0];
	while(nleft > 0) {
		nread = read(fd, ptr, nleft);
		if (nread == -1) perror("Falha a ler mensagem");
		else if (nread == 0) break; // Closed by peer
		nleft -= nread;
		ptr += nread;
	}

	nread = nbytes - nleft;
	close(fd);

	write(1, "echo: ", 6); //stdout
	write(1, buffer, nread);

	exit(0);

}
