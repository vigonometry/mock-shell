#include "myshell.h"

#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#define MAX_PROCESSES 50

size_t process_count = 0;
struct PCBTable processes[MAX_PROCESSES];
int curr_process[2] = {-1, -1}; //curr_process[0] - idx, curr_process[1] - pid

enum process_status { EXITED = 1, RUNNING = 2, TERMINATING = 3, STOPPED = 4 };
enum avail_info {ALL, COUNT_EXITED, COUNT_RUNNING, COUNT_TERMINATING, COUNT_STOPPED};
const char *commands[4] = {"info", "wait", "terminate", "fg"};

static void proc_update_status(size_t idx, int options){
  // Call everytime you need to update status and exit code of a process in PCBTable

  int status = -1;
  if (waitpid(processes[idx].pid, &status, options) <= 0 || processes[idx].status == EXITED) { //status > 0 - waiting for child whose PID is equal to the value of supplied pid
    return;
  }

  if (WIFEXITED(status)) {
    processes[idx].status = EXITED;
    processes[idx].exitCode = WEXITSTATUS(status);
  } else if (WIFSTOPPED(status)) {
    processes[idx].status = STOPPED;
  } else if (WIFSIGNALED(status)) {
    processes[idx].status = EXITED; //terminated by signal
    processes[idx].exitCode = WTERMSIG(status);
  }
}

static void signal_handler(int signo) {

  // Use the signo to identy ctrl-Z or ctrl-C and print “[PID] stopped or print “[PID] interrupted accordingly.
  // Update the status of the process in the PCB table
    
  for (int i = (int)process_count - 1; i >= 0; i--) {
    if (processes[i].status == RUNNING) {
      curr_process[0] = i;
      curr_process[1] = processes[i].pid;
      break;
    }
  }

  if (curr_process[0] != -1) {
    switch (signo) {
      case SIGTSTP:
        if (processes[curr_process[0]].status == RUNNING) {
          kill(curr_process[1], SIGSTOP);
          proc_update_status(curr_process[0], WNOHANG);
          printf("[%d] stopped\n", (int)curr_process[1]);
        }
        break;
      case SIGINT:
        if (processes[curr_process[0]].status == RUNNING) {
          kill(curr_process[1], SIGINT);
          proc_update_status(curr_process[0], WUNTRACED);
          printf("[%d] interrupted\n", (int)curr_process[1]);
        }
        break;
    }
  }
}

/*******************************************************************************
 * Utils
 ******************************************************************************/
char *get_status(int option) {
  switch (option) {
    case 1:
      return "Exited";
    case 2:
      return "Running";
    case 3:
      return "Terminating";
    case 4:
      return "Stopped";
    default:
      return NULL;
  }
}

char **get_command_args(size_t i, size_t num_args, char **tokens) {
  char **tmp = malloc((num_args + 1) * sizeof(char *));
  if (tmp == NULL) {
    exit(1);
  }
  for (size_t k = 0; k < num_args + 1; k++) {
    tmp[k] = NULL;
  }
  for (size_t k = 0; k < num_args; k++) {
    tmp[k] = tokens[i++];
  }

  return tmp;
}


/*******************************************************************************
 * Built-in Commands
 ******************************************************************************/

static void command_info(size_t size_args, char **tokens) {

  // If option is 0
  //Print details of all processes in the order in which they were run. You will need to print their process IDs, their current status (Exited, Running, Terminating, Stopped)
  // For Exited processes, print their exit codes.
  // If option is 1
  //Print the number of exited process.
  // If option is 2
  //Print the number of running process.
  // If option is 3
  //Print the number of terminating process.
  // If option is 4
  //Print the number of stopped process.
  if (size_args != 2 || !(tokens[1][0] >= 48 && tokens[1][0] <= 52)) {
    fprintf(stderr, "Wrong command\n");
    return;
  }

  int option = atoi(tokens[1]);
  int count = 0;
  switch (option) {
    case ALL:
      for (size_t i = 0; i < process_count; i++) {
        char *res = get_status(processes[i].status);
        if (processes[i].exitCode == -1) {
          printf("[%d] %s\n", processes[i].pid, res);
        } else {
          printf("[%d] %s %d\n", processes[i].pid, res, processes[i].exitCode);
        }
      }
      break;
    case COUNT_EXITED:
      for (size_t i = 0; i < process_count; i++) {
        if (processes[i].status == EXITED) {
          count++;
        }
      }
      printf("Total exited process: %d\n", count);
      break;
    case COUNT_RUNNING:
      for (size_t i = 0; i < process_count; i++) {
        if (processes[i].status == RUNNING) {
          count++;
        }
      }
      printf("Total running process: %d\n", count);
      break;
    case COUNT_TERMINATING:
      for (size_t i = 0; i < process_count; i++) {
        if (processes[i].status == TERMINATING) {
          count++;
        }
      }
      printf("Total terminating process: %d\n", count);
      break;
    case COUNT_STOPPED:
      for (size_t i = 0; i < process_count; i++) {
        if (processes[i].status == STOPPED) {
          count++;
        }
      }
      printf("Total stopped processes: %d\n", count);
      break;
    default:
      fprintf(stderr, "Wrong command\n");
      break;
  }

}

