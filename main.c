/*
* Author: Katelyn Lindsey
* Date: 2/4/2021
* Assignment: Homework 3
* Description: smallsh - A program for a shell written in C.
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
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
* struct commandLine* createCommandLine
* 
* Takes a string and creates a struct commandLine, returing a pointer to 
* the commandLine made.
*/
struct commandLine* createCommandLine(char* givenLine) {

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
	currCommand->command = calloc(strlen(token) + 1, sizeof(char));
	strcpy(currCommand->command, token);

	//also store the command in the arguments pointer array
	currCommand->argumentArray[0] = currCommand->command;

	//set inBacground to false
	currCommand->inBackground = false;


	//next, add all of the arguments to the argument pointer array until either
	//the end of the string is reached or <, >, & (at the end), keeping track of
	//the index of the argument array
	token = __strtok_r(NULL, " \n", &token_ptr);
	//printf("%s\n", token);
	int arrayIndex = 1;

	while (token != NULL) {

		//if the token is a < or >, keep track of that with boolean values
		if (strcmp(token, "<") == 0 || strcmp(token, ">") == 0) {

			if (strcmp(token, "<") == 0) {
				redInput = true;
			}
			
			if (strcmp(token, ">") == 0) {
				redOutput = true;
			}
		}

		//if < or > was found, break out of the loop
		if (redInput || redOutput) {
			break;
		}

		//else if & is found 
		/*if (strcmp(token, "&") == 0) {

			//if it is at the end of the file, in background is true
			char* tempPtr = token_ptr;

			char* tempToken = __strtok_r(NULL, " \n", &tempPtr);
			if (tempToken == NULL) {
				currCommand->inBackground = true;
			}
		}*/

		//otherwise, add the current token to the array of arguments
		printf("%s\n", token);
		currCommand->argumentArray[arrayIndex] = token;
		arrayIndex++;

		token = __strtok_r(NULL, " \n", &token_ptr);
	}

	//if the last argument includes \n, remove it
	if (strstr(currCommand->argumentArray[arrayIndex - 1], "\n") != NULL) {
		printf("Contains a newline!\n");

	}

	//if there is input redirection, get the file name and then check for
	//output redirection
	if (redInput) {
		token = __strtok_r(NULL, " \n", &token_ptr);
		currCommand->inputFile = calloc(strlen(token) + 1, sizeof(char));
		strcpy(currCommand->inputFile, token);

		token = __strtok_r(NULL, " \n", &token_ptr);
		if (token != NULL && strcmp(token, ">") == 0) {
			redOutput = true;
		}
	}

	//if there is output redirection, get the file name
	if (redOutput) {
		token = __strtok_r(NULL, " \n", &token_ptr);
		currCommand->outputFile = calloc(strlen(token) + 1, sizeof(char));
		strcpy(currCommand->outputFile, token);
	}

	//if after input/out redirection, there is a &, run in background is true
	if ((redOutput || redInput)) {
		token = __strtok_r(NULL, " \n", &token_ptr);
		if (token != NULL && strcmp(token, "&") == 0) {
			currCommand->inBackground = true;
		}
	}

	return currCommand;
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
		struct commandLine* userCommand = createCommandLine(userInput);

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
		}
		
	}



	//bool exitSmallsh = false;

	//while exitSmallsh is false:

		//print :
		//get the input from the user and store it (clear the string first)
		
		//if the string contains a #, it is a comment - skip this while iteration

		//otherwise, send off to function that tokenizes it (linked list?) and get a link to the head
		
		//if the command (first thing in list) is exit, cd, or status:
			//if exit
				//send to function that ends all child processes (iterates through a linkedlist of processes)
				//bool exitSmallsh = true
			//else
				//send off to the relevant function to be run (use switch statement)

		//else if it is not one of those
			//send off to be executed using one of the exec functions

	//once exited, properly exit and end the program
		

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