//
// Created by Sorour, Amr on 22.05.25.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_INSTR 1024
#define NUM_REGS 32
#define MEM_SIZE 1024

typedef enum {NOP, ADD, SUB, MUL, MOVI, JEQ, AND, XORI, JMP, LSL, LSR, MOVR, MOVM} Opcode;

typedef struct {
    Opcode opcode;
    int rd, rs1, rs2;  // registers
    int imm;           // immediate value
    int shamt;         // shift amount
    int address;       // jump address
    bool valid;        // if stage contains a valid instruction
    int pc;            // instruction address
    int stage_cycles;  // how many cycles left in current stage
    char raw[128];     // original instruction string
} Instruction;

// Pipeline registers
Instruction IF_ID = {NOP,0,0,0,0,0,0,false,0,0,""};
Instruction ID_EX = {NOP,0,0,0,0,0,0,false,0,0,""};
Instruction EX_MEM = {NOP,0,0,0,0,0,0,false,0,0,""};
Instruction MEM_WB = {NOP,0,0,0,0,0,0,false,0,0,""};

// Register file and memory
int registers[NUM_REGS] = {0};
int memory[MEM_SIZE] = {0};
int pc = 0;
int instruction_count = 0;
Instruction instructions[MAX_INSTR];
int clock_cycle = 0;
bool mem_busy = false;

// Function to print instruction details
void print_instruction(Instruction *inst) {
    if (!inst->valid) {
        printf("NOP");
        return;
    }
    printf("%s (PC=%d)", inst->raw, inst->pc);
}

// Function to print pipeline stage
void print_stage(const char *stage_name, Instruction *inst, const char *action) {
    printf("%-4s: ", stage_name);
    print_instruction(inst);
    if (action[0] != '\0') {
        printf(" - %s", action);
    }
    printf("\n");
}

// Function to check if pipeline is empty
bool pipeline_empty() {
    return !IF_ID.valid && !ID_EX.valid && !EX_MEM.valid && !MEM_WB.valid;
}

// Function to fetch instruction
void fetch() {
    if (IF_ID.valid && IF_ID.stage_cycles > 1) {
        IF_ID.stage_cycles--;
        return;
    }

    if (mem_busy) return;

    if (IF_ID.valid) {
        // Move to decode if ready
        if (!ID_EX.valid || ID_EX.stage_cycles == 1) {
            ID_EX = IF_ID;
            ID_EX.stage_cycles = 2; // Decode takes 2 cycles
            IF_ID.valid = false;
        }
    }

    if (pc >= instruction_count) return;

    if (!IF_ID.valid) {
        IF_ID = instructions[pc];
        IF_ID.valid = true;
        IF_ID.pc = pc;
        IF_ID.stage_cycles = 2; // Fetch takes 2 cycles
        pc++;
        mem_busy = true;
        print_stage("IF", &IF_ID, "Fetching instruction");
    }
}

// Function to decode instruction
void decode() {
    if (!ID_EX.valid) return;

    if (ID_EX.stage_cycles > 1) {
        ID_EX.stage_cycles--;
        return;
    }

    // Move to execute if ready
    if (!EX_MEM.valid || EX_MEM.stage_cycles == 1) {
        EX_MEM = ID_EX;
        EX_MEM.stage_cycles = 2; // Execute takes 2 cycles
        ID_EX.valid = false;
        print_stage("ID", &EX_MEM, "Decoded instruction");
    }
}

