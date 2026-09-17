#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_LIMIT 4096

/* Helper function declarations */
void free_symbol_table(Symbol *head);
void build_output_files(char *base_name, Symbol *symbol_table, long *code_image, unsigned char *data_image, int IC, int DC);

/* Main entry point of our assembler. It processes every file passed in arguments */
int main(int argc, char *argv[]) {
    int i;

    /* Check if the user didn't pass any filenames */
    if (argc == 1) {
        printf("Error: No input files provided.\n");
        return 1;
    }

    /* Process each file one by one */
    for (i = 1; i < argc; i++) {
        char file_am_name[MAX_LINE_LENGTH + 5];
        FILE *temp_fp;
        Symbol *symbol_table = NULL;
        unsigned char data_image[MEMORY_LIMIT];
        long code_image[MEMORY_LIMIT];
        int IC = INITIAL_IC;
        int DC = 0;
        int first_pass_status = 0;
        int second_pass_status = 0;
        int ICF;
        Symbol *curr_sym;

        /* Reset memory buffers to 0 for the current file */
        memset(data_image, 0, sizeof(data_image));
        memset(code_image, 0, sizeof(code_image));

        printf("\n----------------------------------------\n");
        printf("Starting process for file: %s.as\n", argv[i]);
        printf("----------------------------------------\n");

        /* Step 1: Pre-assembler (Macro Expansion) */
        printf("Step 1: Running pre-assembler...\n");
        run_pre_assembler(argv[i]);
        sprintf(file_am_name, "%s.am", argv[i]);

        /* Check if .am file exists to verify pre-assembler worked */
        temp_fp = fopen(file_am_name, "r");
        if (temp_fp == NULL) {
            printf("Compilation failed: Pre-assembler found errors in %s.as (Skipping to next file).\n", argv[i]);
            continue;
        }
        fclose(temp_fp);

        /* Step 2: First Pass */
        printf("Step 2: Starting first pass on %s...\n", file_am_name);
        first_pass_status = first_pass(file_am_name, &symbol_table, data_image, &DC, &IC);
        
        if (first_pass_status == 0) {
            ICF = IC;
            curr_sym = symbol_table;

            /* Check if code and data size exceeds our RAM limit */
            if ((IC - INITIAL_IC) + DC > MEMORY_LIMIT) {
                printf("Error: Program size (Code: %d bytes, Data: %d bytes) exceeds available RAM limit of %d bytes.\n",
                       IC - INITIAL_IC, DC, MEMORY_LIMIT);
                free_symbol_table(symbol_table);
                continue;
            }

            /* Update data addresses by adding ICF so they sit after the code */
            while (curr_sym != NULL) {
                if (curr_sym->is_data) {
                    curr_sym->address += ICF;
                }
                curr_sym = curr_sym->next;
            }

            /* Reset IC back to 100 before starting second pass */
            IC = INITIAL_IC;

            /* Step 3: Second Pass */
            printf("Step 3: Starting second pass on %s...\n", file_am_name);
            second_pass_status = second_pass(file_am_name, symbol_table, code_image, &IC);

            /* Step 4: Generate Output Files if second pass succeeded */
            if (second_pass_status == 0) {
                printf("Step 4: Creating output files (.ob, .ext, .ent) for %s...\n", argv[i]);
                build_output_files(argv[i], symbol_table, code_image, data_image, IC, DC);
                printf("Finished successfully processing file: %s.as\n", argv[i]);
            } else {
                printf("Compilation failed: Errors found during second pass.\n");
            }
        } else {
            printf("Compilation failed: Errors found during first pass.\n");
        }

        /* Free the symbol table memory before moving to next file */
        free_symbol_table(symbol_table);
    }

    return 0;
}

