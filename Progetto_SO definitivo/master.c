#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <time.h>
#include "common_file.h"


#define N_ALIMENTAZIONE "./alimentazione"
#define N_ATTIVATORE "./attivatore"	
#define N_INIBITORE "./inibitore"	

char toggle = 0;
char end = 0;
pid_t inib = 0;

void int_handler(int signo) {
    // ctrl+c disattiva/attiva l'inibitore una volta che è stata decisa la sua attivazione
    	char str1[] = "\nL'inibitore è stato fermato\n";
    	char str2[] = "\nL'inibitore è stato fatto ripartire\n";

    	if(inib != 0){

    	 if(toggle % 2 == 0){
    	 	write(1,str1,sizeof(str1));
    	 	kill(inib, SIGUSR1);
    	 	toggle = 1;

    	 }else{
    	 	write(1,str2,sizeof(str2));
    	     	kill(inib, SIGCONT); 
    	     	toggle = 0;
    	 }
	}
}

void alarm_handler(int signo) {

    char str1[] = "\nTerminazione per TIMEOUT\n";
    //nel caso di invio segnale sigint il master si interrompe e pulisce tutto
    char str2[] = "\nIl master è stato interrotto\n";
    char str3[] = "\n\\/ Ultime statistiche prima della terminazione /\\ \n";
    char * str;
    int size;
    if(signo == SIGALRM){
	str = str1;
	size = sizeof(str1);
    }else{
    	str = str2;
    	size = sizeof(str2);
    }
    if(end == 0){
        write(2,str,size);
    	write(2,str3,sizeof(str3));
    	end = 1;

    }

}

void usr1_handler(int signo) {
    char str2[] = "\n\\/ Ultime statistiche prima della terminazione /\\ \n";
    char str1[] = "\nTerminazione per MELTDOWN\n";
    if(end == 0){
        write(2,str1,sizeof(str1));
    	write(2,str2,sizeof(str2));

    	end = 2;

    }
}

/*funzione che fa la pulizia di processi e ipc*/
void rm_ipcs(int * id_arr, pid_t alim, pid_t attiv, pid_t inib){
	
	if(alim  != 0) kill(alim , SIGKILL);
	if(attiv != 0) kill(attiv, SIGTERM);
	if(inib != 0) kill(inib, SIGKILL);
	
	shmctl(id_arr[0] , IPC_RMID, NULL);
	msgctl(id_arr[1] , IPC_RMID, NULL);
	msgctl(id_arr[2] , IPC_RMID, NULL);
	msgctl(id_arr[3] , IPC_RMID, NULL);
	semctl(id_arr[4] , 0, IPC_RMID);

	exit(0);
}

