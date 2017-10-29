#include "myshell.h"
#include <sys/wait.h>

/*
   CITS2002 Project 2 2017
   Name(s):		student-name1 (, student-name2)
   Student number(s):	student-number-1 (, student-number-2)
   Date:		date-of-submission
 */

// -------------------------------------------------------------------

//  THIS FUNCTION SHOULD TRAVERSE THE COMMAND-TREE and EXECUTE THE COMMANDS
//  THAT IT HOLDS, RETURNING THE APPROPRIATE EXIT-STATUS.
//  READ print_shellcmd0() IN globals.c TO SEE HOW TO TRAVERSE THE COMMAND-TREE

void ExitWithError(char* message)
{
	perror(message);
	exit(1);
}

void CallExternalProcess(char* process_name, char* exe_argv[])
{
	int pid = fork();
	printf("Process ID: %d\n", pid);
	if (pid < 0)
		ExitWithError("Unable to fork process");

	if (pid == 0)
	{
		execv(process_name, exe_argv);
		ExitWithError("Unable to execute new process");
	}

	if (pid > 0)
	{
		int status = 0;
		wait(&status);
	}
	printf("Exit ID: %d\n", pid);
}

void execute_node(SHELLCMD *t)
{
	char* cmd_name = t->argv[0];
	CallExternalProcess(cmd_name, t->argv);
}

int execute_shellcmd(SHELLCMD* t)
{
	if (t == NULL)
		return EXIT_FAILURE;

	execute_node(t);
	return EXIT_SUCCESS;
}