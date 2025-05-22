#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_REGISTERS 32
#define NUM_STAGES 5
#define MAX_INSTRUCTIONS 1024
#define LINE_LENGTH 100

// Pipeline stage names
enum Stage {FETCH, DECODE, EXECUTE, MEMORY, WRITEBACK};
const char* stage_names[] = {"Fetch", "Decode", "Execute", "Memory", "Writeback"};

// Structure for instruction
typedef struct {
    char opcode[10];
    int rd, rs, rt;
    int valid; // 0 = empty stage, 1 = instruction present
} Instruction;

// Registers
int registers[NUM_REGISTERS];

// Pipeline stages
Instruction pipeline[NUM_STAGES];

// Instruction memory
Instruction instruction_memory[MAX_INSTRUCTIONS];
int instruction_count = 0;

// PC and cycle counter
int pc = 0;
int cycles = 0;

void print_registers() {
    printf("Final Register Values:\n");
    for (int i = 0; i < NUM_REGISTERS; i++) {
        printf("R%-2d = %d\n", i, registers[i]);
    }
    printf("\n");
}

void print_pipeline() {
    printf("Cycle %d:\n", cycles);
    for (int i = 0; i < NUM_STAGES; i++) {
        if (pipeline[i].valid)
            printf("  %-9s: %s R%d, R%d, R%d\n", stage_names[i], pipeline[i].opcode, pipeline[i].rd, pipeline[i].rs, pipeline[i].rt);
        else
            printf("  %-9s: Empty\n", stage_names[i]);
    }
    printf("\n");
}

void writeback_stage() {
    if (!pipeline[WRITEBACK].valid) return;

    Instruction inst = pipeline[WRITEBACK];
    if (strcmp(inst.opcode, "ADD") == 0)
        registers[inst.rd] = registers[inst.rs] + registers[inst.rt];
    else if (strcmp(inst.opcode, "SUB") == 0)
        registers[inst.rd] = registers[inst.rs] - registers[inst.rt];

    printf("Writeback: %s R%d, R%d, R%d => R%d = %d\n",
           inst.opcode, inst.rd, inst.rs, inst.rt,
           inst.rd, registers[inst.rd]);
}

void advance_pipeline() {
    // Shift pipeline stages (right to left)
    for (int i = NUM_STAGES - 1; i > 0; i--) {
        pipeline[i] = pipeline[i - 1];
    }

    // Fetch next instruction
    if (pc < instruction_count) {
        pipeline[FETCH] = instruction_memory[pc++];
    } else {
        pipeline[FETCH].valid = 0;
    }
}

int is_pipeline_empty() {
    for (int i = 0; i < NUM_STAGES; i++) {
        if (pipeline[i].valid) return 0;
    }
    return 1;
}

void load_instructions_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    char line[LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        Instruction inst;
        char* opcode = strtok(line, " \t\n");
        if (!opcode) continue;

        if (strcmp(opcode, "ADD") != 0 && strcmp(opcode, "SUB") != 0) {
            printf("Unsupported opcode '%s', skipping: %s", opcode, line);
            continue;
        }

        char* rd_str = strtok(NULL, ", \t\n");
        char* rs_str = strtok(NULL, ", \t\n");
        char* rt_str = strtok(NULL, ", \t\n");

        if (!rd_str || !rs_str || !rt_str ||
            rd_str[0] != 'R' || rs_str[0] != 'R' || rt_str[0] != 'R') {
            printf("Skipping invalid instruction format: %s%s%s", opcode, rd_str ? " " : "", rd_str ? rd_str : "");
            if (rs_str) printf(", %s", rs_str);
            if (rt_str) printf(", %s", rt_str);
            printf("\n");
            continue;
        }

        strcpy(inst.opcode, opcode);
        inst.rd = atoi(rd_str + 1);
        inst.rs = atoi(rs_str + 1);
        inst.rt = atoi(rt_str + 1);
        inst.valid = 1;

        instruction_memory[instruction_count++] = inst;
    }

    fclose(file);
}

int main() {
    // Initialize registers with sample values
    registers[1] = 5;
    registers[2] = 10;
    registers[3] = 15;
    registers[4] = 20;
    registers[5] = 3;
    registers[6] = 0;
    registers[7] = 0;

    // Clear pipeline
    memset(pipeline, 0, sizeof(pipeline));

    // Load instructions from file
    load_instructions_from_file("asm.txt");

    // Run pipeline simulation
    while (!is_pipeline_empty() || pc < instruction_count) {
        cycles++;
        writeback_stage();
        print_pipeline();
        advance_pipeline();
    }

    print_registers();
    return 0;
}
