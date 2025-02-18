#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "common_file.h"

int main(){
int sem_init, i, id_mqueue;
struct timespec sleep_time, remaining_time;
struct my_msgbuf snd;
struct sigaction sa;
char * args[] = {N_ATOMO, NULL};
int j = 0;
pid_t atom_pid;
sigset_t mask;

if((sem_init  = semget(SEM_KEY, 1, IPC_CREAT | 0600)) == -1){
	TEST_ERROR;
	exit(0);
}
if((id_mqueue = msgget(MEX_KEY, IPC_CREAT | 0600)) == -1){
	TEST_ERROR;
	exit(0);
}


sigfillset(&mask);
sigprocmask(SIG_SETMASK, &mask, NULL);
  
sleep_time.tv_sec = 0;
sleep_time.tv_nsec = STEP;



/*gestisco i zombie*/
bzero (&sa , sizeof(sa)); 
sa.sa_flags = SA_NOCLDWAIT;
sigaction(SIGCHLD, &sa, NULL);

UNLOCK(0);

/*comincio a immettere nuovo combustibile*/
while(1){
    
    while (nanosleep(&sleep_time, &remaining_time) == -1) {
	sleep_time = remaining_time;
    }
    sleep_time.tv_nsec = STEP;
    
    for(i = 0;i<N_NUOVI_ATOMI;i++){
	switch(atom_pid = fork()){
	case -1:
		kill(getppid(), SIGUSR1);
		exit(0);
	case 0:
		//figli atomi
		execve(N_ATOMO, args, NULL);
		exit(0);
	default:
		//prima assegno il valore del return della fork a atom_pid
		//sapendo quel pid creato, gli invio un num atomico casuale
		snd.mtype = atom_pid; //uso il suo stesso pid per il tipo
		snd.num_atomico = 1+ rand() % N_ATOM_MAX;

		if(msgsnd(id_mqueue, &snd, sizeof(snd) -sizeof(long), 0) == -1
		&& (errno==EIDRM || errno == EINVAL )) break;
	}
}

}

return 0;
}
