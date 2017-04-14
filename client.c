#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

size_t sizemsg = 400;
int status;	//0-niezalogowany, 1-zalogowany
int indeks;	//0-niezalogowany, {1,2,..,9}-zalogowany
//polska nazwa gdyż index globalny jest niedozwolona
//ze względu na klase string.h, a nie chciałem 
//zmieniać konwencji ;)
struct msgbuf {
	long type;	//10-logowanie {1,2,..,9}-obsługa użytkownika
	char text[200];
};

struct message {	//struktura wysyłanych wiadomosci 
	long type;	//od kogo
	char text[400];	//wiadomość
	int address;	//do kogo <1,9> - pojedynczy użytkownik
	time_t msgtime; //{11, 22, 33} - grupa 1, 2 i 3.
};

//OBSLUGA KLIENTA
void out_service(char* word) {	
	int id_s;	//zmienna z kolejką serwera
	id_s = msgget(99999, 0777 | IPC_CREAT);

// 1)	//LOGOWANIE
	if(!strcmp(word, "login")) {
		if(!status) {
			struct msgbuf msg0;			
			//PODAWANIE LOGINU
			do{
				printf("podaj login:\n");
				scanf("%s", msg0.text);
				msg0.type = 100;
				msgsnd(id_s, &msg0, sizeof(msg0)-sizeof(long), 0);
				msg0.type = 10;
				msgrcv(id_s, &msg0, sizeof(msg0)-sizeof(long), 10, 0);
			} while(strcmp(msg0.text, "poprawny"));
			//PODAWANIE HASŁA
			do{
				printf("podaj hasło:\n");
				scanf("%s", msg0.text);
				msg0.type = 100;
				msgsnd(id_s, &msg0, sizeof(msg0)-sizeof(long), 0);
				msg0.type = 10;
				msgrcv(id_s, &msg0, sizeof(msg0)-sizeof(long), 10, 0);
			} while(strcmp(msg0.text, "poprawny"));
			//ODEBRANIE NR UŻYTKOWNIKA
			msgrcv(id_s, &msg0, sizeof(msg0)-sizeof(long), 10, 0);
			sscanf(msg0.text, "%d", &indeks);
			status = 1;		
			printf("zalogowany pod indeksem %d\n", indeks);
		}
		else {
			printf("jestes juz zalogowany!\n");
		}
	}
// 2)	//WYLOGOWYWANIE
	else if(!strcmp(word, "logout")) {
		//SPRAWDZENIE CZY JEST ZALOGOWANY		
		if(status) {
			struct msgbuf msg2;
			msg2.type = 12;
			sprintf(msg2.text, "%d", indeks);
			msgsnd(id_s, &msg2, sizeof(msg2)-sizeof(long), 0);
			status = 0;
			indeks = 0;
			printf("wylogowano\n");		
		}
		else {
			printf("nie jestes zalogowany!!!\n");
		}
	}
// 3)	//PODGLĄD LISTY ZALOGOWANYCH UŻYTKOWNIKÓW
	else if(!strcmp(word, "check_users")) {
		struct msgbuf msg3;
		msg3.type = 13;
		//WYSŁANIE NUMERU UŻYTKOWNIKA
		sprintf(msg3.text, "%d", indeks);
		msgsnd(id_s, &msg3, sizeof(msg3)-sizeof(long), 0);
		msgrcv(id_s, &msg3, sizeof(msg3)-sizeof(long), 133, 0);
		//ODBIÓR I PRINTOWANIE KOMUNIKATU
		printf("%s", msg3.text);		
	}
// 4)	//ZAPISANIE SIĘ DO GRUPY
	else if(!strcmp(word, "join_group")) {
		if(status){
			struct msgbuf msg4;
			msg4.type = 14;
			strcpy(msg4.text, "");
			sprintf(msg4.text, "%d", indeks);
			//WYSYŁANIE INDEKSU UZYTKOWNIKA
			msgsnd(id_s, &msg4, sizeof(msg4)-sizeof(long), 0);
			msgrcv(id_s, &msg4, sizeof(msg4)-sizeof(long), 144, 0);
			printf("grupy: 1 2 3\n");
			printf("-------%s-------\n", msg4.text);
			printf("0-należysz 1-nienależysz\n");
			printf("podaj do ktorej grupy chesz sie dopisac: ");
			strcpy(msg4.text, "");
			int var;
			scanf("%d", &var);
			while(var < 1 || var > 3) {
				printf("błędny numer, podaj z zakresu {1,2,3}: ");
				scanf("%d", &var);
			}
			sprintf(msg4.text, "%d", var);
			msg4.type = 14;
			//WYSYŁANIE NUMERU GRUPY DO KTÓREJ CHCE DOŁĄCZYĆ
			msgsnd(id_s, &msg4, sizeof(msg4)-sizeof(long), 0);
			printf("dodawanie zakończone\n");
		}
		else {
			printf("Pierw się zaloguj!\n");
		}
	}

// 5)	//WYPISANIE SIĘ Z GRUPY
	else if(!strcmp(word, "gout_group")) {
		if(status){		
			struct msgbuf msg5;
			msg5.type = 15;
			strcpy(msg5.text, "");
			sprintf(msg5.text, "%d", indeks);
			//WYSYŁANIE INDEKSU UZYTKOWNIKA
			msgsnd(id_s, &msg5, sizeof(msg5)-sizeof(long), 0);
			msgrcv(id_s, &msg5, sizeof(msg5)-sizeof(long), 155, 0);
			printf("grupy: 1 2 3\n");
			printf("-------%s-------\n", msg5.text);
			printf("0-nienależysz 1-należysz\n");
			printf("podaj z której grupy chcesz sie wypisac: ");
			strcpy(msg5.text, "");
			int var;
			scanf("%d", &var);
			while(var < 1 || var > 3) {
				printf("błędny numer, podaj z zakresu {1,2,3}: ");
				scanf("%d", &var);
			}
			sprintf(msg5.text, "%d", var);
			msg5.type = 15;
			//WYSYŁANIE NUMERU GRUPY Z KTÓREJ CHCE WYJŚĆ
			msgsnd(id_s, &msg5, sizeof(msg5)-sizeof(long), 0);
			printf("wypisywanie zakończone\n");
		}
		else {
			printf("Pierw się zaloguj!\n");
		}
	}
// 6)	//PODGLĄD DANEJ GRUPY
	else if(!strcmp(word, "check_group")) {
		struct msgbuf msg6;
		msg6.type = 16;
		//POBRANIE NUMERU GRUPY
		printf("podaj numer grupy {1, 2, 3}: \n");
		int var;
		scanf("%d", &var);
		while(var < 1 || var > 3) {
			printf("błędny numer, podaj z zakresu powyższego!: ");
			scanf("%d", &var);
		}
		sprintf(msg6.text, "%d", var);
		//WYSŁANIE NUMERU GRUPY
		msgsnd(id_s, &msg6, sizeof(msg6)-sizeof(long), 0);
		msgrcv(id_s, &msg6, sizeof(msg6)-sizeof(long), 166, 0);
		//ODBIÓR I PRINTOWANIE KOMUNIKATU
		printf("osoba|przynależy\n");
		printf("%s", msg6.text);		
	}
// 7)	//WYSŁANIE WIADOMOŚCI DO UŻYTKOWNIKA
	else if(!strcmp(word, "send_user")) {
		if(status){		
			struct message mssg;
			mssg.type = indeks;
			printf("Do kogo chcesz wysłac wiadomosć?: ");
			scanf("%d", &mssg.address);	
			while(mssg.address > 9 || mssg.address < 1) {
				printf("Wprowadż nr użytkownika z zakresu {1,2,..9}: ");
				scanf("%d", &mssg.address);
			}
			printf("Wprowadź wiadomosć(max 400 znaków):");
			//char b[400];
			fgets(mssg.text, 400, stdin);
			fgets(mssg.text, 400, stdin);			
			mssg.msgtime = time(NULL);
			msgsnd(id_s, &mssg, sizeof(mssg)-sizeof(long)-sizeof(int)-sizeof(time_t), 0);
			printf("Wiadomość wysłana\n");

		}
		else {
			printf("Pierw się zaloguj!\n");
		}
	}
//8)	//ODBIÓR WIADOMOŚCI
	else if(!strcmp(word, "recieve")) {
		if(status){
			struct message mssg;
			int clause;
			clause = msgrcv(id_s, &mssg, sizeof(mssg)-sizeof(long)-sizeof(int)
				-sizeof(time_t), indeks * 111, IPC_NOWAIT);
			if(clause != -1) {
				char *msgdata = ctime(&mssg.msgtime);
				printf("%s", msgdata);
				if(mssg.address < 10) {
					printf("Wiadomość od %d użytkownika\n", mssg.address);
					printf("%s", mssg.text);
				}
				else {
					printf("Wiadomość od %d grupy\n", (mssg.address/11));
					printf("%s", mssg.text);					
				}
			}
			else {
				printf("Nie ma żadnych wiadomości\n");
			}
		}
		else {
			printf("Pierw się zaloguj!\n");
		}
	}
//9)	//WYSŁANIE WIADOMOŚCI DO GRUPY
	else if(!strcmp(word, "send_group")) {
		if(status){		
			struct message mssg;
			mssg.type = 19;
			printf("Do której grupy chcesz wysłac wiadomosć?: ");
			scanf("%d", &mssg.address);
			while(mssg.address < 1 || mssg.address > 3) {
				printf("błędny numer, podaj z zakresu {1,2,3}!: ");
				scanf("%d", &mssg.address);
			}
			printf("Wprowadź wiadomosć(max 400 znaków):");
			fgets(mssg.text, 400, stdin);
			fgets(mssg.text, 400, stdin);			
			mssg.msgtime = time(NULL);
			msgsnd(id_s, &mssg, sizeof(mssg)-sizeof(long)-sizeof(int)-sizeof(time_t), 0);
			printf("Wiadomość wysłana\n");

		}
		else {
			printf("Pierw się zaloguj!\n");
		}
	}
	else {
		printf("zła komenda!\n");
	}


}

int main(void) {
	printf("-----WITAJ W KOMUNIKATORZE-----\n");
	printf("wpisz nastepujące komendy:\n");
	printf("1) login       - aby zalogować sie\n");
	printf("2) logout      - aby wylogować sie\n");
	printf("3) check_users - podgląd listy zalogowanych uzytkownikow\n");
	printf("4) join_group  - zapisanie się do grupy\n");
	printf("5) gout_group  - wypisanie się z grupy\n");
	printf("6) check_group - sprawdzenie kto nalezy do grupy\n");
	printf("7) send_user   - wyslanie wiadomosci do uzytkownika\n");
	printf("8) recieve     - odebranie wiadomości\n");
	printf("9) send_group  - wysłanie wiadomości do grupy\n");
	printf("exit   - aby zakończyć\n");	
	
	char string_in[20];
	scanf("%s", string_in);

	status = 0;	//niezalogowany
	while(strcmp(string_in, "exit")) {
		out_service(string_in);
		scanf("%s", string_in);
	}	
		
}
