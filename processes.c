#include <stdio.h>
#include <stdlib.h> 
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> 
#include <time.h>
#include <string.h>

#define MAX_STUDENTS 12
#define TA_NUM 2
#define MAX_NUM_ITERATIONS 3

int student_list[MAX_STUDENTS];
char buffer[250];
int count = 0; // track total num of students 
int student_id;


pthread_t TA_threads[TA_NUM]; // threads for each TA
int TA_ids[TA_NUM]; // ids for each TA
sem_t semaphores[TA_NUM]; // array for 5 TAs 


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


// TA to mark a student
void *TA_marking(void *arg){
    int id = *((int*)arg); // get the TA id 

    printf("The TA %d is ready to mark\n", id); 

    int iterations =0;
    int student_index = 0;// to access the student list
    FILE *marked_students; 

    int current_lock = (id-1);
    int next_lock = (id)%TA_NUM;
    
    // mariking the list 3 times
    while(iterations<MAX_NUM_ITERATIONS){

        sem_wait(&semaphores[current_lock]); // lock for current TA - want id o to access semaphore[0] = the first TA
        sem_wait(&semaphores[next_lock]); // lock for the next TA

        student_id = student_list[student_index]; // acess the next student 
        student_index = (student_index+1) % MAX_STUDENTS;// moves to next 

        if (student_id == 0012){
            student_index = 0; // reset and start over the marking
        }

        int wait_time = rand() % 4+1;
        printf("TA %d is accessing database for student %d\n", id, student_id);
        sleep(wait_time);

        //marking the student 
        char filename[10];
        sprintf(filename, "TA%d.txt", id);
        marked_students = fopen(filename,"a");
        if (marked_students == NULL){
           perror("Can't opne file");
           exit(1);
        }

        int mark = rand() % 10 +1;
        printf("TA %d marking student %d with mark %d\n", id, student_id, mark);
        fprintf(marked_students, "Student: %d, Mark: %d\n", student_id, mark);
        fclose(marked_students); 

        // release semaphores
        sem_post(&semaphores[current_lock]);
        sem_post(&semaphores[next_lock]);

        int access_time = rand() % 4+1; // random val 1-4
        sleep(access_time); // simulate acessing the database and marking


        if (student_index ==0){
            iterations++;
        }
    }
}

// the plan - 
int main (){

    read_data_file();  
    // part 1 - a

    // initialize the semaphore
    for (int i = 0; i<TA_NUM;i++){
        sem_init(&semaphores[i],0,1);
    }

 
    // start threads for TA
    for (int i = 0; i< TA_NUM;i++){
        pthread_create(&TA_threads[i],NULL, TA_marking, (void*)&TA_ids[i]);
    }

    //wait for all threads to be done 
    for(int i =0; i<TA_NUM;i++){
        pthread_join(TA_threads[i],NULL);
    }

    //destroy semaphores
    for(int i = 0; i< TA_NUM; i++){
        sem_destroy(&semaphores[i]);
    }
   

    printf("****Completed****\n");
    

    return 0;  
}