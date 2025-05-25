#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define MEMORY_SIZE 2048
#define REGISTER_COUNT 32
#define MAX_INSTRUCTIONS 1024
#define MAX_LINE_LENGTH 100

#define IMM_MIN (-32768)  // 16-bit signed immediate minimum
#define IMM_MAX 32767     // 16-bit signed immediate maximum
#define SHIFT_MAX 31      // 5-bit unsigned shift amount maximum

int32_t memory[MEMORY_SIZE];
int32_t registers[REGISTER_COUNT];
int32_t PC = 0;
int clock_cycle = 1;
int last_fetch_cycle = -2;  // Ensures fetch occurs at cycle 1

enum {
    ADD, SUB, MUL, MOVI, JEQ, AND, XORI, JMP, LSL, LSR, MOVR, MOVM
};

const char* OPCODE_NAMES[] = {
    "ADD", "SUB", "MUL", "MOVI", "JEQ", "AND", "XORI", "JMP",
    "LSL", "LSR", "MOVR", "MOVM"
};

typedef struct {
    int opcode;
    int r1, r2, r3;
    int immediate;
    int address;
    char type;
    int valid;
} Instruction;
//should include shamt for left and right shift instructions

typedef struct {
    Instruction inst;
    int active;
    int stage_time;
} PipelineSlot;

Instruction instruction_memory[MAX_INSTRUCTIONS];
PipelineSlot pipeline[5];

void print_registers_state() {
    printf("Registers:\n");
    for (int i = 0; i < REGISTER_COUNT; i++) {
        printf("R%d = %d\n", i, registers[i]);
    }
    printf("PC = %d\n", PC);
}

void print_memory_changes(int before[], int after[]) {
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (before[i] != after[i]) {
            printf("MEM[%d] changed to %d\n", i, after[i]);
        }
    }
}

void print_register_changes(int32_t before[], int32_t after[]) {
    for (int i = 1; i < REGISTER_COUNT; i++) {
        if (before[i] != after[i]) {
            printf("R%d changed to %d\n", i, after[i]);
        }
    }
}

void print_pipeline_state() {
    const char* stage_names[5] = {"IF", "ID", "EX", "MEM", "WB"};
    printf("\nClock Cycle %d:\n", clock_cycle);
    for (int i = 0; i < 5; i++) {
        if (pipeline[i].active) {
            Instruction inst = pipeline[i].inst;
            printf("Stage %s: %s R%d R%d R%d IMM=%d\n",
                   stage_names[i], OPCODE_NAMES[inst.opcode], inst.r1, inst.r2, inst.r3, inst.immediate);
        } else {
            printf("Stage %s: --\n", stage_names[i]);
        }
    }
}

int saving_execute = 0;

void execute(Instruction inst) {
    switch (inst.opcode) {
        case ADD: saving_execute = registers[inst.r2] + registers[inst.r3]; break;
        case SUB: saving_execute = registers[inst.r2] - registers[inst.r3]; break;
        case MUL: saving_execute = registers[inst.r2] * registers[inst.r3]; break;
        case MOVI: saving_execute = inst.immediate; break;
        case JEQ: if (registers[inst.r1] == registers[inst.r2]) PC += inst.immediate; break;
        case AND: saving_execute = registers[inst.r2] & registers[inst.r3]; break;
        case XORI: 
            saving_execute = registers[inst.r2] ^ inst.immediate;
            // Debug print to verify values
            printf("Debug: XORI: R%d(%d) ^ %d = %d\n", 
                inst.r1, 
                registers[inst.r1], 
                inst.immediate, 
                saving_execute);
            break;
        case JMP: PC = inst.address; break;
        case LSL: saving_execute = registers[inst.r2] << inst.immediate; break;
        case LSR: saving_execute = (unsigned int)registers[inst.r2] >> inst.immediate; break;
        // Remove MOVR and MOVM from here as they'll be handled in memory stage
    }
}

void memory_stage(Instruction inst) {
    switch (inst.opcode) {
        case MOVR: {
            int addr = registers[inst.r2] + inst.immediate;
            if (addr >= 0 && addr < MEMORY_SIZE) {
                saving_execute = memory[addr];
            } else {
                printf("Memory access error: address %d out of bounds\n", addr);
            }
            break;
        }
        case MOVM: {
            int addr = registers[inst.r2] + inst.immediate;
            if (addr >= 0 && addr < MEMORY_SIZE) {
                memory[addr] = registers[inst.r1];
            } else {
                printf("Memory access error: address %d out of bounds\n", addr);
            }
            break;
        }
    }
}

