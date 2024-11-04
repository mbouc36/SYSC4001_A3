#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include<time.h>
#include <limits.h>
#include "interrupts_101287292_101244907.h"


// function to create partiotine node 
Partition* create_partition( int number,  int size, const char *code){
    Partition* new_partition = (Partition*)malloc(sizeof(Partition));
    if (new_partition == NULL){
        printf("Malloc error creating new partition");
        exit(1);
    }
    new_partition->PartitionNum = number;
    new_partition->size = size;
    strcpy(new_partition->code,code); // copy the code from here to the new one
    return new_partition;

}

Process* create_new_process( int pid_num, int partition_number,  int size, const char name [20]){
    Process* new_pcb = (Process*)malloc(sizeof(Process));
    if (new_pcb == NULL){
        printf("Malloc error creating new PCB");
        exit(1);
    }
    new_pcb->pid = pid_num;
    new_pcb->partition_num = partition_number;
    new_pcb->size = size;
    strcpy(new_pcb->name,name);
    return new_pcb;

}


Program* create_program(char program_name[20], int memory_size){
    Program* new_program = (Program *)malloc(sizeof(Program));
    strcpy(new_program->program_name, program_name);
    new_program->memory_size = memory_size;
    return new_program;
}


//switches user modes (1 ms)
void switch_user_modes(int *current_time, FILE *execution_file){
    usleep(1*1000);
    fprintf(execution_file, "%d, %d, switch to kernel mode\n", *current_time, 1);
    *current_time += 1; 
}

//saves and restores context (1-3 ms random)
void save_restore_context(int *current_time, FILE *execution_file){
    int min = 1;
    int max = 3;

    // Generate a random number between 1 and 3
    int randomInt = rand() % (max - min + 1) + min;
    usleep(randomInt*1000);
    fprintf(execution_file, "%d, %d, context saved\n", *current_time, randomInt);
    *current_time += randomInt;
}

// calculates where in memory the ISR starts (1ms)
void get_ISR_start_address(int *current_time,  int vector_table_address, FILE *execution_file){
    usleep(1*1000);
    fprintf(execution_file, "%d, %d, find vector %d in memory position 0x%04x\n", *current_time, 1, vector_table_address, vector_table_address * 2); // memeory position is vector number in hex + 0x2
    *current_time += 1; 
}


//get the address if the ISR from the vector table (1 ms)
void load_vector_address_to_pc(int *current_time,  int ISR_address, FILE *execution_file){
    usleep(1*1000);
    fprintf(execution_file, "%d, %d, load address 0x%04x into the PC\n", *current_time, 1, ISR_address);
    *current_time += 1; 
}

//get ISR start address, address from vector table, execute the instructions for the ISR (ISR execute time +1 +1)
//This needs to be completelty redone
void execute_ISR(int *current_time, int ISR_duration, FILE *execution_file){
    if (ISR_duration > 1000){
        sleep(ISR_duration/1000);
    } else {
        usleep(ISR_duration*1000);        
    }

    fprintf(execution_file, "%d, %d, SYSCALL: run the ISR\n", *current_time, ISR_duration * 50 / 100);
    *current_time += ISR_duration * 0.50;
    fprintf(execution_file, "%d, %.d, transfer data\n", *current_time, ISR_duration * 40 / 100);
    *current_time += ISR_duration * 0.40;
    fprintf(execution_file, "%d, %.d, check for errors\n", *current_time, ISR_duration - ISR_duration * 50 / 100 - ISR_duration * 40 / 100);

    *current_time += ISR_duration - ISR_duration * 50 /100 - ISR_duration * 40 / 100;
}

//Return the value from the interrupt subroutine ( 1ms)
void IRET(int *current_time, FILE *execution_file){
    usleep(1*1000);
    fprintf(execution_file, "%d, %d, IRET\n", *current_time, 1);
    *current_time += 1; 

}


