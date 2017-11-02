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

#define NUM_PATHS 1000
#define MAX_PATH_LENGTH 255
#define EXIT_UNKNOWN 99
#define EXIT_CANNOT_EXECUTE 100

int prev_exit_status = EXIT_SUCCESS;
bool activate_timer = false;
bool time_next_command = false;
bool g_timer_ready = false;
bool g_timer_active = false;
struct timeval g_tval_start, g_tval_end;

char* g_shell_script_path;
char* g_ext_process_name;
char** g_ext_argv;
int g_pipe_fd[2];

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

int get_search_paths(char* path, char* target, SHELLCMD* t, int path_function(SHELLCMD* cmd, char* f_path))
{
	char* path_copy = malloc(strlen(path) + 1);
	strcpy(path_copy, path);

	char** paths = malloc(NUM_PATHS);
	split(path_copy, ":", paths);

	int result = EXIT_FAILURE;

	for (int i = 0; i < NUM_PATHS; i++)
	{
		if (paths[i] == NULL)
			break;

		char dest[MAX_PATH_LENGTH];
		join_path(dest, paths[i], target);
		result = path_function(t, dest);
		if (result != EXIT_UNKNOWN)
			break;
	}

	free(paths);
	free(path_copy);
	return result;
}

// ----------------------------------------------------------

bool is_internal_cmd(SHELLCMD* t)
{
	char* internal_cmds[3] = {"exit", "cd", "time"};
	char* cmd_name = t->argv[0];
	for (int i = 0; i < 3; i++)
	{
		if (strcmp(cmd_name, internal_cmds[i]) == 0)
			return true;
	}
	return false;
}

void execute_internal_exit(SHELLCMD* t)
{
	int exit_code = t->argc >= 2 ? atoi(t->argv[1]) : prev_exit_status;
	exit(exit_code);
}

int run_chdir(SHELLCMD* t, char* f_path)
{
	return chdir(f_path);
}

int execute_internal_cd(SHELLCMD* t)
{
	char* target_dir = t->argc >= 2 ? t->argv[1] : HOME;
	int result;

	if (target_dir[0] == '/')
		result = chdir(target_dir);
	else
		result = get_search_paths(CDPATH, target_dir, t, &run_chdir);

	if (result != EXIT_SUCCESS)
		perror(target_dir);

	return result;
}

void execute_internal_time()
{
	g_timer_ready = true;
}

void start_timer()
{
	if (g_timer_ready)
	{
		gettimeofday(&g_tval_start, NULL);
		g_timer_active = true;
		g_timer_ready = false;
	}
}

void stop_timer()
{
	if (!g_timer_active)
		return;
	gettimeofday(&g_tval_end, NULL);
	int sec_duration = (g_tval_end.tv_sec - g_tval_start.tv_sec) * 1000;
	int usec_duration = (g_tval_end.tv_usec - g_tval_start.tv_usec) / 1000;
	int ms_duration = sec_duration + usec_duration;
	g_timer_active = false;
	fprintf(stderr, "%imsec\n", ms_duration);
}

void clear_timer()
{
	g_timer_active = false;
	g_timer_ready = false;
}

int execute_internal_command(SHELLCMD* t)
{
	char* cmd_name = t->argv[0];
	if (strcmp(cmd_name, "cd") == 0)
		execute_internal_cd(t);

	if (strcmp(cmd_name, "time") == 0)
		execute_internal_time();

	if (strcmp(cmd_name, "exit") == 0)
		execute_internal_exit(t);

	return EXIT_SUCCESS;
}

// ----------------------------------------------------------

int fork_and_execute(SHELLCMD* t, int child_function(SHELLCMD* cmd), int parent_function(SHELLCMD* cmd))
{
	int pid = fork();
	int exit_status = EXIT_SUCCESS;

	if (pid < 0)
		exit(EXIT_FAILURE);

	if (pid == 0)
		exit_status = child_function(t);

	if (pid > 0)
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
	// printf("RUN SHELL SCRTIPT\n");
	g_shell_script_path = file_path;
	return fork_and_execute(NULL, &read_file_to_input, &wait_for_child);
}

int run_cmd(char* path, char* exe_argv[])
{
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

int run_cmd_at_path(SHELLCMD* t, char* path)
{
	return run_cmd(path, t->argv);
}

int execute_cmd_node(SHELLCMD* t)
{
	start_timer();
	char* cmd_name = t->argv[0];
	int exit_status;

	if (is_internal_cmd(t))
	{
		exit_status = execute_internal_command(t);
	}
	else
	{
		if (strchr(cmd_name, '/') != NULL)
			exit_status = run_cmd(cmd_name, t->argv);
		else
			exit_status = get_search_paths(PATH, cmd_name, t, &run_cmd_at_path);
	}

	if (exit_status == EXIT_UNKNOWN)
		perror("Unable to execute command");

	stop_timer();
	prev_exit_status = exit_status;
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
	if (t == NULL)
		return EXIT_FAILURE;

	if (t->infile != NULL)
		return fork_and_execute(t, &redirect_in, &wait_for_child);

	if (t->outfile != NULL)
		return fork_and_execute(t, &redirect_out, &wait_for_child);

	if (t->type == CMD_SUBSHELL)
		return fork_and_execute(t, &run_subshell, &wait_for_child);

	if (t->type == CMD_COMMAND)
		return execute_cmd_node(t);

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
