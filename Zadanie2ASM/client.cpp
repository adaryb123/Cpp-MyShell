#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/un.h>

//client from classmate

int main(int ac, char** av, char** en) {
	int s, r;
	fd_set rs;	// deskriptory pre select()
	char msg[64] = "hello world!";
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

	printf("running as client\n");
	connect(s, (struct sockaddr*)&ad, sizeof(ad));	// pripojenie na server
	FD_ZERO(&rs);
	FD_SET(0, &rs);
	FD_SET(s, &rs);
	// toto umoznuje klientovi cakat na vstup z terminalu (stdin) alebo zo soketu
	// co je prave pripravene, to sa obsluzi (nezalezi na poradi v akom to pride)
	while (select(s + 1, &rs, NULL, NULL, NULL) > 0)
	{
		if (FD_ISSET(0, &rs))		// je to deskriptor 0 = stdin?
		{
			r = read(0, msg, 64);	// precitaj zo stdin (terminal)
//if (msg[r-1]=='\n') msg[r-1]=0;
			write(s, msg, r);	// posli serveru (cez soket s)
		}
		if (FD_ISSET(s, &rs))		// je to deskriptor s - soket spojenia na server?
		{
			r = read(s, msg, 1);	// precitaj zo soketu (od servera)
			write(1, msg, r);	// zapis na deskriptor 1 = stdout (terminal)
		}
		FD_ZERO(&rs);	// connect() mnoziny meni, takze ich treba znova nastavit
		FD_SET(0, &rs);
		FD_SET(s, &rs);
	}
	perror("select");	// ak server skonci, nemusi ist o chybu
	close(s);
	return 0;
}
