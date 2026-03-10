//Aviel Dahaman
//COP 3402, Summer 2025
//Homework 4: PL/0 Compiler (modified from Homework 3)
/* Description: (This program expands on the lexical analyzer program built for Homework 2 and the 
 * parser and code generator built for Homework 3.)
 * This program combines a lexical analyzer, a syntax parser, and an intermediate code generator
 * to create a PL/0 compiler. It processes the tokens produced by the lexical analyzer to ensure
 * that the next token follows the specifications of the language's syntax. As it processes the syntax,
 * it also generates the assembly language code for each statement. A human-readable assembly code and
 * symbol table are output to the terminal and an assembly code made of integers (not binary)
 * is output to a file named 'elf.txt'. The elf.txt file may be input to the attached VM program
 * to execute the program written in PL/0.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#define PROGRAM_SIZE 500
#define RESERVED_COUNT 34
#define ID_SIZE_LIMIT 12 //11+null terminator
#define MAX_SYMBOL_TABLE_SIZE 500

typedef struct {
    char *resWord;
    int token;
} ReservedWord;

typedef struct {
    int kind;                   //const = 1, var = 2, proc = 3
    char name[ID_SIZE_LIMIT];   //name up to 11 characters
    int val;                    //value number (ASCII value)
    int level;                  //L level
    int addr;                   //M address
    int mark;                   //Mark to indicate in scope or not
} SymbolTable;

typedef struct {
    int OP;
    int L;
    int M;
} Instruction;

//Global symbol table
SymbolTable symbols[MAX_SYMBOL_TABLE_SIZE] = {0};
//symCount is the index for the symbol table
int symCount = 0;
//instrAddress will hold the instruction address for the beginning of a prodecure
int instrAddress = 0;
//procedure address will be the instruction address. Starts at 13,
int procAddress = 13;
//lexLevel will hold current lexical level
int lexLevel = 0;



//Global pointer for tokenList so it doesn't need to be passed to every argument
ReservedWord *tokenList;
//token holds the current token to be processed
int token = 0;
//global counter to keep track of it's size and index
int tokenCount = 0;
//listsize will hold tokenList's capacity and be expanded if needed
int listSize = 200;
//Parser will serve as an index for the token list while processing the tokens and generating code
int parser = 0;

//instructions array will hold the output of the code generator
Instruction *instructions;
//InstIdx will hold the index of the instructions array (created dynamically inside of main so it can be expanded if needed)
int instIdx = 0;
//instrSIZE holds the max capacity of the instructions array
int instrSIZE = 600;



//resWords will hold an array of Reserved Words and Symbols used by the language and the 
//tokens that will represent them
ReservedWord resWords[RESERVED_COUNT] = {0};



//Function prototypes
int getToken();
void markAllSymbols();
void markProcSymbols(char *);
int parsingProgram();
int block();
int constDec();
int varDec();
int procDec();
int statement();
int condition();
int expression();
int term();
int factor();
void codeWriter(int, int, int);
void fileWrite();
int symbolTableCheck(char*);
int checkReserved(char*);
int isValidSymbol(char);
void addSymbolTable(int, char *, int, int, int, int);
void addToken(char*, ReservedWord*, int, int);
void printSource(char *, char *);
void printLexemeTable(ReservedWord*, int);
char* readFile(char*, FILE*, int);
void cleanUp(ReservedWord*, int);
void loadReservedWords(ReservedWord*);




int main(int argc, char **argv){
    //Checking that the program was run with an input file provided
    if (argc < 2){
        printf("No input file provided. Please run the program with an input file.\n");
        return 1;
    }

    //Checking that the file can be found and opened
    FILE *inputFile = fopen(argv[1], "r");
    if (inputFile == NULL){
        printf("Input file not found.");
        return 1;
    }

    //The string (char array) Program will store the entire program being read from the input file
    char *program = malloc(PROGRAM_SIZE*sizeof(char));
    readFile(program,inputFile,PROGRAM_SIZE);
    fclose(inputFile);

    //Initializing variables

    //resWords will hold an array of Reserved Words and Symbols used by the language and the 
    //tokens that will represent them
    //It's a global variable but needs to be loaded with the reserved words and symbols
    loadReservedWords(resWords);

    //tokenList will hold the tokens created for later printing
    tokenList = malloc(listSize*sizeof(ReservedWord));

    //instructions array will hold the output generated code
    instructions = malloc(instrSIZE*sizeof(Instruction));
    

    //Input will hold the char at the cursor and build the reserved word, symbol, or variable being read
    //Since identifiers can have a maximum length of 11, the char array limit is 12 (11+null terminator)
    char *input = malloc(ID_SIZE_LIMIT*sizeof(char));
    input[0] = '\0';
    //lookAhead checks the next character to be read
    char lookAhead = ' ';

    //Count will hold the current index of the program string
    int count = 0;

    //This loop is the main driver of the lexical analyzer
    //It analyzes each char from the program and assigns it an appropriate token
    while (program[count] != '\0') {
        //If input is empty (because it has been reset or the loop has just started)
        //add the current char being read to it
        if (input[0] == '\0'){
            input[0] = program[count];
            input[1] = '\0';
            count++;
        }
        
        //Token holds the result of checking for reserved words
        int scannerToken = 0;
        
        if (input[0] == ' ' || input[0] == '\r' || input[0] == '\t' || input[0] == '\n'){
            //Ignoring whitespace characters
            memset(input, '\0',ID_SIZE_LIMIT);

        } else if (!isalpha(input[0]) && !isdigit(input[0]) && !isValidSymbol(input[0])) {
            //If the symbol being read is not allowed in the program language 
            //throw an error
            printf("Error: Symbol not allowed. %s is not in the programming language.\n", input);
            return 1;
        } else if (strlen(input) < 2) {
            //If input is only 1 character long, check if it is an operator
            scannerToken = checkReserved(input);
            
            if (scannerToken == 0 || scannerToken == 2) {
                //input is a single char and scannerToken is 0 or 2
                //the input might be a variable identifier or it is ':'. Using lookahead to see if 
                //that's the end of the identifier or if it's the beginning of a reserved word
                lookAhead = program[count];

                //First checking to see if '=' is next to form ":="
                if (lookAhead == '=' && input[0] == ':'){
                    //If '=' is next, attach the lookAhead variable, get the proper token, and add it
                    //to the token list
                    input[1] = lookAhead;
                    input[2] = '\0';
                    scannerToken = checkReserved(input);
                    addToken(input,tokenList,tokenCount,scannerToken);
                    tokenCount++; //increment tokenCount after adding the new token
                    count++; //increment count after using lookAhead

                } else if (!isalpha(lookAhead) && !isdigit(lookAhead)){
                    //lookAhead is not a letter or digit
                    //The next character is a space or a symbol, so the current input is an identifier
                    //Add the token to the list and then reset input
                    addToken(input,tokenList,tokenCount,2);
                    tokenCount++;
                    
                }
                //If lookAhead is a letter or digit
                //Let the loop continue

            } else if (scannerToken == 3){
                //input is a single character that is a number literal (0-9)
                //Need to check if the next char is also a number (0-9)
                lookAhead = program[count];
                if (!(lookAhead >= '0' && lookAhead <= '9')){
                    //If the next character is not a digit, add the digit to the token list
                    //otherwise let the loop continue so that larger integers (like 10 vs 1) will
                    //be tokenized as a single integer
                    addToken(input,tokenList,tokenCount,scannerToken);
                    tokenCount++;
                }

            } else if (scannerToken != 0){
                //input has only 1 char and it's one of the reserved symbols
                //Print it as long as it's not part of a <=, >=, :=, or <>
                if (input[0] != '<' && input[0] != '>' && input[0] != ':' && input[0] != '/'){
                    addToken(input,tokenList,tokenCount,scannerToken);
                    tokenCount++;
            
                } else {
                    //If input is currently <, >, :, or /, look ahead to check if next char is part of an operator symbol
                    lookAhead = program[count];
                    if (lookAhead == '=' || lookAhead == '>'){
                        //If next char makes an operator symbol with input, concatenate them, add them to the list, 
                        //reset input, and increment count
                        input[1] = lookAhead;
                        input[2] = '\0';
                        addToken(input,tokenList,tokenCount,scannerToken);
                        tokenCount++;
                        count++; //incrementing count because lookAhead is used so the next char is already used

                    } else if (lookAhead == ' ' || isalpha(lookAhead) || isdigit(lookAhead) || (isValidSymbol(lookAhead) && lookAhead != '*')) {
                        //if next char is not part of an operator symbol, add the operator to the list and reset input
                        addToken(input,tokenList,tokenCount,scannerToken);
                        tokenCount++;

                    } else if (lookAhead == '*') {
                        //If the current input is '/' and next is '*' then it is at the beginning of a comment block
                        while (program[count] != '\0'){
                            //While loop ends at the end of the program to prevent an infinite loop
                            //It breaks early if the end of the comment block is encountered
                            if (program[count] == '*' && program[count+1] == '/'){
                                count += 2;
                                break;
                            }
                            //Advancing the while loop through the comment block
                            count++;
                        }
                        //Clearing input after the comment block
                        memset(input,'\0',ID_SIZE_LIMIT);
                    }
                }
            } 
        } else {
            //if input is not a single char, check input against reserved words
            scannerToken = checkReserved(input);
            if (scannerToken == 0 || scannerToken == 2) { 
                //the input might be a variable identifier. Check to see if that's the end of the identifier
                //or if it's the beginning of a reserved word by using lookAhead
                lookAhead = program[count];
                if (!isalpha(lookAhead) && !isdigit(lookAhead)){
                    //lookAhead is not a letter or digit
                    //The next character is a space or a symbol, so the current input is a variable
                    //Check that it's the appropriate length, if it's too long add it as an error
                    //otherwise, add the identifier token to the list and then reset input
                    if (strlen(input) >= 12) {
                        //If the string is 12 characters or longer, it is over the 11 length constraint.
                        //Throw an error
                        printf("Error: identifier '%s' is too long. Maximum identifier length is 11.\n", input);
                        return 1;
                    } else {
                        //If the string is not over 12 characters, add it as an Identifier
                        addToken(input, tokenList, tokenCount, 2);
                        tokenCount++;
                    }
                    
                }
                //If lookAhead is a letter or digit

                //Let the loop continue
            } else if (scannerToken == 3){
                //input is holding a literal that is multiple digits long
                //Need to check if the next char is also a literal
                lookAhead = program[count];
                if (!(isdigit(lookAhead))){
                    //if the next character is not a number check if the current string is longer
                    //than the constraint allows. If it's not too long, add it to the token list
                    if (strlen(input) > 5) {
                        //If the number is greater than 5 digits long, it does not meet the constraints
                        //Print an error
                        printf("Error: number %s is too long. Maximum length of integers is 5 digits.\n", input);
                        return 1;
                    } else {
                        //Adding it to the token list
                        addToken(input,tokenList,tokenCount,scannerToken);
                        tokenCount++;
                        
                    }
            }
            } else if (scannerToken != 0){
                lookAhead = program[count];
                if (!isalpha(lookAhead) && !isdigit(lookAhead)){
                    //input is a Reserved Word or special symbol
                    //Add the token to the list and then clear input
                    addToken(input,tokenList,tokenCount,scannerToken);
                    tokenCount++;
                }
                
            }

        }
        
        //If an identifier or reserved word has not been detected yet and
        //the string is still under 12 characters (and not empty),
        //attach the next character to the string. 
        if (input[0] != '\0'){
            input[strlen(input)] = program[count];
            count++;
        }

        //if the token list grows too big, double the capacity of the array
        if (tokenCount == listSize){
            listSize *= 2;
            tokenList = realloc(tokenList,listSize*sizeof(ReservedWord));
        }

    } //End while loop
    
    //When an error occurs any function that encounters it returns until it reaches main,
    //where it will cause the program to stop running.
    int errorFlag = 0;

    //Printing the source code
    printSource(program,argv[1]);

    //Print the lexeme table (commented out for this assignment, but useful for debugging)
    //printLexemeTable(tokenList,tokenCount);

    //Begin parsing and generating code
    errorFlag = parsingProgram();
    if (errorFlag == -1){
        return 1;
    }

    //If no errors were produced, write the output to a file
    fileWrite();
    
    //Deallocating memory
    free(program);
    cleanUp(tokenList,tokenCount);
    free(instructions);

}//End main

//getToken reads the next token from the scanner's tokenList and then increments it's index
//It returns the token int but since token is a global variable, it does not need to be stored
//in local scope variables
int getToken(){
    token = tokenList[parser].token;
    parser++;
    return token;
}

//This function marks all symbols in the symbol table as out of scope or deleted
void markAllSymbols() {
    for (int i = 0; i < symCount; i++) {
        symbols[i].mark = 1;
    }
}

//This function marks all variables and constants in the given procedure as deleted or 
//out of scope.
void markProcSymbols(char *procName){
    for (int i = 0; i < symCount; i++){
        if (symbols[i].kind == 3 && strcmp(procName, symbols[i].name) == 0){
            i++;
            while (i < symCount && (symbols[i].kind == 1 || symbols[i].kind == 2)){
                symbols[i].mark = 1;
                i++;
            }
        }
    }
}


/**
 * @brief parsingProgram starts the Context Free Grammar parsing of the program (the name program was used
 * earlier for the scanner's input). The function calls the block function and ensures a period is
 * at the end of the program. Returns 0 if successfully run, -1 if there was an error.
 * 
 * @return int 
 */