static void command_wait(size_t num_tokens, char **tokens){

  /******* FILL IN THE CODE *******/


  // Find the {PID} in the PCBTable
  // If the process indicated by the process id is RUNNING, wait for it (can use waitpid()).
  // After the process terminate, update status and exit code (call proc_update_status())
  // Else, continue accepting user commands.
  if (num_tokens != 2 || tokens[1] == NULL) {
    return;
  }
  pid_t pid = (pid_t)atoi(tokens[1]);
  for (size_t i = 0; i < process_count; i++) {
    if (processes[i].pid == pid) {
      if (processes[i].status == RUNNING) {
        proc_update_status(i, WUNTRACED);
      }
    }
  }
}


static void command_terminate(size_t num_tokens, char **tokens) {

  // Find the pid in the PCBTable
  // If {PID} is RUNNING:
  //Terminate it by using kill() to send SIGTERM
  // The state of {PID} should be “Terminating” until {PID} exits
  if (num_tokens != 2 || tokens[1] == NULL) {
    return;
  }
  pid_t pid = (pid_t)atoi(tokens[1]);

  for (size_t i = 0; i < process_count; i++) {
    if (processes[i].pid == pid && processes[i].status == RUNNING) {
      processes[i].status = TERMINATING; //state of PID should be terminating until PID exits
      kill(pid, SIGTERM); //terminate it by using kill to send SIGTERM
    }
  }

}

static void command_fg(size_t num_tokens, char **tokens) {

  // if the {PID} status is stopped
  //Print “[PID] resumed”
  // Use kill() to send SIGCONT to {PID} to get it continue and wait for it
  // After the process terminate, update status and exit code (call proc_update_status())
  if (num_tokens != 2) {
    return;
  }
  assert(tokens[1] != NULL);
  pid_t pid = (pid_t)atoi(tokens[1]);

  for (size_t i = 0; i < process_count; i++) {
    if (processes[i].pid == pid && processes[i].status == STOPPED) {
      printf("[%d] resumed\n", pid);
      if (kill(pid, SIGCONT) == -1) { //failed to send signal
        return;
      } //use kill() to send SIGCONT to PID to get it continue and wait for it
      processes[i].status = RUNNING;
      processes[i].exitCode = -1;
      proc_update_status(i, WUNTRACED);
    }
  }
}


/*******************************************************************************
 * Program Execution
 ******************************************************************************/

