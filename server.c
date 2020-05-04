#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFSIZE 1024
#define NUMER_KLIENTA 8				//ilosc cyfr numeru klienta

int check_user(char *, double *,struct sockaddr_in);
void wyczysc_zwolnij(FILE *, char *, char *, char *, char *);
int ilosc_znakow(char *, int, int);
void zapisz_log(char *);
void po_zalogowaniu(char *, double *, int);
_Bool modyfikuj_srodki(int, double);
int operacja_srodki(int, double *, _Bool, double);


char filename[] = "/home/szymon/eclipse-workspace/pulx/src/user_account";

int main(int argc, char const *argv[])
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFSIZE] = {0};
    char *hello = "Hello from server";
    int numer_klienta=0;
    double srodki_klienta=0;
	pid_t childpid;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
   // address.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    address.sin_port = htons( PORT );


    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
                                 sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

	if (listen(server_fd, 10) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}


    while(1)
    {
    	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
    	{
    	  	perror("accept");
    	    exit(EXIT_FAILURE);
    	}
		printf("Connection acceptedU\n");

    	if((childpid = fork()) == 0){
			close(server_fd);
			while(1)
			{
				bzero(buffer, BUFSIZE);
				read( new_socket , buffer, BUFSIZE);
				numer_klienta=check_user(buffer, &srodki_klienta, address);
				bzero(buffer, BUFSIZE);
				if(numer_klienta)
				{
					po_zalogowaniu(buffer, &srodki_klienta, 0);
					write( new_socket , buffer, BUFSIZE);
					while(1)
					{
						bzero(buffer, BUFSIZE);
						read(new_socket , buffer, BUFSIZE);
						if(atoi(buffer)==1)
						{
							if(operacja_srodki(numer_klienta, &srodki_klienta, 1, atof(buffer+2)))
							{
								bzero(buffer, BUFSIZE);
								po_zalogowaniu(buffer, &srodki_klienta,
									sprintf(buffer, "%s", "!!!!!Poprawnie dodano srodki do konta!!!!!\n"));
							}
							else
								po_zalogowaniu(buffer, &srodki_klienta,
										sprintf(buffer, "%s", "!!!!!Blad! Nie udalo sie dodac srodkow!!!!!\n"));
							write( new_socket , buffer, BUFSIZE);
						}
						else if(atoi(buffer)==2)
						{
							if(operacja_srodki(numer_klienta, &srodki_klienta, 0, atof(buffer+2)))
							{
								bzero(buffer, BUFSIZE);
								po_zalogowaniu(buffer, &srodki_klienta,
									sprintf(buffer, "%s", "*****  Poprawnie odjeto srodki do konta *****\n"));
							}
							else
								po_zalogowaniu(buffer, &srodki_klienta,
										sprintf(buffer, "%s", "**** Blad! Nie udalo sie odjac srodkow *****\n"));
							write( new_socket , buffer, BUFSIZE);
						}
						else if(*buffer=='q')
						{
							return 0;
						}
					}

				}
				else
				{
					strcpy(buffer, "Bledne dane!");
					write( new_socket , buffer, BUFSIZE);
				}
			}
    	}
    }

    return 0;
}
void po_zalogowaniu(char * buffer, double * srodki, int j)
{
	if(j)
		j += sprintf(buffer+j, "%s\n", "\n\n\n***********************************************\n");
	else
		j += sprintf(buffer+j, "1%s\n", "\n\n\n***********************************************\n");
	j += sprintf(buffer+j, "Witaj w systemie bankowosci elektronicznej\tSrodki: %0.2f PLN\n\n",*srodki);
	j += sprintf(buffer+j, "1-wplac\t\t\t2-wyplac\n");
	j += sprintf(buffer+j, "Wpisz (q) by sie wylogowac\n");
	j += sprintf(buffer+j, "Wpisz pomoc(p),\n");
	j += sprintf(buffer+j, "%s\n", "\n***********************************************\n");
}
int check_user(char * buffer, double * srodki, struct sockaddr_in address)			//weryfikacja konta uzytkownika
{
	_Bool ret;


	FILE *fp = fopen(filename, "r");

	if(fp == NULL)
	{
		fprintf(stderr, "Cannot open file: %s\n", filename);
	    return 0;
	}
	size_t buffer_size = 80;		// rozmiar jednej linii pliku
	char buf[55];
	char * bufor = malloc(buffer_size * sizeof(char));
	char * liczba = malloc((NUMER_KLIENTA+1) * sizeof(char));
	char * liczba_od_klienta = malloc((NUMER_KLIENTA+1) * sizeof(char));
	char * pieniadze = malloc(9 * sizeof(char));
	bzero(liczba_od_klienta, NUMER_KLIENTA+1);
	bzero(liczba, NUMER_KLIENTA+1);
	bzero(pieniadze, 9);
	int numer;
	int numer_od_klienta;
	int position=0;
	char ipAddress[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(address.sin_addr), ipAddress, INET_ADDRSTRLEN);


	for(int i=0 ; *(buffer+i) != ':' ; i++)
		position++;

	if(position > NUMER_KLIENTA)
	{
		sprintf(buf, "%s, IP: %s", "Nieudana proba zalogowania", ipAddress);
		zapisz_log(buf);
		return 0;
	}

	while(getline(&bufor, &buffer_size, fp) != -1)
	{
		for(int i=0 ; *(bufor+i)!= ':' || i < NUMER_KLIENTA ; i++)
		{
			liczba[i]=*(bufor+i);
			liczba_od_klienta[i]=*(buffer+i);
		}
		numer = atoi(liczba);
		numer_od_klienta = atoi(liczba_od_klienta);
		if(numer == numer_od_klienta)
		{
			if(ilosc_znakow(buffer, BUFSIZE, 1) == ilosc_znakow(bufor, buffer_size, 1))
			{
				for(int i=1 ; i < ilosc_znakow(buffer, BUFSIZE, 1)+1 ; i++)
				{
					if(buffer[ilosc_znakow(buffer, BUFSIZE, 0)+i]
							  == bufor[ilosc_znakow(buffer, BUFSIZE, 0)+i])
						continue;
					else
					{
						sprintf(buf, "%s, IP: %s", "Nieudana proba zalogowania", ipAddress);
						zapisz_log(buf);
						return 0;
					}
				}

				for(int i=1 ; bufor[i+ilosc_znakow(buffer, BUFSIZE, 2)] != ':' ; i++)
				{
					pieniadze[i-1]=bufor[i+ilosc_znakow(buffer, BUFSIZE, 2)];
				}
				*srodki = atof(pieniadze);

				ilosc_znakow(bufor, buffer_size, 1);

				fflush(stdout);
				wyczysc_zwolnij(fp, bufor, liczba, liczba_od_klienta, pieniadze);
				sprintf(buf, "%s, id: %d.IP: %s", "Poprawne zalogowanie", numer_od_klienta, ipAddress);
				zapisz_log(buf);
				return numer_od_klienta;
			}
		}
	}

	fflush(stdout);
	wyczysc_zwolnij(fp, bufor, liczba, liczba_od_klienta, pieniadze);
	sprintf(buf, "%s, IP: %s", "Nieudana proba zalogowania", ipAddress);
	zapisz_log(buf);
	return 0;

}
void wyczysc_zwolnij(FILE * fp, char * bufor, char * liczba, char * liczba_od_klienta, char * kasa)
{
	fclose(fp);
	free(bufor);
	free(liczba_od_klienta);
	free(liczba);
	free(kasa);
}
void zapisz_log(char * zdarzenie)
{
	FILE *fp;
	time_t timer;
	char buffer[26];
	struct tm* tm_info;

	if ((fp=fopen("/home/szymon/eclipse-workspace/pulx/src/logs.log", "at"))==NULL)
	    fp = fopen("/home/szymon/eclipse-workspace/pulx/src/logs.log", "wt");

	time(&timer);
	tm_info = localtime(&timer);

	strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

	fprintf (fp, "%s %s\n", buffer, zdarzenie);
	fclose (fp);

}
int ilosc_znakow(char * bufor, int rozmiar, int poz_czy_il)
{
	int position1=0;
	int position2=0;
	int position3=0;
	for(int i=0 ; i<rozmiar ; i++)
	{
		if(bufor[i] == ':')
		{
			if(position1==0)
				position1 = i;
			else if(position2==0)
				position2 = i;
			else
			{
				position3 = i;
				break;
			}
		}
	}
	if(poz_czy_il==1)
		return (position2-position1-1);
	else if(poz_czy_il == 0)
		return position1;
	else if(poz_czy_il == 2)
		return position2;
	else
		return (position3-position2);
}
int operacja_srodki(int numer_klienta, double *srodki, _Bool operacja, double dodatkowe)
{
	if(operacja)	// dodanie srodkow
	{
		*srodki = *srodki + dodatkowe;
		char buf[45];

		sprintf(buf, "%s%.2f. ID: %d", "Dodanie srodkow +", dodatkowe, numer_klienta);
		zapisz_log(buf);
	}
	else   //odjecie srodkow
	{
		if((*srodki - dodatkowe)<0)
			return 0;
		*srodki = *srodki - dodatkowe;
		char buf[45];

		sprintf(buf, "%s%.2f. ID: %d", "Odjecie srodkow -", dodatkowe, numer_klienta);
		zapisz_log(buf);
	}
	return modyfikuj_srodki(numer_klienta, *srodki);
}
_Bool modyfikuj_srodki(int numer_klienta, double srodki)
{
	FILE *fp = fopen(filename, "r+");
	FILE* nf = fopen("u_a", "w");

	char * how_much;

	how_much = malloc(sizeof(double)+1);
	bzero(how_much, sizeof(double)+1);
	sprintf(how_much, "%0.2f",srodki);

	if(fp == NULL)
	{
		fprintf(stderr, "Cannot open file: %s\n", filename);
	}


	size_t buffer_size = 80;		// rozmiar jednej linii pliku
	char * bufor = malloc(buffer_size * sizeof(char));
	char * liczba = malloc((NUMER_KLIENTA+1) * sizeof(char));
	int numer;
	int returning=0;

	while(getline(&bufor, &buffer_size, fp) != -1)
	{
		for(int i=0 ; *(bufor+i)!= ':' || i < NUMER_KLIENTA ; i++)
		{
			liczba[i]=*(bufor+i);
		}
		numer = atoi(liczba);
		if(numer == numer_klienta)
		{
			char * buforu = malloc(buffer_size * sizeof(char));
			strncpy (buforu, bufor, ilosc_znakow(bufor, buffer_size, 2));
			fprintf (nf, "%s:%s:\n", buforu,how_much);
			free(buforu);
			returning =1;
		}
		else
			fprintf (nf, "%s", bufor);
	}
	fflush(stdout);
	fclose(fp);
	fclose(nf);
	remove(filename);
	rename("u_a", filename);
	free(bufor);
	free(how_much);
	free(liczba);
	return returning;
}
