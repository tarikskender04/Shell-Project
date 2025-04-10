#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE  1024
#define MAX_ARGS  64
#define MAX_NOTES 100

// these are ANSI color macros that define specific colors, this was the easiest approach
#define ANSI_RESET      "\x1b[0m"
#define ANSI_RED        "\x1b[31m"
#define ANSI_GREEN      "\x1b[32m"
#define ANSI_YELLOW     "\x1b[33m"
#define ANSI_BLUE       "\x1b[34m"
#define ANSI_MAGENTA    "\x1b[35m"
#define ANSI_CYAN       "\x1b[36m"
#define ANSI_BOLD       "\x1b[1m"

static char notes[MAX_NOTES][MAX_LINE];
static int note_count = 0;

static const char *SHELL_NAME = ANSI_BOLD ANSI_CYAN "RainbowShell" ANSI_RESET;

// Built-in command: mynotes
//  -a "some text" -> add note (prints in green for success, magenta for note content)
//  -l -> list notes
void builtin_mynotes(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "%sUsage:%s mynotes -a \"text\" or mynotes -l\n", ANSI_RED, ANSI_RESET);
        return;
    }

    if (strcmp(args[1], "-a") == 0) {
        if (args[2] == NULL) {
            fprintf(stderr, "%sNo text provided.%s Usage: mynotes -a \"some text\"\n", ANSI_RED, ANSI_RESET);
            return;
        }
        if (note_count < MAX_NOTES) {
            // Combine args[2..] into one string
            char buffer[MAX_LINE] = "";
            for (int i = 2; args[i] != NULL; i++) {
                strcat(buffer, args[i]);
                if (args[i+1] != NULL) strcat(buffer, " ");
            }
            strncpy(notes[note_count], buffer, MAX_LINE);
            note_count++;
            printf("%sNote added!%s\n", ANSI_GREEN, ANSI_RESET);
        } else {
            fprintf(stderr, "%sNote storage is full!%s\n", ANSI_RED, ANSI_RESET);
        }
    }
    else if (strcmp(args[1], "-l") == 0) {
        if (note_count == 0) {
            printf("%sNo notes.%s\n", ANSI_YELLOW, ANSI_RESET);
        } else {
            printf("%sListing notes:%s\n", ANSI_BOLD, ANSI_RESET);
            for (int i = 0; i < note_count; i++) {
                // Print each note in magenta
                printf("%d) %s%s%s\n", i, ANSI_MAGENTA, notes[i], ANSI_RESET);
            }
        }
    }
    else {
        fprintf(stderr, "%sUnknown option %s.%s Usage: mynotes -a \"text\" or mynotes -l\n",
                ANSI_RED, args[1], ANSI_RESET);
    }
}

// Built-in command: go
// "go web", "go os", "go ds", "go prog" 
//  we made that each folder should be printed in a different color to show user feedback

void builtin_go(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "%sUsage:%s go <destination>\n", ANSI_RED, ANSI_RESET);
        return;
    }
    const char *path_web  = "/Users/skendimac/Desktop/faks/Web";
    const char *path_os   = "/Users/skendimac/Desktop/faks/Operating Systems";
    const char *path_ds   = "/Users/skendimac/Desktop/faks/data structures";
    const char *path_prog = "/Users/skendimac/Desktop/faks/Programming";

    // For color-coded feedback
    if (strcmp(args[1], "web") == 0) {
        if (chdir(path_web) == 0) {
            printf("%sChanged directory to:%s %s%s%s\n", ANSI_GREEN, ANSI_RESET, ANSI_BLUE, path_web, ANSI_RESET);
        } else {
            perror("chdir web");
        }
    } 
    else if (strcmp(args[1], "os") == 0) {
        if (chdir(path_os) == 0) {
            printf("%sChanged directory to:%s %s%s%s\n", ANSI_GREEN, ANSI_RESET, ANSI_RED, path_os, ANSI_RESET);
        } else {
            perror("chdir os");
        }
    }
    else if (strcmp(args[1], "ds") == 0) {
        if (chdir(path_ds) == 0) {
            printf("%sChanged directory to:%s %s%s%s\n", ANSI_GREEN, ANSI_RESET, ANSI_YELLOW, path_ds, ANSI_RESET);
        } else {
            perror("chdir ds");
        }
    }
    else if (strcmp(args[1], "prog") == 0) {
        if (chdir(path_prog) == 0) {
            printf("%sChanged directory to:%s %s%s%s\n", ANSI_GREEN, ANSI_RESET, ANSI_CYAN, path_prog, ANSI_RESET);
        } else {
            perror("chdir prog");
        }
    } 
    else {
        fprintf(stderr, "%sUnknown destination:%s %s\n", ANSI_RED, ANSI_RESET, args[1]);
    }
}

// Print a colored prompt: "RainbowShell$ "
void print_colored_prompt() {
    printf("%s$ %s", SHELL_NAME, ANSI_RESET);
    fflush(stdout);
}

int main() {
    char line[MAX_LINE];
    char *args[MAX_ARGS];

    while (1) {
        print_colored_prompt();

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        // Strip newline
        line[strcspn(line, "\n")] = '\0';

        // Tokenize input
        int i = 0;
        args[i] = strtok(line, " ");
        while (args[i] && i < MAX_ARGS - 1) {
            i++;
            args[i] = strtok(NULL, " ");
        }
        if (!args[0]) continue;

        // "exit" built-in
        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        // "mynotes" built-in
        if (strcmp(args[0], "mynotes") == 0) {
            builtin_mynotes(args);
            continue;
        }

        // "go" built-in
        if (strcmp(args[0], "go") == 0) {
            builtin_go(args);
            continue;
        }

        // External command fallback
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
        } else if (pid == 0) {
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            wait(NULL);
        }
    }

    return 0;
}