int parsingProgram(){
    //Program ::= block "."
    //calling block from lexical level 0
    int errorFlag = block();
    if (errorFlag == -1){
        return -1;
    }
    if (token != 19){
        printf("Error: Program must end with period.\n");
        return -1;
    }

    //If no errors have occured, write halt to output and then return
    codeWriter(9,0,3);

    //Once periodsym has been reached, the symbol table should be marked as out of scope
    markAllSymbols();

    return 0;
}


/**
 * @brief Block handles constant declarations, variable declarations, and then begins to process statements
 * It also increments the stack pointer by the number of variables declared. Returns 0 if successfully run,
 * or returns -1 if there was an error.
 * 
 * 
 * @return int 
 */
int block(){
    //Saving the current index of instructions so procedure jumps can be editted later
    int procIndex = instIdx;
    //Writing the jump command. 0 is a place holder
    codeWriter(7, 0, 0); 

    int numVars = 0;
    int errorFlag = 0;
    //Get the first token
    if (token == 0){
        getToken();
    }
    //Start a series of if's, after each is done it the next if statement may still activate
    //since each function calls getToken() before returning
    if (token == 28) { // const
        errorFlag = constDec();
        if (errorFlag == -1){
            return -1;
        }
    }
    if (token == 29) { // var
        numVars = varDec();
        if (numVars == -1){
            return -1;
        }
    }
    if (token == 30) { // procedure
        do {
            errorFlag = procDec();
            if (errorFlag == -1) {
                return -1;
            }
        } while (token == 30);
    }
    
    instructions[procIndex].M = (instIdx*3)+10;

    //Since the constant and variable declarations are optional, the function may just call statement
    codeWriter(1, 0, 3+numVars); //Increment stack by number of variables (plus the 3 activation record information slots)
    errorFlag = statement();
    if (errorFlag == -1){
        return -1;
    }
    
    
    
    return 0;
} //end block()


