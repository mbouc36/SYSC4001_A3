
#ifndef INTERRUPTS_H
#define INTERRUPTS_H
#include <stdio.h>
#include <stdlib.h> 



typedef struct Partition
{
    int PartitionNum; // partitioned number 
    int size; // size of the memory allowed
    int PID; // string, only use free, init, and other program name
}Partition;


// PCB Structure 
typedef struct Process
{
    int pid; // Process identifier
    int partition_num; // partitioner number
    int size;
    char state[30];

}Process;

typedef struct Program {
    char program_name[20]; 
    int memory_size;
} Program;



//setups functions to preapre for reading
void setup_read_file(FILE **filename,  const char *path);

//Used to find an appropriate partition for a program of given size
int find_partition(int size, int PID,  Partition* partitions_array[6]);

//creates Memory Partitions
Partition* create_partition( int number,  int size);

//createds a new Process to be loaded on PCB
Process* create_new_process( int pid_num, int partition_number,  int size);

//update the state of processes table
void update_execution(FILE *execution_file, Process *process, char const old_state[20], int current_time);

//updated the memory state table 
void update_memory_status(FILE *memory_status, Partition* partitions_array[6], int current_time, int size_used, int *usable_memory);

#endif