// Execute stage
void execute() {
    if (!EX_MEM.valid) return;

    if (EX_MEM.stage_cycles > 1) {
        EX_MEM.stage_cycles--;
        return;
    }

    // For branches/jumps, handle in first execute cycle
    if (EX_MEM.stage_cycles == 2) {
        if (EX_MEM.opcode == JEQ || EX_MEM.opcode == JMP) {
            if (EX_MEM.opcode == JEQ) {
                if (registers[EX_MEM.rs1] == registers[EX_MEM.rs2]) {
                    pc = EX_MEM.pc + 1 + EX_MEM.imm;
                    // Flush pipeline
                    IF_ID.valid = false;
                    ID_EX.valid = false;
                    printf("Branch taken to PC=%d\n", pc);
                } else {
                    printf("Branch not taken\n");
                }
            } else if (EX_MEM.opcode == JMP) {
                int pc_high = (EX_MEM.pc >> 28) << 28;
                pc = pc_high | (EX_MEM.address & 0x0FFFFFFF);
                IF_ID.valid = false;
                ID_EX.valid = false;
                printf("Jump to PC=%d\n", pc);
            }
        }
        EX_MEM.stage_cycles--;
        return;
    }

    // Move to memory if ready
    if (!MEM_WB.valid || MEM_WB.stage_cycles == 1) {
        // Handle ALU operations in second execute cycle
        int old_value = 0;
        switch(EX_MEM.opcode) {
            case ADD:
                old_value = registers[EX_MEM.rd];
                registers[EX_MEM.rd] = registers[EX_MEM.rs1] + registers[EX_MEM.rs2];
                printf("R%d changed from %d to %d in EX\n", EX_MEM.rd, old_value, registers[EX_MEM.rd]);
                break;
            case SUB:
                old_value = registers[EX_MEM.rd];
                registers[EX_MEM.rd] = registers[EX_MEM.rs1] - registers[EX_MEM.rs2];
                printf("R%d changed from %d to %d in EX\n", EX_MEM.rd, old_value, registers[EX_MEM.rd]);
                break;
            case MUL:
                old_value = registers[EX_MEM.rd];
                registers[EX_MEM.rd] = registers[EX_MEM.rs1] * registers[EX_MEM.rs2];
                printf("R%d changed from %d to %d in EX\n", EX_MEM.rd, old_value, registers[EX_MEM.rd]);
                break;
            case MOVI:
                old_value = registers[EX_MEM.rd];
                registers[EX_MEM.rd] = EX_MEM.imm;
                printf("R%d changed from %d to %d in EX\n", EX_MEM.rd, old_value, registers[EX_MEM.rd]);
                break;
            case AND:
                old_value = registers[EX_MEM.rd];
                registers[EX_MEM.rd] = registers[EX_MEM.rs1] & registers[EX_MEM.rs2];
                printf("R%d changed from %d to %d in EX\n", EX_MEM.rd, old_value, registers[EX_MEM.rd]);
                break;
            case XORI:
                old_value = registers[EX_MEM.rd];
                registers[EX_MEM.rd] = registers[EX_MEM.rs1] ^ EX_MEM.imm;
                printf("R%d changed from %d to %d in EX\n", EX_MEM.rd, old_value, registers[EX_MEM.rd]);
                break;
            case LSL:
                old_value = registers[EX_MEM.rd];
                registers[EX_MEM.rd] = registers[EX_MEM.rs1] << EX_MEM.shamt;
                printf("R%d changed from %d to %d in EX\n", EX_MEM.rd, old_value, registers[EX_MEM.rd]);
                break;
            case LSR:
                old_value = registers[EX_MEM.rd];
                registers[EX_MEM.rd] = ((unsigned int)registers[EX_MEM.rs1]) >> EX_MEM.shamt;
                printf("R%d changed from %d to %d in EX\n", EX_MEM.rd, old_value, registers[EX_MEM.rd]);
                break;
            case MOVR:
                old_value = registers[EX_MEM.rd];
                registers[EX_MEM.rd] = memory[registers[EX_MEM.rs2] + EX_MEM.imm];
                printf("R%d changed from %d to %d in EX\n", EX_MEM.rd, old_value, registers[EX_MEM.rd]);
                break;
            default:
                break;
        }

        MEM_WB = EX_MEM;
        MEM_WB.stage_cycles = 1; // Memory takes 1 cycle
        EX_MEM.valid = false;
        print_stage("EX", &MEM_WB, "Executed instruction");
    }
}

// Memory stage
void memory_stage() {
    if (!MEM_WB.valid) return;

    if (MEM_WB.stage_cycles > 1) {
        MEM_WB.stage_cycles--;
        return;
    }

    // Handle memory operations
    if (MEM_WB.opcode == MOVM) {
        int addr = registers[MEM_WB.rs2] + MEM_WB.imm;
        if (addr >= 0 && addr < MEM_SIZE) {
            int old_value = memory[addr];
            memory[addr] = registers[MEM_WB.rd];
            printf("Memory[%d] changed from %d to %d in MEM\n", addr, old_value, memory[addr]);
        }
    }

    // Memory stage is done, can allow fetch to proceed
    mem_busy = false;

    // Move to writeback (which happens immediately in this model)
    MEM_WB.valid = false;
    print_stage("MEM", &MEM_WB, "Memory access completed");
}

// Writeback stage - already done in execute for most operations
void writeback() {
    // In our model, writeback happens immediately after memory stage
}

