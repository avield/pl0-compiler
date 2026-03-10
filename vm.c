//Aviel Dahaman
//COP 3402, Summer 25
//Assignment: Homework 4 (modified from Homework 1)
//Description: This program will implement a virtual machine instruction set architecture
// that acts as a P-Machine.

#include <stdio.h>
#include <stdlib.h>

//pas is the Process Address Space. The array functions as the main memory the 
//virtual machine will access. It is a globally accessible array.
int pas[500] = {0};

//All P-Machines use an Instruction Register to show the currently executing instruction
typedef struct IR_s{
    int OP; //OP hold the operation code
    int L; //L holds the lexigraphical level in the stack
    int M; //M is a modifier. It can hold a data address, a literal, an OPR code, or a program address  
} InstructionRegister;

int base(int, int);

int main (int argc, char **argv){
    //Checking that an input file is supplied when the program is run
    if (argc < 2){
        printf("No input file. Please run this again with the input file.\n");
        return 1;
    }

    //Opening the file supplied by the command line and making sure it is found
    FILE *inputFile = fopen(argv[1], "r");
    if (inputFile == NULL){
        printf("Input file not found.\n");
        return 1;
    }

    //Memory data begins at location 10 in the PAS. The spots 0-9 are part of the "unused" partition 
    int textPosition = 10;
    //This loop reads the instructions from the input file and puts them into memory
    while (!feof(inputFile)){
        //Reading the 3 integers and placing each into the pas memory array
        fscanf(inputFile, "%d %d %d", &pas[textPosition], &pas[textPosition+1], &pas[textPosition+2]);
        textPosition += 3;
        
        //To allow comments in the input file, this loop will move the cursor until the next newline character
        char c = ' ';
        while((c = fgetc(inputFile)) != '\n' && c != EOF){
            continue;
        }
    }

    //Closing the input file now that it has been read into memory
    fclose(inputFile);

    
    //Registers that will be used by the P-Machine
    //The instruction register (IR) holds the currently executing instruction. 
    InstructionRegister IR;
    //PC holds the Program Counter, keeping track of the next instruction to be executed
    int PC = 10;
    //SP holds the Stack Pointer. The stack grows from the right in this machine 
    //so it starts outside the memory array at 500 and decreases as it grows
    int SP = 500;
    //BP holds the Base Pointer. It always points to the base of the current activation record.
    //It starts pointing at the last slot in the array
    int BP = 499;

    //AR_Bases is an array that holds the activation record bases for each activation record
    //This will be helpful in separating activation records while displaying the stack
    int AR_Bases[500] = {0};
    //arCount will be the index of the AR_Bases array
    int arCount = 0;

    //Flag to halt the virtual machine
    int haltFlag = 0;

    //Printing headers for the output display
    printf("\t\t\tPC\tBP\tSP\tstack\n");
    printf("Initial values:\t\t%d\t%d\t%d\n", PC, BP, SP);

    //Beginning the Virtual P-Machine
    while (haltFlag == 0){
        //Fetch Cycle
        //Populating the Instruction Register information from the stored instructions.
        IR.OP = pas[PC]; //holds the current op code
        IR.L = pas[PC+1]; //the lexigraphical level of data addresses
        IR.M = pas[PC+2]; //the modifer: a literal or an address
        //Incrementing the Program Counter to point to the next instructions
        PC = PC + 3;

        //Execute Instruction
        //Executes the correct instruction based on the IR's op code
        //Because the stack grows from the right, whenever the SP is substracted, the stack grows
        //whenever the SP is added, the stack shrinks
        switch (IR.OP){
            case 1: { //INC increments the SP by IR.M amount
                SP = SP - IR.M;
                printf("INC %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                break;
            }
            case 2: { //OPR
                //Since OPR can have different operations based on the IR.M, it has a nested switch
                //to decide which operation to perform on the stack
                switch (IR.M){
                    case 0: { //RTN returns from procedure to caller
                        SP = BP + 1;
                        BP = pas[SP-2];
                        PC = pas[SP-3];
                        printf("RTN %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                        break;
                    }
                    case 1: { //ADD adds the top two values of the stack, decrements the stack, and puts the sum on top
                        pas[SP+1] = pas[SP+1] + pas[SP];
                        SP += 1;
                        printf("ADD %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                        break;
                    }
                    case 2: { //SUB Subtracts the top two values of the stack, decrements the stack, and puts the difference on top
                        pas[SP+1] = pas[SP+1] - pas[SP];
                        SP += 1;
                        printf("SUB %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                        break;
                    }
                    case 3: { //MUL Multiplies the top two values of the stack, decrements the stack, and puts the product on top
                        pas[SP+1] = pas[SP+1] * pas[SP];
                        SP += 1;
                        printf("MUL %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                        break;
                    }
                    case 4: { //DIV Divides the top two values of the stack, decrements the stack, and puts the quotient on top
                        pas[SP+1] = pas[SP+1] / pas[SP];
                        SP += 1;
                        printf("DIV %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                        break;
                    }
                    case 5: { //EQL Equals compares the top two values of the stack for equality, decrements the stack, and puts a 1 or 0 on top
                        pas[SP+1] = pas[SP+1] == pas[SP];
                        SP += 1;
                        printf("EQL %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                        break;
                    }
                    case 6:{ //NEQ compares the top two values of the stack for inequality, decrements the stack, and puts a 1 or 0 on top
                        pas[SP+1] = pas[SP+1] != pas[SP];
                        SP += 1;
                        printf("NEQ %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                        break;
                    }
                    case 7: { //LSS compares if the top value is greater than the value below it, decrements the stack, and puts a 1 or 0 on top
                        pas[SP+1] = pas[SP+1] < pas[SP];
                        SP += 1;
                        printf("LSS %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                        break;
                    }
                    case 8: { //LEQ compares if the top value is greater than or equal to the value below it, decrements the stack, and puts a 1 or 0 on top
                        pas[SP+1] = pas[SP+1] <= pas[SP];
                        SP += 1;
                        printf("LEQ %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                        break;
                    }
                    case 9: { //GTR compares if the top value is less than the value below it, decrements the stack, and puts a 1 or 0 on top
                        pas[SP+1] = pas[SP+1] > pas[SP];
                        SP += 1;
                        printf("GTR %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                        break;
                    }
                    case 10: { //GEQ compares if the top value is less than or equal to the value below it, decrements the stack, and puts a 1 or 0 on top
                        pas[SP+1] = pas[SP+1] >= pas[SP];
                        SP += 1;
                        printf("GEQ %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                        break;
                    }
                    case 11: { //Modulus: pop two values from the stack, divide second by first, and push the remainer
                        pas[SP+1] = pas[SP+1] % pas[SP];
                        SP += 1;
                        printf("MOD %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                        break;
                    }
                    default:
                        printf("Invalid OPR instruction.\n");
                        break;
                    } //End of OPR Swtich
                break;
            }
            case 3: { //LOD loads the value at the address determined by (IR.L, IR.M), increments the SP, and puts it on top of the stack
                SP -= 1;
                pas[SP] = pas[base(BP, IR.L) - IR.M];
                printf("LOD %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                break;
            }
            case 4: { //STO stores the value on top of the stack in the address (IR.L, IR.M) and decrements the SP
                pas[base(BP, IR.L) - IR.M] = pas[SP];
                SP += 1;
                printf("STO %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                break;
            }
            case 5: { //CAL calls a procedure by creating a new Activation Record (AR) and populating
                //its Static Link, Dynamic Link, and Return Address. Then setting the base pointer to
                //the bottom of the new record and the PC to the next instruction in that procedure.
                pas[SP-1] = base(BP, IR.L); //Static link
                pas[SP-2] = BP; //Dynamic Link
                pas[SP-3] = PC; //Return Address
                BP = SP-1;
                PC = IR.M;
                AR_Bases[arCount] = BP;
                arCount++;
                printf("CAL %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                break;
            }
            case 6: { //LIT increments the stack and puts a literal (a constant number) on top of the stack
                SP -= 1;
                pas[SP] = IR.M;
                printf("LIT %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                break;
            }
            case 7: { //JMP moves the PC to the address specified by IR.M
                PC = IR.M;
                printf("JMP %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                break;
            }
            case 8: { //JPC moves the PC to IR.M only if the value at the top of the stack is 0, then decrements the SP
                if (pas[SP] == 0){
                    PC = IR.M;
                    SP += 1;
                }
                printf("JPC %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                break;
            }
            case 9: { //SYS Also has a nested switch because it can do 3 operations based on the IR.M
                switch (IR.M){
                    case 1:{ //OUT Prints out the value at the top of the stack and decrements the SP
                        printf("Output result is: %d\n", pas[SP]);
                        SP += 1;
                        break;
                    }
                    case 2: { //IN increments the SP, receives input from the user, and puts it on top of the stack
                        SP -= 1;
                        printf("Please Enter an Integer: ");
                        scanf("%d", &pas[SP]);
                        break;
                    }
                    case 3: { //HALT ends the program
                        haltFlag = 1;
                        break;
                    }
                    default:{
                        printf("Invalid SYS entry.\n");
                        break;
                    }
                } //End of SYS Switch
                printf("SYS %5d %5d\t\t%d\t%d\t%d\t", IR.L, IR.M, PC, BP, SP);
                break;
            }
            default:{
                printf("Invalid instruction.\n");
                break;
            }
        } //End of instruction switch

        //Printing the stack at the end of each instruction
        for (int i = 499; i >= SP; i--){
            printf(" %d", pas[i]);
            
            int arFlag = 0;
            for (int j = 0; j < arCount; j++){
                if (i-1 == AR_Bases[j] && pas[i-1] > 300){
                    arFlag = 1;
                    break;
                }
            }

            if (i == SP){
                printf("\n");
            } else if (arFlag){
                //prints a bar | between activation records
                printf(" | ");
            }
            
        }
        //Special case for if the SP is outside the pas array. The print stack loop
        //doesn't run so this check is required to start a new line.
        if (SP >= 500){
            printf("\n");
        }
        
        //Failsafe to ensure the program doesn't run endlessly or go outside the array bounds
        if (SP > 500 || SP < 0){
            printf("Out of memory.\n");
            break;  
        } 

    } //End of while loop

} //End of main


//Base function provided by the professor
//DO NOT MODIFY
/**********************************************/
/*		Find base L levels down		 */
/*							 */
/**********************************************/
 
int base( int BP, int L)
{
	int arb = BP;	// arb = activation record base
	while ( L > 0)     //find base L levels down
	{
		arb = pas[arb];
		L--;
	}
	return arb;
}