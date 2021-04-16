#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/un.h>

//server from classmate

int main(int ac, char** av, char** en) {
	int i, s, ns, r;
	fd_set rs;
	char buff[64], msg[] = "server ready\n";
	struct sockaddr_un ad;

	memset(&ad, 0, sizeof(ad));
	ad.sun_family = AF_LOCAL;
	strcpy(ad.sun_path, "./sck");
	s = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (s == -1)
	{
		perror("socket: ");
		exit(2);
	}

	unlink("./sck");
	bind(s, (struct sockaddr*)&ad, sizeof(ad));
	listen(s, 5);

	// obsluzime len jedneho klienta
	ns = accept(s, NULL, NULL);

	// toto je nieco ako uvitaci banner servera
	// bude to fungovat vdaka tomu, ze klient je schopny citatat aj zo servera, 
	// aj z terminalu sucasne (v lubovolnom poradi)
////	strcpy(buff, "Hello from server\nSend a string and I'll send you back the upper case...\n");
////	write(ns, buff, strlen(buff)+1);

	while ((r = read(ns, buff, 64)) > 0)	// blokuju citanie, cakanie na poziadavku od klienta
	{
		buff[r] = 0;			// za poslednym prijatym znakom
		printf("received: %d bytes, string: %s\n", r, buff);
		for (i = 0; i < r; i++) buff[i] = toupper(buff[i]);
		printf("sending back: %s\n", buff);
		write(ns, buff, r);		// zaslanie odpovede
	}
	perror("read");	// ak klient skonci (uzavrie soket), nemusi ist o chybu

	close(ns);
	close(s);
	return 0;
}
