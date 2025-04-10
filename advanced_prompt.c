#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/utsname.h>


// ------------------- CONSTANTS -------------------
#define MAX_INPUT   1024
#define MAX_ARGS    64
#define MAX_HISTORY 50
#define MAX_NOTES   100

// ------------------- COLOR MACROS -------------------
#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_BOLD    "\x1b[1m"

// You had ANSI_* macros; let's unify them with the friend's usage:
#define ANSI_RESET      COLOR_RESET
#define ANSI_RED        COLOR_RED
#define ANSI_GREEN      COLOR_GREEN
#define ANSI_YELLOW     COLOR_YELLOW
#define ANSI_BLUE       COLOR_BLUE
#define ANSI_MAGENTA    COLOR_MAGENTA
#define ANSI_CYAN       COLOR_CYAN
#define ANSI_BOLD       COLOR_BOLD

// ------------------- HISTORY STORAGE (friend's) -------------------
static char *history[MAX_HISTORY];
static int history_count = 0;

// ------------------- MYNOTES STORAGE (your code) -------------------
static char notes[MAX_NOTES][MAX_INPUT];
static int note_count = 0;

// ------------------- PATHS FOR "go" COMMAND -------------------
static const char *path_web  = "/Users/skendimac/Desktop/faks/Web";
static const char *path_os   = "/Users/skendimac/Desktop/faks/Operating Systems";
static const char *path_ds   = "/Users/skendimac/Desktop/faks/data structures";
static const char *path_prog = "/Users/skendimac/Desktop/faks/Programming";

// ------------------- FORWARD DECLARATIONS -------------------
void add_to_history(const char *command);
void execute_history();
void execute_free();      
void execute_fortune(char **args, int arg_count);
void execute_cp(char **args, int arg_count);
void execute_forkbomb();

void builtin_mynotes(char **args);
void builtin_go(char **args);

int handle_builtins(char **args, int arg_count);
int parse_input(char *input, char **args);
void execute_command(char **args, int arg_count);

void print_welcome();
void print_goodbye();

void add_to_history(const char *command) {
    // Simple ring buffer for history
    if (history_count < MAX_HISTORY) {
        history[history_count] = strdup(command);
        history_count++;
    } else {
        free(history[0]);
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            history[i] = history[i + 1];
        }
        history[MAX_HISTORY - 1] = strdup(command);
    }
}

void execute_history() {
    printf(COLOR_CYAN "Command History:\n" COLOR_RESET);
    for (int i = 0; i < history_count; i++) {
        printf("%3d: %s\n", i + 1, history[i]);
    }
}

// If you're on Linux:
#ifdef __linux__
#include <sys/sysinfo.h>
#endif

void execute_free() {
#ifdef __linux__
    struct sysinfo info;
    if (sysinfo(&info)) {
        perror("free");
        return;
    }
    printf(COLOR_BLUE "Memory Info:\n" COLOR_RESET);
    printf("Total RAM: %.2f MB\n", (double)info.totalram / (1024 * 1024));
    printf("Free RAM:  %.2f MB\n", (double)info.freeram / (1024 * 1024));
    printf("Processes: %d\n", info.procs);
#else
    fprintf(stderr, COLOR_RED "execute_free() not supported on this OS.\n" COLOR_RESET);
#endif
}

