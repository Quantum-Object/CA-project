#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "cJSON.h" // You'd need to add a C JSON library like cJSON

#define DATA_MEMORY_SIZE 1024     // Data memory: addresses 0-1023
#define INSTRUCTION_MEMORY_SIZE 1024  // Instruction memory: addresses 0-1023
#define REGISTER_COUNT 32
#define MAX_INSTRUCTIONS 1024
#define MAX_LINE_LENGTH 100

#define IMM_MIN (-32768)  // 16-bit signed immediate minimum
#define IMM_MAX 32767     // 16-bit signed immediate maximum
#define SHIFT_MAX 31      // 5-bit unsigned shift amount maximum

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

typedef struct {
    Instruction inst;
    int active;
    int cycles_in_stage;  // Track cycles spent in current stage
} PipelineSlot;

// Separate memory spaces - declare AFTER the structs are defined
int32_t data_memory[DATA_MEMORY_SIZE];        // Data memory: 0-1023
Instruction instruction_memory[INSTRUCTION_MEMORY_SIZE];  // Instruction memory: 0-1023

int32_t registers[REGISTER_COUNT];
int32_t PC = 0;
int clock_cycle = 1;
int total_instructions = 0;
int saving_execute = 0;

PipelineSlot pipeline[5];

// Memory access functions with proper segmentation
int32_t read_data_memory(int address) {
    if (address < 0 || address >= DATA_MEMORY_SIZE) {
        fprintf(stderr, "Error: Data memory address %d out of bounds [0-%d]\\n", address, DATA_MEMORY_SIZE-1);
        return 0;  // Return 0 for invalid access
    }
    return data_memory[address];
}

void write_data_memory(int address, int32_t value) {
    if (address < 0 || address >= DATA_MEMORY_SIZE) {
        fprintf(stderr, "Error: Data memory address %d out of bounds [0-%d]\\n", address, DATA_MEMORY_SIZE-1);
        return;  // Don't write for invalid access
    }
    data_memory[address] = value;
}

Instruction read_instruction_memory(int address) {
    if (address < 0 || address >= INSTRUCTION_MEMORY_SIZE) {
        fprintf(stderr, "Error: Instruction memory address %d out of bounds [0-%d]\\n", address, INSTRUCTION_MEMORY_SIZE-1);
        Instruction invalid = {0};  // Return invalid instruction
        return invalid;
    }
    return instruction_memory[address];
}

void write_instruction_memory(int address, Instruction inst) {
    if (address < 0 || address >= INSTRUCTION_MEMORY_SIZE) {
        fprintf(stderr, "Error: Instruction memory address %d out of bounds [0-%d]\\n", address, INSTRUCTION_MEMORY_SIZE-1);
        return;
    }
    instruction_memory[address] = inst;
}

void print_registers_state() {
    fprintf(stderr, "Registers:\\n");
    for (int i = 0; i < REGISTER_COUNT; i++) {
        fprintf(stderr, "R%d = %d\\n", i, registers[i]);
    }
    fprintf(stderr, "PC = %d\\n", PC);
}

void print_memory_state() {
    fprintf(stderr, "\\nData Memory (non-zero values):\\n");
    int found_data = 0;
    for (int i = 0; i < DATA_MEMORY_SIZE; i++) {
        if (data_memory[i] != 0) {
            fprintf(stderr, "DATA[%d] = %d\\n", i, data_memory[i]);
            found_data = 1;
        }
    }
    if (!found_data) {
        fprintf(stderr, "All data memory locations are zero\\n");
    }
    
    fprintf(stderr, "\\nInstruction Memory (loaded instructions):\\n");
    for (int i = 0; i < total_instructions; i++) {
        if (instruction_memory[i].valid) {
            fprintf(stderr, "INST[%d]: %s", i, OPCODE_NAMES[instruction_memory[i].opcode]);
            fprintf(stderr, "\\n");
        }
    }
}

