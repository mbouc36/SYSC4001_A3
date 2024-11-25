#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include "interrupts_101287292_101244907.h"


// function to create partiotine node 
Partition* create_partition( int number,  int size){
    Partition* new_partition = (Partition*)malloc(sizeof(Partition));
    if (new_partition == NULL){
        printf("Malloc error creating new partition");
        exit(1);
    }
    new_partition->PartitionNum = number;
    new_partition->size = size;
    new_partition->PID = -1;
    return new_partition;

}

Process* create_new_process( int pid_num, int size, int arrival_time, int cpu_time, int IO_frequency, int IO_duration, int partition_index){
    Process* new_process = (Process*)malloc(sizeof(Process));
    if (new_process == NULL){
        printf("Malloc error creating new Process");
        exit(1);
    }
    new_process->pid = pid_num;
    new_process->size = size;
    strcpy(new_process->state, "NEW"); //Since the proccess is just created it is marked as new
    new_process->arrival_time = arrival_time;
    new_process->cpu_time = cpu_time;
    new_process->IO_frequency = IO_frequency;
    new_process->IO_duration = IO_duration;
    new_process->partition_index = partition_index;
    new_process->next = NULL;
    new_process->current_IO_time = new_process->IO_duration;
    return new_process;

}

Queue* create_queue(Process** processes){
    Queue *q = (Queue *)malloc(sizeof(Queue));
    if (q == NULL){
        printf("Memory allocation failed\n");
        exit(0);
    }
    q->head = processes[0];
    q->tail = processes[0];

    q->size = 1;
    for (int i = 1; i < 15; i++){
        if (processes[i] == NULL){// don't need to service
            continue;
        }

        if (q->head == NULL){
            q->head =processes[i];
            q->tail = processes[i];
        } else {
            q->tail->next = processes[i];
            q->tail = processes[i];
        }
        processes[i]->next = NULL;
        q->size++;
    }
    return q;
}


void enqueue(Queue *ready_queue, Process* process){
    process->next = NULL;
    if (ready_queue->head == NULL){
        ready_queue->head = process;
        ready_queue->tail = process;
        
    } else {
        ready_queue->tail->next = process;
        ready_queue->tail = process;
    }
    ready_queue->size++;
}


Process* dequeue(Queue *ready_queue){
    if (ready_queue->head == NULL){
        printf("failed to find head when dequeueing");
        return NULL;
    }

    Process *temp = ready_queue->head;
    ready_queue->head = ready_queue->head->next; //shift head out of queue

    temp->next = NULL;
    ready_queue->size--;
    return temp;

}

void display_ready_queue(Queue *ready_queue){
    printf("In the ready queue we have:\n");
    for (Process *process = ready_queue->head; process != NULL ;process = process->next){
        printf("%d\n", process->pid);
    }
}

void setup_read_file(FILE **filename, const char *path){

    *filename = fopen(path, "r");
    if (NULL == *filename){
        printf("%s file can't open\n", path);
        return;
    }

     char bom[3];
    if (fread(bom, 1, 3, *filename) == 3) {
        if (bom[0] != 0xEF || bom[1] != 0xBB || bom[2] != 0xBF) {
            fseek(*filename, 0, SEEK_SET);
        }
    }
}

Process** get_processes(const char *path, FILE *memory_file ,FILE *execution_file, Partition* partitions_array[6], int* current_time, int* usable_memory){
    //Begin memory state table in memory status file
    fprintf(memory_file, "+-----------------------------------------------------------------------------------------------------+\n");
    fprintf(memory_file, "| Time of Event  | Memory Used |        Partitions State      | Total Free Memory | Usable Free Memory|\n");
    fprintf(memory_file, "+-----------------------------------------------------------------------------------------------------+\n");
    update_memory_status(memory_file, partitions_array, *current_time, 0, usable_memory, true);

    //Begin process state table in execution file
    fprintf(execution_file, "+--------------------------------------------------+\n");
    fprintf(execution_file, "| Time of Transition |PID | Old State | New State |\n");
    fprintf(execution_file, "+--------------------------------------------------+\n");
    FILE  *input_file;

    setup_read_file(&input_file, path);


    char line[256];
    Process *process;

    Process **processes = (Process **) malloc(15 * sizeof(Process *));
    if (processes == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(input_file);
        return NULL;
    }

    int i = 0;
    //Below is for parsing from input file
    int PID, size, arrival_time, CPU_time, IO_frequency, IO_duration, best_partition_index;
    while (fgets(line, sizeof(line), input_file)){
        //lets get all the variables from the file
        if (sscanf(line, "%d, %d, %d, %d, %d, %d", &PID, &size, &arrival_time, &CPU_time, &IO_frequency, &IO_duration)){
            //before we create the process we need to find a memeory address
            best_partition_index = find_partition(size, PID, partitions_array);

            //Mark the partition with the PID value
            partitions_array[best_partition_index]->PID = PID;
            update_memory_status(memory_file, partitions_array, *current_time, size, usable_memory, true);

            //now we create the process
            process = create_new_process(PID, size, arrival_time, CPU_time, IO_frequency, IO_duration, best_partition_index);
            strcpy(process->state, "READY");
            update_execution(execution_file, process, "NEW", *current_time);
            processes[i] = process;
            i++;
        }
    }
    //set all other elements to null
    for (;i < 15 ; i++){
        processes[i] = NULL;
    }
    fclose(input_file);
    return processes;
    
}