/* Frees the dynamically allocated symbol table linked list */
void free_symbol_table(Symbol *head) {
    Symbol *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

/* Generates final output files (.ob, .ent, .ext) */
void build_output_files(char *base_name, Symbol *symbol_table, long *code_image, unsigned char *data_image, int IC, int DC) {
    char filename[MAX_LINE_LENGTH + 5];
    FILE *fp_ob = NULL, *fp_ent = NULL, *fp_ext = NULL, *fp_am = NULL;
    Symbol *curr;
    int has_entry = 0;
    int ext_written = 0;
    int addr;
    int j;

    /* C90 fix: moved all step 3 variable definitions to the top of the function */
    char line[MAX_LINE_LENGTH + 2];
    int track_ic;
    char op[MAX_LINE_LENGTH];
    char op1[MAX_LINE_LENGTH];
    int idx;
    int len;
    Symbol *sym;

    /* 1. Build the Object file (.ob) */
    sprintf(filename, "%s.ob", base_name);
    if ((fp_ob = fopen(filename, "w")) != NULL) {
        /* Write the header line with code size and data size */
        fprintf(fp_ob, "%d %d\n", IC - INITIAL_IC, DC);

        /* Write instructions word by word */
        for (addr = INITIAL_IC; addr < IC; addr += 4) {
            long word = code_image[(addr - INITIAL_IC) / 4];
            fprintf(fp_ob, "%04d %02X %02X %02X %02X\n", addr,
                    (unsigned int)(word & 0xFF), (unsigned int)((word >> 8) & 0xFF),
                    (unsigned int)((word >> 16) & 0xFF), (unsigned int)((word >> 24) & 0xFF));
        }

        /* Write data section byte by byte (little endian) */
        for (addr = 0; addr < DC; addr += 4) {
            int bytes = (DC - addr < 4) ? (DC - addr) : 4;
            fprintf(fp_ob, "%04d", IC + addr);
            for (j = 0; j < bytes; j++) {
                fprintf(fp_ob, " %02X", data_image[addr + j]);
            }
            fprintf(fp_ob, "\n");
        }
        fclose(fp_ob);
    }

    /* 2. Build the Entries file (.ent) */
    curr = symbol_table;
    while (curr != NULL) {
        if (curr->is_entry) {
            has_entry = 1;
            break;
        }
        curr = curr->next;
    }

    if (has_entry) {
        sprintf(filename, "%s.ent", base_name);
        if ((fp_ent = fopen(filename, "w")) != NULL) {
            curr = symbol_table;
            while (curr != NULL) {
                if (curr->is_entry) {
                    if (curr->is_code) {
                        /* Code labels printed as %d (e.g., 116) */
                        fprintf(fp_ent, "%s %d\n", curr->name, curr->address);
                    } else {
                        /* Data labels printed as %04d (e.g., 0161) */
                        fprintf(fp_ent, "%s %04d\n", curr->name, curr->address);
                    }
                }
                curr = curr->next;
            }
            fclose(fp_ent);
        }
    }

    /* 3. Build the Externals file (.ext) */
    sprintf(filename, "%s.am", base_name);
    if ((fp_am = fopen(filename, "r")) != NULL) {
        track_ic = INITIAL_IC;
        sprintf(filename, "%s.ext", base_name);

        while (fgets(line, (int)sizeof(line), fp_am) != NULL) {
            if (is_empty_or_comment(line)) {
                continue;
            }

            idx = 0;
            len = 0;

            /* Parse opcode, skipping labels if present */
            idx = read_word(line, skip_spaces(line, idx), op);
            
            /* C90 fix: cast strlen to int to avoid conversion warnings */
            if (op[(int)strlen(op) - 1] == ':') {
                idx = read_word(line, skip_spaces(line, idx), op);
            }

            /* C90 fix: cast strlen to int explicitly */
            if ((int)strlen(op) == 0 || op[0] == '.') {
                continue;
            }

            /* Process jump instructions and look for externals */
            if (strcmp(op, "jmp") == 0 || strcmp(op, "la") == 0 || strcmp(op, "call") == 0) {
                idx = read_word(line, skip_spaces(line, idx), op1);
                len = (int)strlen(op1);

                while (len > 0 && (op1[len-1] == '\r' || op1[len-1] == '\n' || op1[len-1] == ' ')) {
                    op1[--len] = '\0';
                }

                if (len > 0 && op1[0] != '$') {
                    sym = search_symbol(symbol_table, op1);
                    if (sym != NULL && sym->is_extern) {
                        if (fp_ext == NULL) {
                            fp_ext = fopen(filename, "w");
                        }
                        if (fp_ext != NULL) {
                            fprintf(fp_ext, "%s %04d\n", sym->name, track_ic);
                            ext_written = 1;
                        }
                    }
                }
            }

            track_ic += 4; /* Increment IC only for real assembly instructions */
        }
        fclose(fp_am);

        if (fp_ext != NULL) {
            fclose(fp_ext);
            /* Clean up external file if it's empty */
            if (!ext_written) {
                remove(filename);
            }
        }
    }
}


