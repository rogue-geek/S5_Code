#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPTAB "optab.txt"
#define OPLEN 6
#define SYMLEN 256
#define BUF_LEN 1024
#define WORD_SIZE 3
#define COMMENT_CHAR '.'

struct Op{
    int code;
    char name[24];
};

struct Op **optab;
struct Op **symtab;
int op_n = 0;

void loadOpCode(){
    FILE *f = fopen(OPTAB, "r");
    while(!feof(f)){
        struct Op *o = malloc(sizeof(struct Op));
        fscanf(f, "%s %d", o->name, &o->code);
        optab[op_n++] = o;
    }
    fclose(f);
}

// Compare op-codes ignoring case and whitespace. 1=same, 0=diff
char opMatch(char *c1, char *c2){
    char a, b;
    for(int i=0, j=0; c1[i] != 0 && c2[j] != 0; i++, j++){
        while(c1[i] == 9 || c1[i] == 32) i++;
        while(c2[j] == 9 || c2[j] == 32) j++;
        a = c1[i]; b = c2[j];
        // To make capital, do c - 'a' + 'A', or c - 32
        if(a >= 'a' && a <= 'z')
            a -= 32;
        if(b >= 'a' && b <= 'z')
            b -= 32;
        if(a != b)
            return 0;
    }
    return 1;
}

int isValidOp(char *op){
    for(int i=0; i<op_n; i++){
        if(opMatch(optab[i]->name, op))
            return optab[i]->code;
    }
    return -1;
}

// Counts and parses params in line and identifies comments and blank lines
char paramCount(char *line, char *lab, char *op, char *opr){
    int i=0, j=0, n=0;
    while((line[i] == 32 || line[i] == 9) && line[i] != 0) i++;
    while(line[i] != 32 && line[i] != 9 && line[i] != 10 && line[i] != 0)
        lab[j++] = line[i++];
    lab[j] = 0;
    j = 0; n++;
    
    while((line[i] == 32 || line[i] == 9) && line[i] != 0) i++;
    while(line[i] != 32 && line[i] != 9 && line[i] != 10 && line[i] != 0)
        op[j++] = line[i++];
    op[j] = 0;
    j = 0; n++;
    
    while(line[i] == 32 || line[i] == 9 || line[i] == 10 && line[i] != 0) i++; 
    if(line[i] == 0){
        strcpy(opr, op);
        strcpy(op, lab);
        lab[0] = 0;
        return n;
    }
    while(line[i] != 0 && line[i] != 10)
        opr[j++] = line[i++];
    opr[j] = 0;
    j = 0; n++;
    return n;
}

int str2int(char *str){
    int op = 0; char neg = 0;
    for(int i=0; str[i]!=0; i++){
        if((str[i] >= '0' && str[i] <= '9') || str[i] == '-')
            if(str[i] == '-')
                neg = 1;
            else
                op = op*10 + str[i] - '0';
    }
    if(neg) op *= -1;
    return op;
}

void main(int argc, char **argv){
    int locctr = 0, sym_i = 0;
    optab = malloc(sizeof(optab)*OPLEN);
    symtab = malloc(sizeof(optab)*SYMLEN);
    // Load OPCodes from file
    loadOpCode();
    char *line = malloc(BUF_LEN);
    char *label = malloc(BUF_LEN/2);
    char *operation = malloc(BUF_LEN/2);
    char *operator = malloc(BUF_LEN/2);
    int sym_n, line_n = 0;
    // Read from terminal asm and destination filenames
    if(argc >= 4){
        FILE *prog = fopen(argv[1], "r");
        FILE *objc = fopen(argv[2], "w");
        FILE *symtab_f = fopen(argv[3], "w");
        // Read each lines of program
        while(!feof(prog)){
            fgets(line, BUF_LEN, prog);
            sym_n = paramCount(line, label, operation, operator);
            // Skip if comment line or blank line
            if(sym_n == 0)
                continue;
            // Is code line. Start parsing.
            line_n++;
            // 3 param line
            if(sym_n == 3){
                printf("Line %d has %d symbols [%s] [%s] [%s]\n", line_n, sym_n, label, operation, operator);
                // Write label into symtab file
                struct Op *sym = malloc(sizeof(struct Op));
                for(int k=0; k<sym_i; k++){
                    if(!strcmp(symtab[k]->name, label)){
                        printf("%d | %s\n[ERROR] At line %d: Duplicate label '%s'\n", line_n, line, line_n, label);
                        return;
                    }
                }
                if(strcmp(label, "") != 0){
                    printf("New label %s at %d\n", label, locctr);         
                    strcpy(sym->name, label);
                    sym->code = locctr;
                    symtab[sym_i++] = sym;
                }
            }
            //printf("Line %d has %d symbols [%s] [%s] [%s]\n", line_n, sym_n, label, operation, operator);

            // Translate assembler directives
            if(opMatch(operation, "END"))
                break;
            else if(opMatch(operation, "START"))
                locctr = str2int(operator);
            else if(opMatch(operation, "RESW"))
                locctr += str2int(operator)*WORD_SIZE;
            else if(opMatch(operation, "RESB"))
                locctr += str2int(operator);
            else if(opMatch(operation, "WORD"))
                locctr += WORD_SIZE;
            else if(opMatch(operation, "BYTE"))
                locctr += 1;
            // Translate opcodes
            else{
                int opcode = isValidOp(operation);
                // Write intermediate file
                if(opcode > -1){
                    fprintf(objc, "%x\t%s\n", opcode, operator);
                    locctr += 3;
                }
                else
                    printf("%d | %s\n[ERROR] At line %d: Unknown mnemonic '%s'\n", line_n, line, line_n, operation);
            }
        }
        // Write symtab to file
        for(int i=0; i<sym_n; i++)
            fprintf(symtab_f, "%s %d\n", label, locctr);
    }
    else
        printf("No input files!!\nRun as %s <asm code> <obj dest> <symtab dest>\n", argv[0]);
}