void execute_fortune(char **args, int arg_count) {
    const char *fortunes[] = {
        COLOR_YELLOW "Tarik and Bekir says: You will have a great day today!" COLOR_RESET,
        COLOR_YELLOW "Tarik and Bekir says: The best way to predict the future is to invent it." COLOR_RESET,
        COLOR_YELLOW "Tarik and Bekir says: Give us 15 points :)" COLOR_RESET,
        COLOR_YELLOW "Tarik and Bekir says: A wise shell listens to its user." COLOR_RESET,
        COLOR_YELLOW "Tarik and Bekir says: We love OS" COLOR_RESET
    };
    int fortune_count = sizeof(fortunes)/sizeof(fortunes[0]);
    
    int list = 0, number = -1;
    
    // Parse options
    for (int i = 1; i < arg_count; i++) {
        if (strcmp(args[i], "-l") == 0) {
            list = 1;
        }
        else if (strcmp(args[i], "-n") == 0 && i+1 < arg_count) {
            number = atoi(args[++i]);
        }
    }

    if (list) {
        printf(COLOR_CYAN "Available fortunes:\n" COLOR_RESET);
        for (int i = 0; i < fortune_count; i++) {
            printf("%d: %s\n", i+1, fortunes[i]);
        }
        return;
    }

    int index;
    if (number > 0 && number <= fortune_count) {
        index = number - 1;
    } else {
        srand((unsigned int)time(NULL));
        index = rand() % fortune_count;
    }

    printf("%s\n", fortunes[index]);
}

void execute_cp(char **args, int arg_count) {
    int verbose = 0, interactive = 0, no_clobber = 0;
    char *src = NULL, *dest = NULL;
    
    // parse flags
    for (int i = 1; i < arg_count; i++) {
        if (strcmp(args[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(args[i], "-i") == 0) {
            interactive = 1;
        } else if (strcmp(args[i], "-n") == 0) {
            no_clobber = 1;
        } else if (!src) {
            src = args[i];
        } else if (!dest) {
            dest = args[i];
        }
    }

    if (!src || !dest) {
        printf(COLOR_RED "Usage: cp [-v] [-i] [-n] <source> <destination>\n" COLOR_RESET);
        return;
    }

    if (no_clobber && access(dest, F_OK) == 0) {
        if (verbose) printf("'%s' already exists (no-clobber)\n", dest);
        return;
    }

    if (interactive && access(dest, F_OK) == 0) {
        printf(COLOR_YELLOW "Overwrite '%s'? (y/n): " COLOR_RESET, dest);
        int response = getchar();
        while (getchar() != '\n'); // clear buffer
        if (response != 'y' && response != 'Y') {
            if (verbose) printf("Not overwritten\n");
            return;
        }
    }

    FILE *source = fopen(src, "rb");
    if (!source) {
        perror(COLOR_RED "cp: source file" COLOR_RESET);
        return;
    }

    FILE *destination = fopen(dest, "wb");
    if (!destination) {
        perror(COLOR_RED "cp: destination file" COLOR_RESET);
        fclose(source);
        return;
    }

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytes, destination);
    }

    fclose(source);
    fclose(destination);
    if (verbose) {
        printf(COLOR_GREEN "'%s' -> '%s'\n" COLOR_RESET, src, dest);
    }
}

void execute_forkbomb() {
    printf(COLOR_RED "Tarik Fork Bomb Activated!\n" COLOR_RESET);
    printf(COLOR_YELLOW "Warning: This will create processes exponentially!\n" COLOR_RESET);
    printf("Press Ctrl+C to stop it...\n");
    sleep(2);
    
    while(1) {
        fork();
        printf(COLOR_MAGENTA "Tarik fork bomb: PID %d created!\n" COLOR_RESET, getpid());
    }
}


