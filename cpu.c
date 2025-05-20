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
// function to parse register 
// This function takes a string token and converts it to an integer register number
int parse_register(const char *token) {
    if (token[0] != 'R') {
        printf("Expected register but got '%s'\n", token);
        exit(1);
    }

    char *endptr;
    int reg = strtol(token + 1, &endptr, 10);

    if (*endptr != '\0' || reg < 0 || reg > 31) {
        printf("Invalid register: %s\n", token);
        exit(1);
    }

    return reg;
}


// Read from asm.txt file 
int read_file() {
    int memAddr = 0;
    int param1, param2, param3;
    FILE *file = fopen("asm.txt", "r");

    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    char word[100];

    while (fscanf(file, "%99s", word) == 1) {

        if (word[0] == '#') {
            fgets(word, sizeof(word), file); // Skip comment line
            continue;
        }

        // R-Type: ADD, SUB, MUL, AND
        if (strcmp(word, "ADD") == 0 || strcmp(word, "SUB") == 0 ||
            strcmp(word, "MUL") == 0 || strcmp(word, "AND") == 0) {

            char inst[10]; strcpy(inst, word);
            fscanf(file, "%99s", word); param1 = parse_register(word);
            fscanf(file, "%99s", word); param2 = parse_register(word);
            fscanf(file, "%99s", word); param3 = parse_register(word);
            R_type(inst, param1, param2, param3, 0, memAddr++);
        }

        // R-Type: LSL, LSR (with shamt)
        else if (strcmp(word, "LSL") == 0 || strcmp(word, "LSR") == 0) {
            char inst[10]; strcpy(inst, word);
            fscanf(file, "%99s", word); param1 = parse_register(word);
            fscanf(file, "%99s", word); param2 = parse_register(word);
            fscanf(file, "%99s", word); int shamt = atoi(word);
            R_type(inst, param1, param2, 0, shamt, memAddr++);
        }

        // I-Type: MOVI
        else if (strcmp(word, "MOVI") == 0) {
            char inst[10]; strcpy(inst, word);
            fscanf(file, "%99s", word); param1 = parse_register(word);
            fscanf(file, "%99s", word); param3 = atoi(word);
            I_type(inst, param1, 0, param3, memAddr++);
        }

        // I-Type: JEQ, XORI, MOVR, MOVM
        else if (strcmp(word, "JEQ") == 0 || strcmp(word, "XORI") == 0 ||
                 strcmp(word, "MOVR") == 0 || strcmp(word, "MOVM") == 0) {

            char inst[10]; strcpy(inst, word);
            fscanf(file, "%99s", word); param1 = parse_register(word);
            fscanf(file, "%99s", word); param2 = parse_register(word);
            fscanf(file, "%99s", word); param3 = atoi(word);
            I_type(inst, param1, param2, param3, memAddr++);
        }

        // J-Type: JMP
        else if (strcmp(word, "JMP") == 0) {
            char inst[10]; strcpy(inst, word);
            fscanf(file, "%99s", word); param1 = atoi(word);
            J_type(inst, param1, memAddr++);
        }

        else {
            printf("Unknown or malformed instruction: %s\n", word);
            fclose(file);
            return 1;
        }
    }

    fclose(file);
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
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int address = excution_input[5]; // Get the address
    if (reg[reg1] == reg[reg2]) {
        pc += address; // Jump to the address if equal
    }
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
    int address = excution_input[6]; // Get the address
    pc = pc & (0xF000000) | address; // I am not sure about why it takes the the first 4 bits of 
}
void LSL(){
    WBPipe[0] = 1; // Set the flag to indicate writeback
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int shmt = excution_input[4]; // Get the shamt
    WBPipe[1]= reg1; // Store the destination register
    printf("shmt: %d\n", shmt);
    WBPipe[2]= reg[reg2] << shmt;
}
void LSR(){
    WBPipe[0] = 1; // Set the flag to indicate writeback
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int shmt = excution_input[4]; // Get the shamt
    WBPipe[1]= reg1; // Store the destination register
    WBPipe[2]= reg[reg2] >> shmt;

}
void MOVR(){
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int imm = excution_input[5]; // get the immediate value
    MemPipe[0] = 1; // Set the flag to indicate it should be done
    MemPipe[1] = excution_input[0]; // Store the instruction
    MemPipe[2]= reg1; // Store the destination register
    MemPipe[3] = reg[reg2] + imm;
}
void MOVM(){
    int reg1 = excution_input[1]; // Get the first register
    int reg2 = excution_input[2]; // Get the second register
    int imm = excution_input[5]; // get the immediate value
    MemPipe[0] = 1; // Set the flag to indicate writeback
    MemPipe[1] = excution_input[0];
    MemPipe[2]= reg1; // Store the destination register
    MemPipe[3] = reg[reg2] + imm;
}



void excute(){
    int opcode = excution_input[0]; // Get the opcode
    // now every function is different
    printf("Executing instruction with opcode: %d\n", opcode);
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



// -------------------------------- MEM --------------------------------
// as definded befoe mempipe,memendPipe is (flag, inst, reg , address) 
void memory(){
    if (Mem_endPipe[0]){
        if (Mem_endPipe[1] == 11){
            // MOVM
            int reg1 = Mem_endPipe[2]; // Get the first register
            int address = Mem_endPipe[3]; // Get the address
            mem[address] = reg[reg1]; // Store the value in memory
        }
        else if (MemPipe[1] == 10){
            // MOVR
             int reg1 = Mem_endPipe[2]; // Get the first register
            int address = Mem_endPipe[3]; // Get the address
           // now we should do it in the WB not here 
            WBPipe[0] = 1; // Set the flag to indicate writeback
            WBPipe[1]= reg1; // Store the destination register
            WBPipe[2]= mem[address]; // Store the value in memory
        }
    }
    // now move the pipeline to the next stage
    for (int i = 0; i < 4; i++) {
        Mem_endPipe[i] = MemPipe[i];
        MemPipe[i] = 0;
    }
}

void writeback(){
    if (WB_endPipe[0]){
            reg[WB_endPipe[1]] = WB_endPipe[2]; // Write the result to the register
    // now move the pipeline to the next stage
    for (int i = 0; i < 3; i++) {
        WB_endPipe[i] = WBPipe[i];
    }
    // print the register state
    printf("Writeback: R%d = %d\n", WBPipe[1], WBPipe[2]);
    }
    else{
        printf("No writeback needed\n");
    }
}




















int main() {
    if (read_file()) {
        printf("SYSTEM TERMINATED\n");
        return 1;
    }

    print_memory(0, 5); // Print the first 10 memory locations


    for (int i = 0; i < 100; i++) {
    printf("%d\n",reg[2]);
    fetch();
    decode();
    // now move the pipeline to the next stage
    for (int i = 0; i < 7; i++) {
        
        excution_input[i] = decoded[i];
    }
    excute();
     // now move the pipeline to the next stage
    for (int i = 0; i < 3; i++) {
        WB_endPipe[i] = WBPipe[i];
    }
    for (int i = 0; i < 4; i++) {
        Mem_endPipe[i] = MemPipe[i];
        MemPipe[i] = 0;
    }
    memory();
    writeback();
}

    printf("=== FINAL REGISTER STATE ===\n");
    for (int i = 0; i < 15; i++) {
        printf("R%d: %d\n", i, reg[i]);
    }

    print_memory(1029, 1030); // Print the first 10 memory locations
    printf("%d\n", 4 << 2);
    return 0;
}

