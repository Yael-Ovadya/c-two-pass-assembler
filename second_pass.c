#include "assembler.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This function does the second pass. It finds label addresses and makes the 32-bit machine code. */
int second_pass(char *file_name, Symbol *symbol_table, long *code_image, int *IC) {
    int error_found = 0;
    char line[MAX_LINE_LENGTH + 2];
    int line_number = 0;
    FILE *file;

    /* All variables are defined at the top to match C90 rules */
    int i;
    char first_word[MAX_LINE_LENGTH];
    char op_name[MAX_LINE_LENGTH];
    unsigned long machine_code; /* Use unsigned long to avoid signed shift problems */
    int opcode, rs, rt, rd, funct;
    
    /* Variables for .entry */
    char entry_label[MAX_LINE_LENGTH];
    Symbol *temp_entry;
    int found_entry;

    /* Variables for I-type instructions */
    int immed;
    char jump_label[MAX_LINE_LENGTH];
    Symbol *temp_branch;
    int found_branch;

    /* Variables for J-type instructions */
    int reg;
    long address;
    char j_operand[MAX_LINE_LENGTH];
    Symbol *temp_jump;
    int found_jump;

    /* Open the .am file */
    file = fopen(file_name, "r");
    if (file == NULL) {
        printf("Error: Cannot open file %s\n", file_name);
        return 1;
    }

    while (fgets(line, MAX_LINE_LENGTH + 2, file) != NULL) {
        line_number++;
        i = 0;
        machine_code = 0;
        opcode = 0;
        rs = 0;
        rt = 0;
        rd = 0;
        funct = 0;

        /* Skip spaces at the start of the line */
        while (isspace((unsigned char)line[i])) i++;

        /* Skip empty lines or comments */
        if (line[i] == '\0' || line[i] == '\n' || line[i] == ';') {
            continue;
        }

        /* Read first word to check if there is a label */
        sscanf(line + i, "%s", first_word);

        /* Skip the label if we found one */
        if (first_word[strlen(first_word) - 1] == ':') {
            i += strlen(first_word);
            while (isspace((unsigned char)line[i])) i++;
            if (sscanf(line + i, "%s", op_name) != 1) continue;
        } else {
            strcpy(op_name, first_word);
        }
        i += strlen(op_name);

        /* Handle .entry - find the symbol and mark it as entry */
        if (strcmp(op_name, ".entry") == 0) {
            temp_entry = symbol_table;
            found_entry = 0;
            while (isspace((unsigned char)line[i])) i++;
            sscanf(line + i, "%s", entry_label);

            /* Search for the label in our table */
            while (temp_entry != NULL) {
                if (strcmp(temp_entry->name, entry_label) == 0) {
                    /* Error if the symbol is both extern and entry */
                    if (temp_entry->is_extern == 1) {
                        printf("Error in line %d: Symbol '%s' cannot be defined as both .extern and .entry.\n", line_number, entry_label);
                        error_found = 1;
                    } else {
                        temp_entry->is_entry = 1; /* Mark it as entry */
                    }
                    found_entry = 1;
                    break;
                }
                temp_entry = temp_entry->next;
            }
            if (!found_entry) {
                printf("Error in line %d: Label '%s' not found\n", line_number, entry_label);
                error_found = 1;
            }
            continue;
        }

        /* Skip other directives because first pass already did them */
        if (strcmp(op_name, ".db") == 0 || strcmp(op_name, ".dw") == 0 ||
            strcmp(op_name, ".dh") == 0 || strcmp(op_name, ".asciz") == 0 ||
            strcmp(op_name, ".extern") == 0) {
            continue;
        }

        /* R-type with 3 registers */
        if (strcmp(op_name, "add") == 0 || strcmp(op_name, "sub") == 0 ||
            strcmp(op_name, "and") == 0 || strcmp(op_name, "or")  == 0 ||
            strcmp(op_name, "nor") == 0) {
            opcode = 0;
            if (strcmp(op_name, "add") == 0) funct = 1;
            else if (strcmp(op_name, "sub") == 0) funct = 2;
            else if (strcmp(op_name, "and") == 0) funct = 3;
            else if (strcmp(op_name, "or") == 0) funct = 4;
            else if (strcmp(op_name, "nor") == 0) funct = 5;
            
            /* Read rs, rt, and rd registers */
            sscanf(line + i, " $%d , $%d , $%d", &rs, &rt, &rd);
            
            /* C90 fix: cast to unsigned long before shift to avoid sign problems */
            machine_code = ((unsigned long)opcode << 26) | ((unsigned long)rs << 21) | 
                           ((unsigned long)rt << 16) | ((unsigned long)rd << 11) | 
                           ((unsigned long)funct << 6);
        }
        /* R-type copy with 2 registers */
        else if (strcmp(op_name, "move") == 0 || strcmp(op_name, "mvhi") == 0 || strcmp(op_name, "mvlo") == 0) {
            opcode = 1;
            if (strcmp(op_name, "move") == 0) funct = 1;
            else if (strcmp(op_name, "mvhi") == 0) funct = 2;
            else if (strcmp(op_name, "mvlo") == 0) funct = 3;
            
            /* Read rs and rd registers */
            sscanf(line + i, " $%d , $%d", &rs, &rd);
            rt = 0;
            
            /* C90 fix: cast to unsigned long before shift to avoid sign problems */
            machine_code = ((unsigned long)opcode << 26) | ((unsigned long)rs << 21) | 
                           ((unsigned long)rt << 16) | ((unsigned long)rd << 11) | 
                           ((unsigned long)funct << 6);
        }
        /* I-type instructions */
        else if (strcmp(op_name, "addi") == 0 || strcmp(op_name, "subi") == 0 ||
                 strcmp(op_name, "andi") == 0 || strcmp(op_name, "ori") == 0 ||
                 strcmp(op_name, "nori") == 0 || strcmp(op_name, "bne") == 0 ||
                 strcmp(op_name, "beq")  == 0 || strcmp(op_name, "blt") == 0 ||
                 strcmp(op_name, "bgt")  == 0 || strcmp(op_name, "lb")  == 0 ||
                 strcmp(op_name, "sb")   == 0 || strcmp(op_name, "lw")  == 0 ||
                 strcmp(op_name, "sw")   == 0 || strcmp(op_name, "lh")  == 0 ||
                 strcmp(op_name, "sh")   == 0) {
            immed = 0;
            if (strcmp(op_name, "addi") == 0) opcode = 10;
            else if (strcmp(op_name, "subi") == 0) opcode = 11;
            else if (strcmp(op_name, "andi") == 0) opcode = 12;
            else if (strcmp(op_name, "ori")  == 0) opcode = 13;
            else if (strcmp(op_name, "nori") == 0) opcode = 14;
            else if (strcmp(op_name, "bne")  == 0) opcode = 15;
            else if (strcmp(op_name, "beq")  == 0) opcode = 16;
            else if (strcmp(op_name, "blt")  == 0) opcode = 17;
            else if (strcmp(op_name, "bgt")  == 0) opcode = 18;
            else if (strcmp(op_name, "lb")   == 0) opcode = 19;
            else if (strcmp(op_name, "sb")   == 0) opcode = 20;
            else if (strcmp(op_name, "lw")   == 0) opcode = 21;
            else if (strcmp(op_name, "sw")   == 0) opcode = 22;
            else if (strcmp(op_name, "lh")   == 0) opcode = 23;
            else if (strcmp(op_name, "sh")   == 0) opcode = 24;

            /* For branch, calculate relative jump address */
            if (opcode >= 15 && opcode <= 18) {
                temp_branch = symbol_table;
                found_branch = 0;
                sscanf(line + i, " $%d , $%d , %s", &rs, &rt, jump_label);
                
                while (temp_branch != NULL) {
                    if (strcmp(temp_branch->name, jump_label) == 0) {
                        if (temp_branch->is_extern) {
                            printf("Error in line %d: Branch target '%s' cannot be an external symbol.\n", line_number, jump_label);
                            error_found = 1;
                        } else {
                            /* Calculate relative address from current IC */
                            immed = temp_branch->address - *IC;
                        }
                        found_branch = 1;
                        break;
                    }
                    temp_branch = temp_branch->next;
                }
                if (!found_branch) {
                    printf("Error in line %d: Label '%s' not found\n", line_number, jump_label);
                    error_found = 1;
                }
            }
            /* Read registers and immediate value for other I-type instructions */
            else {
                sscanf(line + i, " $%d , %d , $%d", &rs, &immed, &rt);
            }
            
            /* C90 fix: cast and mask immed to 16 bits */
            machine_code = ((unsigned long)opcode << 26) | ((unsigned long)rs << 21) | 
                           ((unsigned long)rt << 16) | ((unsigned long)immed & 0xFFFFUL);
        }
        /* J-type instructions */
        else if (strcmp(op_name, "jmp") == 0 || strcmp(op_name, "la") == 0 ||
                 strcmp(op_name, "call") == 0 || strcmp(op_name, "hlt") == 0) {
            reg = 0;
            address = 0;
            if (strcmp(op_name, "jmp") == 0) opcode = 30;
            else if (strcmp(op_name, "la") == 0) opcode = 31;
            else if (strcmp(op_name, "call") == 0) opcode = 32;
            else if (strcmp(op_name, "hlt") == 0) opcode = 63;

            if (opcode != 63) {
                sscanf(line + i, " %s", j_operand);
                
                /* C90 fix: check if target is register by looking at first char */
                if (j_operand[0] == '$') {
                    if (opcode != 30) {
                        printf("Error in line %d: Instruction '%s' cannot accept a register operand.\n", line_number, op_name);
                        error_found = 1;
                    }
                    reg = 1;
                    sscanf(j_operand, "$%ld", &address);
                }
                /* Target is a label */
                else {
                    temp_jump = symbol_table;
                    found_jump = 0;
                    reg = 0;
                    
                    while (temp_jump != NULL) {
                        if (strcmp(temp_jump->name, j_operand) == 0) {
                            address = temp_jump->address;
                            found_jump = 1;
                            break;
                        }
                        temp_jump = temp_jump->next;
                    }
                    if (!found_jump) {
                        printf("Error: Label '%s' not found\n", j_operand);
                        error_found = 1;
                    }
                }
            }
            
            /* C90 fix: cast and mask address to 25 bits */
            machine_code = ((unsigned long)opcode << 26) | ((unsigned long)reg << 25) | 
                           ((unsigned long)address & 0x1FFFFFFUL);
        }

        /* Save the final machine code in memory */
        if (machine_code != 0 || strcmp(op_name, "add") == 0 || strcmp(op_name, "hlt") == 0) {
            code_image[(*IC - INITIAL_IC) / 4] = (long)(machine_code & 0xFFFFFFFFUL);
            *IC += 4;
        }
    }
    fclose(file);
    return error_found;
}
