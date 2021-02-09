/*
* Author: Katelyn Lindsey
* Date: 2/4/2021
* Assignment: Homework 3
* Description: smallsh - A program for a shell written in C. Includes built-in 
* commands for exit, status, and cd, as well as variable expansion for $$. Also
* includes signal handling for both SIGINT and SIGTSTP
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>

//a global variable to track if background processses are allowed
bool backgroundAllowed = true;

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
char* varExpansion(char* token, int smallshPid, char* expandedToken) {

	//to keep track of the number of $ found in a row
	int numInRow = 0;

	//temporary storage for the new token
	char newToken[2048] = "\0";

	//the string version of the smallshPid for concatenation
	char pidString[12] = "\0";
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

			//if there was only one $ found, then add it to the newToken and
			//also add the current character to the newToken
			if (numInRow == 1) {
				strncat(newToken, "$", 1);
				strncat(newToken, token + i, 1);
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

	//finally, point expandedToken towards the new token
	expandedToken = newToken;
	return expandedToken;
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

	//if a token contains $$ and needs to be expanded, this will be used to
	//point towards that new token
	char* expandedToken;


	//first, get the command (always the first thing)
	char* token = __strtok_r(givenLine, " \n", &token_ptr);

	//if the token contains $$, send it off for expansion
	if (strstr(token, "$$") != NULL) {

		//printf("Found $$ in %s\n", token);
		token = varExpansion(token, smallshPid, expandedToken);
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
	int arrayIndex = 1;

	while (token != NULL) {

		//always first check for $$ - if the token contains it, send it
		//off for expansion
		if (strstr(token, "$$") != NULL) {
			//printf("Found $$ in %s\n", token);
			token = varExpansion(token, smallshPid, expandedToken);
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
		//printf("%s\n", token);
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
* int executeCommand
* 
* Takes a struct commandLine and executes the command with the specified
* arguments. Returns the exit status.
* Input/Output redirection adapted from the exploration: processes and i/o
* example for stdin and stdout, found here: 
* https://canvas.oregonstate.edu/courses/1798831/pages/exploration-processes-and-i-slash-o
* https://repl.it/@cs344/54sortViaFilesc
* execvp usage adapted from the exploration: process api - executing a new program
* examples, found here:
* https://canvas.oregonstate.edu/courses/1798831/pages/exploration-process-api-executing-a-new-program
*/
int executeCommand(struct commandLine* lineToExecute, int* pidArray) {

	//the status of the child - used to return the status of the child process
	int childStatus;

	//fork a new process
	pid_t childPid = fork();

	//do different things depending on if there is an error, it is in the
	//child process, or if it is in the parent process
	switch (childPid) {

		//if there is an error
		case -1:
			perror("fork()\n");
			exit(1);
			break;

		//if in the child process, run the command
		case 0:

			//if it is a foreground process, have default behavior for sigint
			if (lineToExecute->inBackground == 0) {
				signal(SIGINT, SIG_DFL);
			}

			//make all children ignore sigtstp
			signal(SIGTSTP, SIG_IGN);

			//if there is an input file, redirect the input
			if (lineToExecute->inputFile != NULL) {
				//first, open the file for reading
				int inputFD = open(lineToExecute->inputFile, O_RDONLY);

				//if the file cannot be opened for reading, print error and exit
				if (inputFD == -1) {
					fprintf(stderr, "cannot open %s for input\n", lineToExecute->inputFile);
					fflush(stdout);
					exit(1);
				}

				//next, redirect stdin to the input file
				int inputResult = dup2(inputFD, 0);
				
				//if redirecting the input doesn't work, print error message and exit
				if (inputResult == -1) {
					fprintf(stderr, "cannot redirect input to %s\n", lineToExecute->inputFile);
					fflush(stdout);
					exit(1);
				}

				//if it worked, set the FD_CLOEXEC flag for the input file to close on exit
				fcntl(inputFD, F_SETFD, FD_CLOEXEC);
			}

			//if there is an output file, redirect the output
			if (lineToExecute->outputFile != NULL) {
				//first, open the file for writing only - file will be truncated if it exists
				//and created if it doesn't exist
				int outputFD = open(lineToExecute->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

				//if the file cannot be opened, print error and exit
				if (outputFD == -1) {
					fprintf(stderr, "cannot open %s for output\n", lineToExecute->outputFile);
					fflush(stdout);
					exit(1);
				}

				//next, redirect stdout to the output file
				int outputResult = dup2(outputFD, 1);
				
				//if redirecting the output doesn't work, print error message and exit
				if (outputResult == -1) {
					fprintf(stderr, "cannot redirect output to %s\n", lineToExecute->outputFile);
					fflush(stdout);
					exit(1);
				}

				//if it worked, set the FD_CLOEXEC flag for the output file to close on exit
				fcntl(outputFD, F_SETFD, FD_CLOEXEC);
			}

			//if the process is going to run in the background
			if (lineToExecute->inBackground == 1) {

				//if there was no input file, redirect to /dev/null
				if (lineToExecute->inputFile == NULL) {
					//first, open /dev/null
					int inputFD = open("/dev/null", O_RDONLY);

					//if /dev/null cannot be opened, print error and exit
					if (inputFD == -1) {
						fprintf(stderr, "cannot open /dev/null for input\n");
						fflush(stdout);
						exit(1);
					}

					//redirect to /dev/null
					int inputResult = dup2(inputFD, 0);

					//if redirecting the input to /dev/null doesn't work, print error message and exit
					if (inputResult == -1) {
						fprintf(stderr, "cannot redirect input to /dev/null\n");
						fflush(stdout);
						exit(1);
					}
				}

				//if there was no output file, redirect to /dev/null
				if (lineToExecute->outputFile == NULL) {
					//first, open /dev/null 
					int outputFD = open("/dev/null", O_WRONLY);

					//if /dev/null cannot be opened, print error and exit
					if (outputFD == -1) {
						fprintf(stderr, "cannot open /dev/null for output\n");
						fflush(stdout);
						exit(1);
					}

					//redirect to /dev/null
					int outputResult = dup2(outputFD, 1);

					//if redirection the output to /dev/null doesn't work, print error message and exit
					if (outputResult == -1) {
						fprintf(stderr, "cannot redirect output to /dev/null\n");
						fflush(stdout);
						exit(1);
					}
				}
			}

			execvp(lineToExecute->command, lineToExecute->argumentArray);
			//run the below if there is an error
			perror(lineToExecute->command);
			exit(1);
			break;

		//if in the parent process
		default:
			//if this is a foreground process, wait for the child to be done
			if (lineToExecute->inBackground == 0) {
				waitpid(childPid, &childStatus, 0);
			}

			//otherwise if in the background, don't wait for the child
			else {
				waitpid(childPid, &childStatus, WNOHANG);
				//output that the background process has started
				printf("background pid is %d\n", childPid);
				fflush(stdout);

				//add childPid to the end of the array of background processes
				int index = 0;
				//find the end of the array
				while (pidArray[index] != NULL) {
					index++;
				}
				//add to the end of the array
				pidArray[index] = childPid;

				return 0;
			}

			//if the child terminated because of a signal, get what signal it was
			if (WIFSIGNALED(childStatus)) {
				printf("terminated by signal %d\n", WTERMSIG(childStatus));
				fflush(stdout);
				//when the status is this value, it will print the last signal that terminated it
				return -2;
			}

			//check if the child terminated normally - if it did, get the status
			if (WIFEXITED(childStatus)) {
				//return the child status
				return WEXITSTATUS(childStatus);
			}
			//otherwise if the child did not terminate normally
			else {
				return -1;
			}

			break;
	}
}

/*
* sigtstpHandler
* 
* Handles the SIGTSTP signal by toggling the backgroundAllowed to true or false.
*/
void sigtstpHandler(int sig) {

	//if backgroundAllowed is true, set to false and print message that foreground-only
	//mode is on
	if (backgroundAllowed) {
		backgroundAllowed = false;
		char* notAllowed = "\nEntering foreground-only mode (& is now ignored)\n: ";
		write(STDOUT_FILENO, notAllowed, 53);
	}

	//otherwise, set to true and print message that foreground-only mode is off
	else {
		backgroundAllowed = true;
		char* allowed = "\nExiting foreground-only mode\n: ";
		write(STDOUT_FILENO, allowed, 33);
	}
}


/*
* int main(void)
* 
* The main function that runs the program.
* 
* The signal handlers are adapted from the signal handling example:
* https://repl.it/@cs344/53siguserc
*/
int main(void) {

	//ignore ctrl-c for parent
	signal(SIGINT, SIG_IGN);
	
	//signal handler struct for SIGTSTP 
	struct sigaction SIGTSTP_action = { 0 };
	//register a signal handler
	SIGTSTP_action.sa_sigaction = sigtstpHandler;
	sigfillset(&SIGTSTP_action.sa_mask);
	//set sa_restart flag
	SIGTSTP_action.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	//bool for should the program be exited
	bool keepRunning = true;

	//storage for user input string
	char userInput[2048] = "\0";

	//store the pid of smallsh
	int smallshPid = getpid();

	//store the home environment
	char* home = getenv("HOME"); 

	//to keep track of the status of the last command
	int status = 0;

	//an array to store background processes currently running - used when 
	//exiting the shell
	int pidArray[256];

	//runs until the program should be exited
	while (keepRunning) {

		//check the status of all background processes
		int index = 0;
		int backgroundStatus = 0;

		//iterate through the list of background processes
		while (pidArray[index] != NULL) {
			//if waitpid returns something other than the child's pid, then
			//it is not finished yet - continue
			if (waitpid(pidArray[index], &backgroundStatus, WNOHANG) != pidArray[index]) {
				index++;
				continue;
			}

			//otherwise, print that the process is complete and remove
			//it from the list of processes, moving everything else over
			else {
				//if there was an error when the processes exited, the status is 1
				if (backgroundStatus != 0) {
					backgroundStatus = 1;
				}
				printf("background pid %d is done: exit value %d\n", pidArray[index], backgroundStatus);
				fflush(stdout);
			}
		}

		//clear the user input string if needed
		memset(userInput, "\0", sizeof(userInput));

		printf(": ");
		fflush(stdout);

		//get user input string
		fgets(userInput, 2048, stdin);

		//if the input starts with a \n, #, or space, ignore
		if (strncmp(userInput, "#", 1) == 0 || strncmp(userInput, "\n", 1) == 0 || strncmp(userInput, " ", 1) == 0) {
			continue;
		}

		//turn the user input string into a commandLine
		struct commandLine* userCommand = createCommandLine(userInput, smallshPid);

		//printf("Command: %s\nArgument 1: %s\nArgument 2: %s\n", userCommand->command, userCommand->argumentArray[1], userCommand->argumentArray[2]);
		//printf("Input File: %s\nOutput File: %s\n", userCommand->inputFile, userCommand->outputFile);
		//printf("Background Task? %d\n", userCommand->inBackground);

		//if the command is exit, then exit
		if (strcmp(userCommand->command, "exit") == 0) {

			//iterate through the list of child processes and terminate them
			int index = 0;
			while (pidArray[index] != NULL) {
				kill(pidArray[index], SIGTERM);
				index++;
			}

			//this will stop the while loop from running again
			keepRunning = false;
		}

		//else, if it is another built-in command, send it to be executed
		else if(strcmp(userCommand->command, "cd") == 0 || strcmp(userCommand->command, "status") == 0) {
			//printf("special case\n");

			//if the command is status, return the status of the last foreground process
			if (strcmp(userCommand->command, "status") == 0) {
				//if the status is -2, it was terminated by signal 2
				if (status == -2) {
					printf("terminated by signal 2\n");
					fflush(stdout);
				}
				//otherwise if the process was not terminated by a signal, print
				//status as normal
				else {
					printf("exit value %d\n", status);
					fflush(stdout);
				}
			}

			//else if the command is cd, change the directory to the path given (if the path given is valid)
			//or if just cd, change to the home environment 
			else {
				//if there is a path given, change to that
				if (userCommand->argumentArray[1] != NULL) {
					int changeSuccess = chdir(userCommand->argumentArray[1]);

					//if the path is invalid, print an error message
					if (changeSuccess == -1) {
						printf("%s: invalid directory\n", userCommand->argumentArray[1]);
						fflush(stdout);
					}
				}
				//otherwise, change to the home directory
				else {
					chdir(home);
				}
			}
		}

		//else, send off that command line to be executed
		else {
			//first, check if background processes are allowed - if not,
			//make sure inBackground = false
			if (!backgroundAllowed) {
				userCommand->inBackground = false;
			}

			//printf("not special\n");
			//if it will be a foreground process, save the status
			if (userCommand->inBackground == 0) {
				status = executeCommand(userCommand, pidArray);
			}
			//otherwise, don't save the status
			else {
				executeCommand(userCommand, pidArray);
			}
		}
		
	}

	return 0;
}