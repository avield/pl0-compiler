#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXSYMBOLS    500    // maximum entries in symbol table
#define MAXCODE      2000    // maximum P-code instructions
#define MAXSOURCE   10000    // maximum source file size
#define MAX_ID_LEN     11    // maximum identifier length
#define MAX_NUM_LEN     5    // maximum numeric literal length

// token types
typedef enum {
    modsym=1, identsym, numbersym,
    plussym, minussym, multsym, slashsym,
    fisym,
    eqlsym, neqsym, lessym, leqsym,
    gtrsym, geqsym,
    lparentsym, rparentsym,
    commasym, semicolonsym, periodsym,
    becomessym,
    beginsym, endsym,
    ifsym, thensym, whilesym, dosym,
    callsym, constsym, varsym, procsym,
    writesym, readsym, elsesym
} token_type;

// VM opcodes emitted by compiler
enum { OP_INC=0, OP_OPR, OP_LOD, OP_STO, OP_CAL,
       OP_LIT, OP_JMP, OP_JPC, OP_SYS };

// entry in the symbol table
typedef struct {
    char name[12];    // identifier name
    int kind;         // 1=const,2=var,3=proc
    int value;        // constant value (for const)
    int level;        // lexical nesting level
    int addr;         // memory address or entry point
    int mark;         // 0=active,1=inactive
} symbol;

// a single P-code instruction
typedef struct {
    int op, l, m;
} instr;

// globals for compiler state
static symbol    symTab[MAXSYMBOLS];
static int       symCount  = 0;
static instr     code[MAXCODE];
static int       codeCount = 0;

static token_type tokenArr[MAXCODE];
static char       lexeme[MAXCODE][MAX_ID_LEN+2];
static int        lexCount = 0, lexIndex = -1;
static char      *source;    // entire source text

// forward declarations
void scanAll(const char *fn);
void nextToken(void);
void error(const char *msg);
int  symbolTableCheck(const char *name);
void markSymbols(int level);
void emit(int op,int l,int m);
void program(void);
void block(int level);
void constDecl(int level);
int  varDecl(int level);
void procDecl(int level);
void statement(int level);
void condition(int level);
void expression(int level);
void term(int level);
void factor(int level);
void printSource(void);
void writeElf(void);

// names for printing assembly
static const char *opNames[] = {
    "INC","OPR","LOD","STO","CAL","LIT","JMP","JPC","SYS"
};

// reserved keywords mapping
static const char *reservedWords[] = {
  "mod","begin","end","if","then","else","fi","when","do",
  "call","const","var","procedure","write","read"
};
static const token_type reservedTokenVals[] = {
  modsym, beginsym, endsym, ifsym, thensym, elsesym, fisym,
  whilesym, dosym, callsym, constsym, varsym, procsym, writesym, readsym
};
static const int NUM_RESERVED = sizeof(reservedWords)/sizeof(reservedWords[0]);


// SCANNER: read file, break into tokens

