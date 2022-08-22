#include <stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include <sys/signal.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <string.h>
#include <semaphore.h>

/*
    Declaring variables
*/
int shm_id_1,shm_id_2,sem;
char* shm1_ptr;
int* shm2_ptr;
sem_t* semaphore;

int* p_id;


void connecting_2_shm(){

    //p1 CONNECTING to the MAIN SHARED MEMORY
    shm_id_1=shmget( ftok("/home",'m') , sizeof(char)*100 , IPC_CREAT | S_IRUSR | S_IWUSR);
    if(!(shm_id_1==-1)){
        printf("\nconnecting to the segment.. %d\n",shm_id_1);
        shm1_ptr=shmat(shm_id_1,0,0);
        if(!(shm1_ptr<0)){
            printf("\nP3:Connected Successfully\n");
            /*
                Informing p2 that p3 connected 
                successfully to the main shared memory
            */
            system("killall -SIGUSR1 p2");
        }
    }
}

void print_p1_p2_pids(){
    /*
        p3 is connecting to the small SHM for p1 and p2
        to get their IDs and print them here.
    */

    int p1,p2;
    p1 = shmget ( ftok("/home",'a'),sizeof(int)*2,IPC_CREAT | S_IRUSR);
    p2 = shmget ( ftok("/home",'b'),sizeof(int)*2,IPC_CREAT | S_IRUSR);
    p_id = (int*) shmat (p1, 0, 0);
    printf ("\nP1 id is: %d\n", *p_id);
    p_id = (int*) shmat (p2, 0, 0);
    printf ("\nP2 id is:  %d\n", *p_id);
}


void writing_2_shm(){
    
    sem_wait(semaphore);
    /*
            Starting
        CRITICAL SECTION!!!
    */

    /*
        Reading from SHM
        &
        putting content in str to use it.
    */
    char str[100];
    printf("\nreading shm: %s\n",shm1_ptr);
    strcpy(str,shm1_ptr);
    
    /*
        Writing into SHM
        str + P3 sentence
    */
    sprintf(shm1_ptr,"%sHello from p3:%d",str,getpid());
    
    /*
            Ending
        CRITICAL SECTION
    */
    sem_post(semaphore);

    printf("\nP3: DONE WRITING!!\n");


    //printing Final Reading from SHM
    printf("\nFINAL READING: %s\n",shm1_ptr);
    
}


void s_detach(){
    /*
        Detaching from SHM
    */
    shmdt(shm1_ptr);

    //Telling p2 that p3 detached from SHM.
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
    
    
    /*
        handling first signal
            SIGUSR1
        connecting to SHM
    */
    signal(SIGUSR1,connecting_2_shm);


    /*
        handling second signal
            SIGUS2
        getting p1 and p2 signal IDs
    */
    signal(SIGUSR2,print_p1_p2_pids);

    /*
        p2 writing to SHM
    */
    signal(SIGINT,writing_2_shm);

    /*
        p3 detaching from SHM
    */
    signal(SIGTERM,s_detach);

    while(1);
}