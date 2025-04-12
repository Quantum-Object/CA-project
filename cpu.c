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
void R_type(char instruction[], int p1, int p2, int p3,int memAddr) {
    printf("R_type instruction: %s %d %d %d\n", instruction, p1, p2, p3);
    int opcode= get_opcode(instruction); 
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
    int opcode= get_opcode(instruction); 
    int inst=opcode;
    inst = inst << 5;
    inst +=p1;
    inst = inst << 5;
    inst  +=p2;
    inst = inst << 18;
    inst +=p3;
    print_binary(inst);
    mem[memAddr] = inst;
}


// J type instruction
void J_type(char instruction[], int p1,int memAddr) {
    printf("J_type instruction: %s %d\n", instruction, p1);
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
         strcmp(word, "MUL") == 0 || strcmp(word, "AND") == 0 ||
         strcmp(word, "LSL") == 0 || strcmp(word, "LSR") == 0 ){
            char inst[10]; 
            strcpy(inst, word);
            fscanf(file, "%99s", word);
            param1 = atoi(substring(word, 1, strlen(word))); // Extract substring and convert to int
            fscanf(file, "%99s", word);
            param2 = atoi(substring(word, 1, strlen(word)));
            fscanf(file, "%99s", word);
            param3 = atoi(substring(word, 1, strlen(word)));
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
                param1 = atoi(substring(word, 1, strlen(word)));
                fscanf(file, "%99s", word);
                param2 = atoi(substring(word, 1, strlen(word)));
                I_type(inst, param1, param2, 0,memAddr++);
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



    }

    fclose(file);  // Close the file
    return 0;
}





















int main() {
    while (read_file()) {
        printf("ERRORR:\n");
    }
    return 0;
}