int find_partition(int size, int PID,  Partition* partitions_array[6]){
    int min_fragmentation = INT_MAX;
    int best_partition_index = -1;
    //It finds an empty partition where the executable fits (Assume best-fit policy)
    for (int i = 0; i < 6; i++){
        if (partitions_array[i] != NULL && 
        partitions_array[i]->PID == -1 && 
        partitions_array[i]->size >= size && 
        partitions_array[i]->size - size < min_fragmentation){

            best_partition_index = i;
        } 

    }
    if (best_partition_index == -1){
        printf("Error: No suitable partition found for program %d.\n", PID);
        return 0;
    }
    return best_partition_index;
}

void update_memory_status(FILE *memory_file, Partition* partitions_array[6], int current_time, int size_used, int *usable_memory, bool fill_partition){
    int total_free_memory = 0;

    for (int i = 0; i < 6; i++){
        if (partitions_array[i]->PID == -1){
            total_free_memory += partitions_array[i]->size;
        } 
    }

    if (fill_partition){
        *usable_memory -= size_used;
    } else {
        *usable_memory += size_used;
    }

    fprintf(memory_file, "| %-14d | %-11d | %-3d, %-3d, %-3d, %-3d, %-3d, %-3d | %-17d | %-17d |\n", 
           current_time, fill_partition? size_used: 0, partitions_array[0]->PID, partitions_array[1]->PID, partitions_array[2]->PID, partitions_array[3]->PID,
            partitions_array[4]->PID, partitions_array[5]->PID, total_free_memory, *usable_memory);
}

void update_execution(FILE *execution_file, Process *process, char const old_state[20], int current_time){
    fprintf(execution_file, "|                 %-3d| %-2d |  %-7s  | %-10s|\n", current_time, process->pid, old_state, process->state);
}

void ready_to_running(FILE *execution_file, Process *process, int current_time){
    strcpy(process->state, "RUNNING");
    update_execution(execution_file, process, "READY", current_time);
}

void running_to_waiting(FILE *execution_file, Process *process, int current_time){
    strcpy(process->state, "WAITING");
    update_execution(execution_file, process, "RUNNING", current_time);
}

void waiting_to_ready(FILE *execution_file, Process *process, int current_time){
    strcpy(process->state, "READY");
    update_execution(execution_file, process, "WAITING", current_time);
}

void running_to_terminated(FILE *execution_file, FILE *memory_file, Process *process, Partition* partitions_array[6], int current_time, int *usable_memory){
    strcpy(process->state, "TERMINATED");
    update_execution(execution_file, process, "RUNNING", current_time);
    partitions_array[process->partition_index]->PID = -1;
    update_memory_status(memory_file, partitions_array, current_time, process->size, usable_memory, false);
}


void service_waiting(FILE *execution_file, Process *waiting_array[15], int *processes_waiting, Queue *ready_queue, int current_time){
    if (*processes_waiting > 0){
            for (int i = 0; i < 15; i++){
                if (waiting_array[i] != NULL){
                    Process *process = waiting_array[i]; 
                    process->current_IO_time--;
                    if (process->current_IO_time == 0){
                        enqueue(ready_queue, process);
                        printf("ENQUEUE size of queue: %d\n", ready_queue->size );
                        display_ready_queue(ready_queue);
                        process->current_IO_time = process->IO_duration;
                        waiting_array[i] = NULL; // clear position in array
                        (*processes_waiting)--;
                        printf("Process %d goes to ready, processes waiting: %d\n", process->pid, *processes_waiting);
                        waiting_to_ready(execution_file, process, current_time);
                        //shift down array elements
                        for (int j = i; j < 14; j++){
                            waiting_array[j] = waiting_array[j + 1];
                        }
                        waiting_array[14] = NULL;
                        i--;//reset i to ensure all elements are serviced
                        
                    }
                }
            }
        }
}