// "mynotes" built-in
void builtin_mynotes(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "%sUsage:%s mynotes -a \"text\" or mynotes -l\n", COLOR_RED, COLOR_RESET);
        return;
    }

    // handle -a or -l
    if (strcmp(args[1], "-a") == 0) {
        if (args[2] == NULL) {
            fprintf(stderr, "%sNo text provided.%s Usage: mynotes -a \"some text\"\n", COLOR_RED, COLOR_RESET);
            return;
        }
        if (note_count < MAX_NOTES) {
            char buffer[MAX_INPUT] = "";
            for (int i = 2; args[i] != NULL; i++) {
                strcat(buffer, args[i]);
                if (args[i+1]) strcat(buffer, " ");
            }
            strncpy(notes[note_count], buffer, MAX_INPUT);
            note_count++;
            printf("%sNote added!%s\n", COLOR_GREEN, COLOR_RESET);
        } else {
            fprintf(stderr, "%sNote storage is full!%s\n", COLOR_RED, COLOR_RESET);
        }
    }
    else if (strcmp(args[1], "-l") == 0) {
        if (note_count == 0) {
            printf("%sNo notes.%s\n", COLOR_YELLOW, COLOR_RESET);
        } else {
            printf("%sListing notes:%s\n", COLOR_BOLD, COLOR_RESET);
            for (int i = 0; i < note_count; i++) {
                printf("%d) %s%s%s\n", i, COLOR_MAGENTA, notes[i], COLOR_RESET);
            }
        }
    }
    else {
        fprintf(stderr, "%sUnknown option %s.%s Usage: mynotes -a \"text\" or mynotes -l\n",
                COLOR_RED, args[1], COLOR_RESET);
    }
}

// "go" built-in
void builtin_go(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "%sUsage:%s go <destination>\n", COLOR_RED, COLOR_RESET);
        return;
    }

    if (strcmp(args[1], "web") == 0) {
        if (chdir(path_web) == 0) {
            printf("%sChanged directory to:%s %s%s%s\n", COLOR_GREEN, COLOR_RESET, COLOR_BLUE, path_web, COLOR_RESET);
        } else {
            perror("chdir web");
        }
    } 
    else if (strcmp(args[1], "os") == 0) {
        if (chdir(path_os) == 0) {
            printf("%sChanged directory to:%s %s%s%s\n", COLOR_GREEN, COLOR_RESET, COLOR_RED, path_os, COLOR_RESET);
        } else {
            perror("chdir os");
        }
    }
    else if (strcmp(args[1], "ds") == 0) {
        if (chdir(path_ds) == 0) {
            printf("%sChanged directory to:%s %s%s%s\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, path_ds, COLOR_RESET);
        } else {
            perror("chdir ds");
        }
    }
    else if (strcmp(args[1], "prog") == 0) {
        if (chdir(path_prog) == 0) {
            printf("%sChanged directory to:%s %s%s%s\n", COLOR_GREEN, COLOR_RESET, COLOR_CYAN, path_prog, COLOR_RESET);
        } else {
            perror("chdir prog");
        }
    } 
    else {
        fprintf(stderr, "%sUnknown destination:%s %s\n", COLOR_RED, COLOR_RESET, args[1]);
    }
}

int handle_builtins(char **args, int arg_count) {
    if (arg_count == 0) return 1; 
    {
        char joined[1024];
        joined[0] = '\0';
        for (int i = 0; i < arg_count; i++) {
            strcat(joined, args[i]);
            if (i < arg_count - 1) strcat(joined, " ");
        }
        add_to_history(joined);
    }

    if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0) {
        print_goodbye();
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(args[0], "cd") == 0) {
        if (arg_count < 2) {
            chdir(getenv("HOME"));
        } else if (chdir(args[1])) {
            perror(COLOR_RED "cd failed" COLOR_RESET);
        }
        return 1;
    }
    else if (strcmp(args[0], "history") == 0) {
        execute_history();
        return 1;
    }
    else if (strcmp(args[0], "free") == 0) {
        execute_free();
        return 1;
    }
    else if (strcmp(args[0], "fortune") == 0) {
        execute_fortune(args, arg_count);
        return 1;
    }
    else if (strcmp(args[0], "cp") == 0) {
        execute_cp(args, arg_count);
        return 1;
    }
    else if (strcmp(args[0], "forkbomb") == 0) {
        execute_forkbomb();
        return 1;
    }
    else if (strcmp(args[0], "mynotes") == 0) {
        builtin_mynotes(args);
        return 1;
    }
    else if (strcmp(args[0], "go") == 0) {
        builtin_go(args);
        return 1;
    }

    return 0;
}

