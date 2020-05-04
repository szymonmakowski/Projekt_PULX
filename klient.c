#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/md5.h>

#define PORT 8080
#define BUFSIZE 1024
#define NUMER_KLIENTA 8

void przeczysc_bufor();

//szyfrowanie 
//klient --gets()--> --haslo--> str2md5() --hash--> serwer{compare} <--hash-- user_account
char *str2md5(const char *str, int length) {
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char*)malloc(33);

    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }

    return out;
}

int main(int argc, char const *argv[]) {
	int sock = 0, valread;
	struct sockaddr_in serv_addr;
	char *hello = "Hello from client";
	unsigned char buffer[BUFSIZE] = { 0 };
	_Bool wyloguj = 0;
	char c;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed \n");
		return -1;
	} else {
		int position;
		while (1 && !wyloguj) {
			bzero(buffer, BUFSIZE);
			printf("Podaj numer klienta: ");
			fgets(buffer, BUFSIZE, stdin);

			for (int i = 0; i < BUFSIZE; i++)
				if (buffer[i] == '\n') {
					position = i;
					buffer[i] = ':';
					break;
				}

			printf("Podaj haslo: ");
			//SZYFROWANIE
			//char tmp[33];
			//gets(tmp);
			//fseek(stdin,0,SEEK_END);
			//write(STDOUT_FILENO, str2md5(tmp,strlen(tmp)), 32);
			//fseek(stdin,0,SEEK_END);
			fgets(buffer + position + 1, BUFSIZE - position, stdin);

			for (int i = 0; i < BUFSIZE; i++)
				if (buffer[i] == '\n') {
					position = i;
					buffer[i] = ':';
					break;
				}
			write(sock, buffer, BUFSIZE);
			read(sock, buffer, BUFSIZE);
			if (atoi(buffer)) {
				printf("%s\n", buffer + 1);
				while (1) {
					c = getchar();
					if (c == 'q') {
						przeczysc_bufor();
						buffer[0] = 'q';
						write(sock, buffer, BUFSIZE);
						wyloguj = 1;
						break;
					} else if (c == '1') {
						przeczysc_bufor();
						bzero(buffer, BUFSIZE);
						printf("Ile chcesz dodac?\n");
						float liczba;
						if (scanf("%f", &liczba) == 1) {
							przeczysc_bufor();
							sprintf(buffer + 2, "%0.2f", liczba);
							buffer[0] = '1';
							buffer[1] = 'd';

							write(sock, buffer, BUFSIZE);
							bzero(buffer, BUFSIZE);
							read(sock, buffer, BUFSIZE);
							printf("%s", buffer);
						}
					} else if (c == '2') {
						przeczysc_bufor();
						bzero(buffer, BUFSIZE);
						printf("Ile chcesz odjac?\n");
						float liczba;
						if (scanf("%f", &liczba) == 1) {
							przeczysc_bufor();
							sprintf(buffer + 2, "%0.2f", liczba);
							buffer[0] = '2';
							buffer[1] = 'o';

							write(sock, buffer, BUFSIZE);
							bzero(buffer, BUFSIZE);
							read(sock, buffer, BUFSIZE);
							printf("%s", buffer);
						}

					} else if (c == 'p') {
						przeczysc_bufor();
						printf(
								"*****************************************\n"
								"Witaj w elektronicznym systemie bankowym \n"
								"Jesli chcesz zakonczyc polaczenie z serwerem wpisz q,\n"
								"aby wplacic pieniadze na konto wybierz 1,\n"
								"aby wyplacic pieniadze z konta wybierz 2!\n"
								"******************************************\n");
					} else {
						printf("Bledna operacja, wprowadz poprawna!\n");
					}
				}

			} else {
				printf("%s\n", buffer);
			}
			//recv(sock, buffer, BUFSIZE, 0);
		}
	}
	return 0;
}
void przeczysc_bufor() {
	while (getchar() != '\n');
}