/**
 * @brief constDec creates constant variables in the symbol table as long as the proper
 * syntax is followed. Returns 0 if successful, -1 if there was an error.
 * 
 * @return int 
 */
int constDec(){
    //Token was checked before constDec() was called
    //so current token is a constsym

    int numConst = 0;
    do {
        numConst++;
        getToken();
        //Next token after a const must be an identifier
        if (token != 2){
            printf("Error: An identifier must follow a const.\n");
            return -1;
        }
        //Check that it's not already in the symbol table
        int symIdx = symbolTableCheck(tokenList[parser-1].resWord);
        if (symIdx != -1){
            printf("Error: Identifier %s has already been declared.\n", tokenList[parser-1].resWord);
            return -1;
        }
        //Save the name to be added to the symbol table
        char *constName = tokenList[parser-1].resWord;

        //Get the value of the const
        getToken();
        if (token != 9) {
            printf("Error: A '=' must follow the identifier in a const declaration.\n");
            return -1;
        }
        getToken();
        if (token != 3) {
            printf("Error: A constant variable must be assigned an integer value.\n");
            return -1;
        }
        
        //Converting the string to an integer and storing it in numValue
        int numValue; 
        sscanf(tokenList[parser-1].resWord, "%d", &numValue);

        //Adding the const variable and its value to the symbol table
        //Kind is 1 for const, it has no address because constants are turned into
        //literals
        addSymbolTable(1, constName, numValue, 0, 0, 0);

        getToken();
    } while (token == 17); //while token is a comma ','

    if (token != 18){ // semicolonsym ;
        printf("Error: Const declarations must end with a ';'.\n");
        return -1;
    }
    getToken();
    return 0;
}

