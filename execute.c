#include "myshell.h"
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
CITS2002 Project 2 2017
Name(s):			JAKRIN JUANGBHANICH
Student number(s):	21789272
Date:				3-11-2017
*/


// MACRO CONSTANTS ----------------------------------------------------------

#define NUM_PATHS 1000
#define MAX_PATH_LENGTH 255
#define EXIT_UNKNOWN 99
#define EXIT_CANNOT_EXECUTE 100

// GLOBAL VARIABLES ----------------------------------------------------------

int g_prev_exit_status = EXIT_SUCCESS;
struct timeval g_tval_start, g_tval_end;
char* g_shell_script_path;
char* g_ext_process_name;
char** g_ext_argv;
int g_pipe_fd[2];
int g_pid;

// HELPER FUNCTIONS ----------------------------------------------------------

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

char* copy_str(char* target, char* soruce)
{
	while (*soruce != '\0')
	{
		*target = *soruce;
		target++;
		soruce++;
	}

	return target;
}

void join_path(char* dest, char* str1, char* str2)
{
	dest = copy_str(dest, str1);
	*dest = '/';
	dest++;
	dest = copy_str(dest, str2);
	*dest = '\0';
}

int search_and_execute(char* path, char* target, int path_function(char* f_path))
{
	// Search through the PATH and try to find a match for the target dir/file.

	// Create an array for the path.
	char* path_copy = malloc(strlen(path) + 1);
	strcpy(path_copy, path);

	// Split the path into individual strings.
	char** paths = malloc(NUM_PATHS);
	split(path_copy, ":", paths);

	int result = EXIT_FAILURE;

	// Try to find a successful path to execute/cd.
	for (int i = 0; i < NUM_PATHS; i++)
	{
		if (paths[i] == NULL)
			break;

		char dest[MAX_PATH_LENGTH];
		join_path(dest, paths[i], target);
		result = path_function(dest);
		if (result != EXIT_UNKNOWN)
			break;
	}

	free(paths);
	free(path_copy);
	return result;
}

// INTERNAL FUNCTIONS ----------------------------------------------------------

bool is_internal_cmd(char *cmd_name)
{
	return strcmp(cmd_name, "exit") == 0 || strcmp(cmd_name, "cd") == 0 || strcmp(cmd_name, "time") == 0;
}

void execute_internal_exit(int argc, char* argv[])
{
	int exit_code = argc >= 2 ? atoi(argv[1]) : g_prev_exit_status;
	exit(exit_code);
}

int run_chdir(char* f_path)
{
	// Wrap chdir so we can use it as a delegate.
	return chdir(f_path);
}

int execute_internal_cd(int argc, char* argv[])
{
	// Target is HOME unless provided.
	char* target_dir = argc >= 2 ? argv[1] : HOME;

	// If there is a '/' then cd directly, otherwise search.
	int result = target_dir[0] == '/' ? chdir(target_dir) : search_and_execute(CDPATH, target_dir, &run_chdir);

	if (result != EXIT_SUCCESS)
		perror(target_dir);
	return result;
}

void start_timer()
{
	gettimeofday(&g_tval_start, NULL);
}

void stop_timer()
{
	gettimeofday(&g_tval_end, NULL);
	int sec_duration = (g_tval_end.tv_sec - g_tval_start.tv_sec) * 1000;
	int usec_duration = (g_tval_end.tv_usec - g_tval_start.tv_usec) / 1000;
	int ms_duration = sec_duration + usec_duration;
	fprintf(stderr, "%imsec\n", ms_duration);
}

int execute_internal_time(int argc, char* argv[])
{
	if (argc == 1)
	{
		printf("Usage: time [command]\n");
		return EXIT_FAILURE;
	}

	start_timer();

	// Copy the arguments to duplicate this command.
	int copy_argc = argc - 1;
	char **copy_argv = calloc(copy_argc, sizeof(char*));
	for (int i = 0; i < copy_argc; i++)
		copy_argv[i] = argv[i + 1];

	// Execute the new command.
	execute_cmd_node(copy_argc, copy_argv);

	// Free the pointers.
	if (copy_argv != NULL)
		free(copy_argv);

	// End the timer if this is not a forked process.
	if (g_pid != 0)
		stop_timer();

	return EXIT_SUCCESS;
}

int execute_internal_command(int argc, char* argv[])
{
	char* cmd_name = argv[0];
	if (strcmp(cmd_name, "cd") == 0)
		return execute_internal_cd(argc, argv);

	if (strcmp(cmd_name, "exit") == 0)
		execute_internal_exit(argc, argv);

	if (strcmp(cmd_name, "time") == 0)
		execute_internal_time(argc, argv);

	return EXIT_FAILURE;
}

// NODE FUNCTIONS  ----------------------------------------------------------

int fork_and_execute(SHELLCMD* t, int child_function(SHELLCMD* cmd), int parent_function(SHELLCMD* cmd))
{
	g_pid = fork();
	int exit_status = EXIT_SUCCESS;

	if (g_pid < 0)
		exit(EXIT_FAILURE);

	if (g_pid == 0)
		exit_status = child_function(t);

	if (g_pid > 0)
		exit_status = parent_function(t);

	return exit_status;
}

