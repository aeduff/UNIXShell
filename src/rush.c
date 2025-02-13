/*
Name: Eleanor Duffey
NetID: U70110284
Description: A program that acts as a shell program, allowing the user to type in commands and have them processed
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

// array to hold possible paths
char *paths[30]; 
// initialize path count
int path_count = 0;

// function to update the path with the input list and increase the count
void update_path(char *args[]) {
    path_count = 0;
    for (int i = 1; args[i] != NULL; i++) {
        paths[path_count++] = strdup(args[i]);
    }
}

// function to find the executable path
char* find_executable(char *command) {
    for (int i = 0; i < path_count; i++) {
        char full_path[256];
        snprintf(full_path, sizeof(full_path), "%s/%s", paths[i], command); // loads path into buffer
        if (access(full_path, X_OK) == 0) {
            return strdup(full_path); // if the path is accessible it's returned
        } else {
            continue;
        }
    }
    return NULL; // if the path is inaccessible NULL is returned
}

// handles function calls not in parallel
void function_calls(char *args[], int run_in_background) {
    int redirect = 0;
    char *output_file = NULL;
    int fd;

    int count = 0;
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            count++;
            if (count > 1) { // throws an error if more than one redirection
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                return;
            } else if (args[i + 1] != NULL && args[i + 2] == NULL) { // checks that only one arg follows the >
                output_file = args[i + 1]; // assigns output file name
                args[i] = NULL; // replaces > with NULL
                redirect = 1; // marks that a redirection is needed
            } else if (args[i + 1] != NULL && args[i + 2] != NULL) { // throws an error for two arguments after >
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                return;
            } else if (args[i + 1] == NULL){ // throws an error for nothing after the >
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                return;
            }
            
        }
    }
    

    char *executable = find_executable(args[0]); // sets the path

    if (executable == NULL) { // throws an error for an invalid path
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
    }

    pid_t pid = fork(); // forks the process

    if (pid == 0) { // child process
        if (redirect) {
            fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // opens file for output
            // error processing
            if (fd == -1) {
                char error_message[30] = "An error has occurred\n"; 
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            if (dup2(fd, STDERR_FILENO) == -1) {
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            close(fd); // close file
        }

        if (execv(executable, args) == -1) { // throw error if command fails
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
        }

        fflush(stdout);
        free(executable);
        exit(0); // exit child process
    } else if (pid < 0) { // fork failed
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
    } else { // parent process
        if (!run_in_background) {
            waitpid(pid, NULL, 0);
        }
    }

    free(executable); // Free allocated memory for the executable path
}


void run_parallel_commands(char *paraCmd[], int paraCount) { // function to handle parallel processing
    pid_t pids[paraCount];

    for (int i = 0; i < paraCount; i++) { // loop through each command
        char *arg;
        char *delims = " \n\t";
        char *args[50];
        int n = 0;
        int redirect = 0;
        char *output_file = NULL;
        int fd;

        char *command = paraCmd[i]; // separates commands into arguments
        while ((arg = strsep(&command, delims)) != NULL) {
            if (*arg == '\0') continue;
            if (strcmp(arg, ">") == 0) {
                redirect = 1; // marks for redirect if > is present
                if (command != NULL) { 
                    output_file = strsep(&command, delims); // grab output file name after >
                } else if (command == NULL) { // throws an error if there's no output file name
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    return;
                }
                break;
            }
            args[n++] = arg;
        }
        args[n] = NULL;

        if (args[0] == NULL) {
            continue;
        }

        // Fork for each command
        if ((pids[i] = fork()) == 0) {
            char *executable = find_executable(args[0]); // finds path for command
            if (executable == NULL) {
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1); // Exit child process on error
            }

            if (redirect) { // Handle output redirection for parallel processing
                fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (fd == -1) {
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(1);
                }
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    close(fd);
                    exit(1);
                }
                close(fd);
            }

            // Execute the command
            if (execv(executable, args) == -1) {
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1); // Exit child process with error
            }

            free(executable); // Free allocated memory for the executable path
            exit(0); // Exit child process
        } else if (pids[i] < 0) {
            // Fork failed
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }

    // Parent waits for all child processes to complete before returning to prompt loop
    for (int i = 0; i < paraCount; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

void prompt_loop(void) { // loop to process inputs

    char *buffer = NULL;
    size_t buffsize = 256;
    ssize_t line;
    int status = 1;
    char exitTime[] = "exit";
    

    // Set default path
    paths[0] = "/bin";
    path_count = 1;

    buffer = (char *)malloc(buffsize * sizeof(char));

    while (status == 1) {
        printf("rush> ");
        fflush(stdout);
        line = getline(&buffer, &buffsize, stdin);
        if (line != -1) {
            if (buffer[line - 1] == '\n') {
                buffer[line - 1] = '\0'; // replace newline character with endline character
            }

            char *input = buffer;
            char *paraCmd[30]; // array to hold commands
            int paraCount = 0;

            // Split input based on '&' to handle parallel commands
            while ((paraCmd[paraCount] = strsep(&input, "&")) != NULL) {
                paraCount++;
            }

            if (paraCount > 1) { // runs parallel if multiple parallel commands are present
                run_parallel_commands(paraCmd, paraCount);
                fflush(stdout);
                continue;
            }

            char *arg;
            char *delims = " \n\t";
            char *args[50];
            int n = 0;

            char *command = paraCmd[0]; // process commands into arguments
            while ((arg = strsep(&command, delims)) != NULL) {
                if (*arg == '\0') continue;
                args[n++] = arg;
            }
            args[n] = NULL;

            if (args[0] == NULL) {
                continue;
            }

            int argc = n;
            // process exit command
            if ((strcmp(args[0], exitTime) == 0) && argc == 1) {
                status = 0;
                break;
            } else if ((strcmp(args[0], exitTime) == 0) && argc > 1) { // ensures no arguments for exit command
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }
            // process cd command
            if (strcmp(args[0], "cd") == 0) {
                if (argc > 2 || argc == 1) { // handles no arguments or 2 arguments after cd
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                } else if (chdir(args[1]) != 0) { // throws error id dir does not change
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                continue;
            }
            // process path command
            if (strcmp(args[0], "path") == 0) {
                update_path(args);
                continue;
            }
            // if not parallel, run single command in function_calls
            if (paraCount > 1) {
                continue;
            } else {
                function_calls(args, 0);  // Non-parallel, normal execution
            }
        }
    }

    free(buffer); // free buffer memory
    exit(0); // exit without error if status == 0
}

int main(int argc, char **argv) {
    if (argc > 1) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1); // exits with errors if rush is executed with arguments
    }
    prompt_loop(); // enters input loop
}