//Handles the end of I/O situation
void end_of_IO(int *current_time, int ISR_duration, FILE *execution_file){
    if (ISR_duration > 1000){
        sleep(ISR_duration/1000);
    } else {
        usleep(ISR_duration*1000);
        
    }
    fprintf(execution_file, "%d, %d, END_IO\n", *current_time, ISR_duration);
    *current_time += ISR_duration;
}


//checks interrupt pritority
void check_priority_of_ISR(int *current_time, FILE *execution_file){
    usleep(1*1000);
    fprintf(execution_file, "%d, %d, check priority of interrupt\n", *current_time, 1);
    *current_time += 1; 
}

//check if ISR is masked
void check_if_masked(int *current_time, FILE *execution_file){
    usleep(1*1000);
    fprintf(execution_file, "%d, %d, check if masked\n", *current_time, 1);
    *current_time += 1; 
}


//represetns efforts by the cpu, duration obtained from trace.txt
void cpu_execution(int *current_time, int duration, FILE *execution_file){
    if (duration > 1000){
        sleep(duration/1000);
    } else {
        usleep(duration*1000);
    }
    fprintf(execution_file, "%d, %d, CPU execution\n", *current_time, duration);
    *current_time += duration;
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

/*          This is the Main function from assignment 1                 */
void handle_interrupts(FILE *input_file,  FILE *execution_file, FILE *system_status, int *current_time, int *PID, Partition* partitions_array[6], Program* list_of_external_files[5], Process* PCB[6],int vector_table_array[]){
    char line[256];
    int duration, vector_number;
    
  
    char command_name[20];
    char program_name [20];
    while (fgets(line, sizeof(line), input_file)) {
         if (sscanf(line, "%s %d, %d", command_name, &vector_number, &duration) == 3) {
            //This is either a SYSCALL or END_IO
            if (strcmp(command_name, "SYSCALL") == 0){
            
                switch_user_modes(current_time, execution_file);
                save_restore_context(current_time, execution_file);
                get_ISR_start_address(current_time, vector_number, execution_file);
                load_vector_address_to_pc(current_time, vector_table_array[vector_number], execution_file);
                execute_ISR(current_time, duration, execution_file);
                IRET(current_time, execution_file);
                
            } else if (strcmp(command_name, "END_IO") == 0){
                check_priority_of_ISR(current_time, execution_file);
                check_if_masked(current_time, execution_file);
                switch_user_modes(current_time, execution_file);
                save_restore_context(current_time, execution_file);
                get_ISR_start_address(current_time, vector_number, execution_file);
                load_vector_address_to_pc(current_time, vector_table_array[vector_number], execution_file);
                end_of_IO(current_time, duration, execution_file);
                IRET(current_time, execution_file);
            } 
             
        } else if (sscanf(line, "%s %[^,], %d", command_name, &program_name, &duration) == 3){
            if (strcmp(command_name, "EXEC") == 0){
                //simulate SYSCALL
                switch_user_modes(current_time, execution_file);
                save_restore_context(current_time, execution_file);
                get_ISR_start_address(current_time, 3, execution_file);
                load_vector_address_to_pc(current_time, vector_table_array[3], execution_file);

                //read external_files.txt to find size of respective program
                int process_size = find_process_size(list_of_external_files, program_name);
                if (process_size == -1) {
                    fprintf(execution_file, "Error: Program %s not found in external files.\n", program_name);
                    continue; //goes to next line
                }
                fprintf(execution_file, "%d, %d, EXEC: load %s of size %dMb\n", *current_time, duration * 20/100, program_name, process_size);
                *current_time += duration * 20/100;

                //find the best partition
                int best_partition_index = find_partition(process_size, program_name, partitions_array);
                if (best_partition_index == -1) {
                    fprintf(execution_file, "Error: No suitable partition found for program %s.\n", program_name);
                    continue; //go to next line 
                }
                fprintf(execution_file, "%d, %d, found partition %d with %dMb of space\n", *current_time, duration * 37/100, best_partition_index + 1, partitions_array[best_partition_index]->size);
                *current_time += duration * 37/100;

                //Mark the partition as occupied
                strcpy(partitions_array[best_partition_index]->code, program_name);
                fprintf(execution_file, "%d, %d, partition %d marked as occupied\n", *current_time, duration * 10/100, best_partition_index + 1);
                *current_time += duration * 10/100;
                //Update the PCB
                PCB[*PID] = create_new_process(*PID, best_partition_index + 1, process_size, program_name);
                fprintf(execution_file, "%d, %d, updating PCB with new information\n", *current_time, duration * 30/100);
                *current_time += duration * 30/100;

                //Call the Scheduler
                fprintf(execution_file, "%d, %d, scheduler called\n", *current_time, duration * 3/100);
                *current_time += duration - duration * 97/100;
                
                //IRET
                IRET(current_time, execution_file);
                update_PCB(system_status, PCB, *PID, *current_time);

                FILE *program_file;
                setup_read_file(&program_file, strcat(program_name, ".txt"));
                
                // //Now execute the program
                handle_interrupts(program_file ,execution_file, system_status, current_time, PID, partitions_array, list_of_external_files, PCB, vector_table_array);
                fclose(program_file);

        }} else if (sscanf(line, "%[^,], %d", command_name, &duration) == 2) { // 2 because this is the number of variables returned
            //This is a cpu function
            if (strcmp(command_name, "CPU") == 0){
                cpu_execution(current_time, duration, execution_file);
            } else if (strcmp(command_name, "FORK") == 0){
                //simulate SYSCALL
                switch_user_modes(current_time, execution_file);
                save_restore_context(current_time, execution_file);
                get_ISR_start_address(current_time, 2, execution_file);
                load_vector_address_to_pc(current_time, vector_table_array[2], execution_file);
                //copy parent PCB to child and call scheduler
                handle_fork(current_time, duration, execution_file);
                //IRET
                IRET(current_time, execution_file);
                (*PID)++;
                update_PCB(system_status, PCB, *PID, *current_time);
            } 
           
        } else {
            printf("Failed to parse line: %s", line);
        }
    
}
}


void handle_fork(int *current_time, int duration, FILE *output_file){
    usleep(duration * 1000);
    fprintf(output_file, "%d, %d, FORK: copy parent PCB to child PCB\n", *current_time, duration * 80/100);
    *current_time += duration * 0.80;

    fprintf(output_file, "%d, %d, scheduler called\n", *current_time, duration - duration * 80/100);
    *current_time += duration - duration * 0.80;

}



int find_process_size(Program* list_of_external_files[5], char program_name [20]){
    int size = -1;
    for (int i = 0; i < 5; i++){
        if (list_of_external_files[i] != NULL && strcmp(list_of_external_files[i]->program_name, program_name) == 0){
            size = list_of_external_files[i]->memory_size;
            break;
        }   
    }

    if (size == -1){
        printf("Program was not found in list of external files");
    }
    return size;
}

int find_partition(int size, char program_name [20],  Partition* partitions_array[6]){
    int min_fragmentation = INT_MAX;
    int best_partition_index = -1;
    //It finds an empty partition where the executable fits (Assume best-fit policy)
    for (int i = 0; i < 6; i++){
        if (partitions_array[i] != NULL && 
        strcmp(partitions_array[i]->code, "free") == 0 && 
        partitions_array[i]->size >= size && 
        partitions_array[i]->size - size < min_fragmentation){

            best_partition_index = i;
        } 

    }
    if (best_partition_index == -1){
        printf("There was no space for %s", program_name);
        return 0;
    }
    return best_partition_index;
}

void update_PCB(FILE *system_status, Process* PCB[6], int PID,  int current_time){
    fprintf(system_status, "!-----------------------------------------------------------!\n");
    fprintf(system_status , "Save Time: %d ms\n", current_time);
    fprintf(system_status, "+---------------------------------------------------------+\n");
    fprintf(system_status, "| %-4s | %-12s | %-15s | %-4s |\n", "PID", "Program Name", "Partition Number", "size");
    fprintf(system_status, "+---------------------------------------------------------+\n");
    for (int i = 0; i <= PID; i++){
        if (PCB[i] == NULL && i > 0){
            fprintf(system_status, "| %-4d | %-12s | %-15d | %-4d |\n", PID, PCB[0]->name, PCB[0]->partition_num, PCB[0]->size);
        } else {
            fprintf(system_status, "| %-4d | %-12s | %-15d | %-4d |\n", PCB[i]->pid, PCB[i]->name, PCB[i]->partition_num, PCB[i]->size);
        }
    }
    
    fprintf(system_status, "+---------------------------------------------------------+\n");
    fprintf(system_status, "!-----------------------------------------------------------!\n");
}


int main(int argc, char *argv[]){
    if (argc > 3){
        printf("Too many command line inputs");
    }
    srand(time(NULL)); //initiallize random seed for context switch

    /* ASSIGNEMNT 2 */
    // initialize the partitions 

    Partition* partitions_array[6];

    partitions_array[0] = create_partition(1,40,"free");
    partitions_array[1] = create_partition(2,25, "free");
    partitions_array[2] = create_partition(3,15, "free");
    partitions_array[3] = create_partition(4,10, "free");
    partitions_array[4] = create_partition(5,8, "free");
    partitions_array[5] = create_partition(6,2, "init");


    // initialize the PCB-1 
    Process* init_process = create_new_process(0, 6, 1, "init"); // initialize the PCB table and pid 0, called init, which will use Partition 6; it uses 1 Mb of mm

    Process* PCB[6] = {NULL}; // the PCB stores an array of processes init all values to NULL

    PCB[0] = init_process;
    int PID = 0;


    /* ASSIGNEMNT 2 */
    

    FILE  *trace_file, *vector_table, *execution_file, *external_files, *system_status;

    char line[256];
    int vector_table_array[100]; // we use this array to store the vector table
    int value;
    int current_time = 0;
    

    //handle vector table
    setup_read_file(&vector_table, "vector_table_101287292_101244907.txt");
    int i = 0 ;
    // Read the file line by line
    while (fgets(line, sizeof(line), vector_table)) {
        // Convert the hex string to an integer
        if (sscanf(line, "%x", &value) == 1) {
            vector_table_array[i] = value;
            i++;
        } else {
            printf("Failed to parse line: %s", line);
        }
    }

    Program* list_of_external_files[5];
    /*       Load the external_file names into an array      */
    setup_read_file(&external_files, "external_files_101287292_101244907.txt");
    
    //read the file line by line
    i = 0;
    char program_name [20];
    int size;
    while(fgets(line, sizeof(line), external_files)){
        if (sscanf(line, "%[^,], %d", &program_name, &size) == 2) {
            list_of_external_files[i] = create_program(program_name, size);
            i++;
        } else {
            printf("Failed to parse line in external_files: %s", line);
        }
    }
    fclose(external_files);

    

    //Let's prepare the output files
    char *execution_file_name = argv[2];
    execution_file = fopen(execution_file_name, "w");
    if (execution_file == NULL) {
        printf("Error opening file.\n");
        return 1;  //If file couldn't open
    }
    system_status = fopen("system_status_101287292_101244907.txt", "w");
    if (system_status == NULL) {
        printf("Error opening system status file.\n");
        return 1;  //If file couldn't open
    }

    update_PCB(system_status, PCB, PID, current_time);
    setup_read_file(&trace_file, argv[1]);
    handle_interrupts(trace_file ,execution_file, system_status, &current_time, &PID, partitions_array, list_of_external_files, PCB, vector_table_array);

    //End simulation approprotialy
    for (int i = 0; i < 6; i++) {
        if (partitions_array[i] != NULL) {
            free(partitions_array[i]);
            partitions_array[i] = NULL; 
        }
    }

    for (int i = 0; i < 6; i++) {
        if (PCB[i] != NULL) {
            free(PCB[i]);
            PCB[i] = NULL;
        }
    }

    for (int i = 0; i < 5; i++) {
        if (list_of_external_files[i] != NULL) {
            free(list_of_external_files[i]);
            list_of_external_files[i] = NULL; 
        }
    }


    fclose(trace_file);
    fclose(execution_file);
    fclose(system_status);
    return 0;

}
