#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <signal.h>

char **cmdArgs;
int numArgs;
int numProcesses = 0;
int array[20];

struct Process {
    int id;
    int procArgs;
    pid_t pid;
    pid_t parentPid;
    char status[10];
    char name[50];
    char* args[20];
};

struct Process * Processes = NULL;

void removeProcess(pid_t pidToRemove) {
    int indexToRemove = -1;

    // Find the process with the given PID.
    for (int i = 0; i < numProcesses; i++) {
        if (Processes[i].pid == pidToRemove) {
            indexToRemove = i;
            break;
        }
    }

    // If the process was found, remove it.
    if (indexToRemove != -1) {
        array[Processes[indexToRemove].id - 1] = 0;

        // Shift all subsequent processes down.
        memmove(&Processes[indexToRemove], &Processes[indexToRemove + 1], 
        (numProcesses - indexToRemove - 1) * sizeof(struct Process));
        numProcesses--;

        // Reallocate the Processes array to the new size.
        Processes = (struct Process*)realloc(Processes, numProcesses * sizeof(struct Process));
    }
}


void child_handler(int sig) {
    if (sig == SIGCHLD) {
        int status;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            if (WIFEXITED(status)) {
                removeProcess(pid);
            }
        }
    }
}


void sigtstp_handler(int sig) {
    for (int i = 0; i < numProcesses; i++) {
        if (strcmp(Processes[i].status, "F") == 0) {
            strcpy(Processes[i].status, "S");
            killpg(getpgid(Processes[i].pid), SIGTSTP);
            break;
        }
    }
}

void sigint_handler(int sig) {
    for (int i = 0; i < numProcesses; i++) {
        if (strcmp(Processes[i].status, "F") == 0) {
            killpg(getpgid(Processes[i].pid), SIGINT);
        }
    }
}

void wshExit() {
    free(cmdArgs);
    exit(0);
}

void wshCd() {
    if (numArgs != 2) {
        printf("cd failed\n");
    } else {
        chdir(cmdArgs[1]);
    }
}

void wshJob() {
    for (int i = 0; i < numProcesses; i++) {
        char buffer[256]; // Buffer to construct the string representation of the process

        // Start with the process ID and its name
        sprintf(buffer, "%d: %s", Processes[i].id, Processes[i].name);

        // Add any arguments passed to the process to our buffer
        for (int j = 0; j < Processes[i].procArgs; j++) {
            strcat(buffer, " "); // Separate each argument with a space
            if (strcmp(Processes[i].args[j], "&") != 0) {
                strcat(buffer, Processes[i].args[j]);
            }
        }

        printf("%s\n", buffer);
    }
}

void wshFg() {
    // Determine the target process ID, or set to -1 if not specified
    int targetId = (numArgs == 2) ? atoi(cmdArgs[1]) : -1; 

    // Set loop boundaries based on whether a specific process ID was given
    int startIdx = (targetId == -1) ? numProcesses - 1 : 0;
    int endIdx = (targetId == -1) ? -1 : numProcesses;
    int increment = (targetId == -1) ? -1 : 1;

    // Loop through processes and bring the desired one to the foreground
    for (int i = startIdx; i != endIdx; i += increment) {
        if ((targetId == -1 || Processes[i].id == targetId) && strcmp(Processes[i].status, "F") != 0) {
            strcpy(Processes[i].status, "F");
            killpg(getpgid(Processes[i].pid), SIGCONT);
            waitpid(Processes[i].pid, NULL, WUNTRACED);
            break;
        }
    }
}

void wshBg() {
    // Determine the target process ID, or set to -1 if not specified
    int targetId = (numArgs == 2) ? atoi(cmdArgs[1]) : -1; 

    // Set loop boundaries based on whether a specific process ID was given
    int startIdx = (targetId == -1) ? numProcesses - 1 : 0;
    int endIdx = (targetId == -1) ? -1 : numProcesses;
    int increment = (targetId == -1) ? -1 : 1;

    // Loop through processes and send the desired one to the background
    for (int i = startIdx; i != endIdx; i += increment) {
        if ((targetId == -1 || Processes[i].id == targetId) && strcmp(Processes[i].status, "S") == 0) {
            strcpy(Processes[i].status, "B");
            killpg(getpgid(Processes[i].pid), SIGCONT);
            tcsetpgrp(STDIN_FILENO, getpgid(Processes[i].parentPid));
            break;
        }
    }
}