/**
 * @brief varDec creates variables in the symbol table from identifier tokens and
 * returns the number of variables created. It follows the grammar:
 * var-declaration ::= ["var" identifier {"," identifier}";"].
 * Returns number of variables created if successful, or -1 if there was an error.
 * 
 * @return int 
 */
int varDec(){
    int numVars = 0;
    int address = 3;
    if (token == 29) { // var
        do {
            numVars++;
            getToken();
            if (token != 2){ 
                //if next token is not an identifier, throw an error
                printf("Error: Identifier expected after var.\n");
                return -1;
            }
            int symIdx = symbolTableCheck(tokenList[parser-1].resWord);
            if (symIdx != -1 && symbols[symIdx].level == lexLevel){
                //if the identifier is already in the symbol table at the same lexical level, throw an error
                //if it's at a different level, it's okay to add
                printf("Error: Identifier %s has already been declared.\n", tokenList[parser-1].resWord);
                return -1;
            }
            addSymbolTable(2, tokenList[parser-1].resWord, 0, lexLevel, address, 0);
            address++;
            getToken();
        } while (token == 17); //loop while token is a comma ','

        if (token != 18){ //semicolonsym ;
            printf("Error: Variable declarations must end with a semicolon ';'.\n");
            return -1;
        }
        getToken();
    } 

    return numVars;
} //end varDec()


/**
 * @brief This function declares a procedure and its block of const declarations, var
 * declarations, and statements (and potentially sub-procedures). It returns 0 if successful,
 * and -1 if an error occurs.
 * 
 * @return int 
 */
int procDec() {
    while (token == 30) {
        getToken();
        if (token != 2) {
            printf("Error: Procedure declarations must be followed by an identifier.\n");
            return -1;
        }
        char *procName = tokenList[parser-1].resWord;
        getToken();
        if (token != 18) {
            printf("Error: Procedure declarations must end with a ';'.\n");
            return -1;
        }
        getToken();
        instrAddress = (instIdx*3)+10;
        addSymbolTable(3, procName, 0, lexLevel, instrAddress, 0);
        

        //Increase lexical level and then call the next block
        lexLevel++;
        int errorflag = block();
        if (errorflag == -1){
            return -1;
        }
        if (token != 18){
            printf("Error: procedures must end in a ';'. Procedure %s did not end in a semicolon.\n", procName);
            return -1;
        }
        getToken();

        //Write assembly return operation after writing with block
        codeWriter(2, 0, 0);

        //Mark identifiers as out of scope after returning from block
        markProcSymbols(procName);

        //Decrease lexical level on return from block()
        lexLevel--;
    }
    
    return 0;
}

/**
 * @brief Statement parses most of the program by determining the type of statement,
 * creating the correct code for it, and passing expressions to be evaluated by the
 * expression function. The function returns 0 if it successfully parses a statement,
 * and -1 if there is an error.
 * 
 * Grammar:
 * statement ::= [ident ":=" expression
 * | "begin" statement {";" statement } "end"
 * | "if" condition "then" statement "fi"
 * | "when" condition "do" statement
 * | "read" ident
 * | "write" expression
 * | empty ] .
 * 
 * @return int 
 */
