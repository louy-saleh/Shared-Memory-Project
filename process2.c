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

int s_counter=0;
int s_detach_counter=0;

void connected_successfully(){
    s_counter++;
    if(s_counter==2){
        printf("\np1 and p2 Connected Successfully\n");
        /*
            sending new signal to p1 and p2
            to do some work!
        */
        system("killall -SIGUSR2 p1");
        system("killall -SIGUSR2 p3");
        sleep(15);
        system("killall -SIGINT p1");
       
    }

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
        Copying content into str to use it after.
    */
    char str[100];
    printf("\nreading shm: %s\n",shm1_ptr);
    strcpy(str,shm1_ptr);
    /*
        Writing into SHM
        str + P2 sentence
    */
    sprintf(shm1_ptr,"%sHello from p2:%d...",str,getpid());
    
    /*
            Ending
        CRITICAL SECTION
    */
    sem_post(semaphore);


    printf("\nP2: DONE WRITING!!\n");
    /*
        Telling P3 that p2 done writing and 
        unlocking SHM semaphore to start writing!
    */
    sleep(2);
    system("killall -SIGINT p3");

    /*
        sending signal to DETACH
        process from SHM
    */
    sleep(30);
    system("killall -SIGTERM p3");
    system("killall -SIGTERM p1");
}


void s_detach_destroy_shm(){
    s_detach_counter++;
    if(s_detach_counter==2){
        /*
            p2 detaching from SHM
        */
        shmdt(shm1_ptr);
        /*
            Destroying SHM & semaphore
        */
       sem_destroy(semaphore);
       shmctl(shm_id_1,IPC_RMID,0);
       printf("\nShared memory & Semaphore had been DESTROYED!!\n");
       exit(0);
    }
}


int main(){
    /*
        Creating this semaphore segment
        to synchronize semaphore(wait / post) actions
        in all processes p1 & p2 & p3
    */
    sem = shmget(ftok("/home",'s'), sizeof(sem_t), IPC_CREAT | S_IRUSR | S_IWUSR);
    semaphore= (sem_t *) shmat(sem, 0, 0);
    //Initializing Semaphore
    sem_init(semaphore,1,1); // (our_semaphore, working with processes= (1),intial Value =1)

    //Creating small shared memory (shm_id_2) to save in it the pid for this process(p2)
    shm_id_2=shmget( ftok("/home",'b'), sizeof(int)*2 , IPC_CREAT | S_IRUSR | S_IWUSR);
    if(!(shm_id_2==-1)){
        shm2_ptr=shmat(shm_id_2,0,0);
        *shm2_ptr=getpid();
        shmdt(shm2_ptr);
    }

    //CREATING the MAIN SHARED MEMORY & p2 CONNECTING to it
    shm_id_1=shmget( ftok("/home",'m') , sizeof(char)*100 , IPC_CREAT | S_IRUSR | S_IWUSR);
    if(!(shm_id_1==-1)){
        printf("Segment Created Successfully, connecting to the segment.. %d\n",shm_id_1);
        shm1_ptr=shmat(shm_id_1,0,0);
        if(!(shm1_ptr<0)){
            printf("\nP2:Connected Successfully\n");
            sleep(10);
            /*
                notifying p1 and p3 to
                connect to the MAIN SHARED MEMORY
            */
            system("killall -SIGUSR1 p1");
            system("killall -SIGUSR1 p3");
            /*
                waiting for p1 and p2 to notify p2
                that they are connected to MAIN SHARED MEMORY
            */
            signal(SIGUSR1, connected_successfully);
        }
        
    }
     /*
        p2 writing to SHM
    */
    signal(SIGINT,writing_2_shm);

    signal(SIGTERM,s_detach_destroy_shm);


    while(1);

}