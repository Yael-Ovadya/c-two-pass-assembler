#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Pre-assembler stage: expands macros from .as source file into .am file */
void run_pre_assembler(char *file_name) {
    FILE *fp_in;
    FILE *fp_out;
    char line[MAX_LINE_LENGTH + 2];
    char first_word[MAX_LINE_LENGTH];
    char macro_name[MAX_LINE_LENGTH];
    char file_as[MAX_LINE_LENGTH+5];
    char file_am[MAX_LINE_LENGTH+5];
    Macro *macro_head = NULL;
    Macro *current_macro = NULL;
    int in_macro = 0;
    int found_macro = 0;
    Macro *temp;
    int line_number = 0;
    int i = 0;
    int error_flag = 0;
    int ch; /* C90 fix: ch must be int for fgetc, defined at the top */

    /* Variables for unified syntax check - defined at the top to match C90 rules */
    int temp_idx;
    int word_count;
    int local_err;
    char temp_word[MAX_LINE_LENGTH];

    /* Variables for duplicate check - defined at the top to match C90 rules */
    Macro *check_temp;
    int is_duplicate;

    /* Make filenames with .as and .am endings */
    sprintf(file_as, "%s.as", file_name);
    sprintf(file_am, "%s.am", file_name);

    /* Open the input file to read */
    fp_in = fopen(file_as, "r");
    if (fp_in == NULL) {
        printf("Error: Cannot open input file %s\n", file_as);
        return;
    }

    /* Open the output file to write */
    fp_out = fopen(file_am, "w");
    if (fp_out == NULL) {
        printf("Error: Cannot create output file %s\n", file_am);
        fclose(fp_in);
        return;
    }

    /* Read the input file line by line */
    while (fgets(line, MAX_LINE_LENGTH + 2, fp_in) != NULL) {
        line_number++;
        i = 0;

        /* Check if line is too long (more than 80 characters) */
        if (is_valid_line_length(line) == 0) {
            printf("Error in line %d: Line exceeds maximum length of 80 characters.\n", line_number);
            error_flag = 1;
            /* If line was too long, clear the rest of the line from the file stream */
            if (strchr(line, '\n') == NULL && !feof(fp_in)) {
                while ((ch = fgetc(fp_in)) != '\n' && ch != EOF);
            }
            continue;
        }

        /* Skip comments and empty lines */
        if (is_empty_or_comment(line) == 1) {
            if (in_macro == 1) {
                /* Add line to macro text if we are inside a macro */
                if (strlen(current_macro->content) + strlen(line) < MAX_MACRO_SIZE) {
                    strcat(current_macro->content, line);
                } else {
                    printf("Error in line %d: Macro '%s' exceeded max capacity.\n", line_number, current_macro->name);
                    error_flag = 1;
                }
            } else {
                /* Just copy comment line to output file */
                fputs(line, fp_out);
            }
            continue;
        }

        /* Combined syntax check for macro keywords */
        if (strstr(line, ".asciz") == NULL) {
            temp_idx = 0;
            word_count = 0;
            local_err = 0;
            while (1) {
                temp_idx = skip_spaces(line, temp_idx);
                if (line[temp_idx] == '\0' || line[temp_idx] == '\n') break;
                temp_idx = read_word(line, temp_idx, temp_word);
                if (strlen(temp_word) == 0) break;
                word_count++;

                /* Check for garbage words before "mcro" */
                if (strcmp(temp_word, "mcro") == 0 && word_count > 1) {
                    printf("Error in line %d: Garbage characters before macro definition\n", line_number);
                    error_flag = 1; 
                    local_err = 1; 
                    break;
                }

                /* Check for syntax of "mcroend" keyword */
                if (strcmp(temp_word, "mcroend") == 0) {
                    if (word_count > 1) {
                        printf("Error in line %d: Garbage characters before 'mcroend'\n", line_number);
                        error_flag = 1; 
                        local_err = 1;
                    } else {
                        temp_idx = skip_spaces(line, temp_idx);
                        if (line[temp_idx] != '\0' && line[temp_idx] != '\n') {
                            printf("Error in line %d: Extraneous text after 'mcroend'\n", line_number);
                            error_flag = 1; 
                            local_err = 1;
                        }
                    }
                    in_macro = 0;
                    break;
                }
            }
            if (local_err) continue;
        }

        i = skip_spaces(line, i);
        i = read_word(line, i, first_word);

        /* Handle macro definition start */
        if (strcmp(first_word, "mcro") == 0) {
            i = skip_spaces(line, i);
            i = read_word(line, i, macro_name);

            /* Check if macro has a name */
            if (strlen(macro_name) == 0) {
                printf("Error in line %d: Macro declared without a name (mcro keyword only).\n", line_number);
                error_flag = 1;
                continue;
            }
            /* Check if macro name syntax is valid */
            if (is_valid_syntax(macro_name) == 0) {
                printf("Error in line %d: Invalid macro name syntax '%s'\n", line_number, macro_name);
                error_flag = 1;
                continue;
            }
            /* Check if macro name is not a saved C keyword or opcode */
            if (is_valid_macro_name(macro_name) == 0) {
                printf("Error in line %d: Macro name '%s' is a reserved word\n", line_number, macro_name);
                error_flag = 1;
                continue;
            }

            /* Check for duplicate macro name */
            check_temp = macro_head;
            is_duplicate = 0;
            while (check_temp != NULL) {
                if (strcmp(check_temp->name, macro_name) == 0) {
                    is_duplicate = 1;
                    break;
                }
                check_temp = check_temp->next;
            }
            if (is_duplicate == 1) {
                printf("Error in line %d: Duplicate macro name '%s'\n", line_number, macro_name);
                error_flag = 1;
                continue;
            }

            /* Check if there is extra text on macro declaration line */
            if (check_extraneous_text(line, i) == 0) {
                printf("Error in line %d: Extraneous text after macro definition\n", line_number);
                error_flag = 1;
                continue;
            }

            /* Allocate memory for the new macro and add it to our list */
            in_macro = 1;
            current_macro = (Macro *)malloc(sizeof(Macro));
            if (current_macro == NULL) {
                printf("Error: Memory allocation failed for macro struct.\n");
                error_flag = 1;
                break;
            }
            strcpy(current_macro->name, macro_name);
            strcpy(current_macro->content, "");
            current_macro->next = macro_head;
            macro_head = current_macro;
            continue;
        }

        /* Handle macro definition end */
        else if (strcmp(first_word, "mcroend") == 0) {
            if (check_extraneous_text(line, i) == 0) {
                printf("Error in line %d: Extraneous text after 'mcroend'\n", line_number);
                error_flag = 1;
            }
            in_macro = 0;
            continue;
        }

        /* Handle macro content or macro calls */
        else {
            if (in_macro == 1) {
                /* Inside macro definition - save the line */
                if (strlen(current_macro->content) + strlen(line) < MAX_MACRO_SIZE) {
                    strcat(current_macro->content, line);
                } else {
                    printf("Error in line %d: Macro '%s' exceeded max capacity.\n", line_number, current_macro->name);
                    error_flag = 1;
                }
            }
            else {
                temp = macro_head;
                found_macro = 0;
                while (temp != NULL) {
                    /* If line is a macro call, expand it */
                    if (strcmp(first_word, temp->name) == 0) {
                        if (check_extraneous_text(line, i) == 0) {
                            printf("Error in line %d: Extraneous text after macro call\n", line_number);
                            error_flag = 1;
                        }
                        fputs(temp->content, fp_out);
                        found_macro = 1;
                        break;
                    }
                    temp = temp->next;
                }
                /* If not a macro call, just copy the line to output */
                if (found_macro == 0) {
                    fputs(line, fp_out);
                }
            }
        }
    }

    /* Free all macros from the linked list memory */
    while (macro_head != NULL) {
        temp = macro_head;
        macro_head = macro_head->next;
        free(temp);
    }
    fclose(fp_in);
    fclose(fp_out);

    /* Delete the output .am file if there were any errors */
    if (error_flag == 1) {
        remove(file_am);
        printf("Pre-assembler found errors. Output file %s was not created.\n", file_am);
    }
    else {
        printf("Pre-assembler successfully finished processing: %s\n", file_name);
    }
}
