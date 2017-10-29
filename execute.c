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

void split(char* str, const char* delimiter, char* result[])
{
	int i = 0;
	char* token = strtok(str, delimiter);
	while (token != NULL)
	{
		result[i] = token;
		token = strtok(NULL, delimiter);
		i++;
	}
}

int CallExternalProcess(char* process_name, char* exe_argv[])
{
	int pid = fork();
	int status = 0;
	// int exit_status = EXIT_FAILURE;

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
		wait(&status);
	}
	printf("Exit ID: %d, Status: %d\n", pid, status);
	return status;
}

void join_str(char* dest, char* str1, char* str2)
{
	char* origin = dest;
	while (*str1 != '\0')
	{
		*dest = *str1;
		dest++;
		str1++;
	}

	*dest = '/';
	dest++;

	while (*str2 != '\0')

	{
		*dest = *str2;
		dest++;
		str2++;
	}

	*dest = '\0';
	printf("%s\n", origin);
}

bool is_internal_cmd(SHELLCMD *t)
{
	char* cmd_name = t->argv[0];
	if (strcmp(cmd_name, "exit") == 0)
	{
		return true;
	}
	return false;
}

void execute_node(SHELLCMD *t)
{
	char* cmd_name = t->argv[0];

	// Check if node is an internal command
	if (is_internal_cmd(t))
	{
		printf("INTERNAL COMMAND");
		return;
	}

	// Direct reference command (begins with /)
	if (cmd_name[0] == '/')
		CallExternalProcess(cmd_name, t->argv);
	else
	{
		// Search the PATH for the command and try to execute.
		char *paths[1000];
		// for (int i = 0; i < 100; i++)
			// paths[i] == NULL;

		split(PATH, ":", paths);
		for (int i = 0; i < 1000; i++)
		{
			if (paths[i] == NULL) {
				printf("%s\n", "BREAK");
				break;
			}

			printf("%s\n", paths[i]);
			char dest[500];
			join_str(dest, paths[i], cmd_name);
			int exit_status = CallExternalProcess(dest, t->argv);
			if (exit_status == EXIT_SUCCESS)
			{
				break;
			}
		}
	}
}

int execute_shellcmd(SHELLCMD* t)
{
	if (t == NULL)
		return EXIT_FAILURE;

	execute_node(t);
	return EXIT_SUCCESS;
}