void scanAll(const char *fn) {
    FILE *f = fopen(fn,"r");
    if (!f) { perror("fopen"); exit(1); }
    source = malloc(MAXSOURCE);
    int len = fread(source,1,MAXSOURCE-1,f);
    source[len] = '\0';
    fclose(f);

    char *p = source;
    while (*p) {
        if (lexCount >= MAXCODE) error("Too many tokens.");
        if (isspace((unsigned char)*p)) { p++; continue; }

        // identifiers and reserved words
        if (isalpha((unsigned char)*p)) {
            char buf[MAX_ID_LEN+2]; int n=0;
            while ((isalnum((unsigned char)*p)||*p=='_') && n<MAX_ID_LEN+1)
                buf[n++]=*p++;
            buf[n]='\0';
            if (n>MAX_ID_LEN) error("Identifier too long.");
            int found=0;
            for (int i=0; i<NUM_RESERVED; i++){
                if (!strcmp(buf,reservedWords[i])){
                    tokenArr[lexCount]=reservedTokenVals[i];
                    strcpy(lexeme[lexCount],buf);
                    lexCount++; found=1; break;
                }
            }
            if (!found) {
                tokenArr[lexCount]=identsym;
                strcpy(lexeme[lexCount],buf);
                lexCount++;
            }
            continue;
        }

        // numeric literals
        if (isdigit((unsigned char)*p)) {
            char buf[MAX_NUM_LEN+2]; int n=0;
            while (isdigit((unsigned char)*p)&&n<MAX_NUM_LEN+1)
                buf[n++]=*p++;
            buf[n]='\0';
            if (n>MAX_NUM_LEN) error("This number is too large.");
            tokenArr[lexCount]=numbersym;
            strcpy(lexeme[lexCount],buf);
            lexCount++;
            continue;
        }

        // two-character tokens
        if (p[0]==':'&&p[1]=='=') { tokenArr[lexCount]=becomessym; strcpy(lexeme[lexCount],":="); lexCount++; p+=2; continue; }
        if (p[0]=='<'&&p[1]=='>') { tokenArr[lexCount]=neqsym;  strcpy(lexeme[lexCount],"<>" ); lexCount++; p+=2; continue; }
        if (p[0]=='<'&&p[1]=='=') { tokenArr[lexCount]=leqsym;  strcpy(lexeme[lexCount],"<=" ); lexCount++; p+=2; continue; }
        if (p[0]=='>'&&p[1]=='=') { tokenArr[lexCount]=geqsym;  strcpy(lexeme[lexCount],">=" ); lexCount++; p+=2; continue; }

        // single-character tokens
        switch(*p){
          case '+': tokenArr[lexCount]=plussym;      lexeme[lexCount][0]='+'; break;
          case '-': tokenArr[lexCount]=minussym;     lexeme[lexCount][0]='-'; break;
          case '*': tokenArr[lexCount]=multsym;      lexeme[lexCount][0]='*'; break;
          case '/': tokenArr[lexCount]=slashsym;     lexeme[lexCount][0]='/'; break;
          case '=': tokenArr[lexCount]=eqlsym;       lexeme[lexCount][0]='='; break;
          case '<': tokenArr[lexCount]=lessym;       lexeme[lexCount][0]='<'; break;
          case '>': tokenArr[lexCount]=gtrsym;       lexeme[lexCount][0]='>'; break;
          case '(': tokenArr[lexCount]=lparentsym;   lexeme[lexCount][0]='('; break;
          case ')': tokenArr[lexCount]=rparentsym;   lexeme[lexCount][0]=')'; break;
          case ',': tokenArr[lexCount]=commasym;     lexeme[lexCount][0]=','; break;
          case ';': tokenArr[lexCount]=semicolonsym; lexeme[lexCount][0]=';'; break;
          case '.': tokenArr[lexCount]=periodsym;    lexeme[lexCount][0]='.'; break;
          default:  error("Invalid symbol.");
        }
        lexeme[lexCount][1]=0;
        lexCount++;
        p++;
    }
    // final period token as end marker
    tokenArr[lexCount++] = periodsym;
}

// advance to next token (never past the last)
void nextToken(void) {
    if (lexIndex < lexCount-1)
        lexIndex++;
}

// print an error message and exit
void error(const char *msg) {
    printf("Error: %s\n", msg);
    exit(1);
}

// find most recent active symbol with given name
int symbolTableCheck(const char *name) {
    for (int i = symCount-1; i >= 0; i--)
        if (!symTab[i].mark && !strcmp(symTab[i].name,name))
            return i;
    return -1;
}

// mark all symbols at a given level inactive
void markSymbols(int level) {
    for (int i=0; i<symCount; i++)
        if (symTab[i].level == level) symTab[i].mark = 1;
}

// emit a P-code instruction
void emit(int op,int l,int m) {
    if (codeCount >= MAXCODE) error("Code overflow.");
    code[codeCount].op = op;
    code[codeCount].l  = l;
    code[codeCount].m  = m;
    codeCount++;
}