void write_back(Instruction inst) {
    // Don't write back for MOVM or jumps
    if (inst.opcode != MOVM && inst.opcode != JMP && inst.opcode != JEQ) {
        registers[inst.r1] = saving_execute;
    }
    registers[0] = 0;  // Enforce zero register
}

void advance_pipeline() {
    // Clear WB after one cycle
    if (pipeline[4].active && pipeline[4].stage_time >= 1) {
        write_back(pipeline[4].inst);  // Add write_back here
        pipeline[4].active = 0;
    }

    // Move MEM to WB after one cycle
    if (pipeline[3].active && pipeline[3].stage_time >= 1) {
        if (!pipeline[4].active) {
            pipeline[4] = pipeline[3];
            pipeline[4].stage_time = 0;
            pipeline[3].active = 0;
        }
    }

    // Move EX to MEM after two cycles
    if (pipeline[2].active && pipeline[2].stage_time >= 2) {
        if (!pipeline[3].active) {
            pipeline[3] = pipeline[2];
            pipeline[3].stage_time = 0;
            pipeline[2].active = 0;
            execute(pipeline[3].inst);      // Execute first
            memory_stage(pipeline[3].inst); // Then do memory operations
        }
    }

    // Move ID to EX after two cycles
    if (pipeline[1].active && pipeline[1].stage_time >= 2) {
        if (!pipeline[2].active) {
            pipeline[2] = pipeline[1];
            pipeline[2].stage_time = 0;
            pipeline[1].active = 0;
        }
    }

    // Move IF to ID after one cycle
    if (pipeline[0].active && pipeline[0].stage_time >= 1) {
        if (!pipeline[1].active) {
            pipeline[1] = pipeline[0];
            pipeline[1].stage_time = 0;
            pipeline[0].active = 0;
        }
    }

    // Fetch new instruction every 2 cycles starting from cycle 1
    if (!pipeline[0].active && !pipeline[3].active &&  // Check MEM isn't active
        ((clock_cycle - 1) % 2 == 0) &&               // Fetch on cycles 1,3,5,...
        PC < MAX_INSTRUCTIONS && 
        instruction_memory[PC].valid) {
        pipeline[0].inst = instruction_memory[PC++];
        pipeline[0].active = 1;
        pipeline[0].stage_time = 0;
    }

    // Increment stage times for active stages
    for (int i = 0; i < 5; i++) {
        if (pipeline[i].active) {
            pipeline[i].stage_time++;
        }
    }
}

int is_comment_or_empty(const char* line) {
    while (*line) {
        if (isspace(*line)) {
            line++;
            continue;
        }
        return *line == '#' || *line == '\0' || *line == '\n';
    }
    return 1;
}

