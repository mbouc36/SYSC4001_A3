#include <stdio.h>
#include <stdlib.h> 
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> 
#include <time.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SEM_LOCK 0
#define SEM_UNLOCK 1

#define TA_NUM 5
#define STUDENTS_NUM 80 

int student_list[STUDENTS_NUM];
char buffer[250];

int sem_id; 
struct Student *_marked_list;

// read from the data file 
void read_data_file(){
    FILE *student_data;
    int count =0; // to trach total num of students

    student_data = fopen("class_list_of_students.txt", "r");
    if (student_data == NULL){
        printf("Can't Open\n");
        return ;
    }

    // read the lines 
    while(fgets(buffer, sizeof(buffer), student_data) != NULL){ // read and store in the biffer
        // tokenize  
        const char *token = strtok(buffer,", \n"); // get each token from buffer and take out comma and space
        while(token != NULL){
            if ((strlen(token)== 4) && (strspn(token,"0123456789")== 4)){ // make sure only tae 4 digits and each token consist of only numbers 
                student_list[count++] = atoi(token); 
            }
            token = strtok(NULL,", \n"); // move to the next 
        }
    }
    fclose(student_data);
}




struct Student{
    int id;
    int mark;
};

// function fr sempahore to wait (P)
int semaphore_wait(int semid ){
    struct sembuf sem_opr; 

    sem_opr.sem_num = 0; // 0 is assumning this is single semaphore 
    sem_opr.sem_op = -1; // decrement the semphore 
    sem_opr.sem_flg = 0; // blocked for now 

    if (semop(semid, &sem_opr, 1)==-1){
        printf("Error, semop failed\n");
        return 1;
    }
    return 0;
}


// function for sempahore to sgnal (V)
int semaphore_signal(int semid){
    struct sembuf sem_opr; 

    sem_opr.sem_num = 0; //for a single semaphore
    sem_opr.sem_op = 1; //increment 
    sem_opr.sem_flg = 0; // blocked for now 

    if (semop(semid, &sem_opr,1)== -1){
        printf("Error, semop failed\n");
        return 1;
    }
    return 0;

}

//initialize the semaphores 
void semaphore_init(){

    //create semaphore 
    sem_id =  semget(IPC_PRIVATE,TA_NUM,IPC_CREAT|0666);
    if (sem_id == -1){
        printf("semget failed/n");
        exit(1);
    }
    // now initialize the semaphores , // 5 processes cocurrently therefore 5 semaphores 
      
    for (int i =0; i<TA_NUM;i++){ 
        semctl(sem_id, i, SETVAL,1);
    }
}

// create shared memory and attach it 

void create_shared_memory(){
    int shem_id = shmget(IPC_PRIVATE,sizeof(STUDENTS_NUM),IPC_CREAT|0666 );
    if (shem_id == -1){
        printf("shmget failed\n");
        exit(1); 
    }
    // attach shared memory
    _marked_list = (struct Student *)shmat(shem_id, NULL,0);
    if (_marked_list == (void*)-1){
        printf("shmat failed\n");
        exit(1);
    }
    printf("SHared memory created\n");
}

// function to load the students to a shared memory

/*ta marking function : have 5 TAs, access the database, picks up the next number and save it in a local var
and wiats between 1 and 4 secs at random, it then releases the semaphores for other TAs, last number is 9999 and 
should do this 3 times  */ 

// TA marking 

void TA_marking(int TA_id){
    for (int i = TA_id; i<STUDENTS_NUM; i+= TA_NUM){
        semaphore_wait(sem_id); // wait for semaphore
        _marked_list[i].mark = rand() % 11; // random mark between 1 and 10
        printf("TA %d marked student %04d and student mark is %d\n", TA_id,_marked_list[i].id,_marked_list[i].mark);
        semaphore_signal(sem_id); // signal next one 
    }
}



int main(){

    pid_t pid[TA_NUM]; // all pids of 5 TAs
    
    srand(time(NULL));

    //initialize the semaphore 
    semaphore_init();

    //create shared memory for the student list 
    int shm_id = shmget(IPC_PRIVATE,sizeof(struct Student) * STUDENTS_NUM,IPC_CREAT|0666);
    if (shm_id == -1){
        printf("shmget failed\n");
        return 1;
    }
    _marked_list = (struct  Student*)shmat(shm_id,NULL,0);
    if (_marked_list == (void*)-1){
        printf("shmat failed\n");
        return 1;
    }

    // load the students inti shared memory
    read_data_file();
    for(int i = 0; i< STUDENTS_NUM;i++){
        _marked_list[i].id = student_list[i];
    }

    // fork 5 processes 
    for (int i = 0; i<TA_NUM;i++){
        pid[i] = fork();
        if (pid[i]==0){
            TA_marking(i);
            return 1;
        }
    }
    for (int i = 0; i<TA_NUM; i++){
        waitpid(pid[i], NULL,0); 
    }
    printf("All TAs are done!\n");
    return 0;
  
}


