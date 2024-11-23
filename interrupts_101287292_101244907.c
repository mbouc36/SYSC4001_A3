#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include<time.h>
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
    return new_process;

}

Queue* createQueue(Process** processes){
    Queue *q = (Queue *)malloc(sizeof(Queue));
    if (!q){
        printf("Memory allocation failed\n");
        return NULL;
    }
    q->head = processes[0];
    for (int i = 0; i < 15; i++){
        if (processes[i] == NULL){
            printf("Skipping NULL process at index: %d, when creating queue\n", i);
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

Queue* create_queue(Process ** processes){
    Queue* new_queue = (Queue*) malloc(sizeof(Queue));
    if (new_queue == NULL){
        printf("Malloc error creating new queue");
        exit(1);
    }

    new_queue->head = processes[0];
    new_queue->tail = processes[0];

    int last_index = 0;
    for (int i = 0; i < 15; i++){
        if (processes[i] != NULL){
            last_index = i;
            new_queue->tail->next = processes[i];
            new_queue->tail = processes[i];

        }
    }

    new_queue->tail = processes[last_index];
    new_queue->size = last_index + 1;

    return new_queue;
}

void enqueue(Queue *ready_queue, Process* process){
    if (ready_queue->tail == NULL){
        ready_queue->head = process;
        ready_queue->tail = process;
        
    } else {
        process->next = ready_queue->tail;
        ready_queue->tail = process;
    }
    ready_queue->size++;
}


Process* dequeue(Queue *ready_queue){
    if (ready_queue->head == NULL){
        printf("failed to find head when dequeueing");
        exit(1);
    }

    Process *temp = ready_queue->head;
    ready_queue->head = temp->next; //shift head out of queue


    //if head is null queue is empty
    if (ready_queue->head == NULL){
        ready_queue->tail = NULL;
    }

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
    process->cpu_time -= process->IO_frequency;
    strcpy(process->state, "WAITING");
    update_execution(execution_file, process, "RUNNING", current_time);
}

void waiting_to_running(FILE *execution_file, Process *process, int current_time){
    strcpy(process->state, "READY");
    update_execution(execution_file, process, "WAITING", current_time);
}

void running_to_terminated(FILE *execution_file, FILE *memory_file, Process *process, Partition* partitions_array[6], int current_time, int *usable_memory){
    strcpy(process->state, "TERMINATED");
    update_execution(execution_file, process, "RUNNING", current_time);
    partitions_array[process->partition_index]->PID = -1;
    update_memory_status(memory_file, partitions_array, current_time, process->size, usable_memory, false);
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
    char *execution_file_name = argv[2];
    execution_file = fopen(execution_file_name, "w");
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
    Process **processes = get_processes(argv[1], memory_file, execution_file, partitions_array, &current_time, &usable_memory);

    Queue *ready_queue = create_queue(processes);
    display_ready_queue(ready_queue);
   



    //READY->RUNNING
    ready_to_running(execution_file, processes[0], current_time);
    while (processes[0]->cpu_time > processes[0]->IO_frequency){ 

        //process runs RUNNING->WAITING
        current_time += processes[0]->IO_frequency;
        running_to_waiting(execution_file, processes[0], current_time);

        //process does IO WAITING->READY
        current_time += processes[0]->IO_duration;
        waiting_to_running(execution_file, processes[0], current_time);

        //READY->RUNNING
        ready_to_running(execution_file, processes[0], current_time);
    }

    current_time += processes[0]->cpu_time;
    running_to_terminated(execution_file, memory_file, processes[0], partitions_array, current_time, &usable_memory);




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