int exec() {
    numProcesses++;
    // Allocate memory for the new process and check for allocation errors
    struct Process* currBgProcess = malloc(sizeof(struct Process));
if (!currBgProcess) {
    perror("Error allocating memory for new process");
    exit(1);
}

// Default initialization for the ID
currBgProcess->id = numProcesses;

// Search for available ID
bool idFound = false;
for (int j = 0; j < 20 && !idFound; j++) {
    if (array[j] == 0) {
        currBgProcess->id = j + 1;
        array[j] = j + 1;
        idFound = true;
    }
}

// Copy the name of the process from command arguments
strcpy(currBgProcess->name, cmdArgs[0]);

// Set the parent process ID
currBgProcess->parentPid = getpid();

// Assign command arguments to the process structure, limited by 10 arguments
for (int i = 1; i < numArgs && i <= 10; i++) {
    currBgProcess->args[i - 1] = cmdArgs[i];
}
currBgProcess->procArgs = (numArgs - 1 < 10) ? numArgs - 1 : 10; 

// Set the process status based on the last command argument
strcpy(currBgProcess->status, (strcmp(cmdArgs[numArgs - 1], "&") != 0) ? "F" : "B");

Processes = realloc(Processes, numProcesses * sizeof(struct Process));
int rc = fork();
if (rc < 0) {
    perror("fork failed");
    exit(1);
}

if (rc == 0) {
    setpgid(0, 0);
    
    // Determine the number of arguments based on background/foreground command
    int argsCount = (strcmp(cmdArgs[numArgs - 1], "&") == 0) ? numArgs - 1 : numArgs;

    char *myargs[argsCount + 1];
    for (int i = 0; i < argsCount; i++) {
        myargs[i] = cmdArgs[i];
    }
    myargs[argsCount] = NULL;
    
    execvp(myargs[0], myargs);
    perror(myargs[0]);  // If execvp returns, there was an error
    return 0;
}
else {
    currBgProcess->pid = rc;
    Processes[numProcesses - 1] = *currBgProcess;
    free(currBgProcess);

    if (strcmp(cmdArgs[numArgs - 1], "&") != 0) {
        waitpid(rc, NULL, WUNTRACED);
        if (strcmp(Processes[numProcesses - 1].status, "F") == 0) {
             removeProcess(rc);
         }
    } 
    signal(SIGCHLD, child_handler);
    }
    return 0;
}

int getcmd() {
    char *buf = NULL;
    size_t n = 0;
    printf("wsh> ");
    ssize_t read = getline(&buf, &n, stdin);
    if (read != -1) {
        buf[read - 1] = '\0';
    }
    char *token;
    char *temp = buf; 
    // Allocate memory for storing command arguments
    cmdArgs = malloc(sizeof(char*));
    numArgs = 0;

    // Tokenize the buffer to extract command arguments, and store them in cmdArgs
    while ((token = strsep(&temp, " ")) != NULL) {
        cmdArgs[numArgs] = malloc(strlen(token) + 1);
        strcpy(cmdArgs[numArgs], token);
        numArgs++;
        cmdArgs = realloc(cmdArgs, (numArgs + 1) * sizeof(char*));
    }

    if (strcmp(buf, "exit") == 0) {
        wshExit();
    } else if (strcmp(cmdArgs[0], "cd") == 0) {
        wshCd();
    } else if (strcmp(buf, "jobs") == 0) {
        wshJob();
    } else if (strcmp(cmdArgs[0], "fg") == 0) {
        wshFg();
    } else if (strcmp(cmdArgs[0], "bg") == 0) {
        wshBg();
    } else {
        exec();
    }

    free(buf);
    return 0;
}

int main(int argc, char* argv[]) {
    
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGINT, sigint_handler);

    while (1) {
        getcmd();
    }

    return 0;
}


