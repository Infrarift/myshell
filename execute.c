#include "myshell.h"
#include <sys/wait.h>
#include <sys/time.h>

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

int prev_exit_status = EXIT_SUCCESS;
bool activate_timer = false;
bool time_next_command = false;


void ExitWithError(char* message)
{
	// perror(message);
	exit(EXIT_FAILURE);
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

int call_ext_process(char* process_name, char* exe_argv[])
{
	int pid = fork();
	int status = 0;

	// printf("Process ID: %d\n", pid);
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
	return status;
}

void join_str(char* dest, char* str1, char* str2)
{
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
}

char *get_cmd_name(SHELLCMD *t)
{
	return t->argv[0];
}

bool is_internal_cmd(SHELLCMD *t)
{
	char* internal_cmds[3] = { "exit", "cd", "time" };
	char* cmd_name = get_cmd_name(t);

	for (int i = 0; i < 3; i++)
	{
		if (strcmp(cmd_name, internal_cmds[i]) == 0)
		{
			return true;
		}
	}
	return false;
}

void execute_internal_exit(SHELLCMD *t)
{
	if (t->argc <= 2) {
		int exit_code = t->argc == 2 ? atoi(t->argv[1]) : prev_exit_status;
		exit(exit_code);
	}
}

int execute_internal_cd(SHELLCMD *t)
{
	if (t->argc <= 2) {
		char* target_dir = t->argc == 2 ? t->argv[1] : HOME;
		int result = EXIT_FAILURE;
		if (target_dir[0] == '/')
			result = chdir(target_dir);
		else
		{
			char* new_str = malloc(strlen(CDPATH) + 1);
			strcpy(new_str, CDPATH);

			int path_size = 1000;
			char **paths = malloc(path_size);
			split(new_str, ":", paths);
			for (int i = 0; i < path_size; i++)
			{
				if (paths[i] == NULL)
					break;

				char dest[100];
				join_str(dest, paths[i], target_dir);
				result = chdir(dest);
				if (result == EXIT_SUCCESS)
					break;
			}
			free(paths);
			free(new_str);
		}
		return result;
	}
	return EXIT_FAILURE;
}

void execute_internal_time()
{
	printf("INTERNAL TIME\n");
	activate_timer = true;
}

int execute_internal_command(SHELLCMD *t)
{
	char *cmd_name = get_cmd_name(t);
	if (strcmp(cmd_name, "cd") == 0)
		execute_internal_cd(t);

	if (strcmp(cmd_name, "time") == 0)
		execute_internal_time();

	if (strcmp(cmd_name, "exit") == 0)
		execute_internal_exit(t);

	return EXIT_SUCCESS;
}



int execute_node(SHELLCMD *t)
{
	char* cmd_name = t->argv[0];
	int exit_status = 0;

	// Check if node is an internal command
	if (is_internal_cmd(t))
	{
		exit_status = execute_internal_command(t);
		return exit_status;
	}

	// Direct reference command (begins with /)
	if (cmd_name[0] == '/')
		exit_status = call_ext_process(cmd_name, t->argv);
	else
	{
		char* new_str = malloc(strlen(PATH) + 1);
		strcpy(new_str, PATH);

		int path_size = 1000;
		char **paths = malloc(path_size);
		split(new_str, ":", paths);
		for (int i = 0; i < path_size; i++)
		{
			if (paths[i] == NULL) {
				// printf("%s\n", "BREAK");
				break;
			}

			// printf("%s\n", paths[i]);
			char dest[500];
			join_str(dest, paths[i], cmd_name);
			int exit_status = call_ext_process(dest, t->argv);
			if (exit_status == EXIT_SUCCESS)
			{
				return exit_status;
			}
		}
		free(paths);
		free(new_str);
		return EXIT_FAILURE;
	}
	return exit_status;
}

int execute_trav_cmd(SHELLCMD *t)
{
	print_shellcmd(t);
	printf("\n");
	// Traverse and execute each command
	if (t->type == CMD_AND || t->type == CMD_OR || t->type == CMD_SEMICOLON)
	{
		int left_result = execute_trav_cmd(t->left);
		int right_result = EXIT_FAILURE;

		if (t->type == CMD_AND && left_result == EXIT_SUCCESS)
			right_result = execute_trav_cmd(t->right);

		if (t->type == CMD_OR && left_result == EXIT_FAILURE)
			right_result = execute_trav_cmd(t->right);

		if (t->type == CMD_SEMICOLON)
			right_result = execute_trav_cmd(t->right);

		return right_result;
	}

	if (t->type == CMD_COMMAND)
	{
		return execute_node(t);
	}

	return EXIT_SUCCESS;
}

int execute_shellcmd(SHELLCMD* t)
{
	struct timeval tvalBefore, tvalAfter;
	if (time_next_command)
	{
		gettimeofday(&tvalBefore, NULL);
	}

	if (t == NULL)
		prev_exit_status = EXIT_FAILURE;
	else
		prev_exit_status = execute_trav_cmd(t);

	if (time_next_command)
	{
		gettimeofday(&tvalAfter, NULL);
		time_next_command = false;
		int sec_duration = (tvalAfter.tv_sec - tvalBefore.tv_sec) * 1000;
		int usec_duration = (tvalAfter.tv_usec - tvalBefore.tv_usec) / 1000;
		int ms_duration = sec_duration + usec_duration;
		printf("COMMAND TIME: %ims\n", ms_duration);
	}

	if (activate_timer)
	{
		time_next_command = true;
		activate_timer = false;
	}

	return prev_exit_status;
}