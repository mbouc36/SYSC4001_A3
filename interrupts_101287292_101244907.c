#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include "interrupts_101287292_101244907.h"

#define FCFS 0
#define EP 1
#define RR 2


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
    new_process->current_state_time = new_process->IO_duration; 
    return new_process;

}

Queue* create_queue(Process** processes, Process *late_processes[15]){
    Queue *q = (Queue *)malloc(sizeof(Queue));
    if (q == NULL){
        printf("Memory allocation failed\n");
        exit(0);
    }

    q->head = NULL;
    q->tail = NULL;

    q->size = 0;
    for (int i = 0; i < 15; i++){
        if (processes[i] == NULL){// don't need to service
            continue;
        }

        if (processes[i]->arrival_time == 0){
            if (q->head == NULL){
                q->head =processes[i];
                q->tail = processes[i];
            } else {
                Process *p = q->head;

                //traverse link list till we fit accordingly
                while(processes[i]->pid > p->pid ){

                    //reached tail
                    if (p->next == NULL){
                        p->next = processes[i];
                        q->tail = processes[i];
                        processes[i]->next = NULL;
                        break; 
                    } else if (processes[i]->pid >= p->next->pid){
                        p = p->next;
                    } else {
                        processes[i]->next = p->next;
                        p->next = processes[i];
                        break;
                    }
                    
                }

                if (p->next != processes[i] && p == q->head){ //processes is new head, first conditional checks we havent added it yet
                    processes[i]->next = q->head;
                    q->head = processes[i];
                }
                
            }
            q->size++;
        } else {
            late_processes[i] = processes[i];
            processes[i]->current_state_time = processes[i]->arrival_time;
        }
        
    }


    return q;
}

