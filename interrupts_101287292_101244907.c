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
    return new_process;

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

void update_memory_status(FILE *memory_status, Partition* partitions_array[6], int current_time, int size_used, int *usable_memory, bool fill_partition){
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
    

    fprintf(memory_status, "| %-14d | %-11d | %-3d, %-3d, %-3d, %-3d, %-3d, %-3d | %-17d | %-17d |\n", 
           current_time, fill_partition? size_used: 0, partitions_array[0]->PID, partitions_array[1]->PID, partitions_array[2]->PID, partitions_array[3]->PID,
            partitions_array[4]->PID, partitions_array[5]->PID, total_free_memory, *usable_memory);
}

void update_execution(FILE *execution_file, Process *process, char const old_state[20], int current_time){
    //                      "| Time of Transition | PID | Old State | New State |\n"
    fprintf(execution_file, "|                %-3d |  %-2d |    %-7s| %-10s|\n", current_time, process->pid, old_state, process->state);
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
    FILE  *input_file, *execution_file, *memory_status;

    //Setup varibles used for parsing files
    char line[256];
    int current_time = 0;
    int usable_memory = 100;



    
    //Let's prepare the output files
    char *execution_file_name = argv[2];
    execution_file = fopen(execution_file_name, "w");
    if (execution_file == NULL) {
        printf("Error opening file.\n");
        return 1;  //If file couldn't open
    }
    memory_status = fopen("memory_status_101287292_101244907.txt", "w");
    if (memory_status == NULL) {
        printf("Error opening system status file.\n");
        return 1;  //If file couldn't open
    }

    setup_read_file(&input_file, argv[1]);

    //Begin memory state table in memory status file
    fprintf(memory_status, "+-----------------------------------------------------------------------------------------------------+\n");
    fprintf(memory_status, "| Time of Event  | Memory Used |        Partitions State      | Total Free Memory | Usable Free Memory|\n");
    fprintf(memory_status, "+-----------------------------------------------------------------------------------------------------+\n");
    update_memory_status(memory_status, partitions_array, current_time, 0, &usable_memory, true);

    //Begin process state table in execution file
    fprintf(execution_file, "+--------------------------------------------------+\n");
    fprintf(execution_file, "| Time of Transition | PID | Old State | New State |\n");
    fprintf(execution_file, "+--------------------------------------------------+\n");
    
    
    //for first example
    Process *process;

    int best_partition_index;
    //Below is for parsing from input file
    int PID, size, arrival_time, CPU_time, IO_frequency, IO_duration;
    while (fgets(line, sizeof(line), input_file)){
        //lets get all the variables from the file
        if (sscanf(line, "%d, %d, %d, %d, %d, %d", &PID, &size, &arrival_time, &CPU_time, &IO_frequency, &IO_duration)){
            //before we create the process we need to find a memeory address
            best_partition_index = find_partition(size, PID, partitions_array);

            //Mark the partition with the PID value
            partitions_array[best_partition_index]->PID = PID;
            update_memory_status(memory_status, partitions_array, current_time, size, &usable_memory, true);

            //now we create the process
            process = create_new_process(PID, size, arrival_time, CPU_time, IO_frequency, IO_duration, best_partition_index);
            strcpy(process->state, "READY");
            update_execution(execution_file, process, "NEW", current_time);
        }
    }

    //READY->RUNNING
    strcpy(process->state, "RUNNING");
    update_execution(execution_file, process, "READY", current_time);
    while (process->cpu_time > process->IO_frequency){ 

        //process runs RUNNING->WAITING
        current_time += process->IO_frequency;
        process->cpu_time -= IO_frequency;
        strcpy(process->state, "WAITING");
        update_execution(execution_file, process, "RUNNING", current_time);

        //process does IO WAITING->READY
        current_time += process->IO_duration;
        strcpy(process->state, "READY");
        update_execution(execution_file, process, "WAITING", current_time);

        //READY->RUNNING
        strcpy(process->state, "RUNNING");
        update_execution(execution_file, process, "READY", current_time);


    }

    current_time += process->cpu_time;
    strcpy(process->state, "TERMINATED");
    update_execution(execution_file, process, "RUNNING", current_time);
    partitions_array[process->partition_index]->PID = -1;
    update_memory_status(memory_status, partitions_array, current_time, process->size, &usable_memory, false);



    //close memory status table
    fprintf(memory_status, "+-----------------------------------------------------------------------------------------------------+\n");

    //close process state table
    fprintf(execution_file, "+--------------------------------------------------+\n");
    
    //End simulation approprotialy
    for (int i = 0; i < 6; i++) {
        if (partitions_array[i] != NULL) {
            free(partitions_array[i]);
            partitions_array[i] = NULL; 
        }
    }

    fclose(input_file);
    fclose(execution_file);
    fclose(memory_status);


    return 0;

}
