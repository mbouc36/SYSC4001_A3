
#ifndef INTERRUPTS_H
#define INTERRUPTS_H
#include <stdio.h>
#include <stdlib.h> 


typedef struct Partition
{
     int PartitionNum; // partitioned number 
     int size; // size of the memory allowed
    char code[30]; // string, only use free, init, and other program name
}Partition;


// PCB Structure 
typedef struct Process
{
    int pid; // Process identifier
    int partition_num; // partitioner number
    int size;
    char name[20];
}Process;

typedef struct Program {
    char program_name[20]; 
    int memory_size;
} Program;

//switches user modes (1 ms)
void switch_user_modes(int *current_time, FILE *execution_file);

//saves and restores context (1-3 ms random)
void save_restore_context(int *current_time, FILE *execution_file); 

// calculates where in memory the ISR starts (1ms)
void get_ISR_start_address(int *current_time,int vector_table_address, FILE *execution_file);

//get the address if the ISR from the vector table (1 ms)
void load_vector_address_to_pc(int *current_time, int ISR_address, FILE *execution_file);

//execute the instructions for the ISR (duration found from input on vector table)
void execute_FORK(int *current_time, int fork_duration, FILE *execution_file);

//Return the value from the interrupt subroutine ( 1ms)
void IRET(int *current_time, FILE *execution_file);

//Handles the end of I/O situation
void end_of_IO(int *current_time, int ISR_duration, FILE *execution_file);

//checks interrupt pritority
void check_priority_of_ISR(int *current_time, FILE *execution_file);

//check if ISR is masked
void check_if_masked(int *current_time, FILE *execution_file);

//represetns efforts by the cpu, duration obtained from trace.txt
void cpu_execution(int *current_time, int duration, FILE *execution_file);

//setups functions to preapre for reading
void setup_read_file(FILE **filename,  const char *path);

//main function from Assignemnt 1
void handle_interrupts(FILE *input_file,  FILE *execution_file, FILE *system_status, int *current_time, int *PID, Partition* partitions_array[6], Program* list_of_external_files[5], Process* PCB[6],int vector_table_array[]);
//what do we do when theres a fork
void handle_fork(int *current_time, int duration, FILE *output_file);

//What do we di when theres an exec
void handle_exec(int *current_time, int duration, FILE *output_file, char *program_name, int size);

//use the find the memory size of a given Process in the list of external files
int find_process_size(Program* list_of_external_files[5], char program_name [20]);

//Used to find an appropriate partition for a program of given size
int find_partition(int size, char program_name [20],  Partition* partitions_array[6]);

//creates Memory Partitions
Partition* create_partition( int number,  int size, const char *code);

//createds a new Process to be loaded on PCB
Process* create_new_process( int pid_num, int partition_number,  int size, const char name [20]);

//what happens when we encounter a fork
void handle_fork(int *current_time, int duration, FILE *output_file);

//prints the current PCB in system status
void update_PCB(FILE *system_status, Process* PCB[6], int PID,  int current_time);

#endif