#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


//semaphore operations
void wait(int semid){ //semid is id for synchronization
    struct sembuf sb = {0, -1, 0}; //specefies operation to perform in this case wait operation, -1 specefies decrmenet , wait for incrmenet (singal)
    semop(semid, &sb, 1);  //syscall used to perform operations on semaphores, synchrnoization id, pointer to operation and number of operations
} 

void signal(int semid){ //similar to wait
    struct sembuf sb = {0, 1, 0};
    semop(semid, &sb, 1);
}

int get_semaphore_value(int semid){
    int sem_value = semctl(semid, 0, GETVAL);
    if (sem_value == -1) { // check for error
        perror("semctl GETVAL");
        exit(1);
    }
    return sem_value;
}


int main(void){

    srand(time(NULL)); // ensures different sequence of random numbers is produced

    //semaphores
    int sem_1 = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    int sem_2 = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    int sem_3 = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    int sem_4 = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    int sem_5 = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    int sem_parent = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);



    //init semaphores
    semctl(sem_1, 0, SETVAL, 1);  //init sempahore to 1 (available)
    semctl(sem_2, 0, SETVAL, 0);
    semctl(sem_3, 0, SETVAL, 0);
    semctl(sem_4, 0, SETVAL, 0);
    semctl(sem_5, 0, SETVAL, 0);
    semctl(sem_parent, 0, SETVAL, 0);

    //place students from input file into array
    FILE *student_file = fopen("student_database.txt", "r");
    if (NULL == student_file){
        printf("Student file can't open\n");
        exit(1);
    }

     char bom[3];
    if (fread(bom, 1, 3, student_file) == 3) {
        if (bom[0] != 0xEF || bom[1] != 0xBB || bom[2] != 0xBF) {
            fseek(student_file, 0, SEEK_SET);
        }
    }

    int student_num;
    int students[20];
    int index = 0;
    char line[256];
    while (fgets(line, sizeof(line), student_file)){
        if (sscanf(line, "%d", &student_num)){
            students[index] = student_num;
            index++;
        }
    }

    //get output files ready
    FILE *output_files[5];
    char string_num[2];
    char path_prefix[3];
    for (int i = 0; i < 5; i++){
        strcpy(path_prefix, "TA");
        sprintf(string_num, "%d.txt", i + 1);
        strcat(path_prefix, string_num);
        output_files[i] = fopen(path_prefix, "w");
        if (output_files[i] == NULL){
            printf("Output %d can't open\n", i);
            exit(1);
        }
    }


    //create 5 TAs
    pid_t pids[5];


    int semaphores[] = {sem_1, sem_2, sem_3, sem_4, sem_5}; 


    for (int i = 0; i < 5; i++){
        if ((pids[i] = fork()) == 0){
            int pid = getpid();
            int num_times_ran = 0;
            int current_student;
            for (int j = 0; j < 20; j++){
                wait(semaphores[i]);
                wait(semaphores[i % 5]);
                
                //fetch current student
                printf("I am TA %d and I am accessing the database\n", i + 1);
                current_student = students[j];
                //sleep(rand() % 3 + 1);

                //pass grade to output file 
                printf("I am TA %d and I am marking\n", i + 1);
                fprintf(output_files[i], "%d, %d\n", current_student, rand() % 10 + 1);
                //sleep( rand() % 10 + 1);

                //we've reached the end of the list, check if finished
                if (j == 19){
                    j = -1; // reset j
                    num_times_ran++;
                    if (num_times_ran == 3){
                        if (i == 4){
                            signal(sem_parent);
                        }

                        if (i % 2 == 0 ){ 
                            signal(sem_1);
                        } else {
                            signal(sem_2);
                        }
                        printf("I am process %d, and I am exiting\n", i + 1);
                        exit(0);
                    }
                } 
                
                if (i % 2 == 0 ){ 
                    signal(sem_1);
                } else {
                    signal(sem_2);
                }

            }

        }
    }

    wait(sem_parent);


    return 0;
}