// Print register contents
void print_registers() {
    printf("\nFinal register values:\n");
    for(int i=0; i<NUM_REGS; i++) {
        printf("R%d: %d\n", i, registers[i]);
    }
    printf("PC: %d\n", pc);
}

// Print memory contents
void print_memory() {
    printf("\nMemory contents (non-zero only):\n");
    for(int i=0; i<MEM_SIZE; i++) {
        if (memory[i] != 0) {
            printf("Memory[%d]: %d\n", i, memory[i]);
        }
    }
}

Opcode get_opcode(const char *mnemonic) {
    if (strcmp(mnemonic, "ADD") == 0) return ADD;
    if (strcmp(mnemonic, "SUB") == 0) return SUB;
    if (strcmp(mnemonic, "MUL") == 0) return MUL;
    if (strcmp(mnemonic, "MOVI") == 0) return MOVI;
    if (strcmp(mnemonic, "JEQ") == 0) return JEQ;
    if (strcmp(mnemonic, "AND") == 0) return AND;
    if (strcmp(mnemonic, "XORI") == 0) return XORI;
    if (strcmp(mnemonic, "JMP") == 0) return JMP;
    if (strcmp(mnemonic, "LSL") == 0) return LSL;
    if (strcmp(mnemonic, "LSR") == 0) return LSR;
    if (strcmp(mnemonic, "MOVR") == 0) return MOVR;
    if (strcmp(mnemonic, "MOVM") == 0) return MOVM;
    return NOP;
}

bool parse_register(const char *token, int *reg_num) {
    if (token[0] != 'R') return false;
    int r = atoi(token + 1);
    if (r < 0 || r >= NUM_REGS) return false;
    *reg_num = r;
    return true;
}

bool parse_instruction_line(char *line, Instruction *inst, int line_num) {
    char op[10], r1[10], r2[10], r3[10];
    strcpy(inst->raw, line);
    inst->raw[strlen(inst->raw)-1] = '\0'; // Remove newline

    int matched = sscanf(line, "%s %s %s %s", op, r1, r2, r3);

    inst->opcode = get_opcode(op);
    inst->valid = true;
    inst->pc = line_num;
    inst->stage_cycles = 0;

    if (inst->opcode == NOP) return false;

    switch (inst->opcode) {
        case ADD: case SUB: case MUL: case AND:
            if (matched != 4 || !parse_register(r1, &inst->rd) || !parse_register(r2, &inst->rs1) || !parse_register(r3, &inst->rs2))
                return false;
            break;
        case MOVI: case XORI:
            if (matched != 3 || !parse_register(r1, &inst->rd)) return false;
            inst->imm = atoi(r2);
            break;
        case LSL: case LSR:
            if (matched != 4 || !parse_register(r1, &inst->rd) || !parse_register(r2, &inst->rs1))
                return false;
            inst->shamt = atoi(r3);
            break;
        case MOVR: case MOVM:
            if (matched != 4 || !parse_register(r1, &inst->rd) || !parse_register(r2, &inst->rs2))
                return false;
            inst->imm = atoi(r3);
            break;
        case JEQ:
            if (matched != 4 || !parse_register(r1, &inst->rs1) || !parse_register(r2, &inst->rs2))
                return false;
            inst->imm = atoi(r3);
            break;
        case JMP:
            if (matched != 2) return false;
            inst->address = atoi(r1);
            break;
        default:
            return false;
    }
    return true;
}

void load_instructions_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening instruction file");
        exit(1);
    }

    char line[128];
    int line_num = 0;
    while (fgets(line, sizeof(line), file)) {
        // Skip comments or blank lines
        if (line[0] == '#' || strlen(line) <= 1) continue;

        Instruction inst;
        if (parse_instruction_line(line, &inst, line_num)) {
            instructions[instruction_count++] = inst;
        } else {
            printf("Skipping invalid instruction at line %d: %s", line_num + 1, line);
        }
        line_num++;
    }

    fclose(file);
}

int main() {
    load_instructions_from_file("asm.txt");

    while (1) {
        clock_cycle++;
        printf("\n=== Clock Cycle %d ===\n", clock_cycle);

        // Print pipeline stages
        print_stage("IF", &IF_ID, "");
        print_stage("ID", &ID_EX, "");
        print_stage("EX", &EX_MEM, "");
        print_stage("MEM", &MEM_WB, "");

        // Pipeline stages in reverse order
        writeback();
        memory_stage();
        execute();
        decode();
        fetch();

        if (pc >= instruction_count && pipeline_empty()) {
            break;
        }
    }

    print_registers();
    print_memory();
    return 0;
}