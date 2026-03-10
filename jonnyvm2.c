/*
  Author: Toan Nguyen
  Team: 1 person
  COP 3402: Systems Software, Summer 2025
  Compile: gcc -Wall vm.c -o vm
  Usage:   ./vm input.txt
*/
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE 500

// The entire process-address-space. pas[0..9] unused; pas[10..] holds instructions; pas[..499] is the stack.
static int pas[ARRAY_SIZE];

// PM/0 registers
static int PC;    // Program Counter
static int BP;    // Base Pointer
static int SP;    // Stack Pointer

// Instruction Register (three fields: OP, L, M)
static int IR[3];

// To track Activation Record bases: arBase[0] = newest AR, arBase[arCount−1] = bottommost (initially 499).
static int arBase[ARRAY_SIZE];
static int arCount = 0;

// Prototypes
int base(int b, int L);
void printStack(void);

int main(int argc, char *argv[])
{
    FILE *ifp;
    int i, count, op, L, M;
    int running;

    // 1) Exactly one command-line argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    // 2) Open input file or error out
    ifp = fopen(argv[1], "r");
    if (ifp == NULL) {
        fprintf(stderr, "Error: cannot open \"%s\"\n", argv[1]);
        return 2;
    }

    // 3) Zero out pas[]
    for (i = 0; i < ARRAY_SIZE; i++) {
        pas[i] = 0;
    }

    // 4) Read triples (OP, L, M) into pas[], starting at index 10
    i = 10;
    while (1) {
        count = fscanf(ifp, "%d %d %d", &op, &L, &M);
        if (count != 3) break;
        if (i + 2 >= ARRAY_SIZE) {
            fprintf(stderr, "Error: program too large to fit in memory\n");
            fclose(ifp);
            return 3;
        }
        pas[i    ] = op;
        pas[i + 1] = L;
        pas[i + 2] = M;
        i += 3;
    }
    fclose(ifp);

    // 5) Initialize registers and AR list:
    //    BP = 499, SP = 500, PC = 10 (text starts at pas[10]),
    //    arCount = 1, arBase[0] = 499 (bottommost AR).
    BP = 499;
    SP = 500;
    PC = 10;

    arCount = 1;
    arBase[0] = 499;

    // 6) Print header and initial values exactly as in Appendix C:
    printf("                PC   BP   SP  stack\n");
    printf("Initial values: 10   499  500\n");

    // 7) Fetch-Execute loop until SYS 0 3 (halt).
    running = 1;
    while (running) {
        // fetch (undo the +1 shift from the compiler)
        IR[0] = pas[PC] - 1;      // real opcode is stored as (op + 1)
        IR[1] = pas[PC + 1];
        IR[2] = pas[PC + 2];
        PC += 3;

        // execute
        switch (IR[0]) {
            case 1: // INC  0,M ⇒ SP = SP - M
                SP -= IR[2];
                break;

            case 2: // OPR 0,M ⇒ RTN, arithmetic/logical ops, or modulus
                switch (IR[2]) {
                    case 0: // RTN
                        SP = BP + 1;
                        BP = pas[SP - 2];
                        PC = pas[SP - 3];
                        if (arCount > 0) {
                            for (int j = 0; j < arCount - 1; j++) arBase[j] = arBase[j + 1];
                            arCount--;
                        }
                        break;
                    case 1: // ADD
                        pas[SP+1] = pas[SP+1] + pas[SP];
                        SP++;
                        break;
                    case 2: // SUB
                        pas[SP+1] = pas[SP+1] - pas[SP];
                        SP++;
                        break;
                    case 3: // MUL
                        pas[SP+1] = pas[SP+1] * pas[SP];
                        SP++;
                        break;
                    case 4: // DIV
                        pas[SP+1] = pas[SP+1] / pas[SP];
                        SP++;
                        break;
                    case 5: // EQL
                        pas[SP+1] = (pas[SP+1] == pas[SP]) ? 1 : 0;
                        SP++;
                        break;
                    case 6: // NEQ
                        pas[SP+1] = (pas[SP+1] != pas[SP]) ? 1 : 0;
                        SP++;
                        break;
                    case 7: // LSS
                        pas[SP+1] = (pas[SP+1] < pas[SP]) ? 1 : 0;
                        SP++;
                        break;
                    case 8: // LEQ
                        pas[SP+1] = (pas[SP+1] <= pas[SP]) ? 1 : 0;
                        SP++;
                        break;
                    case 9: // GTR
                        pas[SP+1] = (pas[SP+1] > pas[SP]) ? 1 : 0;
                        SP++;
                        break;
                    case 10: // GEQ
                        pas[SP+1] = (pas[SP+1] >= pas[SP]) ? 1 : 0;
                        SP++;
                        break;
                    case 11: // MOD
                        pas[SP+1] = pas[SP+1] % pas[SP];
                        SP++;
                        break;
                    default:
                        fprintf(stderr, "Illegal OPR code: %d\n", IR[2]);
                        return 4;
                }
                break;

            case 3: { // LOD L,M ⇒ SP = SP - 1; pas[SP] = pas[ base(BP,L) - M ]
                int addr = base(BP, IR[1]) - IR[2];
                SP--;
                pas[SP] = pas[addr];
            } break;

            case 4: { // STO L,M ⇒ pas[ base(BP,L) - M ] = pas[SP] ; SP = SP + 1
                int addr = base(BP, IR[1]) - IR[2];
                pas[addr] = pas[SP];
                SP++;
            } break;

            case 5: { // CAL L,M ⇒ new activation record
                int newSL = base(BP, IR[1]);
                pas[SP-1] = newSL;   // static link
                pas[SP-2] = BP;      // dynamic link
                pas[SP-3] = PC;      // return address
                BP = SP - 1;
                PC = IR[2];
                for (int j = arCount; j > 0; j--) arBase[j] = arBase[j - 1];
                arBase[0] = BP;
                arCount++;
            } break;

            case 6: // LIT 0,M ⇒ SP = SP - 1; pas[SP] = M
                SP--;
                pas[SP] = IR[2];
                break;

            case 7: // JMP 0,M ⇒ PC = M
                PC = IR[2];
                break;

            case 8: // JPC 0,M ⇒ if pas[SP]==0 then PC=M; SP=SP+1
                if (pas[SP] == 0) PC = IR[2];
                SP++;
                break;

            case 9: // SYS 0,M ⇒ I/O or halt
                switch (IR[2]) {
                    case 1: {
                        int val = pas[SP]; SP++;
                        printf("Output result is: %d\n", val);
                    } break;
                    case 2: {
                        int inVal;
                        printf("Please Enter an Integer: "); fflush(stdout);
                        if (scanf("%d", &inVal) != 1) {
                            fprintf(stderr, "Error: expected integer input\n");
                            return 5;
                        }
                        SP--;
                        pas[SP] = inVal;
                    } break;
                    case 3:
                        running = 0;
                        break;
                    default:
                        fprintf(stderr, "Illegal SYS code: %d\n", IR[2]);
                        return 6;
                }
                break;

            default:
                fprintf(stderr, "Illegal OP code: %d\n", IR[0]);
                return 7;
        }

        // (your tracing/printing logic goes here)
        printf(" "); printStack(); printf("\n");
    }

    return 0;
}

// static link lookup
int base(int b, int L)
{
    int arb = b;
    while (L > 0) {
        arb = pas[arb];
        L--;
    }
    return arb;
}

// print stack from pas[499] down to pas[SP]
void printStack(void)
{
    for (int i = 499; i >= SP; i--) {
        for (int j = 0; j < arCount - 1; j++) {
            if (i == arBase[j]) {
                printf("| ");
                break;
            }
        }
        printf("%d ", pas[i]);
    }
}
