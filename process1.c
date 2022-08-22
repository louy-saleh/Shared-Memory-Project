#include <stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include <sys/signal.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <semaphore.h>

/*
    Declaring variables
*/
int shm_id_1,shm_id_2,sem;
char* shm1_ptr;
int* shm2_ptr;
sem_t* semaphore;

void connecting_2_shm(){

    //p1 CONNECTING to the MAIN SHARED MEMORY
    shm_id_1=shmget( ftok("/home",'m') , sizeof(char)*100 , IPC_CREAT | S_IRUSR | S_IWUSR);
    if(!(shm_id_1==-1)){
        printf("connecting to the segment.. %d\n",shm_id_1);
        shm1_ptr=shmat(shm_id_1,0,0);
        if(!(shm1_ptr<0)){
            printf("\nP1:Connected Successfully\n");
            /*
                Informing p2 that p1 connected 
                successfully to the main shared memory
            */
            system("killall -SIGUSR1 p2");
        }
    }
}


void s_attached(){
    sleep(10);
    struct shmid_ds buffer_detail;
    shmctl(shm_id_1 ,IPC_STAT, &buffer_detail);
    printf("\nConnected Processes: %d\n", buffer_detail.shm_nattch);
}


void writing_2_shm(){
    
    sem_wait(semaphore);
    /*
        CRITICAL SECTION!!!
    */
    sprintf(shm1_ptr,"Hello from P1:%d...",getpid());
    sem_post(semaphore);
    printf("\nP1: DONE WRITING!!\n");
    /*
        Telling P2 that p1 done writing and 
        unlocking SHM semaphore to start writing!
    */
    sleep(2);
    system("killall -SIGINT p2");

}


void s_detach(){
    /*
        Detaching from SHM
    */
    shmdt(shm1_ptr);

    //Telling p2 that p1 detached from SHM.
    system("killall -SIGTERM p2");
    exit(0);
}

int main(){

    /*
        Creating this semaphore segment
        to synchronize semaphore(wait / post) actions
        in all processes p1 & p2 & p3
    */
    sem = shmget(ftok("/home",'s'), sizeof(sem_t), IPC_CREAT | S_IRUSR | S_IWUSR);
    semaphore= (sem_t *) shmat(sem, 0, 0);

    //Creating small shared memory (shm_id_2) to save in it the pid for this process(p1)
    shm_id_2=shmget( ftok("/home",'a'), sizeof(int)*2 , IPC_CREAT | S_IRUSR | S_IWUSR);
    if(!(shm_id_2==-1)){
        shm2_ptr=shmat(shm_id_2,0,0);
        *shm2_ptr=getpid();
        shmdt(shm2_ptr);
    }


    /*
        handling first signal
            SIGUSR1
        connecting to SHM
    */
    signal(SIGUSR1,connecting_2_shm);
    /*
        handling second signal
            SIGUSR2
        number of connected signals
    */
    signal(SIGUSR2,s_attached);

    /*
        handling third signal
            SIGINT
        writing into SHM 
    */
   signal(SIGINT,writing_2_shm);

    /*
        p1 detaching from SHM
    */
    signal(SIGTERM,s_detach);
    
    while (1);

}