int main(int argc, char *argv[]){
    if (argc > 3){
        printf("Too many command line inputs");
    }
    srand(time(NULL)); //initiallize random seed for context switch

    // initialize the memory partitions 
    Partition* partitions_array[6];

    partitions_array[0] = create_partition(1,40);
    partitions_array[1] = create_partition(2,25);
    partitions_array[2] = create_partition(3,15);
    partitions_array[3] = create_partition(4,10);
    partitions_array[4] = create_partition(5,8);
    partitions_array[5] = create_partition(6,2);

    // Setup files
    FILE  *execution_file, *memory_file;

    //Setup varibles used for parsing files
    int current_time = 0;
    int usable_memory = 100;


    //Let's prepare the output files
    execution_file = fopen("execution_101287292_101244907.txt", "w");
    if (execution_file == NULL) {
        printf("Error opening file.\n");
        return 1;  //If file couldn't open
    }
    memory_file = fopen("memory_status_101287292_101244907.txt", "w");
    if (memory_file == NULL) {
        printf("Error opening system status file.\n");
        return 1;  //If file couldn't open
    }

    
    //parses input file and creates and array of processes
    Process **processes = get_processes("input_data_101287292_101244907.txt", memory_file, execution_file, partitions_array, &current_time, &usable_memory);


    Queue *ready_queue = create_queue(processes);


    //get number of processes we will need to handle before temrinating 
    int total_num_processes = 0;
    for (int i = 0; i < 15; i++){
        if (processes[i] != NULL){
            total_num_processes++;
        }
    }

    printf("the total num of processes: %d\n", total_num_processes);

    // prpccesses in waiting
    Process *waiting_array[15] = {NULL};
    int processes_waiting = 0;
    for (int i = 0; i < 15; i++){
        if (waiting_array[i] != NULL){
            processes_waiting++;
        }
    }

    Process* running_process = NULL;
    int next_IO;
    while (total_num_processes > 0){
        
        if (current_time % 10 == 0 ) {
            printf("At time: %d\n", current_time);
            //display_ready_queue(ready_queue); 
        }
        
        if (running_process == NULL){
            if (ready_queue->size > 0 ){
                running_process = dequeue(ready_queue);
                printf("DEQUEUE size of queue: %d\n", ready_queue->size );
                printf("Process %d is running\n", running_process->pid);
                ready_to_running(execution_file, running_process, current_time);
                next_IO = running_process->IO_frequency;

            } else { // no elements in ready queue keep incrementing time
                service_waiting(execution_file, waiting_array, &processes_waiting, ready_queue, current_time);
                current_time++;
                continue;
            } 

            if (current_time == 0){
                current_time++;
                continue;
            }
        }


        running_process->cpu_time--;
        next_IO--;
        service_waiting(execution_file, waiting_array, &processes_waiting, ready_queue, current_time);
        if (running_process->cpu_time == 0){
            printf("Process %d terminates\n", running_process->pid);
            running_to_terminated(execution_file, memory_file,running_process, partitions_array,current_time, &usable_memory);
            running_process = NULL;
            total_num_processes--;
            if (ready_queue->size > 0 ){
                running_process = dequeue(ready_queue);
                printf("Process %d is running\n", running_process->pid);
                ready_to_running(execution_file, running_process, current_time);
                next_IO = running_process->IO_frequency;
            }

           
        } else if (next_IO == 0){
            printf("Process %d goes to waiting, processes waiting: %d\n", running_process->pid, processes_waiting + 1);
            running_to_waiting(execution_file, running_process, current_time);
            waiting_array[processes_waiting] = running_process;
            processes_waiting++;
            running_process = NULL;
            if (ready_queue->size > 0 ){
                running_process = dequeue(ready_queue);
                printf("Process %d is running\n", running_process->pid);
                ready_to_running(execution_file, running_process, current_time);
                next_IO = running_process->IO_frequency;
            }

        } 
            
        current_time++;

    }



    //close memory status table
    fprintf(memory_file, "+-----------------------------------------------------------------------------------------------------+\n");

    //close process state table
    fprintf(execution_file, "+--------------------------------------------------+\n");
    
    //End simulation approprotialy
    for (int i = 0; i < 6; i++) {
        if (partitions_array[i] != NULL) {
            free(partitions_array[i]);
            partitions_array[i] = NULL; 
        }
    }

    if (processes) {
        for (int i = 0; processes[i] != NULL; i++) {
            free(processes[i]); 
        }
        free(processes); 
    }

    fclose(execution_file);
    fclose(memory_file);


    return 0;

}


/*


ready queue, waiting array

loop that represents time and hanlde all state changes



*/