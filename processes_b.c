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
#define STUDENTS_NUM 80 
#define Max_Mark_Time 3

int student_list[STUDENTS_NUM];
char buffer[250];

int sem_id, shem_id; 
struct Student * students;
int *mark_counter; // to keep 

struct Student{
    int id;
    int mark;
};


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



// function fr sempahore to wait (P)
int semaphore_wait(int semid, int sem_num){
    struct sembuf sem_opr; 

    sem_opr.sem_num = sem_num; 
    sem_opr.sem_op = -1; // decrement the semphore 
    sem_opr.sem_flg = 0; // blocked for now 

    if (semop(semid, &sem_opr, 1)==-1){
        printf("Error, semop failed\n");
        return 1;
    }
    return 0;
}


// function for sempahore to sgnal (V)
int semaphore_signal(int semid, int sem_num){
    struct sembuf sem_opr; 

    sem_opr.sem_num = sem_num; 
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
    shem_id = shmget(IPC_PRIVATE,sizeof(STUDENTS_NUM),IPC_CREAT|0666 );
    if (shem_id == -1){
        printf("shmget failed\n");
        exit(1); 
    }
    // attach shared memory
    students = (struct Student *)shmat(shem_id, NULL,0);
    if (students == (void*)-1){
        printf("shmat failed\n");
        exit(1);
    }

    mark_counter = (int*)(students+STUDENTS_NUM);
    *mark_counter = 0;

}

// function to load the students to a shared memory

/*ta marking function : have 5 TAs, access the database, picks up the next number and save it in a local var
and wiats between 1 and 4 secs at random, it then releases the semaphores for other TAs, last number is 9999 and 
should do this 3 times  */ 

// TA marking 

void TA_marking(int TA_id){

    int marking_times = 0;
    int student_id = -1;
   

    while (marking_times<Max_Mark_Time){

        semaphore_wait(sem_id, TA_id);
        semaphore_wait(sem_id, (TA_id+1)%TA_NUM);
       

        int student_index = *mark_counter;
        if(student_index <STUDENTS_NUM){
            student_id = students[student_index].id;
            printf("TA %d is accessing student %d\n", TA_id+1, student_id);

            int wait_time = rand() % 4+1;
            sleep(wait_time);

            semaphore_signal(sem_id, TA_id);
            semaphore_signal(sem_id, (TA_id+1)%TA_NUM);

            printf("TA %d is marking student %d\n", TA_id+1, student_id);
            int mark = rand() % 11;
            students[student_index].mark = mark;

            printf("TA %d has marked student %d with mark %d\n", TA_id+1, student_id, mark);

            char marke_temp[10];
            sprintf(marke_temp, "TA%d.txt", TA_id+1);
            FILE *marked_file = fopen(marke_temp, "a"); 
            if (marked_file != NULL){
                fprintf(marked_file, "Student: %d, Mark: %d\n", student_id,mark);
                fclose(marked_file);
            }else{
                perror("Can't open file");
            }
            (*mark_counter)++;

            if(students[student_index].id == 9999){
                *mark_counter = 0;
            }


        }else{
            break;
        }

        marking_times++;   
    }
    //return NULL;
}



int main(){

    pid_t pid[TA_NUM]; // all pids of 5 TAs
    
    srand(time(NULL));

    
    semaphore_init();

      //create shared memory for the student list 

    create_shared_memory();

       // load the students inti shared memory
   
    read_data_file();
    for(int i = 0; i< STUDENTS_NUM;i++){
        students[i].id = student_list[i];
    }
    // fork 5 processes 
    
    for (int i = 0; i<TA_NUM;i++){
        pid[i] = fork();
        if (pid[i]==0){
            TA_marking(i);
            exit(0);
        }
    }
    for (int i = 0; i<TA_NUM; i++){
        waitpid(pid[i], NULL,0); 
    }
    printf("All TAs are done!\n");
    

    semctl(sem_id, 0, IPC_RMID,0);
    shmctl(shem_id,IPC_RMID,NULL);

    return 0; 
    
}


