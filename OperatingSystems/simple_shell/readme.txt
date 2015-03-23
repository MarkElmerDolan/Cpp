Mark Dolan
CIS 3207 Project Two
Write a UNIX shell program

The purpose of this program is to show some of the details of process creation on a UNIX environment.
This is done by implementing a simple shell which includes features such as concurrency and I/O redirection
The environment I used is a virtual machine running Debian, with VirtualBox

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

Key Variables and Functions

struct command
	stores the arguments in char **argv, argv[0] is the executable program
	stores input and output for '<' and '>' commands in char * output, and char * input
	detects if '&' and '|' operators were used and adjusts booleans, concurrent, isPipedOut,
		and isPipedIn accordingly
		
extern char* environ[], used to get commands in PATH

struct command **commander
	pointer to structs that contain command information

void my_prompt(void);
	prints a prompt with current working directory to user

void printCommandInfo(command **);
	loops through all commands and prints values

void executeCommands(command **);
	loops through all commands and executes them
	Detect if pipe operator used then execute command accordingly using namedPipe to store output and input
	If not a piped command then adjust stdin and stdout accordingly and execute command
	If concurrent operator detected parent doesn't wait
	
User Input Parsing
	Should have wrote a function for this since its so messy
	breaks user input into tokens based on " "
	looks for key operator characters '>', '<', '&', and '|' and adjusts data in struct commmand

Other functions
	fork()
		creates new process that is copy of original process
		parent process fork returns the newly created child processes PID
		child process fork returns zero

	execvp(char* path, char* argv[])
		changes the program that the current process is executing
	wait(&int_status)
		Process blocks itself until kernel signals it is ready to execute again
		Can return as a result of a child process terminating

Pipes
	pipes must be created by a common ancestor of the processes prior to creating the processes
	pipe syntax is writer | reader
	Have parent create pipe
	spawn child
	redirect stdout of parent to write end of pipe
	execute parent command
	redirect stdin child to read portion of pipe
	have child execute second command
	reader should flow from parent to child
	call close for any portion of pipe that you aren't using
	
Testing
	ls -l - store ls in command, store ls -l in argv, fork a child execute have parent wait, execute child, I/O unchanged
	ls -l > out.txt
		command 1: ls, argument 1: -l, Output redirected to "out.txt"
	sort < main.cpp
		command 1: sort, Input from "main.cpp"
	ls &firefox
		command 1: ls; Command 2: firefox, runs in background
	ls | wc
		Command 1: ls, Output to "namedPipe"; Command 2: wc, Input from "ls", output to stdin

