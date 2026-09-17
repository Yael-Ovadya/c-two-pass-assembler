#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h> /* C90 standard limits - used for LONG_MIN and LONG_MAX */

/* Helper to check if a word is a real opcode of our CPU */
int is_valid_instruction_opcode(char *op_name) {
    int i;
    char *instructions[] = {
        "add", "sub", "and", "or", "nor", "move", "mvhi", "mvlo",
        "addi", "subi", "andi", "ori", "nori", "bne", "beq", "blt", "bgt",
        "lb", "sb", "lw", "sw", "lh", "sh",
        "jmp", "la", "call", "hlt"
    };
    
    /* C90 fix: calculated the array size dynamically to avoid using magic numbers */
    for (i = 0; i < sizeof(instructions) / sizeof(instructions[0]); i++) {
        if (strcmp(op_name, instructions[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Helper function declarations */
int validate_register_operand(char *token, int line_number);
void trim_spaces(char *str);

/* Parse directives with numbers: .db, .dh, .dw */
int parse_data_directive(char *line, int i, unsigned char *data_image, int *DC, int line_number, int bytes_per_unit) {
    char *endptr;
    long value;
    int success = 1;
    long min_val, max_val;
    int byte_idx; /* C90 fix: variable defined at the top of the block */

    /* Set signed limits based on unit size */
    if (bytes_per_unit == 1) {
        min_val = -128;
        max_val = 127;
    } else if (bytes_per_unit == 2) {
        min_val = -32768;
        max_val = 32767;
    } else {
        /* C90 fix: explicitly using LONG_MIN and LONG_MAX from limits.h to avoid compile errors */
        min_val = LONG_MIN;
        max_val = LONG_MAX;
    }

    while (line[i] != '\0' && line[i] != '\n' && line[i] != ';') {
        i = skip_spaces(line, i);
        if (line[i] == '\0' || line[i] == '\n' || line[i] == ';') {
            break;
        }

        /* Check if the character is a valid number start */
        if (!isdigit((unsigned char)line[i]) && line[i] != '-' && line[i] != '+') {
            printf("Error in line %d: Expected a numeric operand, but found '%c'.\n", line_number, line[i]);
            success = 0;
            while (line[i] != '\0' && line[i] != '\n' && line[i] != ',') i++;
            if (line[i] == ',') i++;
            continue;
        }

        value = strtol(line + i, &endptr, 10);
        i = endptr - line;

        /* Range check for overflow */
        if (value < min_val || value > max_val) {
            printf("Error in line %d: Numeric value %ld is out of range for this directive (allowed range: [%ld, %ld]).\n",
                   line_number, value, min_val, max_val);
            success = 0;
        } else {
            /* Store in little-endian format */
            for (byte_idx = 0; byte_idx < bytes_per_unit; byte_idx++) {
                data_image[*DC] = (value >> (byte_idx * 8)) & 0xFF;
                (*DC)++;
            }
        }

        i = skip_spaces(line, i);
        if (line[i] == ',') {
            i++;
            i = skip_spaces(line, i);
            /* Check for trailing comma */
            if (line[i] == '\0' || line[i] == '\n' || line[i] == ';') {
                printf("Error in line %d: Trailing comma found at the end of directive declaration.\n", line_number);
                success = 0;
            }
        } else if (line[i] != '\0' && line[i] != '\n' && line[i] != ';') {
            printf("Error in line %d: Missing comma separator or extraneous text found.\n", line_number);
            success = 0;
            while (line[i] != '\0' && line[i] != '\n' && line[i] != ',') i++;
            if (line[i] == ',') i++;
        }
    }
    return success;
}

/* Parse string directive .asciz */
int parse_string_directive(char *line, int i, unsigned char *data_image, int *DC, int line_number) {
    i = skip_spaces(line, i);
    
    /* The string must start with double quote */
    if (line[i] != '"') {
        printf("Error in line %d: String literal in .asciz must start with a double quote (\").\n", line_number);
        return 0;
    }
    i++;

    /* Copy characters to data image until closing quote */
    while (line[i] != '"' && line[i] != '\0' && line[i] != '\n') {
        data_image[*DC] = (unsigned char)line[i];
        (*DC)++;
        i++;
    }

    if (line[i] != '"') {
        printf("Error in line %d: Unterminated string literal in .asciz (missing closing double quote).\n", line_number);
        return 0;
    }
    i++;

    /* Append null-terminator */
    data_image[*DC] = '\0';
    (*DC)++;

    /* Check for trailing text after string */
    i = skip_spaces(line, i);
    if (line[i] != '\0' && line[i] != '\n' && line[i] != ';') {
        printf("Error in line %d: Extraneous text found after .asciz string literal.\n", line_number);
        return 0;
    }
    return 1;
}

/* Main function for the first pass */
int first_pass(char *file_name, Symbol **symbol_table, unsigned char *data_image, int *DC, int *IC) {
    int error_found = 0;
    char line[MAX_LINE_LENGTH + 2];
    char first_word[MAX_LINE_LENGTH];
    int line_number = 0;
    int i = 0;
    FILE *file;
    
    /* C90 fix: declared all local variables at the top of the function */
    char label_name[MAX_LINE_LENGTH];
    char entry_label[MAX_LINE_LENGTH];
    char extern_name[MAX_LINE_LENGTH];
    Symbol *existing;
    char operand[MAX_LINE_LENGTH];
    int op_idx;
    int expect_comma;
    int operand_count;
    int expected_operands;

    /* Open the assembly file to read line by line */
    file = fopen(file_name, "r");
    if (file == NULL) {
        printf("Error: Cannot open file %s\n", file_name);
        return 1;
    }

    while (fgets(line, MAX_LINE_LENGTH + 2, file) != NULL) {
        line_number++;
        i = 0;
        
        /* Skip empty or comment lines */
        if (is_empty_or_comment(line) == 1) {
            continue;
        }

        i = skip_spaces(line, i);
        i = read_word(line, i, first_word);

        /* Handle label declaration */
        if (first_word[strlen(first_word) - 1] == ':') {
            strcpy(label_name, first_word);
            label_name[strlen(label_name) - 1] = '\0';

            /* Check label syntax */
            if (is_valid_syntax(label_name) == 0) {
                printf("Error in line %d: Label syntax '%s' is invalid.\n", line_number, label_name);
                error_found = 1;
                continue;
            }

            /* Check if label is a reserved word */
            if (is_valid_macro_name(label_name) == 0) {
                printf("Error in line %d: Label name '%s' is a reserved word.\n", line_number, label_name);
                error_found = 1;
                continue;
            }

            i = skip_spaces(line, i);
            i = read_word(line, i, first_word);

            /* Check if label is already in the Symbol table */
            if (search_symbol(*symbol_table, label_name) != NULL) {
                printf("Error in line %d: Label '%s' is already defined.\n", line_number, label_name);
                error_found = 1;
            }
            else if (strcmp(first_word, ".entry") == 0 || strcmp(first_word, ".extern") == 0) {
                printf("Warning in line %d: Label '%s' before '%s' directive is meaningless and will be ignored.\n",
                       line_number, label_name, first_word);
            }
            else {
                /* C90 fix: Changed first_word == '.' to first_word[0] == '.' */
                if (first_word[0] == '.') {
                    add_symbol(symbol_table, label_name, *DC, 0, 1, 0);
                } else {
                    add_symbol(symbol_table, label_name, *IC, 1, 0, 0);
                }
            }
        }

        /* C90 fix: Changed first_word == '.' to first_word[0] == '.' */
        if (first_word[0] == '.') {
            if (strcmp(first_word, ".db") == 0) {
                if (parse_data_directive(line, i, data_image, DC, line_number, 1) == 0) {
                    error_found = 1;
                }
            }
            else if (strcmp(first_word, ".dh") == 0) {
                if (parse_data_directive(line, i, data_image, DC, line_number, 2) == 0) {
                    error_found = 1;
                }
            }
            else if (strcmp(first_word, ".dw") == 0) {
                if (parse_data_directive(line, i, data_image, DC, line_number, 4) == 0) {
                    error_found = 1;
                }
            }
            else if (strcmp(first_word, ".asciz") == 0) {
                if (parse_string_directive(line, i, data_image, DC, line_number) == 0) {
                    error_found = 1;
                }
            }
            else if (strcmp(first_word, ".entry") == 0) {
                /* Safe checking for .entry syntax in the first pass */
                i = skip_spaces(line, i);
                i = read_word(line, i, entry_label);
                
                if (strlen(entry_label) == 0) {
                    printf("Error in line %d: '.entry' directive is missing a symbol name.\n", line_number);
                    error_found = 1;
                    continue;
                }
                
                if (is_valid_syntax(entry_label) == 0) {
                    printf("Error in line %d: Invalid symbol name '%s' in '.entry' directive.\n", line_number, entry_label);
                    error_found = 1;
                    continue;
                }
                
                if (check_extraneous_text(line, i) == 0) {
                    printf("Error in line %d: Extraneous text after '.entry' directive.\n", line_number);
                    error_found = 1;
                    continue;
                }
                continue;
            }
            else if (strcmp(first_word, ".extern") == 0) {
                i = skip_spaces(line, i);
                i = read_word(line, i, extern_name);

                if (strlen(extern_name) == 0) {
                    printf("Error in line %d: '.extern' directive is missing a symbol name.\n", line_number);
                    error_found = 1;
                    continue;
                }

                if (is_valid_syntax(extern_name) == 0) {
                    printf("Error in line %d: Invalid symbol name '%s' in '.extern' directive.\n", line_number, extern_name);
                    error_found = 1;
                    continue;
                }

                if (check_extraneous_text(line, i) == 0) {
                    printf("Error in line %d: Extraneous text after '.extern' directive.\n", line_number);
                    error_found = 1;
                    continue;
                }

                existing = search_symbol(*symbol_table, extern_name);
                if (existing != NULL) {
                    if (existing->is_extern) {
                        continue; /* duplicate extern is ignored */
                    }
                    printf("Error in line %d: Symbol '%s' is already defined and cannot be declared extern.\n",
                           line_number, extern_name);
                    error_found = 1;
                    continue;
                }

                add_symbol(symbol_table, extern_name, 0, 0, 0, 1);
                continue;
            }
            else {
                printf("Error in line %d: Unknown directive instruction '%s'\n", line_number, first_word);
                error_found = 1;
            }
        }
        /* Handle instructions and check syntax */
        else {
            expect_comma = 0;
            operand_count = 0;

            if (is_valid_instruction_opcode(first_word) == 0) {
                printf("Error in line %d: Unknown instruction opcode '%s'\n", line_number, first_word);
                error_found = 1;
                continue;
            }

            /* Check leading comma */
            i = skip_spaces(line, i);
            if (line[i] == ',') {
                printf("Error in line %d: Illegal comma before the first operand.\n", line_number);
                error_found = 1;
                i++;
            }

            while (line[i] != '\0' && line[i] != '\n' && line[i] != ';') {
                i = skip_spaces(line, i);
                if (line[i] == '\0' || line[i] == '\n' || line[i] == ';') {
                    break;
                }

                /* Check comma placement */
                if (line[i] == ',') {
                    if (expect_comma == 0) {
                        printf("Error in line %d: Multiple consecutive commas.\n", line_number);
                        error_found = 1;
                    }
                    expect_comma = 0;
                    i++;
                    continue;
                }

                if (expect_comma == 1) {
                    printf("Error in line %d: Missing comma separator between operands.\n", line_number);
                    error_found = 1;
                }

                /* Extract operand token */
                op_idx = 0;
                while (line[i] != '\0' && line[i] != '\n' && line[i] != ',' && !isspace((unsigned char)line[i]) && line[i] != ';') {
                    operand[op_idx++] = line[i++];
                }
                operand[op_idx] = '\0';
                operand_count++;
                expect_comma = 1;

                /* C90 fix: Changed operand == '$' to operand[0] == '$' */
                if (operand[0] == '$') {
                    if (validate_register_operand(operand, line_number) == 0) {
                        error_found = 1;
                    }
                }
            }

            /* Check trailing comma */
            if (operand_count > 0 && expect_comma == 0) {
                printf("Error in line %d: Trailing comma found at the end of the operand list.\n", line_number);
                error_found = 1;
            }

            /* Validate expected operand counts */
            expected_operands = 3;
            if (strcmp(first_word, "hlt") == 0) {
                expected_operands = 0;
            } else if (strcmp(first_word, "jmp") == 0 || strcmp(first_word, "la") == 0 || strcmp(first_word, "call") == 0) {
                expected_operands = 1;
            } else if (strcmp(first_word, "move") == 0 || strcmp(first_word, "mvhi") == 0 || strcmp(first_word, "mvlo") == 0) {
                expected_operands = 2;
            }

            if (operand_count < expected_operands) {
                printf("Error in line %d: Too few operands for instruction '%s' (expected %d, found %d).\n", line_number, first_word, expected_operands, operand_count);
                error_found = 1;
            } else if (operand_count > expected_operands) {
                printf("Error in line %d: Too many operands for instruction '%s' (expected %d, found %d).\n", line_number, first_word, expected_operands, operand_count);
                error_found = 1;
            }

            /* Advance Instruction Counter by 4 bytes (size of instruction) */
            *IC += 4;
        }
    }
    fclose(file);
    return error_found;
}
