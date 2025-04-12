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

// R type instruction
void R_type(char instruction[], int p1, int p2, int p3,int memAddr) {
    printf("R_type instruction: %s %d %d %d\n", instruction, p1, p2, p3);
    int opcode; // this needs a function to get crosspodning opcode (hard coded)
    int inst=opcode;
    inst = inst << 5;
    inst +=p1;
    inst = inst << 5;
    inst  +=p2;
    inst = inst << 5;
    inst +=p3;
    inst = inst << 13;
    mem[memAddr] = inst;

}

// I type instruction
void I_type(char instruction[], int p1, int p2, int p3,int memAddr) {
    printf("I_type instruction: %s %d %d %d\n", instruction, p1, p2, p3);
    int opcode; // this needs a function to get crosspodning opcode (hard coded)
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
    printf("J_type instruction: %s %d\n", instruction, p1);
    int opcode; // this needs a function to get crosspodning opcode (hard coded)
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
         strcmp(word, "MUL") == 0 || strcmp(word, "AND") == 0 ||
         strcmp(word, "LSL") == 0 || strcmp(word, "LSR") == 0 ){
            char inst[10]; 
            strcpy(inst, word);
            fscanf(file, "%99s", word);
            param1 = word[1] - '0'; //convert char to int '1' -> 1
            fscanf(file, "%99s", word);
            param2= word[1] - '0';
            fscanf(file, "%99s", word);
            param3 = word[1] - '0';
            R_type(inst, param1, param2, param3,memAddr++);
         }
        else if (
        strcmp(word, "MOVI") == 0 || strcmp(word, "JEQ") == 0 ||
        strcmp(word, "XORI") == 0 || strcmp(word, "MOVR") == 0 ||
        strcmp(word, "MOVM") == 0 ) {
            char inst[10]; 
            strcpy(inst, word);
            fscanf(file, "%99s", word);
            if (strcmp(word,"MOVI")==0){
                fscanf(file, "%99s", word);
                param1 = word[1] - '0'; //convert char to int '1' -> 1
                fscanf(file, "%99s", word);
                param2 =  atoi(word); 
                I_type(inst, param1, param2, 0,memAddr++);
            }
            else {
            param1 = word[1] - '0'; //convert char to int '1' -> 1
            fscanf(file, "%99s", word);
            param2= word[1] - '0';
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



    }

    fclose(file);  // Close the file
    return 0;
}























int main() {
    if (read_file())
        printf("Error parsing file.\n");
    return 0;
}
