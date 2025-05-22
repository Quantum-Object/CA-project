#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_INSTR 1024
#define NUM_REGS 32
#define MEMORY_SIZE 1024
int memory[MEMORY_SIZE];

typedef enum {NOP, ADD, SUB, MUL, MOVI, JEQ, AND, XORI, JMP, LSL, LSR, MOVR, MOVM} Opcode;

typedef struct {
    Opcode opcode;
    int rd, rs1, rs2;  // registers
    int imm;           // immediate value
    int shamt;         // shift amount
    int address;       // jump address
    bool valid;        // if stage contains a valid instruction
    int pc;            // instruction address
    int execute_cycle; // how many execute cycles left (for branches)
} Instruction;

// Pipeline registers
Instruction IF_ID = {NOP,0,0,0,0,0,0,false,0,0};
Instruction ID_EX = {NOP,0,0,0,0,0,0,false,0,0};
Instruction EX_MEM = {NOP,0,0,0,0,0,0,false,0,0};
Instruction MEM_WB = {NOP,0,0,0,0,0,0,false,0,0};

// Register file and memory
int registers[NUM_REGS] = {0};
int pc = 0;
int instruction_count = 0;
Instruction instructions[MAX_INSTR];

// Function to check if pipeline is empty
bool pipeline_empty() {
    return !IF_ID.valid && !ID_EX.valid && !EX_MEM.valid && !MEM_WB.valid;
}

// Function to fetch instruction
void fetch() {
    if (pc >= instruction_count) {
        IF_ID.valid = false;
        return;
    }
    IF_ID = instructions[pc];
    IF_ID.valid = true;
    IF_ID.pc = pc;
    pc++;
}

// Function to decode instruction (NOP for simplicity)
void decode() {
    if (!IF_ID.valid) {
        ID_EX.valid = false;
        return;
    }
    ID_EX = IF_ID;
    ID_EX.valid = true;
}

// Execute stage
void execute() {
    if (!ID_EX.valid) {
        EX_MEM.valid = false;
        return;
    }
    EX_MEM = ID_EX;
    EX_MEM.valid = true;

    // For branch/jump, simulate 2 cycles execute stage by decrementing execute_cycle
    if (EX_MEM.opcode == JEQ || EX_MEM.opcode == JMP) {
        if (EX_MEM.execute_cycle == 0) EX_MEM.execute_cycle = 2;

        EX_MEM.execute_cycle--;

        if (EX_MEM.execute_cycle == 0) {
            // Branch or jump resolved here

            if (EX_MEM.opcode == JEQ) {
                if (registers[EX_MEM.rs1] == registers[EX_MEM.rs2]) {
                    pc = EX_MEM.pc + 1 + EX_MEM.imm;
                    // Flush IF and ID pipeline stages
                    IF_ID.valid = false;
                    ID_EX.valid = false;
                }
            } else if (EX_MEM.opcode == JMP) {
                // PC = PC[31:28] concatenated with address
                int pc_high = (EX_MEM.pc >> 28) << 28;
                pc = pc_high | (EX_MEM.address & 0x0FFFFFFF);
                IF_ID.valid = false;
                ID_EX.valid = false;
            }
        }
    } else {
        EX_MEM.execute_cycle = 0;
        // Normal ALU ops here
        switch(EX_MEM.opcode) {
            case ADD:
                registers[EX_MEM.rd] = registers[EX_MEM.rs1] + registers[EX_MEM.rs2];
                break;
            case SUB:
                registers[EX_MEM.rd] = registers[EX_MEM.rs1] - registers[EX_MEM.rs2];
                break;
            case MUL:
                registers[EX_MEM.rd] = registers[EX_MEM.rs1] * registers[EX_MEM.rs2];
                break;
            case MOVI:
                registers[EX_MEM.rd] = EX_MEM.imm;
                break;
            case AND:
                registers[EX_MEM.rd] = registers[EX_MEM.rs1] & registers[EX_MEM.rs2];
                break;
            case XORI:
                registers[EX_MEM.rd] = registers[EX_MEM.rs1] ^ EX_MEM.imm;
                break;
            case LSL:
                registers[EX_MEM.rd] = registers[EX_MEM.rs1] << EX_MEM.shamt;
                break;
            case LSR:
                registers[EX_MEM.rd] = ((unsigned int)registers[EX_MEM.rs1]) >> EX_MEM.shamt;
                break;
            case MOVR:
                registers[EX_MEM.rd] = registers[registers[EX_MEM.rs2] + EX_MEM.imm];
                break;
            case MOVM:
                registers[registers[EX_MEM.rs2] + EX_MEM.imm] = registers[EX_MEM.rd];
                break;
            default:
                break;
        }
    }
}