int statement(){
    switch (token){
        case 2: { // Identifier
            int symIdx = symbolTableCheck(tokenList[parser-1].resWord);
            //Check if identifier is in the symbol table
            if (symIdx == -1){
                printf("Error: Undeclared identifier %s .\n", tokenList[parser-1].resWord);
                return -1;
            }
            //Check if identifier is a variable
            if (symbols[symIdx].kind != 2) {
                printf("Error: Identifier %s is not a variable. Assignment to constant or procedure is not allowed.\n", tokenList[parser-1].resWord);
                return -1;
            }

            //check that the next token is the becomesym
            getToken();
            if (token != 20){ // ':='
                printf("Error: Assignment operator expected. Assignment statements must use := .\n");
                return -1;
            }
            //:= is handled by store instruction at the end of this case

            //After the becomesym is an expression
            //Parse the expression first to generate it's code
            getToken();
            int errorFlag = expression();
            if (errorFlag == -1){
                return -1;
            }

            //Then generate the store instruction for the variable assignment
            codeWriter(4, lexLevel-symbols[symIdx].level, symbols[symIdx].addr);
            break;
            }
        case 21: { // Begin
            do {
                getToken();
                int errorFlag = statement();
                //If an error occurs during a recursive call, errorFlag will move it up to main to stop running
                if (errorFlag == -1){
                    return -1;
                }

            } while (token == 18); //loop while statements end with a ';'

            //End must come after all statements
            if (token != 22){
                printf("Error: Begin must be followed by end.\n");
                return -1;
            }
            getToken();
            break;
        }
        case 23: { // If
            //Get the next token and call condition to parse the condition
            getToken();
            int errorFlag = condition();
            if (errorFlag == -1){
                return -1;
            }

            //instIdx is the instruction array's index
            //Saving it in jpcIdx to be used in case the condition is not true
            int jpcIdx = instIdx;
            codeWriter(8, 0, 0); //temporarily putting address 0 in the instructions
            
            //Check that the next token after the condition is then
            if (token != 24){
                printf("Error: If must be followed by then.\n");
                return -1;
            }
            getToken();
            //Statements to be executed if true
            errorFlag = statement();
            if (errorFlag == -1){
                return -1;
            }

            //Need to jump over the statements that would have been executed if the condition were false
            int trueIdx = instIdx;
            //0 is place holder here
            codeWriter(7, 0, 0);

            //After parsing the true statements, update the JPC's address
            instructions[jpcIdx].M = (instIdx*3)+10;

            //Checking that the next token is else
            if (token != 33){
                printf("Error: Then must be followed by else.\n");
                return -1;
            }
            getToken();
            errorFlag = statement();
            if (errorFlag == -1){
                return -1;
            }
        
            //Check that the next token is fi
            if (token != 8){
                printf("Error: Else must be followed by fi.\n");
                return -1;
            }
            
            //update the true's jump instruction
            instructions[trueIdx].M = (instIdx*3)+10;
            
            getToken();
            break;
        }
        case 25: { // When
            getToken();
            //Saving the start of the loop's index to jump back to
            int loopIdx = instIdx;
            int errorFlag = condition();
            if (errorFlag == -1){
                return -1;
            }

            //checking the next token after the condition is a dosym
            if (token != 26) {
                printf("Error: When must be followed by do.\n");
                return -1;
            }
            getToken();
            //Save the current index to adjust the JPC instruction after statements are parsed
            int jpcIdx = instIdx;
            codeWriter(8,0,0);

            //Parse statement
            do {
                errorFlag = statement();
                if (errorFlag == -1){
                    return -1;
                }
            } while (token == 18); //loop while statements end in a semicolon
            //After the statements write the loop jump instruction
            codeWriter(7, 0, (loopIdx*3)+10);
            //Adjust the JPC instruction to the correct post-statement address
            instructions[jpcIdx].M = (instIdx*3)+10;

            break;
        }
        case 27: { // Call
            getToken();
            //Checking if next token is an identifer
            if (token != 2){
                printf("Error: Call must be followed by an identifer.\n");
                return -1;
            }
            //Checking that the identifier is in the symbols table
            int symIdx = symbolTableCheck(tokenList[parser-1].resWord);
            if (symIdx == -1){
                printf("Error: undeclared identifier %s \n", tokenList[parser-1].resWord);
                return -1;
            }
            //Checking that the identifier is a procedure name
            if (symbols[symIdx].kind != 3){
                printf("Error: Call identifier must be a procedure. '%s' is not a procedure.\n", tokenList[parser-1].resWord);
                return -1;
            }
            //If there were no errors, write the call
            codeWriter(5, lexLevel-symbols[symIdx].level, symbols[symIdx].addr);
            getToken();
            break;
        }
        case 31: { // Write
            getToken();
            int errorFlag = expression();
            if (errorFlag == -1){
                return -1;
            }
            //instruction to write to screen
            codeWriter(9,0,1);
            break;
        }
        case 32: { //Read
            getToken();
            //Check if the next token is an identifier 
            if (token != 2){ 
                printf("Error: Read must be followed by an identifier.\n");
                return -1;
            }
            //Checking that the identifier is in the symbol table
            int symIdx = symbolTableCheck(tokenList[parser-1].resWord);
            if (symIdx == -1){
                printf("Error: Undeclared identifier %s .\n", tokenList[parser-1].resWord);
                return -1;
            }
            //Checking that the identifier is a variable
            if (symbols[symIdx].kind != 2){
                printf("Error: Only variable values may be altered. %s is not a variable.\n", tokenList[parser-1].resWord);
                return -1;
            }
            getToken();
            //Instruction for read
            codeWriter(9,0,2);
            //Instruction for store
            codeWriter(4, lexLevel-symbols[symIdx].level, symbols[symIdx].addr);
            break;
        }
        
    } //End switch

    return 0;  

} //End statement()


/**
 * @brief Condition parses conditional expressions. These expressions either begin with oddsym or
 * contain a relational operator =, <>, <, <=, >, >=. Returns 0 if successfully parsed, or -1 if
 * an error occurs.  
 * 
 * @return int 
 */
int condition(){

    //If the conditional expression does not begin with oddsym
    //Parse the first part of the expression
    int errorFlag = expression();
    if (errorFlag == -1){
        return -1;
    }

    //Check the relational operator and parse the second part of the expression
    //then write the correct instruction for the relational operator
    if (token == 9) { // eqlsym =
        getToken();
        errorFlag = expression();
        if (errorFlag == -1){
            return -1;
        }
        //Finally, checking equality by writing the instruction
        codeWriter(2,0,5);

    } else if (token == 10){ //nqlsym <>
        getToken();
        errorFlag = expression();
        if (errorFlag == -1){
            return -1;
        }
        //Finally, checking nonequality by writing the instruction
        codeWriter(2,0,6);

    } else if (token == 11){ // lesssym <
        getToken();
        errorFlag = expression();
        if (errorFlag == -1){
            return -1;
        }
        //Finally, checking relation by writing the instruction
        codeWriter(2,0,7);

    } else if (token == 12){ //leqsym <=
        getToken();
        errorFlag = expression();
        if (errorFlag == -1){
            return -1;
        }
        //Finally, checking relation by writing the instruction
        codeWriter(2,0,8);

    } else if (token == 13){ //gtrsym >
        getToken();
        errorFlag = expression();
        if (errorFlag == -1){
            return -1;
        }
        //Finally, checking relation by writing the instruction
        codeWriter(2,0,9);

    } else if (token == 14){ //geqsym >=
        getToken();
        errorFlag = expression();
        if (errorFlag == -1){
            return -1;
        }
        //Finally, checking relation by writing the instruction
        codeWriter(2,0,10);

    } else {
        //If none were found, throw an error
        printf("Error: Condition must contain a relational operator.\n");
        return -1;
    }

    return 0;
} //end condition()