void print_register_changes(int32_t before[], int32_t after[]) {
    for (int i = 1; i < REGISTER_COUNT; i++) {
        if (before[i] != after[i]) {
            fprintf(stderr, "R%d changed to %d\\n", i, after[i]);
        }
    }
}

void print_pipeline_state() {
    const char* stage_names[5] = {"IF", "ID", "EX", "MEM", "WB"};
    fprintf(stderr, "\\nClock Cycle %d:\\n", clock_cycle - 1);
    for (int i = 0; i < 5; i++) {
        if (pipeline[i].active) {
            Instruction inst = pipeline[i].inst;
            fprintf(stderr, "Stage %s: ", stage_names[i]);
            
            // Print instruction details
            if (inst.opcode == JMP) {
                fprintf(stderr, "%s %d", OPCODE_NAMES[inst.opcode], inst.address);
            } else if (inst.opcode == MOVI) {
                fprintf(stderr, "%s R%d, %d", OPCODE_NAMES[inst.opcode], inst.r1, inst.immediate);
            } else if (inst.opcode == JEQ) {
                fprintf(stderr, "%s R%d, R%d, %d", OPCODE_NAMES[inst.opcode], inst.r1, inst.r2, inst.immediate);
            } else if (inst.type == 'R') {
                fprintf(stderr, "%s R%d, R%d, R%d", OPCODE_NAMES[inst.opcode], inst.r1, inst.r2, inst.r3);
            } else {
                fprintf(stderr, "%s R%d, R%d, %d", OPCODE_NAMES[inst.opcode], inst.r1, inst.r2, inst.immediate);
            }
            
            // Print input values for each stage
            switch (i) {
                case 0: // IF
                    fprintf(stderr, " (Fetching from PC=%d)", PC-1);
                    break;
                case 1: // ID
                    fprintf(stderr, " (Decoding)");
                    break;
                case 2: // EX
                    if (inst.type == 'R') {
                        fprintf(stderr, " (R%d=%d, R%d=%d)", inst.r2, registers[inst.r2], inst.r3, registers[inst.r3]);
                    } else if (inst.opcode == MOVI) {
                        fprintf(stderr, " (IMM=%d)", inst.immediate);
                    } else if (inst.opcode == JEQ) {
                        fprintf(stderr, " (R%d=%d, R%d=%d)", inst.r1, registers[inst.r1], inst.r2, registers[inst.r2]);
                    } else {
                        fprintf(stderr, " (R%d=%d, IMM=%d)", inst.r2, registers[inst.r2], inst.immediate);
                    }
                    break;
                case 3: // MEM
                    if (inst.opcode == MOVR) {
                        int addr = registers[inst.r2] + inst.immediate;
                        fprintf(stderr, " (Addr=%d, DATA[%d]=%d)", addr, addr, (addr >= 0 && addr < DATA_MEMORY_SIZE) ? data_memory[addr] : -1);
                    } else if (inst.opcode == MOVM) {
                        int addr = registers[inst.r2] + inst.immediate;
                        fprintf(stderr, " (Addr=%d, Store R%d=%d)", addr, inst.r1, registers[inst.r1]);
                    } else {
                        fprintf(stderr, " (Pass-through)");
                    }
                    break;
                case 4: // WB
                    if (inst.opcode != MOVM && inst.opcode != JMP && inst.opcode != JEQ) {
                        fprintf(stderr, " (Writing %d to R%d)", saving_execute, inst.r1);
                    } else {
                        fprintf(stderr, " (No writeback)");
                    }
                    break;
            }
            fprintf(stderr, "\\n");
        } else {
            fprintf(stderr, "Stage %s: --\\n", stage_names[i]);
        }
    }
}

