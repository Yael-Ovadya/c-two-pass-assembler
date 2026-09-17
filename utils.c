#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "assembler.h"

int num_reserved;

/* Skips spaces and tabs in a string starting from index i */
int skip_spaces(char *line, int i) {
    while (line[i] == ' ' || line[i] == '\t') {
        i++;
    }
    return i;
}

/* Reads the next word token until whitespace or end of line */
int read_word(char *line, int i, char *word) {
    int j = 0;
    while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\0') {
        word[j] = line[i];
        i++;
        j++;
    }
    word[j] = '\0';
    return i;
}

/* Checks if a name conflicts with reserved assembler keywords or registers */
int is_valid_macro_name(char *macro_name) {
    int i;
    char *reserved_words[] = {
        "mcro", "mcroend",
        "add", "sub", "and", "or", "nor", "move", "mvhi", "mvlo",
        "addi", "subi", "andi", "ori", "nori", "bne", "beq", "blt", "bgt",
        "lb", "sb", "lw", "sw", "lh", "sh",
        "jmp", "la", "call", "hlt",
        ".db", ".dw", ".dh", ".asciz", ".entry", ".extern"
    };

    /* C90 fix: calculate the array size dynamically to avoid using the magic number 35 */
    num_reserved = (int)(sizeof(reserved_words) / sizeof(reserved_words[0]));

    /* Check against reserved words */
    for (i = 0; i < num_reserved; i++) {
        if (strcmp(macro_name, reserved_words[i]) == 0) {
            return 0;
        }
    }
    /* Check that it is not a register */
    if (macro_name[0] == '$') {
        return 0;
    }
    return 1;
}

/* Checks if there is any extraneous text before the end of the line */
int check_extraneous_text(char *line, int i) {
    i = skip_spaces(line, i);
    if (line[i] == '\n' || line[i] == '\0') {
        return 1;
    }
    return 0;
}

/* Validates identifier syntax: starts with a letter, max 31 chars, alphanumeric/underscore */
int is_valid_syntax(char *name) {
    int i;
    /* C90 fix: cast strlen to int to avoid conversion warnings */
    if ((int)strlen(name) > 31) {
        return 0;
    }
    if (!isalpha((unsigned char)name[0])) {
        return 0;
    }
    for (i = 1; name[i] != '\0'; i++) {
        if (!isalnum((unsigned char)name[i]) && name[i] != '_') {
            return 0;
        }
    }
    return 1;
}

/* Verifies that the line does not exceed the maximum allowed length */
int is_valid_line_length(char *line) {
    /* C90 fix: cast strlen to int explicitly */
    int len = (int)strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        len--;
    }
    if (len > MAX_LINE_LENGTH) {
        return 0;
    }
    return 1;
}

/* Checks whether a line is empty or starts with a comment (';') */
int is_empty_or_comment(char *line) {
    int i = 0;
    i = skip_spaces(line, i);
    if (line[i] == '\n' || line[i] == '\0' || line[i] == ';') {
        return 1;
    }
    return 0;
}

/* Checks for an illegal leading comma directly after instruction */
int check_illegal_comma(char *line, int i) {
    i = skip_spaces(line, i);
    if (line[i] == ',') {
        return 1;
    }
    return 0;
}

/* Validates and skips a comma separator between operands */
int check_and_skip_comma(char *line, int i) {
    i = skip_spaces(line, i);
    if (line[i] == ',') {
        i++;
        i = skip_spaces(line, i);
        /* Check for consecutive commas */
        if (line[i] == ',') {
            return -1;
        }
        /* Check for trailing comma */
        if (line[i] == '\n' || line[i] == '\0') {
            return -2;
        }
        return i;
    }
    return -3; /* Missing comma */
}

/* Adds a new symbol node to the symbol table */
void add_symbol(Symbol **head, char *name, int address, int is_code, int is_data, int is_extern) {
    Symbol *new_node;
    Symbol *temp;

    new_node = (Symbol *)malloc(sizeof(Symbol));
    if (new_node == NULL) {
        printf("Error: Memory allocation failed for symbol table node.\n");
        exit(1);
    }
    strcpy(new_node->name, name);
    new_node->address = address;
    new_node->is_code = is_code;
    new_node->is_data = is_data;
    new_node->is_extern = is_extern;
    new_node->is_entry = 0;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
    } else {
        temp = *head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_node;
    }
}

/* Searches for a symbol by name in the symbol table */
Symbol *search_symbol(Symbol *head, char *name) {
    Symbol *curr = head;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

/* Validates register format: $0 to $31 with no leading zeros */
int validate_register_operand(char *token, int line_number) {
    int reg_num;
    int i = 1;
    if (*token != '$') {
        printf("Error in line %d: Expected register operand to start with '$', but found '%s'.\n", line_number, token);
        return 0;
    }
    if (*(token + 1) == '\0') {
        printf("Error in line %d: Register prefix '$' found without a register number.\n", line_number);
        return 0;
    }
    while (*(token + i) != '\0') {
        if (!isdigit((unsigned char)*(token + i))) {
            printf("Error in line %d: Invalid register format '%s'. Register number must be an integer.\n", line_number, token);
            return 0;
        }
        i++;
    }
    /* Disallow leading zeros (e.g., $05) */
    if (*(token + 1) == '0' && *(token + 2) != '\0') {
        printf("Error in line %d: Invalid register format '%s'. Leading zeros are not allowed.\n", line_number, token);
        return 0;
    }
    reg_num = atoi(token + 1);
    if (reg_num < 0 || reg_num > 31) {
        printf("Error in line %d: Register number '%d' is out of range. Allowed range is $0..$31.\n", line_number, reg_num);
        return 0;
    }
    return 1;
}

/* Trims leading and trailing whitespace characters in-place */
void trim_spaces(char *str) {
    /* C90 fix: cast strlen to int explicitly */
    int len = (int)strlen(str);
    int start = 0;
    int end = len - 1;
    int i;
    while (start < len && isspace((unsigned char)str[start])) {
        start++;
    }
    while (end >= start && isspace((unsigned char)str[end])) {
        end--;
    }
    for (i = start; i <= end; i++) {
        str[i - start] = str[i];
    }
    str[end - start + 1] = '\0';
}



