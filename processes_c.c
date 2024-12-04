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
#define STUDENTS_NUM 20
#define MAX_ITERATION 3


int student_list[STUDENTS_NUM];
char buffer[250];

typedef struct{
    int students[STUDENTS_NUM];
    sem_t sem[TA_NUM]; // semaphore for each TA
    int current_TA; // Want to make sure TAs go in order
    int marked_count; // tracks marked students so TAs won't repeat
}Shared_Mem;

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

// semaphore init
void semaphore_init(Shared_Mem *shared_memory) {
    // Initialize semaphores (one for each TA)
    for (int i = 0; i < TA_NUM; i++) {
        if (sem_init(&shared_memory->sem[i], 1, 1) < 0) {  // Shared memory
            perror("sem_init failed");
            exit(1);
        }
    }
    shared_memory->current_TA = 0;
    shared_memory->marked_count = 0;
}

void TA_Marking(int TA_id, Shared_Mem *shared_memory){

    int count_times = 0;
    while(count_times<MAX_ITERATION){

        while(shared_memory->current_TA != TA_id){ // make sure TAs come in order
            usleep(1000); // short delay to avoid busy wait
        }

        if (sem_wait(&shared_memory->sem[TA_id])== 0){
            if (sem_trywait(&shared_memory->sem[(TA_id+1)%TA_NUM])==0){

                int student_id = -1;

                // start the from the first student of the array in the shared memory
                for(int i = 0; i<STUDENTS_NUM;i++){
                    if(shared_memory->students[i] != 0){
                        student_id = shared_memory->students[i];
                        shared_memory->students[i] = 0; // mark as taken
                        shared_memory->marked_count++; 
                        break;
                        // printf("TA %d: student id %d aquired\n", TA_id + 1,student_id);
                        // break;
                    }
                }
                // reset student list of all students are marked 
                if (shared_memory->marked_count>= STUDENTS_NUM){
                    for (int i =0; i<STUDENTS_NUM;i++){
                        shared_memory->students[i] = i+1;
                    }
                    shared_memory->marked_count = 0;
                    printf("Reset student list\n");

                }
                sem_post(&shared_memory->sem[(TA_id + 1) % TA_NUM]); // Release next semaphore
                sem_post(&shared_memory->sem[TA_id]); 

                if(student_id != -1){
                    sleep(rand()% 4+1);

                    //mark
                    int mark = rand() % 10 +1;
                    printf("TA %d has marked student %d with mark %d\n", TA_id, student_id,mark);

                    //write to TA files
                    char filename[20];
                    sprintf(filename, "TA%d.txt", TA_id+1);
                    FILE *student_marks = fopen(filename,"a");
                    if (student_marks){
                        fprintf(student_marks, "TA %d: Student %d: Mark %d \n", TA_id,student_id,mark);
                        fclose(student_marks);
                    }else{
                        printf("Failed to open file\n");
                    }

                    //marking process delay
                    sleep(rand()%4+1);
                    count_times++;
                }

            }else{
                sem_post(&shared_memory->sem[TA_id]); // is the second semaphore not availbale let go of the first one
                usleep(rand() % 500 + 100);
            }
        }
        shared_memory->current_TA = (shared_memory->current_TA + 1) % TA_NUM; // Move to next TA    
    }
}


int main(){

    srand(time(NULL));
    Shared_Mem *shared_memory;

     //create shared memory
    int shem_id = shmget(IPC_PRIVATE, sizeof(Shared_Mem), IPC_CREAT|0666);
    if(shem_id <0){
        perror("shmget faild");
        exit(1);
    }
    //attach the shared memory
    shared_memory = (Shared_Mem *)shmat(shem_id,NULL,0 );
    if (shared_memory == (void*)-1){ // if the function fails it retirns (void*)-1, otherwise void* (a pointer to shared mem region )
        perror("shamt failed");
        exit(1);
    }

    //initialize student array to 0 

    memset(shared_memory->students, 0, sizeof(shared_memory->students));

    //read the txt file into shared memory

    read_data_file(shared_memory);

    //initialize the semaphores
    
    semaphore_init(shared_memory);

    // for the processes
    pid_t pid;
    for(int i =0 ; i<TA_NUM; i++){
        pid = fork();
        if(pid == 0){
            // child process TA
            TA_Marking(i,shared_memory);
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
    shmctl(shem_id, IPC_RMID, NULL);

    return 0;
}

