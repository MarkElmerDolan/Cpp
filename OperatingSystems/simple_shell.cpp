// Mark Dolan - Implementation of simple UNIX shell for Operating Systems Class
// features are concurrency with the & operator, I/O redirection with <, >, and |(pipes)
// The program's purpose is to familiarize the programmer with process creation in a UNIX environment.
// The environment I used is a virtual machine running Debian, with VirtualBox

// commands are executable programs stored in /bin, implemented within the shell, or a specified path
// extern char* environ[], used to get commands in PATH
// strerror(errno) - troubleshoot errors by viewing global variable errno

/*
Pseudocode
	Set up shell/Initialize variables
	Display the current directory
	Listen for user input
	while(input != "exit"){
		Parse commands entered
		organize commands
		if( & operator){ Parent doesn't wait for child;}
		if( < operator){ map next arg to stdin for relevant command;}
		if( > operator){ map next arg to stdout for relevant command;}
		if( | operator){ 
			pipe syntax is writer | reader 
			fork the writer arg
			have writer arg create pipe
			spawn reader
			redirect stdout of writer to write end of pipe
			execute writer command
			redirect stdin reader to read portion of pipe
			have reader execute second command
			reader should flow from writer to reader 
			call close for any portion of pipe that you aren't using
		}
		execute the commands
		display current working directory
		continue listening for user input
	}
	
	sample commands
	ls -l - store ls in command, store ls -l in argv, fork a child execute have parent wait, 
		execute child, I/O unchanged
	ls > out.txt
		command 1: ls, Output redirected to "out.txt"
	sort < main.cpp
		command 1: sort, Input from "main.cpp"
	ls &firefox
		command 1: ls; Command 2: firefox, runs in background
	ls | less
		Command 1: ls, Output to "less"; Command 2: less, Input from "ls"
*/


#include <iostream>
#include <stdio.h>
#include <fstream>
#include <unistd.h> // extern char **environ, execvp(const char*, char *const)
#include <sys/types.h> //pid_t
#include <fcntl.h>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h> // status for wait()
using namespace std;

struct command {
	char **argv;
	char *input;
	char *output;
	bool concurrent;
};

void my_prompt(void);
void printCommandInfo(command **);
void executeCommands(command **);

int main(int argc, char **argv){

	string buffer;
	my_prompt(); // print current directory and indicator your using my shell
	getline(cin, buffer);

	while( buffer.compare( "exit") != 0){

		struct command **commander; // pointer to structs that contain command info 
		commander = (command **) malloc(sizeof(command **) * 64);
		int numOfCommands = 0;
		char *stringBuf = (char *) malloc (128); // for storing c strings

		// parse the user input
		vector<string> args;
		cout << "the buffer is:" << buffer << endl;
		char *cstr = new char[buffer.length() + 1];
		strcpy(cstr,  buffer.c_str()); // convert std::string c-string for strtok
		char *token = strtok( cstr, " ");
		while( token != NULL){
			args.push_back(token);
			token = strtok(NULL, " ");
		}
		// user input is now separated into tokens delimited by " "

		//initialize storage for command struct
		struct command tempCommand;
		tempCommand.argv = (char **) malloc(sizeof(char **) * 64); // I swear to God I always forget the sizeof part
		tempCommand.input = strdup("stdin\0");
		tempCommand.output = strdup("stdout\0");
		tempCommand.concurrent = false;
		commander[numOfCommands] = &tempCommand;
		commander[numOfCommands + 1] = NULL;
		bool io = false;

		int count = 0;
		for(vector<string>::iterator i = args.begin(); i != args.end(); ++i){

			// the executable is command[i].argv[0], the arguments are the elements after argv[0]
			// if reaches end then it is an argument to an executable
			if(count == 0){ // first word is always a command
				strcpy(stringBuf, i->c_str());
				commander[numOfCommands]->argv[count] = strdup(stringBuf);
				count++;
				commander[numOfCommands]->argv[count] = NULL;
			}else if( i->compare("<") == 0){
				strcpy(stringBuf, (i + 1)->c_str());
				commander[numOfCommands]->input = strdup(stringBuf);
				io = true;
			}else if( i->compare(">") == 0){
				strcpy(stringBuf, (i + 1)->c_str());
				commander[numOfCommands]->output = strdup(stringBuf);
				io = true;
			}else if( i->at(0) == '&'){
				cout << "Concurrent execution of program: " << i->substr(1) << endl;
				numOfCommands++;
				string tempBuf = i->substr(1);
				struct command concCommand;
				concCommand.argv = (char **) malloc(sizeof(char **) * 64); 
				concCommand.input = strdup("stdin\0");
				concCommand.output = strdup("stdout\0");
				commander[numOfCommands] = &concCommand;
				commander[numOfCommands + 1] = NULL;
				strcpy(stringBuf, tempBuf.c_str());
				commander[numOfCommands]->argv[0] = strdup(stringBuf);
				commander[numOfCommands]->concurrent = true;
				count = 1;
				commander[numOfCommands]->argv[count] = NULL;
			}else if( i->compare("|") == 0){
				cout << "Pipe detected: output of " << commander[numOfCommands]->argv[0] << " is input of " 
				<< *(i + 1) << endl;
				struct command pipeCommand;
				pipeCommand.argv = (char **) malloc(sizeof(char **) * 64); 
				strcpy(stringBuf, (i + 1)->c_str());
				pipeCommand.argv[0] = strdup(stringBuf); // set command of pipe
				commander[numOfCommands]->output = strdup(stringBuf); // change output of last command
				pipeCommand.input = strdup(commander[numOfCommands]->argv[0]); // set input of pipe
				pipeCommand.output = strdup("stdout\0");
				numOfCommands++;
				commander[numOfCommands] = &pipeCommand;
				commander[numOfCommands + 1] = NULL;
				commander[numOfCommands]->concurrent = false;
				count = 1;
				io = true;
			}else if(io == true){
				io = false;
			}else{
				strcpy(stringBuf, i->c_str());
				commander[numOfCommands]->argv[count] = strdup(stringBuf);
				count++;
				commander[numOfCommands]->argv[count] = NULL;
			}
		}
		printCommandInfo(commander);

		//done parsing commands, execution time
		executeCommands(commander);
		
		my_prompt();
		getline(cin, buffer); // loop exiting condition
	}
	return 0;
}