// RECURSIVE DESCENT PARSER


// factor ::= ident | number | "(" expression ")"
void factor(int lvl) {
    if (tokenArr[lexIndex] == identsym) {
        char *nm = lexeme[lexIndex];
        int idx = symbolTableCheck(nm);
        if (idx < 0) { printf("Error: undeclared identifier %s\n", nm); exit(1); }
        if (symTab[idx].kind == 3) error("Expression must not contain a procedure identifier.");
        if (symTab[idx].kind == 1)
            emit(OP_LIT,0,symTab[idx].value);
        else
            emit(OP_LOD, lvl - symTab[idx].level, symTab[idx].addr);
        nextToken();
    }
    else if (tokenArr[lexIndex] == numbersym) {
        emit(OP_LIT,0,atoi(lexeme[lexIndex]));
        nextToken();
    }
    else if (tokenArr[lexIndex] == lparentsym) {
        nextToken(); expression(lvl);
        if (tokenArr[lexIndex] != rparentsym) error("Right parenthesis missing.");
        nextToken();
    }
    else
        error("The preceding factor cannot begin with this symbol.");
}

// term ::= factor {("*"|"/"|"mod") factor}
void term(int lvl) {
    factor(lvl);
    while (tokenArr[lexIndex]==multsym
        || tokenArr[lexIndex]==slashsym
        || tokenArr[lexIndex]==modsym) {
        token_type op = tokenArr[lexIndex++];
        factor(lvl);
        if (op == multsym)       emit(OP_OPR,0,3);   // MUL
        else if (op == slashsym) emit(OP_OPR,0,4);   // DIV
        else                     emit(OP_OPR,0,11);  // MOD
    }
}

// expression ::= term {("+"|"-") term}
void expression(int lvl) {
    token_type s = tokenArr[lexIndex];
    if (s==plussym||s==minussym) lexIndex++;
    term(lvl);
    if (s==minussym)            // unary minus
        emit(OP_OPR,0,2);       // SUB
    while (tokenArr[lexIndex]==plussym
        || tokenArr[lexIndex]==minussym) {
        token_type op = tokenArr[lexIndex++];
        term(lvl);
        emit(OP_OPR,0, op==plussym?1:2);  // ADD=1, SUB=2
    }
}

// condition ::= expression rel-op expression
void condition(int lvl) {
    expression(lvl);
    token_type rel = tokenArr[lexIndex++];
    if (rel < eqlsym || rel > geqsym) error("Relational operator expected.");
    expression(lvl);
    int m = 0;
    switch (rel) {
      case eqlsym:  m = 5;  break;  // EQL
      case neqsym:  m = 6;  break;  // NEQ
      case lessym:  m = 7;  break;  // LSS
      case leqsym:  m = 8;  break;  // LEQ
      case gtrsym:  m = 9;  break;  // GTR
      case geqsym:  m = 10; break;  // GEQ
      default: break;
    }
    emit(OP_OPR,0,m);
}

// varDecl ::= ["var" ident {"," ident} ";"]
int varDecl(int lvl) {
    int n = 0;
    if (tokenArr[lexIndex] == varsym) {
        do {
            nextToken();
            if (tokenArr[lexIndex] != identsym) error("Identifier must be followed by =.");
            char *nm = lexeme[lexIndex];
            for (int i = 0; i < symCount; i++)
                if (!symTab[i].mark && symTab[i].level==lvl && !strcmp(symTab[i].name,nm))
                    error("Duplicate identifier.");
            strcpy(symTab[symCount].name, nm);
            symTab[symCount].kind  = 2;
            symTab[symCount].level = lvl;
            symTab[symCount].addr  = 3 + n++;
            symTab[symCount].mark  = 0;
            symCount++;
            nextToken();
        } while (tokenArr[lexIndex] == commasym);
        if (tokenArr[lexIndex] != semicolonsym) error("Semicolon or comma missing.");
        nextToken();
    }
    return n;
}

