#include "myshell.h"
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>

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

int execute_shell_script(char* file_path)
{
	printf("EXECUTE SHELL SCRIPT\n");

	int status = 0;
	int pid = fork();

	if (pid == 0)
	{
		printf("CHILD OPEN\n");
		interactive = false;
		int in = open(file_path, O_RDONLY);
		dup2(in, 0);
		close(in);
		// exit(0);
	}

	if (pid > 0)
	{
		wait(&status);
		printf("PARENT END\n");
		return status;
	}

	return 0;
}

int execute_or_script(char* file_path, char* exe_argv[])
{
	int exe_status = access(file_path, X_OK);
	int r_status = access(file_path, R_OK);
	// printf("Access Status: %s: X: %i R: %i\n", file_path, exe_status, r_status);
	if (exe_status == 0)
	{
		int stat = call_ext_process(file_path, exe_argv);
		printf("EXE STATUS: %i\n", stat);
		if (stat == 0)
		{
			return stat;
		}
		else if (r_status == 0)
		{
			return execute_shell_script(file_path);
		}
		return -1;
	}
	else
	{
		if (r_status == 0)
		{
			return execute_shell_script(file_path);
		}
	}

	return -1;
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

char* get_cmd_name(SHELLCMD* t)
{
	return t->argv[0];
}

bool is_internal_cmd(SHELLCMD* t)
{
	char* internal_cmds[3] = {"exit", "cd", "time"};
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

void execute_internal_exit(SHELLCMD* t)
{
	if (t->argc <= 2)
	{
		int exit_code = t->argc == 2 ? atoi(t->argv[1]) : prev_exit_status;
		exit(exit_code);
	}
}

int execute_internal_cd(SHELLCMD* t)
{
	if (t->argc <= 2)
	{
		char* target_dir = t->argc == 2 ? t->argv[1] : HOME;
		int result = EXIT_FAILURE;
		if (target_dir[0] == '/')
			result = chdir(target_dir);
		else
		{
			char* new_str = malloc(strlen(CDPATH) + 1);
			strcpy(new_str, CDPATH);

			int path_size = 1000;
			char** paths = malloc(path_size);
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

int execute_internal_command(SHELLCMD* t)
{
	char* cmd_name = get_cmd_name(t);
	if (strcmp(cmd_name, "cd") == 0)
		execute_internal_cd(t);

	if (strcmp(cmd_name, "time") == 0)
		execute_internal_time();

	if (strcmp(cmd_name, "exit") == 0)
		execute_internal_exit(t);

	return EXIT_SUCCESS;
}


int execute_node(SHELLCMD* t)
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
	if (cmd_name[0] == '/' || (cmd_name[0] == '.'))
		exit_status = execute_or_script(cmd_name, t->argv);
	else
	{
		char* new_str = malloc(strlen(PATH) + 1);
		strcpy(new_str, PATH);

		int path_size = 1000;
		char** paths = malloc(path_size);
		split(new_str, ":", paths);
		for (int i = 0; i < path_size; i++)
		{
			if (paths[i] == NULL)
			{
				// printf("%s\n", "BREAK");
				break;
			}

			// printf("%s\n", paths[i]);
			char dest[500];
			join_str(dest, paths[i], cmd_name);
			int exit_status = execute_or_script(dest, t->argv);
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

void create_redirect_output(char* file_name, bool append)
{
	int file = append ?
		           open(file_name, O_CREAT | O_WRONLY | O_APPEND) :
		           open(file_name, O_CREAT | O_WRONLY | O_TRUNC);
	dup2(file, 1);
	close(file);
}

void create_redirect_input(char* file_name)
{
	int file = open(file_name, O_CREAT | O_RDONLY);
	dup2(file, 0);
	close(file);
}

int in_file(SHELLCMD* cmd)
{
	printf("IN FUNCTION\n");
	create_redirect_input(cmd->infile);
	cmd->infile = NULL;
	int sub_exit = execute_shellcmd(cmd);
	exit(sub_exit);
}

int out_file(SHELLCMD* cmd)
{
	// Child Process, execute the cmd.
	create_redirect_output(cmd->outfile, cmd->append);
	cmd->outfile = NULL;
	int sub_exit = execute_shellcmd(cmd);
	exit(sub_exit);
}

int wait_for_child(SHELLCMD* cmd)
{
	int status = EXIT_SUCCESS;
	wait(&status);
	return status;
}

int fork_and_execute(SHELLCMD* t, int child_function(SHELLCMD* cmd), int parent_function(SHELLCMD* cmd))
{
	int pid = fork();
	int exit_status = EXIT_SUCCESS;

	if (pid < 0)
		ExitWithError("Unable to fork process");

	if (pid == 0)
		child_function(t);

	if (pid > 0)
		parent_function(t);

	return exit_status;
}

int subshell(SHELLCMD* t)
{
	// Child Process, execute the cmd.
	int sub_exit = execute_shellcmd(t->left);
	exit(sub_exit);
}

int fd[2];

int pipe_writer(SHELLCMD* t)
{
	// Write End
	dup2(fd[1], 1);
	close(fd[0]);
	int sub_exit = execute_shellcmd(t->left);
	exit(sub_exit);
}

int pipe_reader(SHELLCMD* t)
{
	int status;
	// Read End
	dup2(fd[0], 0);
	close(fd[1]);
	wait(&status);
	int sub_exit = execute_shellcmd(t->right);
	exit(sub_exit);
}

int pipe_cmd(SHELLCMD* t)
{
	pipe(fd);
	return fork_and_execute(t, &pipe_writer, &pipe_reader);
}

int execute_trav_cmd(SHELLCMD* t)
{
	// Traverse and execute each command
	if (t->infile != NULL)
		return fork_and_execute(t, &in_file, &wait_for_child);

	if (t->outfile != NULL)
		return fork_and_execute(t, &out_file, &wait_for_child);

	if (t->type == CMD_SUBSHELL)
		return fork_and_execute(t, &subshell, &wait_for_child);

	if (t->type == CMD_COMMAND)
		return execute_node(t);

	if (t->type == CMD_PIPE)
		return fork_and_execute(t, &pipe_cmd, &wait_for_child);

	int left_result = execute_trav_cmd(t->left);

	if (t->type == CMD_AND && left_result == EXIT_SUCCESS)
		return execute_trav_cmd(t->right);

	if (t->type == CMD_OR && left_result != EXIT_SUCCESS)
		return execute_trav_cmd(t->right);

	if (t->type == CMD_SEMICOLON)
		return execute_trav_cmd(t->right);

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

		// TODO: Print this to stderr instead.
		printf("COMMAND TIME: %ims\n", ms_duration);
	}

	if (activate_timer)
	{
		time_next_command = true;
		activate_timer = false;
	}

	return prev_exit_status;
}