/**
 * @brief Expression parses the arithmetic expressions and writes the appropriate
 * assembly language instructions. Expressions must take the form of:
 * expression ::= term {("+"|"-") term}.
 * This function returns 0 if successfully parsed, or -1 if an error occurs.
 * 
 * @return int 
 */
int expression(){
    //Parse first term
    int errorFlag = term();
    if (errorFlag == -1){
        return -1;
    }

    
    //Evaluating the arithmetic by first getting the second term and then writing the code for the operation
    while (token == 4 || token == 5){ // while the token is either a + or -
        if (token == 4){ 
            //if 2 terms need to be added, get second term written first
            getToken();
            errorFlag = term();
            if (errorFlag == -1){
                return -1;
            }
            //then write instruction for add
            codeWriter(2,0,1);
        } else { // -
            //if 2 terms need to be subtracted
            getToken();
            errorFlag = term();
            if (errorFlag == -1){
                return -1;
            }
            //write instruction for subtract
            codeWriter(2,0,2);
        }
    }

    return 0;

} //end expression()


/**
 * @brief term generates code for a part of an arithmetic expression, such as x in x + y.
 * It follows the form: Term ::= factor {("*"|"/") factor}. This function returns
 * 0 if successful, or -1 if an error occurs.
 * 
 * @return int 
 */
int term(){
    int errorFlag = factor();
    if (errorFlag == -1){
        return -1;
    }
    
    //Evaluating the arithmetic by first getting the second factor, then writing the code for the operation
    while (token == 6 || token == 7 || token == 1){ //while the token is either a *, /, or mod
        if (token == 6){
            //To multiply 2 factors, write the second factor (first already done) 
            getToken();
            errorFlag = factor();
            if (errorFlag == -1){
                return -1;
            }
            //them write instruction for multiply
            codeWriter(2,0,3);

        } else if (token == 7) {
            //To divide 2 factors, write the second factor
            getToken();
            errorFlag = factor();
            if (errorFlag == -1){
                return -1;
            }
            //them write instruction for divide
            codeWriter(2,0,4);

        } else {
            //To do modulus (odd) 2 factors, write the second factor
            getToken();
            errorFlag = factor();
            if (errorFlag == -1){
                return -1;
            }
            //them write instruction for modulus (odd)
            codeWriter(2,0,11);

        }
    }

    return 0;

} //end term()


/**
 * @brief Factor evaluates a single element of an arithmetic expression, without the 
 * arithmetic operators. It should be an identifier, a number, or an expression within parentheses
 * Factor ::= identifier | number | "(" expression ")"
 * This function returns 0 if code is successfully generated, or -1 if an error occurs. 
 * 
 * @return int 
 */
int factor(){
    //Factor will be an identifier, a number, or an expression enclosed in parentheses
    if (token == 2){
        //Check that identifier is in the symbol table
        int symIdx = symbolTableCheck(tokenList[parser-1].resWord);
        if (symIdx == -1){
            printf("Error: Undeclared identifier %s .\n", tokenList[parser-1].resWord);
            return -1;
        }
        //checking if identifier is a const or variable
        if (symbols[symIdx].kind == 1){
            //if it's a constant, put the literal on the stack
            codeWriter(6, 0, symbols[symIdx].val);
        } else if (symbols[symIdx].kind == 2) {
            //if it's a variable, load it
            codeWriter(3, lexLevel-symbols[symIdx].level, symbols[symIdx].addr);
        } else {
            //if it's a procedure
            printf("Error: expression must not contain a procedure identifier. %s is a procedure.\n", symbols[symIdx].name);
            return -1;
        }
        getToken();

    } else if (token == 3){
        //if it's a number, put the literal on the stack
        int tempNum;
        sscanf(tokenList[parser-1].resWord, "%d", &tempNum);
        codeWriter(6, 0, tempNum);
        getToken();

    } else if (token == 15){
        //left parenthesis
        getToken();
        int errorFlag = expression();
        if (errorFlag == -1){
            return -1;
        }
        //After the expression, a ')' must follow
        if (token != 16){
            printf("Error: Right parenthesis must follow left parenthesis.\n");
            return -1;
        }
        getToken();

    } else {
        //If none of the above were present, throw an error
        printf("Error: Expected a factor. Expressions must contain operands, parentheses, numbers, or symbols.\n");
        return -1;
    }

    return 0;
    
} //end factor()


/**
 * @brief This function stores the results of the code generation into an array of Instruction structs.
 * The structs format the data into the integer assembly language that project 1 used so it will be
 * readable by that program when printed out to a file.
 * 
 * @param token 
 * @param level 
 * @param M_address 
 */
void codeWriter(int instr, int level, int M_Address){

    instructions[instIdx].OP = instr;
    instructions[instIdx].L = level;
    instructions[instIdx].M = M_Address;
    instIdx++;

    //Expand the instructions array if necessary
    if (instIdx == instrSIZE){
        instructions = realloc(instructions,2*instrSIZE*sizeof(Instruction));
        instrSIZE *= 2;
    }
}


/**
 * @brief fileWrite outputs the result of code generation onto the console and into an output file. 
 * The code generation is stored in the instructions array, so this function iterates through the array.
 *  
 */