void execute(Instruction inst) {
    switch (inst.opcode) {
        case ADD: 
            saving_execute = registers[inst.r2] + registers[inst.r3];
            fprintf(stderr, "EX Stage: ADD result = %d + %d = %d\\n", registers[inst.r2], registers[inst.r3], saving_execute);
            break;
        case SUB: 
            saving_execute = registers[inst.r2] - registers[inst.r3];
            fprintf(stderr, "EX Stage: SUB result = %d - %d = %d\\n", registers[inst.r2], registers[inst.r3], saving_execute);
            break;
        case MUL: 
            saving_execute = registers[inst.r2] * registers[inst.r3];
            fprintf(stderr, "EX Stage: MUL result = %d * %d = %d\\n", registers[inst.r2], registers[inst.r3], saving_execute);
            break;
        case MOVI: 
            saving_execute = inst.immediate;
            fprintf(stderr, "EX Stage: MOVI result = %d\\n", saving_execute);
            break;
        case JEQ: 
            fprintf(stderr, "EX Stage: JEQ comparing R%d=%d with R%d=%d\\n", inst.r1, registers[inst.r1], inst.r2, registers[inst.r2]);
            if (registers[inst.r1] == registers[inst.r2]) {
                fprintf(stderr, "EX Stage: Branch taken, PC changed from %d to %d\\n", PC, inst.immediate);
                PC = inst.immediate;  // Direct jump to absolute address, not relative
            } else {
                fprintf(stderr, "EX Stage: Branch not taken\\n");
            }
            break;
        case AND: 
            saving_execute = registers[inst.r2] & registers[inst.r3];
            fprintf(stderr, "EX Stage: AND result = %d & %d = %d\\n", registers[inst.r2], registers[inst.r3], saving_execute);
            break;
        case XORI: 
            saving_execute = registers[inst.r2] ^ inst.immediate;
            fprintf(stderr, "EX Stage: XORI result = %d ^ %d = %d\\n", registers[inst.r2], inst.immediate, saving_execute);
            break;
        case JMP: 
            fprintf(stderr, "EX Stage: JMP, PC changed from %d to %d\\n", PC, inst.address);
            PC = inst.address;
            break;
        case LSL: 
            saving_execute = registers[inst.r2] << inst.immediate;
            fprintf(stderr, "EX Stage: LSL result = %d << %d = %d\\n", registers[inst.r2], inst.immediate, saving_execute);
            break;
        case LSR: 
            saving_execute = (unsigned int)registers[inst.r2] >> inst.immediate;
            fprintf(stderr, "EX Stage: LSR result = %d >> %d = %d\\n", registers[inst.r2], inst.immediate, saving_execute);
            break;
    }
}

void memory_stage(Instruction inst) {
    switch (inst.opcode) {
        case MOVR: {
            int addr = registers[inst.r2] + inst.immediate;
            int32_t value = read_data_memory(addr);
            if (addr >= 0 && addr < DATA_MEMORY_SIZE) {
                saving_execute = value;
                fprintf(stderr, "MEM Stage: Loading DATA[%d]=%d\\n", addr, value);
            }
            break;
        }
        case MOVM: {
            int addr = registers[inst.r2] + inst.immediate;
            if (addr >= 0 && addr < DATA_MEMORY_SIZE) {
                int32_t old_value = read_data_memory(addr);
                fprintf(stderr, "MEM Stage: DATA[%d] changed from %d to %d\\n", addr, old_value, registers[inst.r1]);
                write_data_memory(addr, registers[inst.r1]);
            }
            break;
        }
    }
}

void write_back(Instruction inst) {
    // Don't write back for MOVM or jumps
    if (inst.opcode != MOVM && inst.opcode != JMP && inst.opcode != JEQ) {
        fprintf(stderr, "WB Stage: R%d changed from %d to %d\\n", inst.r1, registers[inst.r1], saving_execute);
        registers[inst.r1] = saving_execute;
    }
    if (registers[0] != 0) {
        fprintf(stderr, "WB Stage: R0 enforced to 0 (was %d)\\n", registers[0]);
        registers[0] = 0;  // Enforce zero register
    }
}

