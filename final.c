#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INSTRUCTIONS 256
#define REGISTER_COUNT 32
#define MEMORY_SIZE 1024

// Instruction types
typedef enum { R_TYPE, I_TYPE, J_TYPE, EMPTY_TYPE } InstructionType;

// Instruction structure
typedef struct {
    InstructionType type;
    char opcode[10];
    int rs, rt, rd, immediate, address;
} Instruction;

// Pipeline register
typedef struct {
    Instruction instr;
    int cycle_count;
    int result;
} PipelineRegister;

// Global state
Instruction instructions[MAX_INSTRUCTIONS];
int instruction_count = 0;
int registers[REGISTER_COUNT] = {0};
int memory[MEMORY_SIZE] = {0};
int pc = 0;

PipelineRegister IF, ID, EX, MEM, WB;

// Util functions
Instruction parse_instruction(char *line) {
    Instruction instr = {.type = EMPTY_TYPE};
    char opcode[10];
    sscanf(line, "%s", opcode);

    if (strcmp(opcode, "add") == 0 || strcmp(opcode, "sub") == 0) {
        instr.type = R_TYPE;
        strcpy(instr.opcode, opcode);
        sscanf(line, "%*s $%d, $%d, $%d", &instr.rd, &instr.rs, &instr.rt);
    } else if (strcmp(opcode, "lw") == 0 || strcmp(opcode, "sw") == 0 || strcmp(opcode, "addi") == 0) {
        instr.type = I_TYPE;
        strcpy(instr.opcode, opcode);
        sscanf(line, "%*s $%d, %d($%d)", &instr.rt, &instr.immediate, &instr.rs);
    } else if (strcmp(opcode, "j") == 0) {
        instr.type = J_TYPE;
        strcpy(instr.opcode, opcode);
        sscanf(line, "%*s %d", &instr.address);
    }
    return instr;
}

void fetch_stage() {
    if (pc < instruction_count && MEM.instr.type == EMPTY_TYPE) {
        IF.instr = instructions[pc++];
    } else {
        IF.instr.type = EMPTY_TYPE;
    }
}

void decode_stage() {
    if (ID.cycle_count > 0) {
        ID.cycle_count--;
        return;
    }
    ID = IF;
    ID.cycle_count = 1; // Take 2 cycles total
}

void execute_stage() {
    if (EX.cycle_count > 0) {
        EX.cycle_count--;
        return;
    }
    EX = ID;
    EX.cycle_count = 1; // Take 2 cycles total

    if (EX.instr.type == R_TYPE) {
        if (strcmp(EX.instr.opcode, "add") == 0) {
            EX.result = registers[EX.instr.rs] + registers[EX.instr.rt];
        } else if (strcmp(EX.instr.opcode, "sub") == 0) {
            EX.result = registers[EX.instr.rs] - registers[EX.instr.rt];
        }
    } else if (EX.instr.type == I_TYPE) {
        if (strcmp(EX.instr.opcode, "addi") == 0) {
            EX.result = registers[EX.instr.rs] + EX.instr.immediate;
        } else if (strcmp(EX.instr.opcode, "lw") == 0 || strcmp(EX.instr.opcode, "sw") == 0) {
            EX.result = registers[EX.instr.rs] + EX.instr.immediate; // effective address
        }
    }
}

void memory_stage() {
    MEM = EX;

    if (MEM.instr.type == I_TYPE) {
        if (strcmp(MEM.instr.opcode, "lw") == 0) {
            if (MEM.result >= 0 && MEM.result < MEMORY_SIZE)
                MEM.result = memory[MEM.result];
        } else if (strcmp(MEM.instr.opcode, "sw") == 0) {
            if (MEM.result >= 0 && MEM.result < MEMORY_SIZE)
                memory[MEM.result] = registers[MEM.instr.rt];
        }
    }
}

void write_back_stage() {
    WB = MEM;

    if (WB.instr.type == R_TYPE) {
        registers[WB.instr.rd] = WB.result;
    } else if (WB.instr.type == I_TYPE) {
        if (strcmp(WB.instr.opcode, "addi") == 0 || strcmp(WB.instr.opcode, "lw") == 0) {
            registers[WB.instr.rt] = WB.result;
        }
    }
}

void print_state(int cycle) {
    printf("\n=== Cycle %d ===\n", cycle);
    printf("PC: %d\n", pc);
    printf("Registers: ");
    for (int i = 0; i < 8; i++) printf("$%d=%d ", i, registers[i]);
    printf("...\n");
}

int main() {
    char line[100];
    while (fgets(line, sizeof(line), stdin)) {
        instructions[instruction_count++] = parse_instruction(line);
    }

    int cycle = 0;
    while (pc < instruction_count || IF.instr.type != EMPTY_TYPE || ID.instr.type != EMPTY_TYPE || EX.instr.type != EMPTY_TYPE || MEM.instr.type != EMPTY_TYPE || WB.instr.type != EMPTY_TYPE) {
        write_back_stage();
        memory_stage();
        execute_stage();
        decode_stage();
        fetch_stage();
        print_state(++cycle);
    }

    printf("\nFinal Register State:\n");
    for (int i = 0; i < REGISTER_COUNT; i++) {
        printf("$%d: %d\n", i, registers[i]);
    }

    printf("\nFinal Memory State (non-zero):\n");
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i] != 0)
            printf("MEM[%d]: %d\n", i, memory[i]);
    }
    return 0;
}