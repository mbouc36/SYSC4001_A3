#include <stdio.h>
#include <stdlib.h> 
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> 
#include <time.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define MAX_STUDENTS 20
#define TA_NUM 5
#define MAX_NUM_ITERATIONS 3

int student_list[MAX_STUDENTS];
char buffer[250];
int count = 0; // track total num of students 
int marked_students[MAX_STUDENTS] = {0};
//int student_id;
int student_index = 0;

pthread_mutex_t student_index_mutex;
int TA_ids[TA_NUM]; // ids for each TA
int sem_id;

// read from the data file 
void read_data_file(){
    FILE *student_data;
    
    student_data = fopen("class_list_of_students_copy.txt", "r");
    if (student_data == NULL){
        printf("Can't Open\n");
        exit(1);
    }

    // read the lines 
    while(fgets(buffer, sizeof(buffer), student_data) != NULL){ // read and store in the biffer
        // tokenize 
        //const char *delim = ", \n"; // want to not consider comma and space 
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
}


// TA to mark a student
void TA_marking(int TA_id){
    int iterations = 0; // Initialize iteration counter
    FILE *marked_students_file;
    char filename[30];
    sprintf(filename, "TA%d.txt", TA_id + 1); // Precompute the filename once

    int current_lock = TA_id;
    int next_lock = (TA_id + 1) % TA_NUM;

    printf("The TA %d is ready to mark\n", TA_id);

    while (iterations < MAX_NUM_ITERATIONS) {
        // Always acquire semaphores in increasing order
        int first_lock = (current_lock < next_lock) ? current_lock : next_lock;
        int second_lock = (current_lock < next_lock) ? next_lock : current_lock;

        // Acquire locks on semaphores for the current and next TAs
        semaphore_wait(sem_id, first_lock);
        printf("TA %d locked semaphore %d (first lock)\n", TA_id, first_lock);
        semaphore_wait(sem_id, second_lock);
        printf("TA %d locked semaphore %d (second lock)\n", TA_id, second_lock);

        int local_student_id = -1;

        // Access the shared database (critical section)
        pthread_mutex_lock(&student_index_mutex);

        if (student_index < MAX_STUDENTS && student_list[student_index] != 9999) {
            local_student_id = student_list[student_index];
            marked_students[student_index] = 1; // Mark the student as picked
            student_index++; // Move to the next student
        }

        pthread_mutex_unlock(&student_index_mutex);

        // Release semaphores to allow other TAs to access the database
        semaphore_signal(sem_id, first_lock);
        printf("TA %d unlocked semaphore %d (first lock)\n", TA_id, first_lock);
        semaphore_signal(sem_id, second_lock);
        printf("TA %d unlocked semaphore %d (second lock)\n", TA_id, second_lock);

        if (local_student_id == -1) {
            printf("TA %d reached the end of the student list.\n", TA_id);
            break; // End marking for this TA
        }

        // Mark the student
        marked_students_file = fopen(filename, "a");
        if (marked_students_file == NULL) {
            perror("Error opening file");
            continue; // Skip this iteration and try again
        }

        int mark = rand() % 10 + 1; // Random mark between 1 and 10
        fprintf(marked_students_file, "Student: %d, Mark: %d\n", local_student_id, mark);
        fclose(marked_students_file);

        printf("TA %d marked student %d with mark %d\n", TA_id, local_student_id, mark);

        // Simulate marking delay
        sleep(rand() % 10 + 1);

        // Increment iteration count
        iterations++;
    }
    printf("TA %d completed marking after %d iterations.\n", TA_id, iterations);
}


// the plan - 
int main (){

    semaphore_init();

    read_data_file();  
    
    pthread_mutex_init(&student_index_mutex, NULL);

    //initialized all TA ids
    for (int i = 0; i<TA_NUM;i++){
        //printf("begining\n");
        pid_t pid = fork();
        if(pid == 0){
            // child process TA
            TA_marking(i);
            //printf("TA %d is done\n",i);
            exit(EXIT_SUCCESS); 
        }
    }

    for (int i = 0; i<TA_NUM; i++){
        wait(NULL); 
    }
    semctl(sem_id, 0, IPC_RMID,0);
    pthread_mutex_destroy(&student_index_mutex);

    printf("****Completed****\n");
    return 0;  
}