int main(){
char scelta;
int i, atom_pid, wait_zero;
char * args[][2] = {
	{N_ATOMO, NULL},
	{N_ALIMENTAZIONE,NULL}, 
	{N_ATTIVATORE, NULL}, 
	{N_INIBITORE, NULL}
};
struct my_msgbuf snd;
struct sigaction sa;
struct timespec sleep_time, remaining_time;
struct shared_stats latest_stats, * my_stats, copy_stats;
sigset_t mask;
float max;
int sem_init, id_mqueue, id_mex,id_scis, id_stats, k = 0;
pid_t alim, attiv;
int id_arr[5] = {0};

srand(getpid());


// Imposta l'handler per SIGCHLD con SA_NOCLDWAIT cosi evito di lasciare zombie  
bzero (&sa , sizeof(sa)); 
sa.sa_flags = SA_NOCLDWAIT;
sigaction(SIGCHLD, &sa, NULL);
bzero (&sa , sizeof(sa)); 
sa.sa_handler = alarm_handler;
sigaction(SIGALRM, &sa, NULL);
bzero (&sa , sizeof(sa)); 
sa.sa_handler = usr1_handler;
sigaction(SIGUSR1, &sa, NULL);

/*interrompere per ctrl+c dealloca gli ipc*/
bzero (&sa , sizeof(sa)); 
sa.sa_handler = alarm_handler;
sigaction(SIGINT, &sa, NULL);



sigfillset(&mask);
sigdelset(&mask,SIGALRM);
sigdelset(&mask,SIGUSR1);
sigdelset(&mask,SIGINT);
sigprocmask(SIG_SETMASK, &mask, NULL);

/*mem condivisa*/
if((id_arr[0] = id_stats = shmget(STATS_KEY, sizeof(*my_stats),IPC_CREAT | 0600)) == -1){
	TEST_ERROR;
	rm_ipcs(id_arr, 0, 0, 0);
}

if((my_stats= shmat(id_stats, NULL, 0)) == NULL){
	TEST_ERROR;
	rm_ipcs(id_arr, 0, 0, 0);
}

/* init stats == 0 */
my_stats->num_attivati = 0;  
my_stats->num_scissi = 0;   
my_stats->energia_prodotta = 0;   
my_stats->energia_consumata = 0;   
my_stats->scorie = 0;    

/*il limite iniziale è il massimo di energia che una scissione può produrre, senza inibitore*/
my_stats->limit = max = ((N_ATOM_MAX/2) * (N_ATOM_MAX/2)) - (N_ATOM_MAX/2);
/*la percentuale di successo della scissione è del 100% senza inibitore*/
my_stats->prob_scis = 100;


if((id_arr[4] = sem_init = semget(SEM_KEY, 5, IPC_CREAT | 0600)) == -1){
	TEST_ERROR;
	rm_ipcs(id_arr, 0, 0, 0);
}
semctl(sem_init, 0, SETVAL, 1);/*wait for zero*/
semctl(sem_init, 1, SETVAL, 1);/*semaforo degli atomi*/
semctl(sem_init, 2, SETVAL, 1);/*semaforo attivatore*/
semctl(sem_init, 3, SETVAL, 1);/*semaforo inibitore*/
semctl(sem_init, 4, SETVAL, 1);/*wait inibitore*/


if((id_arr[2] = id_mqueue = msgget(MEX_KEY, IPC_CREAT | 0600)) == -1){
	TEST_ERROR;
	rm_ipcs(id_arr, 0, 0, 0);
}
if((id_arr[3] = id_mex = msgget(ATOM_KEY, IPC_CREAT | 0600)) == -1){
	TEST_ERROR;
	rm_ipcs(id_arr, 0, 0, 0);
}
if((id_arr[1] = id_scis = msgget(SCIS_KEY, IPC_CREAT | 0600)) == -1){
	TEST_ERROR;
	rm_ipcs(id_arr, 0, 0, 0);
}



/*inibitore*/
do{
	printf("Attivare inibitore? S/N: ");
	scanf("%c", &scelta);
	printf("\n");

}
while(scelta != 'S' && scelta != 'N');

if(scelta == 'S'){
	wait_zero = N_ATOMI_INIT+ 3;
	semctl(sem_init, 0, SETVAL, wait_zero);/*wait for zero*/
	/*creo inibitore*/
	switch(inib = fork()){
	case -1:
		dprintf(2,"MELTDOWN inibitore non creato\n");
		TEST_ERROR;
		rm_ipcs(id_arr, 0, 0, 0);
		break;
	case 0:
		execve(N_INIBITORE, args[3], NULL);
		TEST_ERROR;
		exit(0);
	default:
		/*il padre aspetta che inibitore abbia impostato i limiti di sciurezza*/
		my_op.sem_num = 4;
		my_op.sem_flg = 0;
		my_op.sem_op = 0;
		semop(sem_init, &my_op, 1);
	}
	/*imposto handler per attivare disattivare inibitore*/
	bzero (&sa , sizeof(sa)); 
	sa.sa_handler = int_handler;
	sigaction(SIGINT, &sa, NULL);
}else{
	wait_zero = N_ATOMI_INIT+ 2;
	semctl(sem_init, 0, SETVAL, wait_zero);/*wait for zero*/
}



/*creo figli*/
for(i = 0;i<N_ATOMI_INIT;i++){
	switch(atom_pid = fork()){
	case -1:
		dprintf(2, "MELTDOWN: Atomo non creato in master\n");
		rm_ipcs(id_arr, 0, 0, inib);
		exit(0);
	case 0:
		//figli atomi
		execve(N_ATOMO, args[0], NULL);
		TEST_ERROR;
		exit(0);
	default:
		/*prima assegno il valore del return della fork a atom_pid*/
		/*sapendo quel pid creato, gli invio un num atomico casuale*/
		snd.mtype = atom_pid; /*uso il suo stesso pid per il tipo*/
		snd.num_atomico = 1+ rand() % N_ATOM_MAX;
		msgsnd(id_mqueue, &snd, sizeof(snd) -sizeof(long), 0);
	}
}
/*invio il pid del master stesso in caso di meltdown*/
snd.mtype = 1; /*uso il suo stesso pid per il tipo*/
snd.num_atomico = getpid();
msgsnd(id_mqueue, &snd, sizeof(snd) -sizeof(long), 0);

/*creo processo alimentazione*/
if((alim = fork()) == 0){
	execve(N_ALIMENTAZIONE, args[1], NULL);
	TEST_ERROR;
	exit(0);
}else if(alim == -1){
	dprintf(2,"MELTDOWN alimentazione non creata\n");
	TEST_ERROR;
	rm_ipcs(id_arr, 0, 0, inib);
}

/*creo processo attivatore*/
if((attiv = fork()) == 0){
	execve(N_ATTIVATORE, args[2], NULL);
	TEST_ERROR;
	exit(0);
}else if(attiv == -1){
	dprintf(2,"MELTDOWN attivatore non creato\n");
	TEST_ERROR;
	rm_ipcs(id_arr, alim, 0, inib);
}




/*padre aspetta tutti i figli inizializzati e può iniziare la simulazione*/

my_op.sem_num = 0;
my_op.sem_flg = 0;
my_op.sem_op = -wait_zero;
semop(sem_init, &my_op, 1);
alarm(SIM_DURATION);

/*timer per stampare ogni secondo*/
bzero (&sleep_time, sizeof(sleep_time)); 
bzero (&remaining_time, sizeof(remaining_time)); 
sleep_time.tv_sec = 0;
sleep_time.tv_nsec = 999999999;

printf("\n_________________________________________________________________________\n");
while(1){

    	latest_stats = *my_stats;

	while (nanosleep(&sleep_time, &remaining_time) == -1) {
		sleep_time = remaining_time;
    	}
    	sleep_time.tv_nsec = 999999999;

    	/*chiudo le modifiche per le statistiche in questo modo non possono cambiare durante la stampa ma solo durante la nanosleep*/
   	LOCK(3);
   	LOCK(1);
	LOCK(2);

	my_stats->energia_consumata += ENERGY_DEMAND;
	/*Nessun processo puo modificare le statistiche qui*/
    	copy_stats = *my_stats;


	
	printf("Statistiche totali:\n Numero attivazioni:\t%li\n Numero di Scissioni:\t%li\n Energia prodotta:\t%li\n Energia consumata:\t%li\n Scorie prodotte:\t%li\n\n",
	copy_stats.num_attivati,
	copy_stats.num_scissi,
	copy_stats.energia_prodotta,
	copy_stats.energia_consumata,
	copy_stats.scorie);
	
	printf("Statistiche correnti del secondo %i:\n Numero attivazioni:\t%li\n Numero di Scissioni:\t%li\n Energia prodotta:\t%li\n Energia consumata:\t%li\n Scorie prodotte:\t%li\n",
	k,
	copy_stats.num_attivati - latest_stats.num_attivati,
	copy_stats.num_scissi - latest_stats.num_scissi,
	copy_stats.energia_prodotta - latest_stats.energia_prodotta,
	copy_stats.energia_consumata - latest_stats.energia_consumata,
	copy_stats.scorie - latest_stats.scorie);
	
	k++;
	if(scelta == 'S'){
			printf("\nStatistiche inibitore: (%s)\n Massima energia liberata:\t%li (%.1f%%)\n Probabilità di scissione:\t%i%%\n Probabilità di terminazione:\t%i%%\n Atomi terminati:\t\t%li\n Energia assorbita:\t\t%li\n",
			(toggle) ? "fermo" : "attivo",
			copy_stats.limit, (((float)copy_stats.limit/max) * 100),
			copy_stats.prob_scis,
			100 - copy_stats.prob_scis,
			copy_stats.atomi_terminati,
			copy_stats.energia_assorbita

			);
	}

	printf("\n_________________________________________________________________________\n");
	if(end != 0){
		rm_ipcs(id_arr, alim, attiv, inib);
	}
	//BLACKOUT
	if((long)(my_stats->energia_prodotta - my_stats->energia_consumata) <= 0){
		dprintf(2, "BLACKOUT\n");
		rm_ipcs(id_arr, alim,  attiv, inib);
	}//EXPLODE
	else if((long)(my_stats->energia_prodotta - my_stats->energia_consumata) >= ENERGY_EXPLODE_THERESHOLD ){
		dprintf(2, "EXPLOSION\n");
		rm_ipcs(id_arr, alim, attiv, inib);
	}
	

	UNLOCK(2);
	UNLOCK(1);
	UNLOCK(3);

}

return 0;
}
