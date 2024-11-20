#include <stdio.h>
#include <stdlib.h> 
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> 
#include <time.h>
#include <string.h>

int student_list[80];
char buffer[250];


//sem_t student_list_sem; // semaphore for controlling access to database
pthread_t TA_threads[5]; // threads for each TA
int TA_ids[5] = {1,2,3,4,5}; // ids for each TA
sem_t semaphores[5]; // array for 5 TAs 


// read from the data file 
void read_data_file(){
    FILE *student_data;
    int count =0; // to trach total num of students

    student_data = fopen("class_list_of_students.txt", "r");
    if (student_data == NULL){
        printf("Can't Open\n");
        return 1;
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
void *TA_marking(void *TA_ids){
    int id = *((int*)TA_ids); // get the TA id 

    printf("The TA %d is marking\n", id); 

    int total_marks = 0; 
    int student_index = 0;// to access the student list

    // mariking the list 3 times
    for (int i =0; i<3; i++){
        while(1){

            sem_wait(&semaphores[id-1]); // lock for current TA - want id o to access semaphore[0] = the first TA
            sem_wait(&semaphores[id % 5]); // lock for the next TA

            
            int count = 0; // track total num of students 

            if (student_index >= count || student_list[student_index]>=9999){ // if all students are marked 
                printf("TA %d has finished marking the list\n", id);
                student_index = 0; // reset the count
                read_data_file(); // reload the list of students
            }

            int student_id = student_list[student_index]; // acess the next student 
            student_index++; // move to next 

            printf("TA %d is marking student: %d\n", id,student_id);

            int access_time = rand() % 5; // random val 1-4
            sleep(access_time); // simulate acessing the database and marking

            // mark the student and save it 
            FILE *marked_students = fopen("marked_students.txt", "a");// append the marsk into the file 
            if(marked_students != NULL){
                int mark = rand() % 11; // random mark between 0 - 10
                fprintf("TA: %d Student:%d Mark: %d\n", id, student_id,mark);
                fclose(marked_students);
            }

            sem_post(semaphores[id-1]); //unlock the semaphores
            sem_post(semaphores[id % 5]);

            int marking_time = rand() % 11; // delay marking time 
            sleep(marking_time);

        }
    }
}

// the plan - 
int main (){
    
    // testing purposes 
    for(int i=0; i<index;i++){
        printf("%04d\n", student_list[i]);
    }
    return 0;

    // part 1 - a

    // initialize the semaphores 
    for (int i = 0; i<5;i++){
        sem_init(&semaphores[i], 0, 1); // initialized it and made it available with value 1 
    }

    // start threads for TA
    for (int i = 0; i< 5;i++){
        pthread_create(&TA_threads[i],NULL, TA_marking, (void*)&TA_ids[i]);
    }

    //wait for all threads to be done 
    for(int i =0; i<5;i++){
        pthread_join(TA_threads[i],NULL);
    }

    //destroy semaphores
    sem_destroy(&semaphores);

    return 0;

    
}