int call_ext_process(SHELLCMD* t)
{
	execv(g_ext_process_name, g_ext_argv);
	exit(EXIT_CANNOT_EXECUTE);
}

int wait_for_child(SHELLCMD* t)
{
	int status = EXIT_SUCCESS;
	wait(&status);
	return status;
}

int read_file_to_input(SHELLCMD* t)
{
	interactive = false;
	int fd_read = open(g_shell_script_path, O_RDONLY);
	dup2(fd_read, 0);
	close(fd_read);
	return EXIT_SUCCESS;
}

int run_shell_script(char* file_path)
{
	g_shell_script_path = file_path;
	return fork_and_execute(NULL, &read_file_to_input, &wait_for_child);
}

int run_cmd(char* path, char* exe_argv[])
{
	// If the file is executable, try to run it first.
	// If the execution fails, then try to read it as a shell script.

	int x_status = access(path, X_OK);
	int r_status = access(path, R_OK);

	if (x_status == 0)
	{
		g_ext_process_name = path;
		g_ext_argv = exe_argv;
		int result = fork_and_execute(NULL, &call_ext_process, &wait_for_child);
		if (WEXITSTATUS(result) != EXIT_CANNOT_EXECUTE)
			return result;
	}

	return r_status == 0 ? run_shell_script(path) : EXIT_UNKNOWN;
}

int run_cmd_at_path(char* path)
{
	return run_cmd(path, g_ext_argv);
}

int execute_cmd_node(int argc, char* argv[])
{
	// Execute an actual command node.
	char* cmd_name = argv[0];
	int exit_status;

	if (is_internal_cmd(cmd_name))
	{
		// Internal command.
		exit_status = execute_internal_command(argc, argv);
	}
	else
	{
		if (strchr(cmd_name, '/') != NULL)
			// Direct command.
			exit_status = run_cmd(cmd_name, argv);
		else
		{
			// Search paths for the command.
			g_ext_argv = argv;
			exit_status = search_and_execute(PATH, cmd_name, &run_cmd_at_path);
		}
	}

	if (exit_status == EXIT_UNKNOWN)
		perror("Unable to execute command");

	g_prev_exit_status = exit_status;
	return exit_status;
}

int redirect_in(SHELLCMD* t)
{
	int file = open(t->infile, O_CREAT | O_RDONLY);
	dup2(file, 0);
	close(file);

	t->infile = NULL;
	int sub_exit = execute_shellcmd(t);
	exit(sub_exit);
}

int redirect_out(SHELLCMD* t)
{
	int file = t->append ?
		           open(t->outfile, O_CREAT | O_WRONLY | O_APPEND) :
		           open(t->outfile, O_CREAT | O_WRONLY | O_TRUNC);
	dup2(file, 1);
	close(file);

	t->outfile = NULL;
	int sub_exit = execute_shellcmd(t);
	exit(sub_exit);
}

int run_subshell(SHELLCMD* t)
{
	int sub_exit = execute_shellcmd(t->left);
	exit(sub_exit);
}

int run_pipe_writer(SHELLCMD* t)
{
	dup2(g_pipe_fd[1], 1);
	close(g_pipe_fd[0]);
	int sub_exit = execute_shellcmd(t->left);
	exit(sub_exit);
}

int run_pipe_reader(SHELLCMD* t)
{
	int status;
	dup2(g_pipe_fd[0], 0);
	close(g_pipe_fd[1]);
	wait(&status);
	int sub_exit = execute_shellcmd(t->right);
	exit(sub_exit);
}

int execute_pipe_node(SHELLCMD* t)
{
	pipe(g_pipe_fd);
	return fork_and_execute(t, &run_pipe_writer, &run_pipe_reader);
}

int execute_shellcmd(SHELLCMD* t)
{
	// Traverse the CMD tree and execute all nodes.

	if (t == NULL)
		return EXIT_FAILURE;

	if (t->infile != NULL)
		return fork_and_execute(t, &redirect_in, &wait_for_child);

	if (t->outfile != NULL)
		return fork_and_execute(t, &redirect_out, &wait_for_child);

	if (t->type == CMD_SUBSHELL)
		return fork_and_execute(t, &run_subshell, &wait_for_child);

	if (t->type == CMD_COMMAND)
		return execute_cmd_node(t->argc, t->argv);

	if (t->type == CMD_PIPE)
		return fork_and_execute(t, &execute_pipe_node, &wait_for_child);

	int left_result = execute_shellcmd(t->left);

	if (t->type == CMD_AND && left_result == EXIT_SUCCESS)
		return execute_shellcmd(t->right);

	if (t->type == CMD_OR && left_result != EXIT_SUCCESS)
		return execute_shellcmd(t->right);

	if (t->type == CMD_SEMICOLON)
		return execute_shellcmd(t->right);

	return EXIT_SUCCESS;
}