void parse_instruction(char *line, int index) {
    Instruction inst = {0};  // Initialize all fields to 0
    char mnemonic[10];
    char r1_str[10], r2_str[10], r3_str[10];
    int r1 = 0, r2 = 0, r3 = 0, imm = 0;

    // Remove newline if present
    char *newline = strchr(line, '\n');
    if (newline) *newline = 0;

    // Initialize strings
    memset(mnemonic, 0, sizeof(mnemonic));
    memset(r1_str, 0, sizeof(r1_str));
    memset(r2_str, 0, sizeof(r2_str));
    memset(r3_str, 0, sizeof(r3_str));

    // Parse instruction type
    if (sscanf(line, "%s", mnemonic) != 1) {
        printf("Error: Could not parse instruction: %s\n", line);
        return;
    }

    // R-type instructions (ADD, SUB, MUL, AND)
    if (strcmp(mnemonic, "ADD") == 0 || strcmp(mnemonic, "SUB") == 0 || 
        strcmp(mnemonic, "MUL") == 0 || strcmp(mnemonic, "AND") == 0) {
        
        if (sscanf(line, "%*s %[^,], %[^,], %s", r1_str, r2_str, r3_str) == 3) {
            r1 = atoi(r1_str + 1);  // Skip 'R' prefix
            r2 = atoi(r2_str + 1);
            r3 = atoi(r3_str + 1);
            
            inst.type = 'R';
            inst.r1 = r1;
            inst.r2 = r2;
            inst.r3 = r3;
            inst.valid = 1;
            
            if (strcmp(mnemonic, "ADD") == 0) inst.opcode = ADD;
            else if (strcmp(mnemonic, "SUB") == 0) inst.opcode = SUB;
            else if (strcmp(mnemonic, "MUL") == 0) inst.opcode = MUL;
            else if (strcmp(mnemonic, "AND") == 0) inst.opcode = AND;
        }
    }
    // I-type instructions (MOVI, XORI, LSL, LSR)
    else if (strcmp(mnemonic, "MOVI") == 0 || strcmp(mnemonic, "XORI") == 0) {
        // For MOVI: format is "MOVI Rx, imm"
        // For XORI: format is "XORI Rx, Ry, imm"
        if (strcmp(mnemonic, "MOVI") == 0) {
            if (sscanf(line, "%*s %[^,], %d", r1_str, &imm) == 2) {
                r1 = atoi(r1_str + 1);
                inst.type = 'I';
                inst.r1 = r1;
                inst.immediate = imm;
                inst.valid = 1;
                inst.opcode = MOVI;
            }
        } else { // XORI case
            if (sscanf(line, "%*s %[^,], %[^,], %d", r1_str, r2_str, &imm) == 3) {
                r1 = atoi(r1_str + 1);
                r2 = atoi(r2_str + 1);
                inst.type = 'I';
                inst.r1 = r1;
                inst.r2 = r2;
                inst.immediate = imm;
                inst.valid = 1;
                inst.opcode = XORI;
                
                // Debug print
                printf("Debug: Parsed XORI: R%d, R%d, %d\n", inst.r1, inst.r2, inst.immediate);
            } else {
                printf("Error: Invalid XORI format (expected 'XORI Rx, Ry, imm'): %s\n", line);
                inst.valid = 0;
            }
        }

        // Immediate value validation for MOVI and XORI
        if (imm < IMM_MIN || imm > IMM_MAX) {
            printf("Error: Immediate value %d out of range [%d, %d] in instruction: %s\n", 
                   imm, IMM_MIN, IMM_MAX, line);
            inst.valid = 0;
            return;
        }
    }
    // LSL, LSR instructions
    else if (strcmp(mnemonic, "LSL") == 0 || strcmp(mnemonic, "LSR") == 0) {
        if (sscanf(line, "%*s %[^,], %[^,], %d", r1_str, r2_str, &imm) == 3) {
            r1 = atoi(r1_str + 1);
            r2 = atoi(r2_str + 1);
            
            inst.type = 'I';
            inst.r1 = r1;
            inst.r2 = r2;
            inst.immediate = imm;
            inst.valid = 1;
            
            if (strcmp(mnemonic, "LSL") == 0) inst.opcode = LSL;
            else if (strcmp(mnemonic, "LSR") == 0) inst.opcode = LSR;
        }

        // Immediate value validation for LSL and LSR
        if (imm < 0 || imm > SHIFT_MAX) {
            printf("Error: Shift amount %d out of range [0, %d] in instruction: %s\n", 
                   imm, SHIFT_MAX, line);
            inst.valid = 0;
            return;
        }
    }
    // Memory instructions (MOVR, MOVM)
    else if (strcmp(mnemonic, "MOVR") == 0 || strcmp(mnemonic, "MOVM") == 0) {
        if (sscanf(line, "%*s %[^,], %[^,], %d", r1_str, r2_str, &imm) == 3) {
            r1 = atoi(r1_str + 1);
            r2 = atoi(r2_str + 1);
            
            inst.type = 'I';
            inst.r1 = r1;
            inst.r2 = r2;
            inst.immediate = imm;
            inst.valid = 1;
            
            if (strcmp(mnemonic, "MOVR") == 0) inst.opcode = MOVR;
            else if (strcmp(mnemonic, "MOVM") == 0) inst.opcode = MOVM;
        }

        // Memory offset validation for MOVR and MOVM
        if (imm < IMM_MIN || imm > IMM_MAX) {
            printf("Error: Memory offset %d out of range [%d, %d] in instruction: %s\n", 
                   imm, IMM_MIN, IMM_MAX, line);
            inst.valid = 0;
            return;
        }
    }
    // Jump instructions (JMP, JEQ)
    else if (strcmp(mnemonic, "JMP") == 0) {
        if (sscanf(line, "%*s %d", &imm) == 1) {
            inst.type = 'J';
            inst.address = imm;
            inst.valid = 1;
            inst.opcode = JMP;
        }
    }
    else if (strcmp(mnemonic, "JEQ") == 0) {
        if (sscanf(line, "%*s %[^,], %[^,], %d", r1_str, r2_str, &imm) == 3) {
            r1 = atoi(r1_str + 1);
            r2 = atoi(r2_str + 1);
            
            inst.type = 'I';
            inst.r1 = r1;
            inst.r2 = r2;
            inst.immediate = imm;
            inst.valid = 1;
            inst.opcode = JEQ;
        }

        // Branch offset validation for JEQ
        if (imm < IMM_MIN || imm > IMM_MAX) {
            printf("Error: Branch offset %d out of range [%d, %d] in instruction: %s\n", 
                   imm, IMM_MIN, IMM_MAX, line);
            inst.valid = 0;
            return;
        }
    }

    // Validate register numbers
    if (inst.valid) {
        if (inst.r1 < 0 || inst.r1 >= REGISTER_COUNT ||
            inst.r2 < 0 || inst.r2 >= REGISTER_COUNT ||
            inst.r3 < 0 || inst.r3 >= REGISTER_COUNT) {
            printf("Error: Invalid register number in instruction: %s\n", line);
            inst.valid = 0;
        }
    }

    // Debug information for parsed instructions
    if (inst.valid) {
        printf("\nDebug: Parsed instruction %d:\n", index);
        printf("  Mnemonic: %s\n", mnemonic);
        printf("  Type: %c\n", inst.type);
        printf("  Opcode: %s\n", OPCODE_NAMES[inst.opcode]);
        
        switch (inst.type) {
            case 'R':
                printf("  Registers: R%d, R%d, R%d\n", inst.r1, inst.r2, inst.r3);
                break;
            case 'I':
                if (inst.opcode == MOVI || inst.opcode == XORI) {
                    printf("  Register: R%d, Immediate: %d\n", inst.r1, inst.immediate);
                } else if (inst.opcode == LSL || inst.opcode == LSR) {
                    printf("  Registers: R%d, R%d, Shift: %d\n", inst.r1, inst.r2, inst.immediate);
                } else if (inst.opcode == MOVR || inst.opcode == MOVM) {
                    printf("  Registers: R%d, R%d, Offset: %d\n", inst.r1, inst.r2, inst.immediate);
                }
                break;
            case 'J':
                printf("  Jump Address: %d\n", inst.address);
                break;
        }
        printf("  Original line: %s\n", line);
    } else {
        printf("\nDebug: Invalid instruction at index %d: %s\n", index, line);
    }

    instruction_memory[index] = inst;
}

