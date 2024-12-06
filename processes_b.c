#include <stdio.h>
#include <stdlib.h> 
#include <semaphore.h>
#include <unistd.h> 
#include <time.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define TA_NUM 5
#define STUDENTS_NUM 20
#define Max_Mark_Time 3

int student_list[STUDENTS_NUM];
char buffer[250];

int sem_id, shem_id, limit_access_id;


struct Student{
    int id;
    int mark;
    int mark_count; // track how many times a student being marked 
};
// Define the shared memory structure
struct SharedMemory {
    struct Student students[STUDENTS_NUM];
    int mark_counter;// index of the student being marked 
};
struct SharedMemory *shared_mem;


// read from the data file 
void read_data_file(){
    FILE *student_data;
    int count =0; // to trach total num of students

    student_data = fopen("class_list_of_students_copy.txt", "r");
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
                shared_mem->students[count].id = atoi(token);
                shared_mem->students[count].mark = -1; // Initialize marks to -1
                //shared_mem->students[count].mark_count = 0;
                count++;
            }
            token = strtok(NULL,", \n"); // move to the next 
        }
    }
    fclose(student_data);
}

// function fr sempahore to wait (P)
int semaphore_wait(int semid, int sem_num){
    struct sembuf sem_op = {sem_num, -1,0}; 

    if (semop(semid, &sem_op, 1)==-1){
        printf("Error, semop failed\n");
        exit(1);
    }
    return 0;
}


// function for sempahore to sgnal (V)
int semaphore_signal(int semid, int sem_num){
    struct sembuf sem_op = {sem_num, 1,0}; 

    if (semop(semid, &sem_op,1)== -1){
        printf("Error, semop failed\n");
        exit(0);
    }
    return 0;
}

//initialize the semaphores 
void semaphore_init(){

    //create semaphore 
    sem_id =  semget(IPC_PRIVATE,TA_NUM,IPC_CREAT|0666);
    if (sem_id == -1){
        printf("semget failed\n");
        exit(1);
    }
    // now initialize the semaphores , // 5 processes cocurrently therefore 5 semaphores 
      
    for (int i =0; i<TA_NUM;i++){ 
        semctl(sem_id, i, SETVAL,1);
    }

    
    // Semaphore to limit two TAs marking simultaneously
    limit_access_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (limit_access_id == -1) {
        perror("semget failed for limit_access_id");
        exit(EXIT_FAILURE);
    }

    if (semctl(limit_access_id, 0, SETVAL, 2) == -1) { // Limit to 2 TAs
        perror("semctl failed for limit_access_id");
        exit(EXIT_FAILURE);
    }

}

// create shared memory and attach it 

void create_shared_memory(){
    shem_id = shmget(IPC_PRIVATE,sizeof(struct SharedMemory),IPC_CREAT|0666 );
    if (shem_id == -1){
        perror("shmget failed");
        exit(1); 
    }
    // attach shared memory
    shared_mem = (struct SharedMemory *)shmat(shem_id, NULL, 0);
    if (shared_mem == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    shared_mem->mark_counter = 0; // Initialize mark_counter to 0

}
// TA marking 

void TA_marking(int id){
    int TA_id = id;
    int marking_times = 0;

    while(marking_times < Max_Mark_Time) {
            // Concurrency control
        semaphore_wait(limit_access_id, 0);
     // Lock the current TA's semaphore and the next one
        semaphore_wait(sem_id, TA_id);
        semaphore_wait(sem_id, (TA_id + 1) % TA_NUM);
     // Access shared memory
        int student_index = shared_mem->mark_counter;// get the student to mark

        if (student_index >= STUDENTS_NUM || shared_mem->students[student_index].id == 9999) {
            shared_mem->mark_counter = 0;
            marking_times++;
            printf("resetting and starting over.\n"); 
        }else{
            // select the student to mark
            int student_id = shared_mem->students[student_index].id;
            shared_mem->mark_counter++;
            printf("TA %d is marking student %d \n",(TA_id+1), student_id);
        // Simulate marking process
            sleep(rand()% 4+1); // delay

        //mark the student
            int mark = rand() % 11;
            shared_mem->students[student_index].mark = mark;

        // Write to the TA's file
            char filename[20];
            sprintf(filename, "TA%d.txt", TA_id + 1);
            FILE *file = fopen(filename, "a");
            if (file != NULL) {
                fprintf(file, "Student: %d, Mark: %d\n", student_id, mark);
                fclose(file);
            } else {
                perror("File write error");
            }
        //simulate marking process
            sleep(rand()%10+1);
        }
        semaphore_signal(sem_id, (TA_id + 1) % TA_NUM);
        semaphore_signal(sem_id, TA_id);
        semaphore_signal(limit_access_id, 0);
        //printf("TA %d released Semaphore %d and Semaphore %d\n", TA_id + 1, (TA_id + 1) % TA_NUM, TA_id);
    }
}



int main(){

    //pid_t pid; // all pids of 5 TAs
    
    srand(time(NULL));

    semaphore_init();
    create_shared_memory();
    read_data_file();


    // fork 5 processes 
    //printf("Testing\n");
    for (int i = 0; i<TA_NUM;i++){
        //printf("begining\n");
        pid_t pid = fork();
        if(pid == 0){
            // child process TA
            TA_marking(i);
            exit(EXIT_SUCCESS); 
        }

    }

    for (int i = 0; i<TA_NUM; i++){
        wait(NULL); 
    }
    semctl(sem_id, 0, IPC_RMID,0);
    semctl(limit_access_id, 0, IPC_RMID,0);
    shmctl(shem_id,IPC_RMID,NULL);


    printf("All TAs are done!\n");

    return 0; 
    
}
