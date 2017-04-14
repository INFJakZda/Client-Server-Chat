#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <stdlib.h>

#define USERS_NR 9	//stała liczba użytkowników

struct User {		//struktura uzytkownika
	char name[20];	
	char pass[20];
	int status;	//0-niezalogowany 1-zalogowany
	int group[4];	//0-nienalezy 1-nalezy
};

struct msgbuf {		//struktura wiadomości
	long type;
	char text[200];
};
struct message {	//struktura przekierowywanych wiadomosci 
	long type;	//od kogo
	char text[400];	//wiadomość
	int address;	//do kogo <1,9> - pojedynczy użytkownik
	time_t msgtime; //{11, 22, 33} - grupa 1, 2 i 3.
};
//ŁADOWANIE UŻYTKOWNIKÓW Z PLIKU
void load_users(struct User *user) {
	FILE *fp;
	fp = fopen("test_users", "r");
		
	int index;
	index = 1;
	//ŁADOWANIE NAZW I HASEŁ
	while(index <= USERS_NR)
	{
		fscanf(fp, "%s", user[index].name);
		fscanf(fp, "%s", user[index].pass);
		user[index].status = 0;
		user[index].group[1] = 0;
		user[index].group[2] = 0;
		user[index].group[3] = 0;
		index++;
	}
	//ŁADOWANIE PRZYNALEŻNOŚCI DO GRUP
	index = 1;
	int var;
	while(index <= USERS_NR)
	{
		fscanf(fp, "%d", &var);
		if(var > 0) {
			user[index].group[var] = 1;
		}		
		index++;
	}
	fclose(fp);
}
//SPRAWDZENIE CZY TAKI UZYTKOWNIK ISTNIEJE
int check_index(char* name, struct User *user, int id) {
	int it;
	it = 1;
	struct msgbuf msg10;
	msg10.type = 10;
	printf("próba logowania\n");
	//PRZEGLĄD PO WSZYSTKICH NAZWACH I POROWNANIE
	while(strcmp(name, user[it].name)) {
		it++;	
		//GDY NIE ZNALAZŁ TAKIEJ NAZWY PROSI O POPRAWE	
		if(it > USERS_NR) {		
			printf("błędny login\n");
			strcpy(msg10.text, "error");
			msg10.type = 10;
			msgsnd(id, &msg10, sizeof(msg10)-sizeof(long), 0);
			msg10.type = 100;
			msgrcv(id, &msg10, sizeof(msg10)-sizeof(long), 100, 0);
			strcpy(name, msg10.text);
			it = 1;
		}
	}
	//POPRAWNA NAZWA UZYTKOWNIKA
	msg10.type = 10;
	strcpy(msg10.text, "poprawny");
	msgsnd(id, &msg10, sizeof(msg10)-sizeof(long), 0);
	//ZWRACA NUMER UŻYTKOWNIKA
	return it;
}
//OBSŁUGA KLIENTÓW
void comm_service(int id_s, struct User *user) {
	
	struct msgbuf msg;	//odbieranie wiadomości	
	int clause;		//zmienna pomocnicza - odbiór komunikatu
// 1)	//LOGOWANIE
	clause = msgrcv(id_s, &msg, sizeof(msg)-sizeof(long), 100, IPC_NOWAIT);
	if(clause != -1) {
		//SPRAWDZENIE NAZWY		
		int index;
		index = check_index(msg.text, user, id_s);
		printf("uzytkownik %d chce sie zalogowac\n", index);
		
		//bufor pomocniczy do kontaku z userem
		struct msgbuf msg10;	
		msg10.type = 10;
		//warunek stopu
		int var;		
		var = 1;
		//SPRAWDZENIE HASLA
		do {		
			msgrcv(id_s, &msg, sizeof(msg10)-sizeof(long), 100, 0);
			//BŁĘDNE HASŁO
			if(strcmp(user[index].pass, msg.text)) {
				msg10.type = 10;
				strcpy(msg10.text, "error");
				printf("uzytkownik %d błędne hasło\n", index);
				msgsnd(id_s, &msg10, sizeof(msg10)-sizeof(long), 0);
			}
			//POPRAWNE HASŁO
			else {	
				msg10.type = 10;
				strcpy(msg10.text, "poprawny");
				msgsnd(id_s, &msg10, sizeof(msg10)-sizeof(long), 0);
				var = 0;
			}
		} while(var);
		//UZYTKOWNIK ZALOGOWANY
		printf("uzytkownik %d zalogował się\n", index);
		sprintf(msg10.text, "%d", index);
		//WYSLANIE UZYTKOWNIKOWI SWOJEGO INDEKSU
		msg10.type = 10;		
		msgsnd(id_s, &msg10, sizeof(msg10)-sizeof(long), 0);
		user[index].status = 1;
	}
// 2)	//WYLOGOWANIE
	clause = msgrcv(id_s, &msg, sizeof(msg)-sizeof(long), 12, IPC_NOWAIT);
	if(clause != -1) {
		printf("użytkownik %s wylogował się\n", msg.text);
		int pom;		
		sscanf(msg.text, "%d", &pom);
		user[pom].status = 0;
	}
// 3)	//PODGLĄD LISTY UŻYTKOWNIKÓW
	clause = msgrcv(id_s, &msg, sizeof(msg)-sizeof(long), 13, IPC_NOWAIT);
	if(clause != -1) {
		//POBRANIE NUMERU UZYTKOWNIKA		
		int number;
		sscanf(msg.text, "%d", &number);
		printf("podgląd listy %d uzytkownika\n", number);
		//PRZYGOTOWANIE WIADOMOŚCI
		char help[3];
		//wklejam do msg.text po kolei nazwy i status
		strcpy(msg.text, user[1].name);
		strcat(msg.text, " ");
		sprintf(help, "%d", user[1].status);
		strcat(msg.text, help);
		strcat(msg.text, "\n");
		msg.type = 133;
		//SKLEJANIE WYSYŁKI
		int it;	
		for(it = 2; it <= USERS_NR; it++) {
			strcat(msg.text, user[it].name);
			strcat(msg.text, " ");
			sprintf(help, "%d", user[it].status);
			strcat(msg.text, help);
			strcat(msg.text, "\n");
		}
		//wysyłam caly podgląd w jednym stringu
		msgsnd(id_s, &msg, sizeof(msg)-sizeof(long), 0);
	}
// 4)	//ZAPISANIE SIĘ DO GRUPY
	clause = msgrcv(id_s, &msg, sizeof(msg)-sizeof(long), 14, IPC_NOWAIT);
	if(clause != -1) {
		//POBRANIE NR UZYTKOWNIKA
		int number;
		sscanf(msg.text, "%d", &number);
		printf("uzytkownik %d chce sie zapisac do grupy\n", number);	
		char help[6];
		//PRZYGOTOWANIE WIADOMOŚCI
		help[0] = user[number].group[1] + '0';
		help[1] = ' ';
		help[2] = user[number].group[2] + '0';
		help[3] = ' ';
		help[4] = user[number].group[3] + '0';
		help[5] = '\0';
		//WYSŁANIE PRZYNALEŻNOŚCI DO KTÓRYCH GRUP JUŻ NALEŻY
		strcpy(msg.text, help);
		msg.type = 144;
		msgsnd(id_s, &msg, sizeof(msg)-sizeof(long), 0);
		msgrcv(id_s, &msg, sizeof(msg)-sizeof(long), 14, 0);
		//ODEBRANIE NR GRUPY I PRZYPISANIE
		int nr_group;
		sscanf(msg.text, "%d", &nr_group);
		printf("uzytkownik %d zapisał sie do grupy %d\n", number, nr_group);
		user[number].group[nr_group] = 1;		
	}	

// 5)	//WYPISANIE SIĘ Z GRUPY
	clause = msgrcv(id_s, &msg, sizeof(msg)-sizeof(long), 15, IPC_NOWAIT);
	if(clause != -1) {
		//POBRANIE NR UZYTKOWNIKA		
		int number;
		sscanf(msg.text, "%d", &number);
		printf("uzytkownik %d chce sie wypisac z grupy\n", number);
		//PRZYGOTOWANIE WIADOMOŚCI	
		char help[6];
		help[0] = user[number].group[1] + '0';
		help[1] = ' ';
		help[2] = user[number].group[2] + '0';
		help[3] = ' ';
		help[4] = user[number].group[3] + '0';
		help[5] = '\0';
		//WYSŁANIE PRZYNALEŻNOŚCI DO KTÓRYCH GRUP JUŻ NALEŻY
		strcpy(msg.text, help);
		msg.type = 155;
		msgsnd(id_s, &msg, sizeof(msg)-sizeof(long), 0);
		msgrcv(id_s, &msg, sizeof(msg)-sizeof(long), 15, 0);
		//ODEBRANIE NR GRUPY I WYPISANIE
		int nr_group;
		sscanf(msg.text, "%d", &nr_group);
		printf("uzytkownik %d wypisał sie z grupy %d\n", number, nr_group);
		user[number].group[nr_group] = 0;		
	}	
// 6) 	//PODGLĄD DANEJ GRUPY
	clause = msgrcv(id_s, &msg, sizeof(msg)-sizeof(long), 16, IPC_NOWAIT);
	if(clause != -1) {
		//POBRANIE NUMERU GRUPY		
		int gnumber;
		sscanf(msg.text, "%d", &gnumber);
		printf("podgląd %d grupy\n", gnumber);
		//PRZYGOTOWANIE WIADOMOŚCI
		char help[3];
		//wklejam do msg.text po kolei nazwy i status
		strcpy(msg.text, user[1].name);
		strcat(msg.text, "   ");
		sprintf(help, "%d", user[1].group[gnumber]);
		strcat(msg.text, help);
		strcat(msg.text, "\n");
		//SKLEJANIE WYSYŁKI
		int it;	
		for(it = 2; it <= USERS_NR; it++) {
			strcat(msg.text, user[it].name);
			strcat(msg.text, "   ");
			sprintf(help, "%d", user[it].group[gnumber]);
			strcat(msg.text, help);
			strcat(msg.text, "\n");
		}
		//wysyłam caly podgląd w jednym stringu
		msg.type = 166;
		msgsnd(id_s, &msg, sizeof(msg)-sizeof(long), 0);
	}
// 7)	//PRZEKAZANIE WIADOMOŚCI UZYTKOWNIKA
	struct message mssg;
	clause = msgrcv(id_s, &mssg, sizeof(mssg)-sizeof(long)-sizeof(int)-sizeof(time_t), -9, IPC_NOWAIT);
	if(clause != -1) {
		printf("Wiadomosc od %ld do %d\n", mssg.type, mssg.address);
		int pom;
		pom = mssg.type;		
		mssg.type = mssg.address * 111;
		mssg.address = pom;
		mssg.msgtime = time(NULL);		
		msgsnd(id_s, &mssg, sizeof(mssg)-sizeof(long)-sizeof(int)-sizeof(time_t), 0);
	}
// 9)	//PRZEKAZANIE WIADOMOŚCI GRUPY
	clause = msgrcv(id_s, &mssg, sizeof(mssg)-sizeof(long)-sizeof(int)-sizeof(time_t), 19, IPC_NOWAIT);
	if(clause != -1) {
		printf("Wiadomosc do grupy%d\n", mssg.address);
		int it;		
		for(it = 1; it <= USERS_NR; it++) {
			if(user[it].group[mssg.address] == 1) {
				mssg.type = it * 111;
				mssg.msgtime = time(NULL);
				mssg.address = mssg.address * 11;		
				msgsnd(id_s, &mssg, sizeof(mssg)-sizeof(long)-sizeof(int)-sizeof(time_t), 0);
				mssg.address = mssg.address / 11;
			}
		}	
		
	}
	
}

int main(void) {
	printf("-----WITAJ W SERWERZE-----\n");
	printf("aby zakończycz dzialanie wpisz: ctrl+c\n");
	//tablica struktur użytkowników
	struct User test[USERS_NR + 1];	
	//wczytanie nazw i haseł użytkowników
	load_users(test);		
	//zmienna z kolejką serwera
	int ipc_s;		
	ipc_s = msgget(99999, 0777 | IPC_CREAT);	
		
	while(1) {
		comm_service(ipc_s, test);		
	}
	return 0;
}
