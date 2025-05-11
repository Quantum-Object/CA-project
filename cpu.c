#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>



#define MEM_SIZE 2048
#define REG_SIZE 32





int mem[MEM_SIZE];// Memory array
int reg[REG_SIZE];// Register array
int pc = 0; // Program counter


int pipleine[5]; // pipeline array

int MemPipe[4]; // Memory pipeline array (flag, inst, reg , address)
int Mem_endPipe[4]; 
int WBPipe[3]; // Writeback pipeline array (flag, reg, address/result) "no need for instruction here"
int WB_endPipe[3]; 
int decoded[7]; // Decode array to store decoded values
int excution_input[7]; //same as decoded but the old one (from previous cycle)


/*
function to read from asm.txt and recognize the which one of the 3 types it is 

then it pass to one of these based on the type



function for R 
and function for I
function for J


these turn the (string) int int which is the opcode+m....asm


*/
//-------------------------------- FUNCTIONS THAT C DOES NOT HAVE --------------------------------
//print array
void print_array(int arr[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}
void print_decoded(){
    printf("Decoded Instruction:\n");
    printf("Opcode: %d, ", decoded[0]);
    printf("First Register: %d, ", decoded[1]);
    printf("Second Register: %d, ", decoded[2]);
    printf("Third Register: %d, ", decoded[3]);
    printf("Shamt: %d, ", decoded[4]);
    printf("Immediate Value: %d, ", decoded[5]);
    printf("Address: %d\n", decoded[6]);
}

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
void decode() {
    // Array to store decoded values
    int instruction = pipleine[0]; // Get the instruction from the pipeline
    int opcode = (instruction >>  28) & 0xF;// get 1st 4 bits(opcode)
    decoded[0] = opcode; // Store opcode in the first position

    if (opcode==0 || opcode==1 || opcode==2 || opcode==5 || opcode==8 || opcode==9){
        decoded[1] = (instruction >> 23) & 0x1F; // get the first register
        decoded[2] = (instruction >> 18) & 0x1F; // Get the second register
        decoded[3] = (instruction >> 13) & 0x1F; // Get the third register
        decoded[4] = (instruction >> 0) & 0x1FFF; // Get the shamt
        
    } else if (opcode==3 || opcode==4 || opcode==6 || opcode==10 || opcode==11){
        decoded[1] = (instruction >> 23) & 0x1F; // get the first register
        decoded[2] = (instruction >> 18) & 0x1F; // Get the second register
        decoded[5] = (instruction >> 0) & 0x3FFFF; // Get the immediate value
    }
    else if (opcode==7){
        decoded[6] = (instruction >> 0) & 0xFFFFFFF; // Get the address
    }
    else {
        printf("Unknown instruction type\n");
    }
}

// -------------------------------- EXCUTE--------------------------------
/*
ok, here we should do the following:
1.get the decoded instruction from decode which is stored in the pipeline
2. check the opcode and based on it call a crossponding function   
3. compute the result and store it in the register
NOTE: we still have MEM and WB
so the following should happen for inst(MOVR,MORM) we stop at we only do the summition and based on some flag 
we put (flag,sum) in pipeline:mem  -> for the mem to do its job
but for the others we just put the result in the pipeline:wb but also say where it should be placed and what to be place
(reg, result) we also need a flag for both to tell if we should WB or not 
this should also be done in the mem functions.
*/

void ADD(){
    WBPipe[0] = 1; // Set the flag to indicate writeback
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int reg3 = excution_input[3]; // Get the third register
    WBPipe[1]= reg1; // Store the destination register
    WBPipe[2] = reg[reg2] + reg[reg3]; // Perform addition
}

void SUB(){
    WBPipe[0] = 1; // Set the flag to indicate writeback
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int reg3 = excution_input[3]; // Get the third register
    WBPipe[1]= reg1; // Store the destination register
    WBPipe[2] = reg[reg2] - reg[reg3]; // Perform addition
}
void MUL(){
    WBPipe[0] = 1; // Set the flag to indicate writeback
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int reg3 = excution_input[3]; // Get the third register
    WBPipe[1]= reg1; // Store the destination register
    WBPipe[2] = reg[reg2] * reg[reg3]; // Perform addition
}
void MOVI(){
    WBPipe[0] = 1; // Set the flag to indicate writeback
    int reg1 = excution_input[1]; // Get the first register
    WBPipe[1]= reg1; // Store the destination register
    WBPipe[2] = excution_input[5]; // get the immediate value
}
void JEQ(){

}

void AND(){
    WBPipe[0] = 1; // Set the flag to indicate writeback
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int reg3 = excution_input[3]; // Get the third register
    WBPipe[1]= reg1; // Store the destination register
    WBPipe[2] = reg[reg2] & reg[reg3]; // Perform addition
}

void XORI(){
    WBPipe[0] = 1; // Set the flag to indicate writeback
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int imm = excution_input[5]; // get the immediate value
    WBPipe[1]= reg1; // Store the destination register
    WBPipe[2] = reg[reg2] ^ imm; // Perform addition

}

void JMP(){

}
void LSL(){
    WBPipe[0] = 1; // Set the flag to indicate writeback
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int shmt = excution_input[4]; // Get the shamt
    WBPipe[1]= reg1; // Store the destination register
    WBPipe[2]= reg2 << shmt;
}
void LSR(){
    WBPipe[0] = 1; // Set the flag to indicate writeback
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int shmt = excution_input[4]; // Get the shamt
    WBPipe[1]= reg1; // Store the destination register
    WBPipe[2]= reg2 >> shmt;

}
void MOVR(){
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int imm = excution_input[5]; // get the immediate value
    MemPipe[0] = 1; // Set the flag to indicate writeback
    MemPipe[1] = excution_input[0];
    MemPipe[2]= reg1; // Store the destination register
    MemPipe[2] = reg[reg2] + imm;
}
void MOVM(){
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int imm = excution_input[5]; // get the immediate value
    MemPipe[0] = 1; // Set the flag to indicate writeback
    MemPipe[1] = excution_input[0];
    MemPipe[2]= reg1; // Store the destination register
    MemPipe[2] = reg[reg2] + imm;
}



void excute(){
    int opcode = excution_input[0]; // Get the opcode
    // now every function is different
    switch (opcode)
    {
    case 0:
        ADD();
        break;
    
    case 1:
        SUB();
        break;
    case 2:
        MUL();
        break;
    case 3:
        MOVI();
        break;
    case 4:
        JEQ();
        break;
    case 5:   
        AND();
        break;
    case 6:
        XORI();
        break;
    case 7:
        JMP();
        break;
    case 8:
        LSL();
        break;
    case 9:
        LSR();
        break;
    case 10:
        MOVR();
        break;
    case 11:
        MOVM();
        break;
    default:    
        printf("Unknown opcode while Excuting: %d\n", opcode);
        break;
    }
}



























int main() {
    if (read_file())
        printf("SYSTEM TERMINATED");  // Read instructions from file
    // Print memory contents
    print_memory(0, 5); // Print first 10 memory locations
    fetch(); // Fetch the instruction
    decode();
    print_decoded();
    return 0;
}