void fileWrite(){
    FILE *output = fopen("elf.txt", "w");
    if (output == NULL){
        printf("Error: failed to create output file.\n");
        return;
    }

    printf("No errors, program is syntactically correct.\n\n");
    
    //Headers
    printf("%5s\t%3s\t%3s\t%3s\n", "Line", "OP", "L", "M");

    //Printing generated assembly code instuctions
    int current = 0;
    char *str;
    while (current < instIdx){
        switch (instructions[current].OP){
            case 1:{
                str = "INC";
                break;
            }
            case 2: {
                str = "OPR";
                break;
            }
            case 3:{
                str = "LOD";
                break;
            }
            case 4:{
                str = "STO";
                break;
            }
            case 5:{
                str = "CAL";
                break;
            }
            case 6:{
                str = "LIT";
                break;
            }
            case 7:{
                str = "JMP";
                break;
            }
            case 8:{
                str = "JPC";
                break;
            }
            case 9:{
                str = "SYS";
                break;
            }
                
        }

        printf("%5d\t%3s\t%3d\t%3d\n", current, str, instructions[current].L, instructions[current].M);
        fprintf(output, "%d %d %d\n", instructions[current].OP, instructions[current].L, instructions[current].M);
        current++;
    }

    //Symbol table headers
    printf("\n");
    printf("Symbol Table:\n");
    printf("\n");

    printf("%4s | %11s | %5s | %5s | %7s | %4s\n", "Kind", "Name", "Value", "Level", "Address", "Mark");
    printf("---------------------------------------------------\n");
    for (int i = 0; i < symCount; i++){
        printf("%4d | %11s | %5d | %5d | %7d | %4d\n", symbols[i].kind, symbols[i].name, symbols[i].val, symbols[i].level, symbols[i].addr, symbols[i].mark);
    }

}


/**
 * @brief This function searches the symbol table for the passed string (the identifier's name).
 * If the identifier is present in the table, the index is returned. If it is not present, -1 is returned. 
 * 
 * @param input 
 * @return int 
 */
int symbolTableCheck(char *input){
    for (int i = symCount; i >= 0; i--){
        if (strcmp(input, symbols[i].name) == 0){
            if (symbols[i].mark == 0 && symbols[i].level <= lexLevel){
                return i; //return the index variable was found at
            } else if (symbols[i].mark == 0 && symbols[i].level > lexLevel) {
                //if the identifier was found at a higher lexical level continue moving
                //down the symbol table to see if it's found at the current lexical level
                //or less than the current level
                continue;
            } else if (symbols[i].mark == 1 && symbols[i].level > 0) {
                //Continue checking if the symbol appears at lower levels
                continue;
            } else if (symbols[i].mark == 1 && symbols[i].level == 0) {
                //This executes if the mark is 1 and there are no levels beneath the
                printf("Error: variable %s is out of scope.\n", input);
                exit(1);
            }
        }
    }
    //Return -1 if it was not found
    return -1;
}

/**
 * @brief checkReserved checks to see if the current input being read has formed a reserved word,
 * symbol, or identifier
 * 
 * @param input string that has already been read
 * @param length length of current string
 * @return int integer token for reserved word, 0 for not a reserved word
 */
int checkReserved(char *input){
    //Cycle through the reserved words and compare each to the current input
    //2 is for identifiers. It will produce a token and the parcer then inserts to the symbol 
    //table if the proper grammar is followed
    for (int i=1; i<=33; i++){
        if (i == 2){
            if (symbolTableCheck(input) != -1){
                return 2;
            }
        } else if (i == 3){
            if ((input[0] >= '0') && (input[0] <= '9')){
                return 3; //All integer literals have token 3
            }
        } else if (strcmp(input,resWords[i].resWord) == 0){
            //Return token number
            return resWords[i].token;
        }
    }

    return 0;
} //End checkReserved()

/**
 * @brief This function checks that a symbol is permited in the PL/0 language
 * The only allowed symbols are + - * / ( ) = , . ; :
 * 
 * @param c char being checked
 * @param resWords the array of reservedWords and their tokens
 * @return int 1 for valid symbol, 0 for invalid symbol
 */
int isValidSymbol(char c) {
    //Loop through all reserved symbols
    //resWords 4-20 are symbols except 8, which is fi
    //20 is ':=' but the loop only compares the ':' because '=' is already compared earlier
    for (int i = 4; i <= 20; i++) {
        if (resWords[i].resWord[0] == c) {
            return 1;  // Valid symbol
        }
    }
    return 0;  // Invalid symbol
}

/**
 * @brief This function adds a token to the token list for later printing
 * 
 * @param input the character or string being added
 * @param tokenList the array of token objects being added too
 * @param tokenCount the index of the array of token objects
 * @param token the token number of the string being added
 */
void addToken(char *input, ReservedWord *tokenList, int tokenCount, int token){
    //allocate memory for the string
    tokenList[tokenCount].resWord = malloc((strlen(input)+1)*sizeof(char));
    //copy it into the new space
    strcpy(tokenList[tokenCount].resWord, input);
    //set the token number
    tokenList[tokenCount].token = token;
    //clear input
    memset(input, '\0',ID_SIZE_LIMIT);
}

/**
 * @brief This function adds an identifier to the symbol table. It then increments the 
 * symbol table's index.
 * 
 * @param input the char or string to be added
 * @param varIdentifiers the list of identifiers
 * @param varCount the index of the identifiers list
 * @param tokenList the list of tokens
 * @param tokenCount the index of the tokens list
 */
