#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_LINE_LENGTH		255
#define MAX_LABEL_LENGTH	20
#define INT_MAX				65535

// Struct for symbol table
struct symbol
{
	char label[MAX_LABEL_LENGTH + 1];
	int address;
};

// Enumeration for line parser
// Obtained from http://users.ece.utexas.edu/~patt/15s.460N/labs/lab1/Lab1Functions.html
enum
{
	DONE, OK, EMPTY_LINE
};

// Set of valid LC3 OPCODES
static const char* const opcodes[] = { "ADD", "AND", "BR", "BRN", "BRZ", "BRP", "BRNZ", "BRZP", "BRNP", "BRNZP", "HALT", "JMP", "JSR", "JSRR", "LDB", "LDW",
"LEA", "NOP", "NOT", "RET", "LSHF", "RSHFL", "RSHFA", "RTI", "STB", "STW", "TRAP", "XOR" };
// Set of valid LC3 pseudo OPCODES

static const char* const pseudos[] = { ".ORIG", ".END", ".FILL" };
// Set of reserved strings, (Invalid label names)
static const char* const reservedNames[] = { "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "IN", "OUT", "GETC", "PUTS"};



// Prototypes
int toNum(char *pStr);
int readAndParse(FILE *pInfile, char *pLine, char **pLabel, char
	**pOpcode, char **pArg1, char **pArg2, char **pArg3, char **pArg4);
int isOpcode(char *pStr);
int validateLabel(char *pStr);

void main(int argc, char *argv[])
{

	// Check command line argument count
	if (argc != 3)
	{
		printf("incorrect # of arguments, exit code 4");
		exit(4);
	}

	// Attempt to open file
	FILE *source = fopen(argv[1], "r");
	FILE *output = fopen(argv[2], "w");
	if (!source) {
		printf("unable to open file, exit code 4");
		exit(4);
	}

	if (!output)
	{
		printf("unable to open output file, exit code 4");
		exit(4);
	}

	// Create variables for line parser
	// obtained from http://users.ece.utexas.edu/~patt/15s.460N/labs/lab1/Lab1Functions.html
	char lLine[MAX_LINE_LENGTH + 1], *lLabel, *lOpcode, *lArg1,
		*lArg2, *lArg3, *lArg4;
	int lRet = OK;

	// Store machine code instruction to be written to output
	int lInstr;
	// Store initial memory address
	int lOrig = 0;
	// location counter, offset from lOrig (lOrig+lLocCount)
	int lLocCount = 0;
	// Maximum possible number of labels (assuming no invalid names)
	int lLabelCount = 0;

	// First loop, determines starting memory location, and maximum possible number of labels
	while (lRet != DONE)
	{
		lLabel = NULL;
		lOpcode = NULL;
		lRet = readAndParse(source, lLine, &lLabel, &lOpcode, &lArg1, &lArg2, &lArg3, &lArg4);
		if (lRet != DONE && lRet != EMPTY_LINE)
		{
			// Is this the first line of code, if so it should be .ORIG with a valid address
			if (lLocCount == 0)
			{
				// Check that first line of code is an .ORIG pseudoOP
				if (strcmp(lOpcode, pseudos[0]) == 0)
				{
					lOrig = toNum(lArg1);

					// Check if memory address is too large
					if (lOrig > INT_MAX)  
					{
						printf("invalid starting memory address constant, Exit Code 3");
						exit(3);
					}

					// Check if memory address is misaligned
					if (lOrig & 0x01)
					{
						printf("starting address not word aligned, Exit Code 3");
						exit(3);
					}
				}
				else
				{
					printf("code does not begin with .ORIG, Exit Code 3");
					exit(4);
				}
			}

			if (lLabel != NULL) lLabelCount++;

			// increment Location counter
			lLocCount++;

		}
	}

	// Create Symbol table, from lLabelCount information
	struct symbol table[lLabelCount];

	// Reset label count, location count, return value, and file pointer for second loop
	lRet = OK;
	lLabelCount = 0;
	lLocCount = 0;
	rewind(source);

	// Second loop, creates symbol table
	while (lRet != DONE)
	{
		lLabel = NULL;
		lOpcode = NULL;
		lArg1 = NULL;
		lArg2 = NULL;
		lArg3 = NULL;
		lArg4 = NULL;
		lRet = readAndParse(source, lLine, &lLabel, &lOpcode, &lArg1, &lArg2, &lArg3, &lArg4);
		if (lRet != DONE && lRet != EMPTY_LINE)
		{
			switch (validateLabel(lLabel))
			{
				case -1:
					break;

				case 0:
					printf("invalid label name");
					exit(4);
					break;

				case 1:
					printf("valid symbol %s found at address %s", lLabel, lArg1);
					
					// Store label and symbol in the table, this will not segfault as lLabel's size has already been validated 
					strcpy(table[lLabelCount].label, lLabel);
					table[lLabelCount].address = lOrig + lLocCount;
					lLabelCount++;
					break;
			}


			// Lastly, incrememnt location counter
			lLocCount++;
		}
	}


	fclose(source);
	fclose(output);
}
	
/*
 * @brief: convert a string sequence to an integer number
 * function obtained from http://users.ece.utexas.edu/~patt/15s.460N/labs/lab1/Lab1Functions.html
 */
int toNum(char * pStr)
{
	char * t_ptr;
	char * orig_pStr;
	int t_length, k;
	int lNum, lNeg = 0;
	long int lNumLong;

	orig_pStr = pStr;
	if (*pStr == '#')				/* decimal */
	{
		pStr++;
		if (*pStr == '-')				/* dec is negative */
		{
			lNeg = 1;
			pStr++;
		}
		t_ptr = pStr;
		t_length = strlen(t_ptr);
		for (k = 0; k < t_length; k++)
		{
			if (!isdigit(*t_ptr))
			{
				printf("Error: invalid decimal operand, %s\n", orig_pStr);
				exit(4);
			}
			t_ptr++;
		}
		lNum = atoi(pStr);
		if (lNeg)
			lNum = -lNum;

		return lNum;
	}
	else if (*pStr == 'x')	/* hex     */
	{
		pStr++;
		if (*pStr == '-')				/* hex is negative */
		{
			lNeg = 1;
			pStr++;
		}
		t_ptr = pStr;
		t_length = strlen(t_ptr);
		for (k = 0; k < t_length; k++)
		{
			if (!isxdigit(*t_ptr))
			{
				printf("Error: invalid hex operand, %s\n", orig_pStr);
				exit(4);
			}
			t_ptr++;
		}
		lNumLong = strtol(pStr, NULL, 16);    /* convert hex string into integer */
		lNum = lNumLong;
		if (lNeg)
			lNum = -lNum;
		return lNum;
	}
	else
	{
		printf("Error: invalid operand, %s\n", orig_pStr);
		exit(4);  /* This has been changed from error code 3 to error code 4, see clarification 12 */
	}
}

/*
* @brief: parses one line of assembly code
* function obtained from http://users.ece.utexas.edu/~patt/15s.460N/labs/lab1/Lab1Functions.html
*/
int readAndParse(FILE * pInfile, char * pLine, char ** pLabel, char
	** pOpcode, char ** pArg1, char ** pArg2, char ** pArg3, char ** pArg4)
{
	char * lRet, *lPtr;
	int i;
	if (!fgets(pLine, MAX_LINE_LENGTH, pInfile))
		return(DONE);
	for (i = 0; i < strlen(pLine); i++)
		pLine[i] = tolower(pLine[i]);

	/* convert entire line to lowercase */
	*pLabel = *pOpcode = *pArg1 = *pArg2 = *pArg3 = *pArg4 = pLine + strlen(pLine);

	/* ignore the comments */
	lPtr = pLine;

	while (*lPtr != ';' && *lPtr != '\0' &&
		*lPtr != '\n')
		lPtr++;

	*lPtr = '\0';
	if (!(lPtr = strtok(pLine, "\t\n ,")))
		return(EMPTY_LINE);

	if (isOpcode(lPtr) == -1 && lPtr[0] != '.') /* found a label */
	{
		*pLabel = lPtr;
		if (!(lPtr = strtok(NULL, "\t\n ,"))) return(OK);
	}

	*pOpcode = lPtr;

	if (!(lPtr = strtok(NULL, "\t\n ,"))) return(OK);

	*pArg1 = lPtr;

	if (!(lPtr = strtok(NULL, "\t\n ,"))) return(OK);

	*pArg2 = lPtr;
	if (!(lPtr = strtok(NULL, "\t\n ,"))) return(OK);

	*pArg3 = lPtr;

	if (!(lPtr = strtok(NULL, "\t\n ,"))) return(OK);

	*pArg4 = lPtr;

	return(OK);
}
/*
 * @brief checks if string is a valid LC-3 Opcode
 * returns 1 if valid, 2 if a pseudoOP, -1 if invalid, and 0 if NULL
 */
int isOpcode(char* pStr)
{
	// Check if pStr is NULL
	if (pStr == NULL) return 0;

	// Check if PseudoOP
	for (int i = 0; i < sizeof(pseudos); i++)
	{
		if (strcmp(pStr, pseudos[i]) == 0) return 2;
	}

	// Check if OPCODE
	for (int i = 0; i < sizeof(opcodes); i++)
	{
		if (strcmp(pStr, opcodes[i]) == 0) return 1;
	}

	// All checks failed, is not a valid opcode
	return -1;
}

/*
 * @brief checks if label is valid, returns 0 if invalid and 1 if valid. Returns -1 if pStr is NULL
 *
 */
int validateLabel(char* pStr)
{
	// Check if pointer is NULL
	if (pStr == NULL) return -1;

	// Check that first symbol is a letter
	if (!isalpha(pStr[0])) return 0;

	// Check that all symbols are alphanumeric
	for (int i = 0; i < sizeof(pStr); i++)
	{
		if (!isalnum(pStr[i])) return 0;
	}

	// Check that first symbol is not x
	if (pStr[0] == 'x') return 0;

	// Check that the label is not too long
	if (sizeof(pStr) > MAX_LABEL_LENGTH) return 0;

	// Check that label is not a reserved name
	for (int i = 0; i < sizeof(reservedNames); i++)
	{
		if (strcmp(pStr, reservedNames[i]) == 0) return 0;
	} 

	// All checks passed, is a valid label
	return 1;
}