void advance_pipeline() {
    // Increment cycle counters for all active stages first
    for (int i = 0; i < 5; i++) {
        if (pipeline[i].active) {
            pipeline[i].cycles_in_stage++;
        }
    }

    // Process stages in reverse order to prevent overwriting

    // Write-Back Stage (WB) - 1 cycle
    if (pipeline[4].active && pipeline[4].cycles_in_stage >= 1) {
        write_back(pipeline[4].inst);
        pipeline[4].active = 0;
        pipeline[4].cycles_in_stage = 0;
    }

    // Memory Stage (MEM) - 1 cycle
    if (pipeline[3].active && pipeline[3].cycles_in_stage >= 1) {
        memory_stage(pipeline[3].inst);
        if (!pipeline[4].active) {
            pipeline[4] = pipeline[3];
            pipeline[4].active = 1;
            pipeline[4].cycles_in_stage = 0;
        }
        pipeline[3].active = 0;
        pipeline[3].cycles_in_stage = 0;
    }

    // Execute Stage (EX) - 2 cycles
    if (pipeline[2].active && pipeline[2].cycles_in_stage >= 2) {
        // Store original PC before execute
        int original_PC = PC;
        
        // Execute the instruction
        execute(pipeline[2].inst);
        
        // Check if PC was changed by branch/jump instruction
        int pc_changed = (PC != original_PC);
        
        // Move instruction to memory stage
        if (!pipeline[3].active) {
            pipeline[3] = pipeline[2];
            pipeline[3].active = 1;
            pipeline[3].cycles_in_stage = 0;
        }
        pipeline[2].active = 0;
        pipeline[2].cycles_in_stage = 0;
        
        // Flush pipeline if branch/jump was taken
        if (pc_changed) {
            fprintf(stderr, "Pipeline flush: Branch/Jump taken, flushing IF and ID stages\\n");
            
            // Clear IF stage (flush instruction that was fetched after branch)
            if (pipeline[0].active) {
                fprintf(stderr, "Flushing IF stage: %s\\n", OPCODE_NAMES[pipeline[0].inst.opcode]);
                pipeline[0].active = 0;
                pipeline[0].cycles_in_stage = 0;
            }
            
            // Clear ID stage (flush instruction that was decoded after branch)
            if (pipeline[1].active) {
                fprintf(stderr, "Flushing ID stage: %s\\n", OPCODE_NAMES[pipeline[1].inst.opcode]);
                pipeline[1].active = 0;
                pipeline[1].cycles_in_stage = 0;
            }
        }
    }

    // Decode Stage (ID) - 2 cycles
    if (pipeline[1].active && pipeline[1].cycles_in_stage >= 2) {
        if (!pipeline[2].active) {
            pipeline[2] = pipeline[1];
            pipeline[2].active = 1;
            pipeline[2].cycles_in_stage = 0;
        }
        pipeline[1].active = 0;
        pipeline[1].cycles_in_stage = 0;
    }

    // Instruction Fetch (IF) - Improved logic
    // Fetch on odd cycles when MEM is not active
    if (clock_cycle % 2 == 1 && !pipeline[3].active) {
        // Move IF to ID if possible
        if (pipeline[0].active && !pipeline[1].active) {
            pipeline[1] = pipeline[0];
            pipeline[1].active = 1;
            pipeline[1].cycles_in_stage = 0;
            pipeline[0].active = 0;
            pipeline[0].cycles_in_stage = 0;
        }
        
        // Fetch new instruction if we have a valid PC and empty IF stage
        if (!pipeline[0].active && PC >= 0 && PC < total_instructions) {
            Instruction inst = read_instruction_memory(PC);
            if (inst.valid) {
                pipeline[0].inst = inst;
                pipeline[0].active = 1;
                pipeline[0].cycles_in_stage = 0;
                fprintf(stderr, "Fetching instruction at PC=%d: %s\\n", PC, OPCODE_NAMES[pipeline[0].inst.opcode]);
                PC++;
            }
        }
    }
    // On even cycles, just move IF to ID if needed
    else if (pipeline[0].active && pipeline[0].cycles_in_stage >= 1 && !pipeline[1].active) {
        pipeline[1] = pipeline[0];
        pipeline[1].active = 1;
        pipeline[1].cycles_in_stage = 0;
        pipeline[0].active = 0;
        pipeline[0].cycles_in_stage = 0;
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
        fprintf(stderr, "Error: Could not parse instruction: %s\\n", line);
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
            } else {
                fprintf(stderr, "Error: Invalid XORI format (expected 'XORI Rx, Ry, imm'): %s\\n", line);
                inst.valid = 0;
            }
        }

        // Immediate value validation for MOVI and XORI
        if (imm < IMM_MIN || imm > IMM_MAX) {
            fprintf(stderr, "Error: Immediate value %d out of range [%d, %d] in instruction: %s\\n", 
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
            fprintf(stderr, "Error: Shift amount %d out of range [0, %d] in instruction: %s\\n", 
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
            fprintf(stderr, "Error: Memory offset %d out of range [%d, %d] in instruction: %s\\n", 
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
            fprintf(stderr, "Error: Branch offset %d out of range [%d, %d] in instruction: %s\\n", 
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
            fprintf(stderr, "Error: Invalid register number in instruction: %s\\n", line);
            inst.valid = 0;
        }
    }

    // Debug information for parsed instructions
    if (inst.valid) {
        fprintf(stderr, "\\nDebug: Parsed instruction %d:\\n", index);
        fprintf(stderr, "  Mnemonic: %s\\n", mnemonic);
        fprintf(stderr, "  Type: %c\\n", inst.type);
        fprintf(stderr, "  Opcode: %s\\n", OPCODE_NAMES[inst.opcode]);
        
        switch (inst.type) {
            case 'R':
                fprintf(stderr, "  Registers: R%d, R%d, R%d\\n", inst.r1, inst.r2, inst.r3);
                break;
            case 'I':
                if (inst.opcode == MOVI || inst.opcode == XORI) {
                    fprintf(stderr, "  Register: R%d, Immediate: %d\\n", inst.r1, inst.immediate);
                } else if (inst.opcode == LSL || inst.opcode == LSR) {
                    fprintf(stderr, "  Registers: R%d, R%d, Shift: %d\\n", inst.r1, inst.r2, inst.immediate);
                } else if (inst.opcode == MOVR || inst.opcode == MOVM) {
                    fprintf(stderr, "  Registers: R%d, R%d, Offset: %d\\n", inst.r1, inst.r2, inst.immediate);
                }
                break;
            case 'J':
                fprintf(stderr, "  Jump Address: %d\\n", inst.address);
                break;
        }
        fprintf(stderr, "  Original line: %s\\n", line);
    } else {
        fprintf(stderr, "\\nDebug: Invalid instruction at index %d: %s\\n", index, line);
    }

    // At the end, use the new function to store the instruction
    write_instruction_memory(index, inst);
}

// Simple function to print all loaded instructions
void print_loaded_instructions() {
    fprintf(stderr, "\\n=== LOADED INSTRUCTIONS ===\\n");
    for (int i = 0; i < total_instructions; i++) {
        if (instruction_memory[i].valid) {
            Instruction inst = instruction_memory[i];
            fprintf(stderr, "[%d] %s", i, OPCODE_NAMES[inst.opcode]);
            
            if (inst.opcode == JMP) {
                fprintf(stderr, " %d", inst.address);
            } else if (inst.opcode == MOVI) {
                fprintf(stderr, " R%d, %d", inst.r1, inst.immediate);
            } else if (inst.opcode == JEQ) {
                fprintf(stderr, " R%d, R%d, %d", inst.r1, inst.r2, inst.immediate);
            } else if (inst.type == 'R') {
                fprintf(stderr, " R%d, R%d, R%d", inst.r1, inst.r2, inst.r3);
            } else {
                fprintf(stderr, " R%d, R%d, %d", inst.r1, inst.r2, inst.immediate);
            }
            fprintf(stderr, "\\n");
        }
    }
}

// Simple function to print all memory contents
void print_all_memory_contents() {
    fprintf(stderr, "\\n=== MEMORY CONTENTS ===\\n");
    fprintf(stderr, "Data Memory (all addresses 0-%d):\\n", DATA_MEMORY_SIZE-1);
    
    for (int i = 0; i < DATA_MEMORY_SIZE; i++) {
        if (data_memory[i] != 0) {
            fprintf(stderr, "Address %d: %d\\n", i, data_memory[i]);
        } else {
            fprintf(stderr, "Address %d: 0\\n", i);
        }
        
        // Only show first 100 addresses to avoid too much output
        if (i >= 99) {
            fprintf(stderr, "... (remaining addresses all contain 0)\\n");
            break;
        }
    }
}

// Simple final registers display
void print_final_registers() {
    fprintf(stderr, "\\n=== FINAL REGISTERS ===\\n");
    for (int i = 0; i < REGISTER_COUNT; i++) {
        fprintf(stderr, "R%d = %d\\n", i, registers[i]);
    }
    fprintf(stderr, "PC = %d\\n", PC);
}

// Simple cycle info
void print_cycle_info() {
    fprintf(stderr, "\\n=== CYCLE DETAILS ===\\n");
    fprintf(stderr, "Total cycles executed: %d\\n", clock_cycle - 1);
    fprintf(stderr, "Instructions processed: %d\\n", total_instructions);
}

void run_simulation_and_output_json() {
    cJSON *root = cJSON_CreateObject();

    // Add register data
    cJSON *registers_json = cJSON_CreateArray();
    for (int i = 0; i < REGISTER_COUNT; i++) {
        cJSON_AddItemToArray(registers_json, cJSON_CreateNumber(registers[i]));
    }
    cJSON_AddItemToObject(root, "registers", registers_json);
    cJSON_AddNumberToObject(root, "PC", PC);

    // Add memory data (only non-zero for brevity, or as needed)
    cJSON *memory_json = cJSON_CreateObject();
    for (int i = 0; i < DATA_MEMORY_SIZE; i++) {
        if (data_memory[i] != 0) {
            char addr_str[10];
            sprintf(addr_str, "%d", i);
            cJSON_AddNumberToObject(memory_json, addr_str, data_memory[i]);
        }
    }
    cJSON_AddItemToObject(root, "dataMemory", memory_json);

    // Add pipeline state (this would be more complex, perhaps an array of objects per cycle)
    // For simplicity, let's just add final cycle info
    cJSON_AddNumberToObject(root, "finalClockCycle", clock_cycle -1);
    cJSON_AddNumberToObject(root, "totalInstructions", total_instructions);


    // ... (add other data like pipeline history, instruction memory etc.)

    char *json_string = cJSON_PrintUnformatted(root);
    printf("%s\n", json_string); // Print the single JSON string to stdout

    cJSON_Delete(root);
    free(json_string);
}

int main() {
    FILE *file = fopen("asm.txt", "r");
    if (!file) {
        fprintf(stderr, "Error opening asm.txt\\n");
        return 1;
    }

    // Initialize memory spaces
    memset(data_memory, 0, sizeof(data_memory));
    memset(instruction_memory, 0, sizeof(instruction_memory));
    memset(registers, 0, sizeof(registers));

    char line[MAX_LINE_LENGTH];
    int index = 0;
    while (fgets(line, sizeof(line), file) && index < MAX_INSTRUCTIONS) {
        if (is_comment_or_empty(line)) continue;
        parse_instruction(line, index++);
    }
    fclose(file);

    total_instructions = index;
    clock_cycle = 1;

    // Print loaded instructions before simulation starts
    print_loaded_instructions(); // This now prints to stderr
    
    // Simulate to completion
     while (1) {
        if (PC >= total_instructions &&
            !pipeline[0].active && !pipeline[1].active &&
            !pipeline[2].active && !pipeline[3].active && !pipeline[4].active) {
            break;
        }
        advance_pipeline(); // This function would internally update global state
        clock_cycle++;
        if (clock_cycle > 10000) { // Increased limit for safety
             // Handle timeout, maybe add to JSON output
            break;
        }
    }

    run_simulation_and_output_json(); // New function to output all results as JSON

    return 0;
}