void addSymbolTable(int kind, char *name, int value, int level, int address, int mark){
    //Filling out symbol's information
    symbols[symCount].kind = kind;
    strcpy(symbols[symCount].name, name);
    symbols[symCount].val = value;
    symbols[symCount].level = level;
    symbols[symCount].addr = address;
    symbols[symCount].mark = mark;
    symCount++;
}

/**
 * @brief This function prints out the source PL/0 code being compiled.
 * 
 * @param program the code that was read from the input file
 * @param name argv[1] the name of the file that was provided when this program was opened
 */
void printSource(char *program, char *name){
    //Printing out the source program
    printf("Source Program: %s\n", name);
    printf("%s\n", program);
    printf("\n");
}

/**
 * @brief This function prints out the results of the lexical analyzer. It prints to both
 * the console and to an output file named tokens.txt . It first prints out the source
 * code that was analyzed (along with the file name). Then it prints out a table of all
 * lexemes encountered and corresponding token number. 
 * 
 * If no errors were encountered, the function will print out a token list.
 * 
 * @param tokenList array of objects that holds all lexemes and tokens encountered
 * @param tokenCount index of the token list
 * @param program source code analyzed
 * @param name name of the file that held the source code
 */
void printLexemeTable(ReservedWord *tokenList, int tokenCount){

    //Headers
    printf("\nLexeme Table:\n\n");
    printf("Lexeme token\ttype\n");

    //errorFlag is 1 if an error is detected while printing the Lexeme table
    // 0 if no error detected.
    //If an error is detected, the Lexeme Table will still be printed but the
    //Token List will NOT be printed.
    int errorFlag = 0;
    
    //Print the token list
    for (int i=0; i<tokenCount;i++){
        printf("%s\t\t%d\n", tokenList[i].resWord,tokenList[i].token);
    }

    printf("\n");
    
    //Printing the Token List
    //Only prints if there are no errors in the source code
    if (errorFlag == 0){
        printf("Token List:\n");

        //Prints each token separated by a space
        for (int i=0; i<tokenCount;i++){
            printf("%d ",tokenList[i].token);

            //If the token is a 2, print the identifier after the 2
            if (tokenList[i].token == 2){
                printf("%s ", tokenList[i].resWord);
            }
            //If the token is a 3, print the integer after the 3
            if (tokenList[i].token == 3){
                printf("%s ", tokenList[i].resWord);
            }
        }
    }
    printf("\n");
}

/**
 * @brief This function reads the input file and stores it into the string program. It
 * allocates memory for the program as needed.
 * 
 * @param program 
 * @param inputFILE 
 * @param limit 
 * @return char* 
 */
char* readFile(char *program, FILE *inputFILE, int limit){
    int count = 0;
    char c;
    while ((c = fgetc(inputFILE)) != EOF){
        program[count] = c;
        count++;
        if (count == limit){
            limit *= 2;
            program = realloc(program,limit*sizeof(char));
        }
    }

    //Adding a space at the end of the program to force it to recognize any symbols or identifiers that
    //butt up against the end of the file.
    program[count] = ' ';
    count++;
    program[count] = '\0';
    return program;
}

//Deallocating memory
void cleanUp(ReservedWord *tokenList, int count){
    for (int i = 0; i < count; i++){
        free(tokenList[i].resWord);
    }
    free(tokenList);
}

//Initilizes the array of resWords with all allowed symbols and reserved words that PL/0 uses
void loadReservedWords(ReservedWord *resWords){
    resWords[1].resWord = "mod";
    resWords[1].token = 1;
    //2 is variable identifier
    //3 is number literal
    resWords[4].resWord = "+";
    resWords[4].token = 4;
    resWords[5].resWord = "-";
    resWords[5].token = 5;
    resWords[6].resWord = "*";
    resWords[6].token = 6;
    resWords[7].resWord = "/";
    resWords[7].token = 7;
    resWords[8].resWord = "fi";
    resWords[8].token = 8;
    resWords[9].resWord = "=";
    resWords[9].token = 9;
    resWords[10].resWord = "<>";
    resWords[10].token = 10;
    resWords[11].resWord = "<";
    resWords[11].token = 11;
    resWords[12].resWord = "<=";
    resWords[12].token = 12;
    resWords[13].resWord = ">";
    resWords[13].token = 13;
    resWords[14].resWord = ">=";
    resWords[14].token = 14;
    resWords[15].resWord = "(";
    resWords[15].token = 15;
    resWords[16].resWord = ")";
    resWords[16].token = 16;
    resWords[17].resWord = ",";
    resWords[17].token = 17;
    resWords[18].resWord = ";";
    resWords[18].token = 18;
    resWords[19].resWord = ".";
    resWords[19].token = 19;
    resWords[20].resWord = ":=";
    resWords[20].token = 20;
    resWords[21].resWord = "begin";
    resWords[21].token = 21;
    resWords[22].resWord = "end";
    resWords[22].token = 22;
    resWords[23].resWord = "if";
    resWords[23].token = 23;
    resWords[24].resWord = "then";
    resWords[24].token = 24;
    resWords[25].resWord = "when"; //Specifications changed "while" to "when"
    resWords[25].token = 25;
    resWords[26].resWord = "do";
    resWords[26].token = 26;
    resWords[27].resWord = "call";
    resWords[27].token = 27;
    resWords[28].resWord = "const";
    resWords[28].token = 28;
    resWords[29].resWord = "var";
    resWords[29].token = 29;
    resWords[30].resWord = "procedure";
    resWords[30].token = 30;
    resWords[31].resWord = "write";
    resWords[31].token = 31;
    resWords[32].resWord = "read";
    resWords[32].token = 32;
    resWords[33].resWord = "else";
    resWords[33].token = 33;
}
