
#ifndef INTERRUPTS_H
#define INTERRUPTS_H
#include <stdio.h>
#include <stdlib.h> 



typedef struct Partition
{
    int PartitionNum; 
    int size; 
    int PID; 
}Partition;


// PCB Structure 
typedef struct Process
{
    int pid; 
    int size;
    char state[30];
    int arrival_time;
    int cpu_time;
    int IO_frequency;
    int IO_duration;
    int partition_index;
    struct Process* next;

}Process;

typedef struct Queue
{
    struct Process* head;
    struct Process* tail;
    int size;
} Queue;





//setups functions to preapre for reading
void setup_read_file(FILE **filename,  const char *path);

//Used to find an appropriate partition for a program of given size
int find_partition(int size, int PID,  Partition* partitions_array[6]);

//creates Memory Partitions
Partition* create_partition( int number,  int size);

//createds a new Process to be loaded on PCB
Process* create_new_process( int pid_num, int size, int arrival_time, int cpu_time, int IO_frequency, int IO_duration, int partition_index);

//update the state of processes table
void update_execution(FILE *execution_file, Process *process, char const old_state[20], int current_time);

//updated the memory state table 
void update_memory_status(FILE *memory_file, Partition* partitions_array[6], int current_time, int size_used, int *usable_memory, bool fill_partition);

// Ready->Running state
void ready_to_running(FILE *execution_file, Process *process, int current_time);

// Running->Waiting state
void running_to_waiting(FILE *execution_file, Process *process, int current_time);

// Waiting->Running state
void waiting_to_running(FILE *execution_file, Process *process, int current_time);

// Running->Terminated state
void running_to_terminated(FILE *execution_file, FILE *memory_file, Process *process, Partition* partitions_array[6], int current_time, int *usable_memory);


//this creates the array from the input file and creates an array of processes 
Process** get_processes(const char *path, FILE *memory_file, FILE *execution_file, Partition* partitions_array[6], int* current_time, int* usable_memory);

Queue* create_queue(Process ** processes);

Process* dequeue(Queue *ready_queue);

void display_ready_queue(Queue *ready_queue);

#endif