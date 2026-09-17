#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- General Constants --- */
#define MAX_LINE_LENGTH 80  /* Max line length in source file, according to project rules */
#define INITIAL_IC 100      /* IC starts at 100 because this is where the code memory starts */
#define MAX_MACRO_SIZE 10000

/* --- Opcodes --- */
/* R-type opcodes */
#define OP_R_ARITHMETIC 0   /* add, sub, and, or, nor */
#define OP_R_COPY 1         /* move, mvhi, mvlo */

/* I-type opcodes */
#define OP_ADDI 10
#define OP_SUBI 11
#define OP_ANDI 12
#define OP_ORI 13
#define OP_NORI 14
#define OP_BNE 15
#define OP_BEQ 16
#define OP_BLT 17
#define OP_BGT 18
#define OP_LB 19
#define OP_SB 20
#define OP_LW 21
#define OP_SW 22
#define OP_LH 23
#define OP_SH 24

/* J-type opcodes */
#define OP_JMP 30
#define OP_LA 31
#define OP_CALL 32
#define OP_HLT 63

/* --- Data Structures --- */

/* Symbol table node - simple linked list to save all our labels and where they are in memory */
typedef struct symbol {
    char name[MAX_LINE_LENGTH]; /* The label name */
    int address;                /* Address in memory (IC or DC) */
    int is_code;                /* Flag: 1 if it is code, 0 if not */
    int is_data;                /* Flag: 1 if it is data (.db, .dw, .dh, .asciz), 0 if not */
    int is_entry;               /* Flag: 1 if declared as .entry, 0 if not */
    int is_extern;              /* Flag: 1 if declared as .extern, 0 if not */
    struct symbol *next;        /* Pointer to the next label in the list */
} Symbol;

/* Macro storage node - used in the pre-assembler phase to save macro text and expand it later */
typedef struct Macro {
    char name[MAX_LINE_LENGTH];   /* The macro name we search for */
    char content[MAX_MACRO_SIZE]; /* The lines of code inside the macro */
    struct Macro *next;           /* Pointer to the next macro in the list */
} Macro;

/* --- 32-bit Machine Instruction Formats (Bit-fields) --- */
/* We use bit-fields so we do not have to do manual bit shifting. The compiler does it for us! */

/* R-type instruction structure */
typedef struct {
    unsigned int unused: 6;     /* Bits 0-5: unused bits in R-type */
    unsigned int funct: 5;      /* Bits 6-10: sub-operation */
    unsigned int rd: 5;         /* Bits 11-15: destination register */
    unsigned int rt: 5;         /* Bits 16-20: source register 2 */
    unsigned int rs: 5;         /* Bits 21-25: source register 1 */
    unsigned int opcode: 6;     /* Bits 26-31: operation code */
} R_Instruction;

/* I-type instruction structure */
typedef struct {
    /* Using "signed int" explicitly for immed is required by C90 standard, otherwise negative numbers might fail */
    signed int immed: 16;      /* Bits 0-15: immediate signed value */
    unsigned int rt: 5;         /* Bits 16-20: target register */
    unsigned int rs: 5;         /* Bits 21-25: source register */
    unsigned int opcode: 6;     /* Bits 26-31: operation code */
} I_Instruction;

/* J-type instruction structure */
typedef struct {
    unsigned int address: 25;   /* Bits 0-24: target address of the jump */
    unsigned int reg: 1;        /* Bit 25: flag if we jump to a register (1) or address (0) */
    unsigned int opcode: 6;     /* Bits 26-31: operation code */
} J_Instruction;

/* Union representation of a 32-bit instruction word - allows us to write to any format easily */
typedef union {
    R_Instruction r_inst;
    I_Instruction i_inst;
    J_Instruction j_inst;
    unsigned int word;          /* We can read this "word" as 32-bit unsigned int to write to file */
} MachineWord;

/* --- Function Prototypes --- */

/* Pipeline steps */
int pre_assembler(char *file_name); /* Opens .as file and expands macros to create .am file */
int first_pass(char *file_name, Symbol **symbol_table, unsigned char *data_image, int *DC, int *IC); /* Finds labels and syntax errors */
int second_pass(char *file_name, Symbol *symbol_table, long *code_image, int *IC); /* Completes the machine code and target addresses */
void build_output_files(char *base_name, Symbol *symbol_table, long *code_image, unsigned char *data_image, int IC, int DC); /* Saves .ob, .ent, .ext files */

/* Helper functions for the Symbol list */
void add_symbol(Symbol **head, char *name, int address, int is_code, int is_data, int is_extern);
int is_symbol_exists(Symbol *head, char *name);
Symbol *search_symbol(Symbol *head, char *name);

/* Helper functions for string parsing and syntax checks */
void print_error(char *message, int line_number);
void run_pre_assembler(char *file_name);
int skip_spaces(char *line, int i);
void trim_spaces(char *str);
int read_word(char *line, int i, char *word);
int is_valid_macro_name(char *macro_name);
int check_extraneous_text(char *line, int i);
int is_valid_syntax(char *name);
int is_valid_line_length(char *line);
int is_empty_or_comment(char *line);
int check_illegal_comma(char *line, int i);
int check_and_skip_comma(char *line, int i);
int is_valid_instruction_opcode(char *op_name);

#endif
