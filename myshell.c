#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

extern char **readline();

// execute the command given
int execute (char **input, int type, int special, char * home) {
	int f;							// file descriptor
	char *newInput[100] = {NULL};	// new input up to special characters
	char *leftInput[100] = {NULL};	// left half of pipe command
	char *rightInput[100] = {NULL};	// right half of pipe command

	//switch to case which is found in main
	switch (type) {
		case 1: { // cd case
			if (input[1]) { 				// if the user inputs a directory for cd, chdir to it
				char * newDir = input[1];
				chdir(newDir);
			}
			else { 							// if the user doesn't input a directory, go back to home directory (which is where program gets run in this case)					
				chdir(home);
			}
			break;
		}
		case 2: { // normal argument(s)
			pid_t pid = fork(); 	// fork into 2 processes
			if (pid < 0)
				return -1;
			else if (pid == 0) { 	// execute in child process
				execvp(input[0], input);
			}
			else {					// wait for child in parent
				wait (NULL);
			}
			break;			
		}
		case 4: { // redirect output 
			for(int i = 0; i < special; i++) { // copy args into new array until '<'
				newInput[i] = (char *) calloc(1, sizeof(input[i]));
				memcpy(newInput[i], input[i], sizeof(input[i]));
			}

			char outFile[100]; // copy the output given into outFile
			strcpy(outFile, input[special+1]);


			pid_t pid = fork(); // fork into 2 processes
			if (pid < 0)
				return -1;
			else if (pid == 0) { // child
				f = open(outFile, O_CREAT | O_TRUNC | O_WRONLY, 0666); // open file
			    close(STDOUT_FILENO); // close std out and copy file descriptor of given file into it
			    dup(f);
			    close(f);
				execvp(newInput[0], newInput); // execute command now
			}
			else { // parent
				wait(NULL);	
				for(int i = 0; i < special; i++) { // free newinput
					free(newInput[i]);
				}
			}
			break;
		}
		case 5: { // redirect output (append) is exactly the same as case 4 except with append flag instead of truncate
			for(int i = 0; i < special; i++) {
				newInput[i] = (char *) calloc(1, sizeof(input[i]));
				memcpy(newInput[i], input[i], sizeof(input[i]));
			}

			char outFile[100]; 
			strcpy(outFile, input[special+2]);


			pid_t pid = fork();
			if (pid < 0)
				return -1;
			else if (pid == 0) {
				f = open(outFile, O_CREAT | O_APPEND | O_WRONLY, 0666); // APPEND MODE
			    close(STDOUT_FILENO);
			    dup(f);
			    close(f);
				execvp(newInput[0], newInput);
			}
			else {
				wait(NULL);	
				for(int i = 0; i < special; i++) {
					free(newInput[i]);
				}
			}
			break; 
		}
			break;
		case 6: { // redirect input is exactly the same as case 4 except replacing stdin with the input given
			for(int i = 0; i < special; i++) {
				newInput[i] = (char *) calloc(1, sizeof(input[i]));
				memcpy(newInput[i], input[i], sizeof(input[i]));
			}

			char inFile[100]; 
			strcpy(inFile, input[special+1]);

			pid_t pid = fork();
			if (pid < 0)
				return -1;
			else if (pid == 0) {
				f = open(inFile, O_RDONLY, 0666);
			    close(STDIN_FILENO); // close std in and copy file descriptor of given file into it
			    dup(f);
			    close(f);
				execvp(newInput[0], newInput);
			}
			else {
				wait(NULL);	
				for(int i = 0; i < special; i++) {
					free(newInput[i]);
				}
			}
			break;
		}
		case 7: { // piping
			int pipeFD[2]; // ends of pipe

			for(int i = 0; i < special; i++) { // parse left half of command into left input
				leftInput[i] = (char *) calloc(1, sizeof(input[i]));
				memcpy(leftInput[i], input[i], sizeof(input[i]));
			}

			for(int i = special+1; input[i] != NULL; i++) { // parse right half of command into left input
				rightInput[i-special-1] = (char *) calloc(1, sizeof(input[special+i+1]));
				memcpy(rightInput[i-special-1], input[i], sizeof(input[i]));
			}

			pipe(pipeFD); // pipe 

			pid_t pid1 = fork(); // fork into 2 processes
			if (pid1 < 0)
			 	return -1;
			else if (pid1 == 0) {	 // child			
			    close(STDOUT_FILENO); // close std out and copy f into it
			    dup(pipeFD[1]);
			    close(pipeFD[1]);
			    close(pipeFD[0]);
			    execvp(leftInput[0], leftInput); // execute left half now
			}

			pid_t pid2 = fork(); // fork into 2 processes
			if (pid2 < 0)
			 	return -1;
			else if (pid2 == 0) { // child
			    close(STDIN_FILENO); // close std in and copy f into it
			    dup(pipeFD[0]);
			    close(pipeFD[1]);
			    close(pipeFD[0]);
			    execvp(rightInput[0], rightInput);// execute right half now
			}
			else { // parent
				close(pipeFD[0]); //close ends of pipe to restore back to normal
  				close(pipeFD[1]);
				waitpid(pid2, (void *)0, 0);
				for(int i = 0; leftInput[i]; i++) { // free left input
					free(leftInput[i]);
				}
				for(int i = 0; rightInput[i]; i++) { // free right input
					free(rightInput[i]);
				}
			}
			break;
		}
		default: { // error just in case
			printf("Invalid argument\n");
			break;
		}
	}

	return 0;
}

int main() {
	int cases, special;	// different cases and special char indexes
	char **args;		// from lex

	char homeDir[256];	// home dir string
	getcwd(homeDir, sizeof(homeDir)); // get home directory at the start

	do {
		printf("\n%s", "$$$ "); // print prompt
		args = readline();

		cases = 2; // default to case 2 if no special chars

		for(int i = 0; args[i] != NULL; i++) {

			if (!strcmp(args[i], ">")) {
				if (!strcmp(args[i-1], ">")) { // check for double carrots, case 5, and save special char index
					cases = 5;
					special = i-1;
				}
				else { // case 4 if single carrot
					cases = 4;
					special = i;					
				}
			}
			else if (!strcmp(args[i], "<")) { // case 6 if backwards carrot, and save special char index
				cases = 6;
				special = i;
			}
			else if (!strcmp(args[i], "|")) { // case 7 if pipe char, and save special char index
				cases = 7;
				special = i;
			}
		}

		if (!strcmp(args[0], "cd")) { // special case if cd
			cases = 1;
		}
		execute(args, cases, special, homeDir);
	} while (strcmp(args[0], "exit")); // case 1 if exit, out of loops

	return(0);
}

