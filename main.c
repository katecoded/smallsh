/*
* Author: Katelyn Lindsey
* Date: 2/4/2021
* Assignment: Homework 3
* Description: smallsh - A program for a shell written in C. Includes built-in 
* commands for exit, status, and cd, as well as variable expansion for $$.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>


/*
* struct commandLine 
* 
* A struct for a command line with the command, the argument(s), the optional
* input and output files, and a bool for whether the task should be run
* in the background or not.
*/
struct commandLine {
	char* command;
	char* argumentArray[515]; //array of char pointers (includes command at front)
	char* inputFile; //can be NULL if there isn't one
	char* outputFile; //can be NULL if there isn't one
	bool inBackground; //true if & is at the end, false otherwise
};

/*
* char* varExpansion
* 
* Takes a string that contains $ and the smallshPid, and expands each
* instance of $$ to become the smallshPid, returning a pointer to the
* resulting string.
*/
char* varExpansion(char* token, int smallshPid) {

	//to keep track of the number of $ found in a row
	int numInRow = 0;

	//temporary storage for the new token being built
	char newToken[2048] = "\0";

	//the string version of the smallshPid for concatenation
	char pidString[11] = "\0";
	sprintf(pidString, "%d", smallshPid);

	//the length of the token
	int length = strlen(token);

	//loop counter
	int i;

	//loop through every char in the token, looking for $ - if there are two
	//in a row, it is replaced with the smallshPid
	for (i = 0; i < length; i++) {

		//if $ is found, increment numInRow
		if (strncmp(token + i, "$", 1) == 0) {
			numInRow++;
		}

		//else if it is not found
		else {

			//if there was one $ found, then add it to the newToken
			if (numInRow == 1) {
				strncat(newToken, "$", 1);
			}
			//otherwise, add the current character to the newToken
			else {
				strncat(newToken, token + i, 1);
			}
			//reset numInRow to 0
			numInRow = 0;
		}

		//if there were two $ in a row, add the pid to the newToken
		if (numInRow == 2) {
			strcat(newToken, pidString);
			//reset numInRow
			numInRow = 0;
		}
	}

	//if there is 1 in numInRow after the loop, add one more $ to the end
	if (numInRow == 1) {
		strcat(newToken, "$");
	}

	//printf("Before: %s, After: %s\n", token, newToken);

	//finally, copy over the new token 
	strcpy(token, newToken);

	return token;
}

/*
* struct commandLine* createCommandLine
* 
* Takes a string and creates a struct commandLine, returing a pointer to 
* the commandLine made. Also takes the smallshPid in case variable expansion
* is needed.
*/
struct commandLine* createCommandLine(char* givenLine, int smallshPid) {

	//reserve space on the heap for the new commandLine
	struct commandLine* currCommand = malloc(sizeof(struct commandLine));

	//used to keep track of the current position in the given line
	char* token_ptr;

	//store the length of the string
	int length = strlen(givenLine);

	//keep track of if there is input or output redirection
	bool redInput = false;
	bool redOutput = false;


	//first, get the command (always the first thing)
	char* token = __strtok_r(givenLine, " \n", &token_ptr);

	//if the token contains $$, send it off for expansion
	if (strstr(token, "$$") != NULL) {
		printf("Found $$ in %s\n", token);
		token = varExpansion(token, smallshPid);
	}

	//then, add to the currCommand struct
	currCommand->command = calloc(strlen(token) + 1, sizeof(char));
	strcpy(currCommand->command, token);

	//also store the command in the arguments pointer array
	currCommand->argumentArray[0] = currCommand->command;

	//set inBackground to false
	currCommand->inBackground = false;


	//next, add all of the arguments to the argument pointer array until either
	//the end of the string is reached or < or > are reached, keeping track of
	//the index of the argument array
	token = __strtok_r(NULL, " \n", &token_ptr);
	//printf("%s\n", token);
	int arrayIndex = 1;

	while (token != NULL) {

		//always first check for $$ - if the token contains it, send it
		//off for expansion
		if (strstr(token, "$$") != NULL) {
			printf("Found $$ in %s\n", token);
			token = varExpansion(token, smallshPid);
		}

		//if the token is a < or >, keep track of that with boolean values
		if (strcmp(token, "<") == 0 || strcmp(token, ">") == 0) {

			//there is an input file
			if (strcmp(token, "<") == 0) {
				redInput = true;
			}
			
			//there is an output file
			if (strcmp(token, ">") == 0) {
				redOutput = true;
			}
		}

		//if < or > was found, break out of the loop since there are no more
		//arguments
		if (redInput || redOutput) {
			break;
		}

		//otherwise, add the current token to the array of arguments
		printf("%s\n", token);
		currCommand->argumentArray[arrayIndex] = token;
		arrayIndex++;

		token = __strtok_r(NULL, " \n", &token_ptr);
	}

	//if the last argument includes \n, remove it
	/*if (strstr(currCommand->argumentArray[arrayIndex - 1], "\n") != NULL) {
		printf("Contains a newline!\n");

	}*/

	//if the last argument is an &, set inBackground to true and remove it
	//from the list of arguments
	if (strstr(currCommand->argumentArray[arrayIndex - 1], "&") != NULL) {
		currCommand->inBackground = true;
		currCommand->argumentArray[arrayIndex - 1] = NULL;
	}