// -------------- parse_input + command execution --------------
int parse_input(char *input, char **args) {
    int arg_count = 0;
    char *token = strtok(input, " \t\n");
    while (token != NULL && arg_count < MAX_ARGS - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[arg_count] = NULL;
    return arg_count;
}

void execute_command(char **args, int arg_count) {
    // Check for redirection (>)
    int redirect_output = 0;
    char *output_file = NULL;
    
    for (int i = 0; i < arg_count; i++) {
        if (strcmp(args[i], ">") == 0 && i+1 < arg_count) {
            redirect_output = 1;
            output_file = args[i+1];
            args[i] = NULL;
            break;
        }
    }

    // Check for pipe (|)
    int pipe_pos = -1;
    for (int i = 0; i < arg_count; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipe_pos = i;
            break;
        }
    }

    // If we have a pipe
    if (pipe_pos != -1) {
        args[pipe_pos] = NULL;
        char **cmd1_args = args;
        char **cmd2_args = &args[pipe_pos + 1];
        
        int pipefd[2];
        if (pipe(pipefd) < 0) {
            perror(COLOR_RED "pipe failed" COLOR_RESET);
            return;
        }
        
        pid_t pid1 = fork();
        if (pid1 == 0) {
            // child1
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            execvp(cmd1_args[0], cmd1_args);
            perror(COLOR_RED "execvp failed" COLOR_RESET);
            exit(EXIT_FAILURE);
        }
        pid_t pid2 = fork();
        if (pid2 == 0) {
            // child2
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            execvp(cmd2_args[0], cmd2_args);
            perror(COLOR_RED "execvp failed" COLOR_RESET);
            exit(EXIT_FAILURE);
        }
        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
        return;
    }

    // Normal fork/exec
    pid_t pid = fork();
    if (pid < 0) {
        perror(COLOR_RED "fork failed" COLOR_RESET);
        return;
    }
    if (pid == 0) {
        // Child
        if (redirect_output && output_file) {
            int fd = creat(output_file, 0644);
            if (fd < 0) {
                perror(COLOR_RED "open" COLOR_RESET);
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        execvp(args[0], args);
        perror(COLOR_RED "execvp failed" COLOR_RESET);
        exit(EXIT_FAILURE);
    } else {
        // Parent
        waitpid(pid, NULL, 0);
    }
}

// -------------- WELCOME & GOODBYE --------------
void print_welcome() {
    printf(COLOR_MAGENTA "\n"
           "╔════════════════════════════════════════╗\n"
           "║                                        ║\n"
           "║       Welcome to BekirTarik Shell      ║\n"
           "║                                        ║\n"
           "╚════════════════════════════════════════╝\n" COLOR_RESET);
    printf(COLOR_CYAN "Type 'help' for available commands\n\n" COLOR_RESET);
}

void print_goodbye() {
    printf(COLOR_MAGENTA "\n"
           "╔════════════════════════════════════════╗\n"
           "║                                        ║\n"
           "║    Thank you for using Our shell!      ║\n"
           "║                                        ║\n"
           "╚════════════════════════════════════════╝\n\n" COLOR_RESET);
}

// -------------- MAIN FUNCTION --------------
int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    
    print_welcome();

    while (1) {
        struct utsname uts;
        uname(&uts);
        struct passwd *pw = getpwuid(getuid());
        printf(COLOR_GREEN "%s" COLOR_RESET "@" COLOR_BLUE "%s" COLOR_RESET ":" COLOR_MAGENTA "~" COLOR_RESET "$ ",
               uts.nodename, pw->pw_name);
        fflush(stdout);

        if (!fgets(input, MAX_INPUT, stdin)) {
            printf("\n");
            break;
        }

        // parse input
        int arg_count = parse_input(input, args);
        if (arg_count == 0) continue;

        // check built-ins
        if (!handle_builtins(args, arg_count)) {
            execute_command(args, arg_count);
        }
    }

    // free allocated history strings
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }

    return 0;
}