void my_prompt(){
	char prompt[512];
	char *cur = get_current_dir_name();
	char tmp[] = "<<<<Dolan_Shell>>>>";
	strcpy(prompt, cur);
	strcat(prompt,tmp);
	fputs(prompt, stdout); // use fputs to not have newline character
}

// print the command information for all commands stored in commander
void printCommandInfo(command **com){
	int i = 0;
	while(com[i] != NULL){
		cout << "Command " <<  i +1 << " is:" << com[i]->argv[0] << endl;
		int j = 1; 
		while(com[i]->argv[j] != NULL){
			cout << "Argument " << j << " is:" << com[i]->argv[j] << endl;
			j++;
		}
		if(com[i]->input != NULL){
			cout << "input is: " << com[i]->input << endl;
		}else{
			cout << "input redirection failure" << endl;
		}
		if(com[i]->output != NULL){
			cout << "output is: " << com[i]->output << endl;
		}else{
			cout << "output redirection failure " << endl;
		}
		if(com[i]->concurrent == true){
			cout << "program to run concurrently" << endl;
		}
		i++;
	}
}

// executes commands
void executeCommands(command **com){
	int i = 0;
	pid_t pid;
	while(com[i] != NULL){
		pid = fork();
		if(pid == 0){
			if(strcmp(com[i]->input, "stdin") != 0){
				cout << "changing stdin " << endl;
				int newstdin = open(com[i]->input, O_RDONLY);
				close(0);	//wipes out file descriptor table entry 0, stdin
				dup(newstdin);	//copies contents of newstdin to first empty table entry, 0 since we just closed it
				close(newstdin);//cleans things up so only one reference to input as stdin
			}
			if(strcmp(com[i]->output, "stdout") != 0){
				cout << "Changing stdout " << endl;
				int newstdout = open(com[i]->output,O_WRONLY|O_CREAT,S_IRWXG|S_IRWXO);
				close(1);
				dup(newstdout);	//copies contents of newstdout to first empty table entry
				close(newstdout);
			}
			execvp(com[i]->argv[0], com[i]->argv);
		}else if(com[i]->concurrent == true){
			break;
		}else{
			int status = 0;
			wait (&status);
			cout << "Child exited with status of " << status << endl;
		}
		i++;
	}
}

/*
Include list of functions used and their relationship to the solution

Document each function
	Include as comments next to statements
	State its purpose
	Describe algorithms used
	Decribe the input and output variables of a function
	Describe key local variables
	Choose good variable and function names

Testing
	Detail testing objectives
	Include critical points of failure
	Detail specific tests
	Present test data and output
*/