// constDecl ::= ["const" ident "=" number {"," ident "=" number} ";"]
void constDecl(int lvl) {
    if (tokenArr[lexIndex] == constsym) {
        do {
            nextToken();
            if (tokenArr[lexIndex] != identsym) error("const, var, procedure must be followed by identifier.");
            char name[12]; strcpy(name,lexeme[lexIndex]);
            for (int i = 0; i < symCount; i++)
                if (!symTab[i].mark && symTab[i].level==lvl && !strcmp(symTab[i].name,name))
                    error("Duplicate identifier.");
            nextToken();
            if (tokenArr[lexIndex] != eqlsym) error("Use = instead of :=.");
            nextToken();
            if (tokenArr[lexIndex] != numbersym) error("= must be followed by a number.");
            symTab[symCount].kind  = 1;
            strcpy(symTab[symCount].name, name);
            symTab[symCount].value = atoi(lexeme[lexIndex]);
            symTab[symCount].level = lvl;
            symTab[symCount].mark  = 0;
            symCount++;
            nextToken();
        } while (tokenArr[lexIndex] == commasym);
        if (tokenArr[lexIndex] != semicolonsym) error("Semicolon or comma missing.");
        nextToken();
    }
}

// procDecl ::= { "procedure" ident ";" block ";" }
void procDecl(int lvl) {
    while (tokenArr[lexIndex] == procsym) {
        nextToken();
        if (tokenArr[lexIndex] != identsym) error("const, var, procedure must be followed by identifier.");
        char name[12]; strcpy(name,lexeme[lexIndex]);
        for (int i = 0; i < symCount; i++)
            if (!symTab[i].mark && symTab[i].level==lvl && !strcmp(symTab[i].name,name))
                error("Duplicate identifier.");
        nextToken();
        if (tokenArr[lexIndex] != semicolonsym) error("Incorrect symbol after procedure declaration.");
        nextToken();
        symTab[symCount].kind  = 3;
        strcpy(symTab[symCount].name, name);
        symTab[symCount].level = lvl;
        symTab[symCount].addr  = codeCount*3 + 10;
        symTab[symCount].mark  = 0;
        symCount++;
        block(lvl+1);
        emit(OP_OPR,0,0);  // RTN
    }
}

