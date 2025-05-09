#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>



#define MEM_SIZE 2048
#define REG_SIZE 32





int mem[MEM_SIZE];// Memory array
int reg[REG_SIZE];// Register array
int pc = 0; // Program counter


/*
function to read from asm.txt and recognize the which one of the 3 types it is 

then it pass to one of these based on the type



function for R 
and function for I
function for J


these turn the (string) int int which is the opcode+m....asm


*/
//-------------------------------- FUNCTIONS THAT C DOES NOT HAVE --------------------------------
//min function
int min(int a, int b) {
    return (a < b) ? a : b;
}

//substring function
char* substring(char s[], int i, int j) {
    // get substring from (i,j-1) inclusive
    char *sub = (char*)malloc((j - i + 1) * sizeof(char));
    if (sub == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    int k = 0;
    for (int m = i; m < j; m++) {
        sub[k++] = s[m];
    }
    sub[k] = '\0'; // Null-terminate the string
    return sub;

}
//binary printing function
void print_binary(int n) {
    for (int i = 31; i >= 0; i--) {
        int bit = (n >> i) & 1;
        printf("%d", bit);
    }
    printf("\n");
}
//-------------------------------- FUNCTIONS FOR MEMORY --------------------------------
// function to print memory
void print_memory(int start, int end) {
    printf("Memory contents:\n");
    for (int i = start; i < min(end+1,MEM_SIZE); i++) {
            printf("Address %d: ", i);
            print_binary(mem[i]);

    }
}


//-------------------------------- FUNCTIONS FOR INPUT Reading --------------------------------


// function for maping instruction to opcode
int get_opcode(char instruction[]) {
    if (strcmp(instruction, "ADD") == 0) return 0;
    else if (strcmp(instruction, "SUB") == 0) return 1;
    else if (strcmp(instruction, "MUL") == 0) return 2;
    else if (strcmp(instruction, "MOVI") == 0) return 3;
    else if (strcmp(instruction, "JEQ") == 0) return 4;
    else if (strcmp(instruction, "AND") == 0) return 5;
    else if (strcmp(instruction, "XORI") == 0) return 6;
    else if (strcmp(instruction, "JMP") == 0) return 7;
    else if (strcmp(instruction, "LSL") == 0) return 8;
    else if (strcmp(instruction, "LSR") == 0) return 9;
    else if (strcmp(instruction, "MOVR") == 0) return 10;
    else if (strcmp(instruction, "MOVM") == 0) return 11;
    return -1; // Unknown instruction
}

// R type instruction
void R_type(char instruction[], int p1, int p2, int p3,int shamt,int memAddr) {
    int opcode= get_opcode(instruction); 
    int inst=opcode;
    inst = inst << 5;
    inst +=p1;
    inst = inst << 5;
    inst  +=p2;
    inst = inst << 5;
    inst +=p3;
    inst = inst << 13;
    inst +=shamt;
    mem[memAddr] = inst;
}

// I type instruction
void I_type(char instruction[], int p1, int p2, int p3,int memAddr) {
    int opcode= get_opcode(instruction); 
    int inst=opcode;
    inst = inst << 5;
    inst +=p1;
    inst = inst << 5;
    inst  +=p2;
    inst = inst << 18;
    inst +=p3;
    mem[memAddr] = inst;
}


// J type instruction
void J_type(char instruction[], int p1,int memAddr) {
    int opcode= get_opcode(instruction); 
    int inst=opcode;
    inst = inst << 28;
    inst +=p1;
    mem[memAddr] = inst;

}


// Read from asm.txt file 
int read_file() {
    int memAddr = 0; // Memory address to store instructions
    int param1, param2, param3; // for reading registers and values
    FILE *file = fopen("asm.txt", "r");  
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    char word[100];  // Buffer to store each word
    while (fscanf(file, "%99s", word) == 1) {
        
        if (word[0] == '#'){
            fgets(word, sizeof(word), file);
        }
        else if 
        (strcmp(word, "ADD") == 0 || strcmp(word, "SUB") == 0 ||
         strcmp(word, "MUL") == 0 || strcmp(word, "AND") == 0
         ){
            char inst[10]; 
            strcpy(inst, word);
            fscanf(file, "%99s", word);
            param1 = atoi(substring(word, 1, strlen(word))); // Extract substring and convert to int
            fscanf(file, "%99s", word);
            param2 = atoi(substring(word, 1, strlen(word)));
            fscanf(file, "%99s", word);
            param3 = atoi(substring(word, 1, strlen(word)));
            R_type(inst, param1, param2, param3,0,memAddr++);
         }
        else if  (strcmp(word, "LSL") == 0 || strcmp(word, "LSR") == 0){
            char inst[10]; 
            strcpy(inst, word);
            fscanf(file, "%99s", word);
            param1 = atoi(substring(word, 1, strlen(word))); // Extract substring and convert to int
            fscanf(file, "%99s", word);
            param2 = atoi(substring(word, 1, strlen(word)));
            fscanf(file, "%99s", word);
            int shamt = atoi(word); // Extract substring and convert to int);
             R_type(inst, param1, param2, 0,shamt,memAddr++);
        }
        else if (
        strcmp(word, "MOVI") == 0 || strcmp(word, "JEQ") == 0 ||
        strcmp(word, "XORI") == 0 || strcmp(word, "MOVR") == 0 ||
        strcmp(word, "MOVM") == 0 ) {
            char inst[10]; 
            strcpy(inst, word);
            if (strcmp(word,"MOVI")==0){
                fscanf(file, "%99s", word);
                param1 = atoi(substring(word, 1, strlen(word)));
                fscanf(file, "%99s", word);
                param3 = atoi(word);
                I_type(inst, param1, 0, param3,memAddr++);
            }
            else {
            param1 = atoi(substring(word, 1, strlen(word)));
            fscanf(file, "%99s", word);
            param2 = atoi(substring(word, 1, strlen(word)));
            fscanf(file, "%99s", word);
            param3 =  atoi(word); // convert string to int
            I_type(inst, param1, param2, param3,memAddr++);}
        }
        else if (strcmp(word, "JMP")==0){
            char inst[10]; 
            strcpy(inst, word);
            fscanf(file, "%99s", word);
            param1 = atoi(word);
            J_type(inst, param1,memAddr++);

        }
        else {
            printf("Unknown instruction: %s\n", word);
            return 1;
        }


        printf("%d",memAddr);
    }

    fclose(file);  // Close the file
    return 0;
}



int pipleine[5]; // pipeline array



// -------------------------------- FETCH --------------------------------
// Fetch the instruction from memory
void fetch() {
    int instruction = mem[pc]; // Fetch the instruction from memory
    pipleine[0] = instruction; // Store it in the pipeline
    pc++; // Increment the program counter  
}


//--------------------------------- DECODE --------------------------------
// Decode the instruction 
// : this takes :: inst(int) -> array of all possible needed values based on the instruction
// some values of array will not be used

// array is as follows:
// 0: opcode
// 1: first register
// 2: second register
// 3: third register
// 4: shamt
// 5: immediate value
// 6: address
int* decode() {
    int decode[7]; // Array to store decoded values
    int instruction = pipleine[0]; // Get the instruction from the pipeline
    int opcode = (instruction >>  28) & 0xF;// get 1st 4 bits(opcode)
    decode[0] = opcode; // Store opcode in the first position

    if (opcode==0 || opcode==1 || opcode==2 || opcode==5 || opcode==8 || opcode==9){
        decode[1] = (instruction >> 23) & 0x1F; // get the first register
        decode[2] = (instruction >> 18) & 0x1F; // Get the second register
        decode[3] = (instruction >> 13) & 0x1F; // Get the third register
        decode[4] = (instruction >> 0) & 0x1FFF; // Get the shamt
        
    } else if (opcode==3 || opcode==4 || opcode==6 || opcode==10 || opcode==11){
        decode[1] = (instruction >> 23) & 0x1F; // get the first register
        decode[2] = (instruction >> 18) & 0x1F; // Get the second register
        decode[5] = (instruction >> 0) & 0x3FFFF; // Get the immediate value
    }
    else if (opcode==7){
        decode[6] = (instruction >> 0) & 0xFFFFFFF; // Get the address
    }
    else {
        printf("Unknown instruction type\n");
        return NULL;
    }
    // Print decoded values
    printf("Decoded Instruction:\n");
    printf("Opcode: %d\n", decode[0]);
    printf("First Register: %d\n", decode[1]);
    printf("Second Register: %d\n", decode[2]);
    printf("Third Register: %d\n", decode[3]);
    printf("Shamt: %d\n", decode[4]);
    printf("Immediate Value: %d\n", decode[5]);
    printf("Address: %d\n", decode[6]);
    return decode;
}


// -------------------------------- EXCUTE--------------------------------






























int main() {
    if (read_file())
        printf("SYSTEM TERMINATED");  // Read instructions from file
    // Print memory contents
    print_memory(0, 5); // Print first 10 memory locations
    fetch(); // Fetch the instruction
    decode();
    fetch();
    decode();

    return 0;
}