int main() {
    FILE *file = fopen("asm.txt", "r");
    if (!file) {
        printf("Error opening asm.txt\n");
        return 1;
    }

    // Register initial values (you can modify)
    // registers[2] = 5;
    // registers[3] = 10;
    // registers[4] = 2;
    // registers[11] = 4;

    char line[MAX_LINE_LENGTH];
    int index = 0;
    while (fgets(line, sizeof(line), file) && index < MAX_INSTRUCTIONS) {
        if (is_comment_or_empty(line)) continue;
        parse_instruction(line, index++);
    }
    fclose(file);

    int total_instructions = index;
    int total_cycles = 7 + (total_instructions - 1) * 2;  // Increased cycles to ensure completion

    while (clock_cycle <= total_cycles && (PC < total_instructions || 
           pipeline[0].active || pipeline[1].active || 
           pipeline[2].active || pipeline[3].active || pipeline[4].active)) {

        // Add debug print
        printf("\nDebug: PC=%d, total_inst=%d, Pipeline states: %d %d %d %d %d\n", 
               PC, total_instructions,
               pipeline[0].active, pipeline[1].active, pipeline[2].active,
               pipeline[3].active, pipeline[4].active);
    
        int32_t reg_snapshot[REGISTER_COUNT];
        int mem_snapshot[MEMORY_SIZE];
        memcpy(reg_snapshot, registers, sizeof(registers));
        memcpy(mem_snapshot, memory, sizeof(memory));

        print_pipeline_state();
        advance_pipeline();
        print_register_changes(reg_snapshot, registers);
        print_memory_changes(mem_snapshot, memory);
        print_registers_state();
        clock_cycle++;
    }

    printf("\nFinal State:\n");
    print_registers_state();
    printf("\nMemory (non-zero values):\n");
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i] != 0)
            printf("MEM[%d] = %d\n", i, memory[i]);
    }

    return 0;
}


//immediate value validation for MOVI and XORI
// memory offset validation for MOVR and MOVM
// branch offset validation for JEQ
// shift amount validation for LSL and LSR