// Memory stage - for simplicity just move instruction forward
void memory_stage() {
    if (!EX_MEM.valid) {
        MEM_WB.valid = false;
        return;
    }
    MEM_WB = EX_MEM;
    MEM_WB.valid = true;
}



// Writeback stage - already done in execute for simplicity
void writeback() {
    // We updated registers in execute stage directly in this example
    MEM_WB.valid = false;  // Instruction leaves pipeline
}

// Print register contents
void print_registers() {
    printf("Registers after execution:\n");
    for(int i=0; i<NUM_REGS; i++) {
        printf("R%d: %d\n", i, registers[i]);
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
    int matched = sscanf(line, "%s %s %s %s", op, r1, r2, r3);

    inst->opcode = get_opcode(op);
    inst->valid = true;
    inst->pc = line_num;
    inst->execute_cycle = 0;

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

char* instruction_to_string(Instruction inst) {
    static char buffer[100];
    if (!inst.valid || inst.opcode == NOP) {
        snprintf(buffer, sizeof(buffer), "NOP");
        return buffer;
    }

    switch (inst.opcode) {
        case ADD:
            snprintf(buffer, sizeof(buffer), "ADD R%d, R%d, R%d", inst.rd, inst.rs1, inst.rs2);
            break;
        case SUB:
            snprintf(buffer, sizeof(buffer), "SUB R%d, R%d, R%d", inst.rd, inst.rs1, inst.rs2);
            break;
        case MUL:
            snprintf(buffer, sizeof(buffer), "MUL R%d, R%d, R%d", inst.rd, inst.rs1, inst.rs2);
            break;
        case MOVI:
            snprintf(buffer, sizeof(buffer), "MOVI R%d, %d", inst.rd, inst.imm);
            break;
        case XORI:
            snprintf(buffer, sizeof(buffer), "XORI R%d, %d", inst.rd, inst.imm);
            break;
        case AND:
            snprintf(buffer, sizeof(buffer), "AND R%d, R%d, R%d", inst.rd, inst.rs1, inst.rs2);
            break;
        case LSL:
            snprintf(buffer, sizeof(buffer), "LSL R%d, R%d, %d", inst.rd, inst.rs1, inst.shamt);
            break;
        case LSR:
            snprintf(buffer, sizeof(buffer), "LSR R%d, R%d, %d", inst.rd, inst.rs1, inst.shamt);
            break;
        case MOVR:
            snprintf(buffer, sizeof(buffer), "MOVR R%d, R%d, %d", inst.rd, inst.rs2, inst.imm);
            break;
        case MOVM:
            snprintf(buffer, sizeof(buffer), "MOVM R%d, R%d, %d", inst.rd, inst.rs2, inst.imm);
            break;
        case JEQ:
            snprintf(buffer, sizeof(buffer), "JEQ R%d, R%d, %d", inst.rs1, inst.rs2, inst.imm);
            break;
        case JMP:
            snprintf(buffer, sizeof(buffer), "JMP %d", inst.address);
            break;
        default:
            snprintf(buffer, sizeof(buffer), "NOP");
            break;
    }
    return buffer;
}




int main() {
    load_instructions_from_file("asm.txt");
    int cycle = 1;

    while (1) {

        if (cycle % 5 == 0 || (pc >= instruction_count && pipeline_empty())) {
            print_registers();
        }

        printf("\n===== Clock Cycle %d =====\n", cycle);

        writeback();
        execute();

        if (cycle % 2 == 1) {
            // Odd cycle: include Fetch, skip Memory
            decode();
            fetch();
        } else {
            // Even cycle: include Memory, skip Fetch
            memory_stage();
            decode(); // Still decode before execute
        }

        // Print pipeline state
        printf("Pipeline:\n");
        printf("  IF:  %s\n", IF_ID.valid ? instruction_to_string(IF_ID) : "NOP");
        printf("  ID:  %s\n", ID_EX.valid ? instruction_to_string(ID_EX) : "NOP");
        printf("  EX:  %s\n", EX_MEM.valid ? instruction_to_string(EX_MEM) : "NOP");
        printf("  WB:  %s\n", MEM_WB.valid ? instruction_to_string(MEM_WB) : "NOP");

        cycle++;

        if (pc >= instruction_count && pipeline_empty()) {
            break;
        }
    }

    print_registers();
    return 0;
}