static void command_exec(char *command, size_t cmdlen, char **args, bool *cont) {

  /******* FILL IN THE CODE *******/


  // check if program exists and is executable : use access()
  if (access(command,F_OK) == -1 || access(command, X_OK) == -1) { //if file does not exist or is not executable
    fprintf(stderr, "%s not found\n", command);
    return;
  }
  // fork a subprocess and execute the program
  pid_t pid;
  if ((pid = fork()) == 0) {
    // CHILD PROCESS



    // if < or > or 2> present: 
    // use fopen/open file to open the file for reading/writing with  permission O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, O_SYNC and 0644
    // use dup2 to redirect the stdin, stdout and stderr to the files
    // call execv() to execute the command in the child process

    for (size_t i = 0; i < cmdlen; i++) {
      if (args[i] != NULL) {
        if (strcmp(args[i], "<") == 0) {
          int file_desc = open(args[i + 1], O_RDONLY, 0644);
          if (file_desc == -1) {
            fprintf(stderr, "%s does not exist\n", args[i + 1]);
            exit(EXIT_FAILURE);
          }
          dup2(file_desc, STDIN_FILENO);
          close(file_desc);
          args[i++] = NULL;
          args[i] = NULL;
        } else if (strcmp(args[i], ">") == 0) {
          int file_desc = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644); 
          if (file_desc == -1) {
            fprintf(stderr, "%s does not exist\n", args[i + 1]);
            exit(EXIT_FAILURE);
          }
          dup2(file_desc, STDOUT_FILENO);
          close(file_desc);
          args[i++] = NULL;
          args[i] = NULL;
        } else if (strcmp(args[i], "2>") == 0) {
          int file_desc = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644); 
          if (file_desc == -1) {
            fprintf(stderr, "%s does not exist\n", args[i + 1]);
            exit(EXIT_FAILURE);
          }
          dup2(file_desc, STDERR_FILENO);
          close(file_desc);
          args[i++] = NULL;
          args[i] = NULL;
        }
      }
    }
    // call execv() to execute the command in the child process
    execv(command, args);
    // Exit the child
    exit(EXIT_FAILURE);

  } else {


    // PARENT PROCESS
    // register the process in process table 
    int curr = process_count++;
    processes[curr] = (struct PCBTable) {pid, RUNNING, -1};
    // If  child process need to execute in the background  (if & is present at the end )
    //print Child [PID] in background
    if (*cont) {
      printf("Child [%d] in background\n", pid);
      proc_update_status(curr, WNOHANG);
    }
    // else wait for the child process to exit 
    else {
      proc_update_status(curr, WUNTRACED);
    }
    // Use waitpid() with WNOHANG when not blocking during wait and  waitpid() with WUNTRACED when parent needs to block due to wait 
  }
}

/*******************************************************************************
 * Command Processor
 ******************************************************************************/

static void command(size_t i, size_t j, char **tokens) {

  size_t size_args = j - i;
  char *command = tokens[i];
  assert(command != NULL);

  if (strcmp(command, commands[0]) == 0) {
    command_info(size_args, tokens);
  } else if (strcmp(command, commands[1]) == 0) {
    command_wait(size_args, tokens);
  } else if (strcmp(command, commands[2]) == 0) {
    command_terminate(size_args, tokens);
  } else if (strcmp(command, commands[3]) == 0) {
    command_fg(size_args, tokens);
  } else {
    bool cont = false;

    char **cmd_args = get_command_args(i, size_args, tokens);
    if (strcmp(cmd_args[size_args - 1], "&") == 0) {//process continues in bg
      cont = true; //sets flag to true
      cmd_args[size_args - 1] = NULL; //removes ampersand since execv looks for NULL terminator
    }
    command_exec(command, size_args, cmd_args, &cont);
    free(cmd_args);
  }
  
}

/*******************************************************************************
 * High-level Procedure
 ******************************************************************************/

void my_init(void) {
    
  // use signal() with SIGTSTP to setup a signalhandler for ctrl+z : ex4
  signal(SIGTSTP, signal_handler);
  // use signal() with SIGINT to setup a signalhandler for ctrl+c  : ex4
  signal(SIGINT, signal_handler);
  // anything else you require
  process_count = 0; //initialize process count to zero


}


void my_process_command(size_t num_tokens, char **tokens) {


  // Split tokens at NULL or ; to get a single command (ex1, ex2, ex3, ex4(fg command))

  // for example :  /bin/ls ; /bin/sleep 5 ; /bin/pwd
  // split the above line as first command : /bin/ls , second command: /bin/pwd  and third command:  /bin/pwd
  // Call command() and pass each individual command as arguements
  for (size_t i = 0; i < process_count; i++) {
    proc_update_status(i, WNOHANG);
  }
  size_t i = 0;
  for (size_t j = 0; j < num_tokens; j++) {
    if (tokens[j] == NULL || strcmp(tokens[j], ";") == 0) {
      command(i, j, tokens);
      i = j + 1;
    }
  }
}

void my_quit(void) {

  // Kill every process in the PCB that is either stopped or running
  for (size_t i = 0; i < process_count; i++) {
    if (processes[i].status == RUNNING || processes[i].status == STOPPED) {
      pid_t tmp = processes[i].pid;
      kill(processes[i].pid, SIGTERM);
      printf("Killing [%d]\n", tmp);
    }
  }
  printf("\nGoodbye\n");
}