// statement ::= assignment | call | begin...end | if...then...else...fi | while...do | read | write | empty
void statement(int lvl) {
    if (tokenArr[lexIndex] == identsym) {
        char *nm = lexeme[lexIndex];
        int idx = symbolTableCheck(nm);
        if (idx < 0) { printf("Error: undeclared identifier %s\n", nm); exit(1); }
        if (symTab[idx].kind != 2) error("Assignment to constant or procedure is not allowed.");
        nextToken();
        if (tokenArr[lexIndex] != becomessym) error("Assignment operator expected.");
        nextToken();
        expression(lvl);
        emit(OP_STO, lvl - symTab[idx].level, symTab[idx].addr);
    }
    else if (tokenArr[lexIndex] == callsym) {
        nextToken();
        if (tokenArr[lexIndex] != identsym) error("call must be followed by an identifier.");
        char *nm = lexeme[lexIndex];
        int idx = symbolTableCheck(nm);
        if (idx < 0) { printf("Error: undeclared identifier %s\n", nm); exit(1); }
        if (symTab[idx].kind != 3) error("Call of a constant or variable is meaningless.");
        emit(OP_CAL, lvl - symTab[idx].level, symTab[idx].addr);
        nextToken();
    }
    else if (tokenArr[lexIndex] == beginsym) {
        nextToken();
        statement(lvl);
        while (tokenArr[lexIndex] == semicolonsym) {
            nextToken();
            statement(lvl);
        }
        if (tokenArr[lexIndex] != endsym) error("Semicolon between statements missing.");
        nextToken();  // consume 'end'
        if (tokenArr[lexIndex] == semicolonsym) nextToken();
    }
    else if (tokenArr[lexIndex] == ifsym) {
        nextToken();
        condition(lvl);
        if (tokenArr[lexIndex] != thensym) error("then expected.");
        nextToken();
        int jpcIdx = codeCount; emit(OP_JPC,0,0);
        statement(lvl);
        if (tokenArr[lexIndex] != elsesym) error("Incorrect symbol following statement.");
        nextToken();
        int jmpIdx = codeCount; emit(OP_JMP,0,0);
        code[jpcIdx].m = (codeCount)*3 + 10;
        statement(lvl);
        if (tokenArr[lexIndex] != fisym) error("Incorrect symbol following statement.");
        nextToken();
        code[jmpIdx].m = (codeCount)*3 + 10;
    }
    else if (tokenArr[lexIndex] == whilesym) {
        nextToken();
        int loopIdx = codeCount;
        condition(lvl);
        if (tokenArr[lexIndex] != dosym) error("do expected.");
        nextToken();
        int jpcIdx = codeCount; emit(OP_JPC,0,0);
        statement(lvl);
        emit(OP_JMP,0,loopIdx*3+10);
        code[jpcIdx].m = (codeCount)*3 + 10;
    }
    else if (tokenArr[lexIndex] == readsym) {
        nextToken();
        if (tokenArr[lexIndex] != identsym) error("read must be followed by identifier.");
        char *nm = lexeme[lexIndex];
        int idx = symbolTableCheck(nm);
        if (idx < 0) { printf("Error: undeclared identifier %s\n", nm); exit(1); }
        if (symTab[idx].kind != 2) error("Incorrect symbol following statement.");
        emit(OP_SYS,0,2);
        emit(OP_STO, lvl - symTab[idx].level, symTab[idx].addr);
        nextToken();
    }
    else if (tokenArr[lexIndex] == writesym) {
        nextToken();
        expression(lvl);
        emit(OP_SYS,0,1);
    }
    // else empty
}

// parse a block of code
void block(int lvl) {
    int jmpIdx = codeCount; emit(OP_JMP,0,0);
    constDecl(lvl);
    int nvars = varDecl(lvl);
    procDecl(lvl);
    emit(OP_INC,0,3 + nvars);
    code[jmpIdx].m = (codeCount-1)*3 + 10;
    statement(lvl);
    markSymbols(lvl);
}

// top-level program entry
void program(void) {
    block(0);
    while (tokenArr[lexIndex] == semicolonsym) nextToken();
    if (tokenArr[lexIndex] != periodsym) error("Period expected.");
    nextToken();
    emit(OP_SYS,0,3);
}

// echo original source
void printSource(void) {
    printf("%s\n", source);
}

// write P-code to elf.txt for VM (opcodes +1)
void writeElf(void) {
    FILE *elf = fopen("elf.txt","w");
    if (!elf) { perror("fopen elf.txt"); exit(1); }
    for (int i = 0; i < codeCount; i++) {
        fprintf(elf, "%d %d %d\n",
                code[i].op + 1,
                code[i].l,
                code[i].m);
    }
    fclose(elf);
}

// main: tokenize, compile, print tables, and emit elf.txt
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <sourcefile>\n", argv[0]);
        return 1;
    }
    scanAll(argv[1]);
    printSource();
    nextToken();
    program();

    printf("No errors, program is syntactically correct.\n\n");
    printf("Assembly Code:\nLine\tOP\tL\tM\n");
    for (int i = 0; i < codeCount; i++) {
        printf("%2d\t%-3s\t%1d\t%2d\n",
               i, opNames[code[i].op],
               code[i].l, code[i].m);
    }

    printf("\nSymbol Table:\n");
    printf("Kind | Name        | Value | Level | Address | Mark\n");
    printf("----------------------------------------------------\n");
    for (int i = 0; i < symCount; i++) {
        printf("  %d  | %-10s |  %4d |   %2d   |   %4d   |   %d\n",
               symTab[i].kind,
               symTab[i].name,
               symTab[i].value,
               symTab[i].level,
               symTab[i].addr,
               symTab[i].mark);
    }

    writeElf();
    return 0;
}