	//if there is input redirection and it was caught first, get the file name 
	//and then check for output redirection - if there is output redirection,
	//also get that filename
	if (redInput) {
		//get the input file name
		token = __strtok_r(NULL, " \n", &token_ptr);
		currCommand->inputFile = calloc(strlen(token) + 1, sizeof(char));
		strcpy(currCommand->inputFile, token);

		//check for output redirection
		token = __strtok_r(NULL, " \n", &token_ptr);

		if (token != NULL && strcmp(token, ">") == 0) {
			//get the output file name if there is output redirection
			token = __strtok_r(NULL, " \n", &token_ptr);
			currCommand->outputFile = calloc(strlen(token) + 1, sizeof(char));
			strcpy(currCommand->outputFile, token);
		}
	}

	//if there is output redirection and it was caught first, get the file name 
	//and then check for input redirection - if there is input redirection,
	//also get that filename
	if (redOutput) {
		//get the output file name
		token = __strtok_r(NULL, " \n", &token_ptr);
		currCommand->outputFile = calloc(strlen(token) + 1, sizeof(char));
		strcpy(currCommand->outputFile, token);

		//check for input redirection
		token = __strtok_r(NULL, " \n", &token_ptr);

		if (token != NULL && strcmp(token, "<") == 0) {
			//get the input filename if there is input redirection
			token = __strtok_r(NULL, " \n", &token_ptr);
			currCommand->inputFile = calloc(strlen(token) + 1, sizeof(char));
			strcpy(currCommand->inputFile, token);
		}
	}

	//if after input/out redirection, there is a &, run in background is true
	if ((redOutput || redInput)) {

		//first, check if the current token is & - if it is, inBackground is true
		if (token != NULL && strcmp(token, "&") == 0) {
			currCommand->inBackground = true;
		}

		//otherwise, if token is not & but is also not NULL
		else if (token != NULL) {

			//get the next token and check for & - if & is there, inBackground is true
			token = __strtok_r(NULL, " \n", &token_ptr);
			if (token != NULL && strcmp(token, "&") == 0) {
				currCommand->inBackground = true;
			}
		}
	}

	return currCommand;
}


/*
* void executeCommand
* 
* Takes a struct commandLine and executes the command with the specified
* arguments.
* Input/Output redirection adapted from the exploration: processes and i/o
* example for stdin and stdout, found here: 
* https://canvas.oregonstate.edu/courses/1798831/pages/exploration-processes-and-i-slash-o
* execvp usage adapted from the exploration: process api - executing a new program
* examples, found here:
* https://canvas.oregonstate.edu/courses/1798831/pages/exploration-process-api-executing-a-new-program
*/
void executeCommand(struct commandLine* lineToExecute) {

	//the status of the child - used for waitpid()
	int childStatus;

	//fork a new process
	pid_t spawnPid = fork();

	//do different things depending on if there is an error, it is in the
	//child process, or if it is in the parent process
	switch (spawnPid) {
		//if there is an error
		case -1:
			perror("fork()\n");
			exit(1);
			break;
		//if in the child process, run the command
		case 0:
			execvp(lineToExecute->command, lineToExecute->argumentArray);
			//run the below if there is an error
			perror(lineToExecute->command);
			exit(2);
			break;
		//if in the parent process
		default:
			//wait for the child to be done
			spawnPid = waitpid(spawnPid, &childStatus, 0);
			break;
	}
}


/*
* int main(void)
* 
* The main function that runs the program.
*/
int main(void) {

	//bool for should the program be exited
	bool keepRunning = true;

	//storage for user input string
	char userInput[2048] = "\0";

	//store the pid of smallsh
	int smallshPid = getpid();

	//runs until the program should be exited
	while (keepRunning) {

		//clear the user input string if needed
		memset(userInput, "\0", sizeof(userInput));

		printf(": ");

		//get user input string
		fgets(userInput, 2048, stdin);

		//if the input starts with a \n, #, or space, ignore
		if (strncmp(userInput, "#", 1) == 0 || strncmp(userInput, "\n", 1) == 0 || strncmp(userInput, " ", 1) == 0) {
			continue;
		}

		//turn the user input string into a commandLine
		struct commandLine* userCommand = createCommandLine(userInput, smallshPid);

		printf("Command: %s\nArgument 1: %s\nArgument 2: %s\n", userCommand->command, userCommand->argumentArray[1], userCommand->argumentArray[2]);
		printf("Input File: %s\nOutput File: %s\n", userCommand->inputFile, userCommand->outputFile);
		printf("Background Task? %d\n", userCommand->inBackground);

		//if the command is exit, then exit
		if (strcmp(userCommand->command, "exit") == 0) {
			keepRunning = false;
		}

		//else, if it is another built-in command, send it to be executed
		else if(strcmp(userCommand->command, "cd") == 0 || strcmp(userCommand->command, "status") == 0) {
			printf("special case\n");
		}

		//else, send off that command line to be executed
		else {
			printf("not special\n");
			executeCommand(userCommand);
		}
		
	}


		

	/*
	char* testString = "hi";
	printf("%p\n", &testString);
	testString = "yo";
	printf("%p\n", &testString);



	char* testArray[512]; //an array of char pointers
	char* token1 = "Hello";

	testArray[0] = token1;
	printf("%p, %p\n", &token1, &testArray[0]);

	token1 = "hi";
	testArray[1] = token1;

	printf("%p, %p\n", &token1, &testArray[1]);

	printf("%s\n", testArray[0]);
	printf("%s\n", testArray[1]);

	
	//get main's pid
	int mainPID = getpid();

	printf("Hello!\n");
	printf("My pid is %d and my parent's pid is %d\n", getpid(), getppid());
	printf("%d\n", mainPID);
	*/

	return 0;
}