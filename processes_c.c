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

#define TA_NUM 5
#define STUDENTS_NUM 80


int student_list[STUDENTS_NUM];
char buffer[250];

typedef struct{
    int students[STUDENTS_NUM];
}Shared_Mem;

void semaphore_init(sem_t *sem){ // wrapper function to initialize all 5 processes 
    for(int i=0; i< TA_NUM; i++){

        if(sem_init(&sem[i], 0, 1)<0){ // sem_int part of POSIX sempahore API that initializes a semaphore 
            perror("semaphore_init falied");
            exit(1);
        }
    }
}

// read from the data file 
void read_data_file(Shared_Mem *shared_memory){
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
                shared_memory->students[count++] = atoi(token);
            }
            token = strtok(NULL,", \n"); // move to the next 
        }
    }
    fclose(student_data);
}

void TA_Marking(int TA_id, Shared_Mem *shared_memory, sem_t * sem){

    int count_times = 0;
    int student_id = -1;


    while(count_times<3){
        if (sem_wait(&sem[TA_id])== 0){

            if (sem_trywait(&sem[(TA_id+1)%TA_NUM])==0){

            // start the from the first student of the array in the shared memory
                for(int i = 0; i<STUDENTS_NUM;i++){
                    if(shared_memory->students[i] == 0000){
                        student_id = i;
                        printf("TA %d: student id %d aquired\n", TA_id,student_id);
                        break;
                    }
                }

                if (student_id == -1){
                    break; 
                }  

                sleep(rand() % 4+1);

            // mark the students
                int mark = rand() % 10 +1;
                printf("TA %d has marked student %d\n", TA_id, student_id);
                FILE *student_mark = fopen("student_marks.txt","a");
                if(student_list != NULL){
                    int mark = rand() % 11; 
                    fprintf(student_mark," TA %d: Student %d : Mark %d\n", TA_id,student_id,mark);
                    fclose(student_mark);
                }

                sleep(rand()%4+1);

                sem_post(&sem[(TA_id+1)%TA_NUM]);
                sem_post(&sem[TA_id]);

                count_times++;


            }else{
                sem_post(&sem[TA_id]); // is the second semaphore not availbale let go of the first one
            }
        }
        sleep(rand()%4+1);
          
    }
}

int main(){

    srand(time(NULL));
    Shared_Mem *shared_memory;
    sem_t sem[TA_NUM]; // array of sempahores
   

     //create shared memory
    int shm_id = shmget(IPC_PRIVATE, sizeof(Shared_Mem), IPC_CREAT|0666);
    if(shm_id <0){
        perror("shmget faild");
        exit(1);
    }
    //attach the shared memory
    shared_memory = (Shared_Mem *)shmat(shm_id,NULL,0 );
    if (shared_memory == (void*)-1){ // if the function fails it retirns (void*)-1, otherwise void* (a pointer to shared mem region )
        perror("shamt failed");
        exit(1);
    }

    //initialize student array to 0 

    for(int i = 0; i<STUDENTS_NUM;i++){
        shared_memory->students[i] =0; 
    }

    //read the txt file into shared memory

    read_data_file(shared_memory);

    //initialize the semaphores
    semaphore_init(sem);

    // for the processes
    pid_t pid;
    for(int i =0 ; i<TA_NUM; i++){
        pid = fork();
        if(pid == 0){
            // child process TA
            TA_Marking(i,shared_memory,sem);
            exit(0);
        } else if (pid <0){
            perror ("fork failed");
            exit(1);
        }
    }

    // wait for all TAs to be done
    for(int i = 0; i< TA_NUM; i++){
        wait(NULL);
    }

    //clean shared memory
    shmdt(shared_memory);
    shmctl(shm_id, IPC_RMID, NULL);

    // destroy sempahores
    for(int i =0 ; i< TA_NUM; i++){
        sem_destroy(&sem[i]);
    }
    return 0;
}