//FOR EP when adding to queue we need to know the value of the current process to know if we should preept it
void enqueue(Queue *ready_queue, Process* process){
    process->next = NULL;
    
    //set process as next head
    if (ready_queue->head == NULL){
        ready_queue->head = process;
        ready_queue->tail = process;
        
    } else {
        Process *p = ready_queue->head;
        
        //place the new process in according to it's arrival time
        while (process->arrival_time > p->arrival_time){
            
                    //reached tail
                    if (p->next == NULL){
                        p->next = process;
                        ready_queue->tail = process;
                        process->next = NULL;
                        break; 

                        //current process arrives later then the next process, note if the arrival time is the same we want to place it before
                    } else if (process->arrival_time > p->next->arrival_time){ 
                        p = p->next;
                    } else { //current process has found it position
                        process->next = p->next;
                        p->next = process;
                        break;
                    }

        }

        //now place the process based on pid

        
        while (process->pid > p->pid && process->arrival_time == p->arrival_time){
            //reached tail
            if (p->next == NULL){
                p->next = process;
                ready_queue->tail = process;
                process->next = NULL;
                break; 
            } else if (process->arrival_time == p->next->arrival_time && process->pid > p->next->pid){ //buggy here, should be checking activation time too so we don't put something that came first ahead
                p = p->next;
            } else {
                process->next = p->next;
                p->next = process;
                break;
            }
            
        } 

        //new process is head
        if (process->arrival_time < ready_queue->head->arrival_time || (process->arrival_time == ready_queue->head->arrival_time && process->pid < ready_queue->head->pid)){
            process->next = ready_queue->head;
            ready_queue->head = process;
        } 

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

void increment_waiting_times(Queue *ready_queue){
    if(ready_queue->head != NULL){
        for (Process *process = ready_queue->head; process != NULL; process = process->next){
            process->waiting_time++;
        }
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

Process** get_processes(const char *path, FILE *memory_file ,FILE *execution_file, Partition* partitions_array[6], int* current_time, int* total_free_memory){
    //Begin memory state table in memory status file
    fprintf(memory_file, "+-----------------------------------------------------------------------------------------------------+\n");
    fprintf(memory_file, "| Time of Event  | Memory Used |        Partitions State      | Total Free Memory | Usable Free Memory|\n");
    fprintf(memory_file, "+-----------------------------------------------------------------------------------------------------+\n");
    update_memory_status(memory_file, partitions_array, *current_time, 0, total_free_memory, true);

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
            if (arrival_time == 0){
                //before we create the process we need to find a memeory address
                best_partition_index = find_partition(size, PID, partitions_array);

                //Mark the partition with the PID value
                partitions_array[best_partition_index]->PID = PID;
                update_memory_status(memory_file, partitions_array, *current_time, size, total_free_memory, true);

                //now we create the process
                process = create_new_process(PID, size, arrival_time, CPU_time, IO_frequency, IO_duration, best_partition_index);
            
                strcpy(process->state, "READY");
                update_execution(execution_file, process, "NEW", *current_time);
            } else {
                best_partition_index = -1;
                //now we create the process
                process = create_new_process(PID, size, arrival_time, CPU_time, IO_frequency, IO_duration, best_partition_index);
            }

            
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

void update_memory_status(FILE *memory_file, Partition* partitions_array[6], int current_time, int size_used, int *total_free_memory, bool fill_partition){
    int usable_memory = 0;

    for (int i = 0; i < 6; i++){
        if (partitions_array[i]->PID == -1){
            usable_memory += partitions_array[i]->size;
        } 
    }

    if (fill_partition){
        *total_free_memory -= size_used;
    } else {
        *total_free_memory += size_used;
    }

    fprintf(memory_file, "| %-14d | %-11d | %-3d, %-3d, %-3d, %-3d, %-3d, %-3d | %-17d | %-17d |\n", 
           current_time, 100 - *total_free_memory, partitions_array[0]->PID, partitions_array[1]->PID, partitions_array[2]->PID, partitions_array[3]->PID,
            partitions_array[4]->PID, partitions_array[5]->PID, *total_free_memory, usable_memory);
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

void running_to_terminated(FILE *execution_file, FILE *memory_file, Process *process, Partition* partitions_array[6], int current_time, int *total_free_memory){
    strcpy(process->state, "TERMINATED");
    update_execution(execution_file, process, "RUNNING", current_time);
    partitions_array[process->partition_index]->PID = -1;
    update_memory_status(memory_file, partitions_array, current_time, process->size, total_free_memory, false);
}

//state chnage from RUNNING->READY when preempted
void running_to_ready(FILE *execution_file, Process *process, int current_time){
    strcpy(process->state, "READY");
    update_execution(execution_file, process, "RUNNING", current_time); 
}

//this function returns false when there is no more late processes to load
bool load_late_processes(FILE *execution_file, FILE *memory_file, Partition** partitions_array, Queue *ready_queue, Process *late_processes[15], int current_time, int *total_free_memory){
    bool any_late_process = false;
    for (int i = 0; i < 15 ;i++){
        if (late_processes[i] != NULL){        
            if (late_processes[i]->current_state_time > 0){
                any_late_process = true;
                late_processes[i]->current_state_time--;
            } else {
                enqueue(ready_queue, late_processes[i]);
                late_processes[i]->current_state_time = late_processes[i]->IO_duration;
                //before we create the process we need to find a memeory address
                int best_partition_index = find_partition(late_processes[i]->size, late_processes[i]->pid, partitions_array);
                late_processes[i]->partition_index = best_partition_index;

                //Mark the partition with the PID value
                partitions_array[best_partition_index]->PID = late_processes[i]->pid;
                update_memory_status(memory_file, partitions_array, current_time, late_processes[i]->size, total_free_memory, true);
                strcpy(late_processes[i]->state, "READY");
                update_execution(execution_file, late_processes[i], "NEW", current_time);
                late_processes[i] = NULL;
            }
        }
    }

    return any_late_process;
}


void service_waiting(FILE *execution_file, Process *waiting_array[15], int *processes_waiting, Queue *ready_queue, int current_time){
    if (*processes_waiting > 0){
            for (int i = 0; i < 15; i++){
                if (waiting_array[i] != NULL){
                    Process *process = waiting_array[i]; 
                    process->current_state_time--;
                    if (process->current_state_time == 0){
                        enqueue(ready_queue, process);
                        process->current_state_time = process->IO_duration;
                        waiting_array[i] = NULL; // clear position in array
                        (*processes_waiting)--;
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
    int total_free_memory = 100;
    int scheduler;
    if (strcmp(argv[2], "FCFS") == 0){
        scheduler = FCFS;
    } else if (strcmp(argv[2], "EP") == 0){
        scheduler = EP;
    } else if (strcmp(argv[2], "RR") == 0){
        scheduler = RR;
    } else {
        printf("Can't recognize scheduler, should be FCFS, EP or RR\n");
        exit(1);
    }


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
    Process **processes = get_processes(argv[1], memory_file, execution_file, partitions_array, &current_time, &total_free_memory);

    //processes which arrive later
    Process *late_processes[15] = {NULL};
    Queue *ready_queue = create_queue(processes, late_processes);


    //get number of processes we will need to handle before temrinating 
    int total_num_processes = 0;
    for (int i = 0; i < 15; i++){
        if (processes[i] != NULL){
            total_num_processes++;
        }
    }

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
    bool any_late_process = true;
    bool no_process_running; //to ensure we don't service at the same time we init it
    int quantum;
    while (total_num_processes > 0){
        if (running_process == NULL){
            //queue is empty but could fill if a process is finished with IO
            if (ready_queue->size == 0){
                service_waiting(execution_file, waiting_array, &processes_waiting, ready_queue, current_time);

            } 

            //get new running process based on queue
            if (ready_queue->size > 0 ){
                running_process = dequeue(ready_queue);
                ready_to_running(execution_file, running_process, current_time);
                next_IO = running_process->IO_frequency;
                quantum = 100;
                
            } 
            no_process_running= true;
        }
        

        if (any_late_process){
            any_late_process = load_late_processes(execution_file, memory_file, partitions_array, ready_queue, late_processes, current_time, &total_free_memory);
        }
        increment_waiting_times(ready_queue);
        if (!no_process_running){
            running_process->cpu_time--;
            next_IO--;
            quantum--;
            service_waiting(execution_file, waiting_array, &processes_waiting, ready_queue, current_time);
            if (next_IO == 0){
                running_to_waiting(execution_file, running_process, current_time);
                waiting_array[processes_waiting] = running_process;
                processes_waiting++;
                running_process = NULL;
            
            } else if (running_process->cpu_time <= 0){
                running_to_terminated(execution_file, memory_file, running_process, partitions_array,current_time, &total_free_memory);
                running_process->current_state_time = current_time; //used for finding mean turn around time
                running_process = NULL;
                total_num_processes--;            
            } else if (scheduler == RR && quantum == 0){
                running_to_ready(execution_file, running_process, current_time);
                enqueue(ready_queue, running_process);
                printf("Running process finishes it's quantum\n");
                running_process = NULL;
            }

            if (ready_queue->size > 0 ){
                //should we preempt
                if (scheduler == EP && running_process != NULL &&  ready_queue->head->pid < running_process->pid){
                    running_to_ready(execution_file, running_process, current_time);
                    enqueue(ready_queue, running_process);
                    running_process = NULL;

                }
                
                if (running_process == NULL){
                    running_process = dequeue(ready_queue);
                    ready_to_running(execution_file, running_process, current_time);
                    next_IO = running_process->IO_frequency;
                    quantum = 100;
                }
            }
        }

       no_process_running = false;
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

    //print mean waiting time
    int total_waiting_time = 0;
    int total_turnaround_time = 0;
    for (int i =0; i < 15; i++){
        if (processes[i] != NULL ){
            total_num_processes++;
            total_waiting_time += processes[i]->waiting_time;
            total_turnaround_time += processes[i]->current_state_time - processes[i]->arrival_time;            
        }

    }
    printf("Throughput: %.2f\n", (float) (current_time - 1)/total_num_processes);
    printf("Mean Turn around Time: %.2f\n", (float) total_turnaround_time/total_num_processes);
    printf("Average waiting time: %.2f\n", (float) total_waiting_time/total_num_processes);
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

//Throughput = Number of Processes / Total Time
// avg wait time time in ready queue
// TAT = Completion Time - Arrival Time