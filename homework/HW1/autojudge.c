#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>

#define MAX_TESTS 20
#define MAX_FILE_NAME 512
#define MAX_BUF_SIZE 4096

// Function prototypes
void print_usage(); // Print usage instructions
void run(char *targetsrc, char *inputfile, char *answerfile, int timelimit); // Compile and run the targetsrc program
int compareFiles(char *file1, char *file2); // Compare contents of two files
void printResult() ; // Print all result

// Global variables
int correctCnt = 0;
int wrongCnt = 0;
int timeoutCnt = 0;
int runtimeCnt = 0;
char *wrongArr[MAX_TESTS];
char *timeoutArr[MAX_TESTS];
char *runtimeArr[MAX_TESTS]; 
char *correctArr[MAX_TESTS];

// Global variable to track timeout
volatile sig_atomic_t timeout_flag = 0;

// Signal handler for SIGALRM
void handle_alarm(int signum) {
    timeout_flag = 1;
}

int main(int argc, char *argv[]) {
    char *inputdir = NULL;
    char *answerdir = NULL;
    char *targetsrc = NULL;
    int opt;
    int timelimit = 0;
    int fileCount = 0;

    // Parse command line arguments
    while ((opt = getopt(argc, argv, "i:a:t:")) != -1) {
        switch (opt) {
            case 'i':
                inputdir = optarg;
                break;
            case 'a':
                answerdir = optarg;
                break;
            case 't':
                timelimit = atoi(optarg);
                break;
            default:
                print_usage(); // Print usage instructions
                return 1;
        }
    }

    if (inputdir == NULL || answerdir == NULL || timelimit <= 0 || optind >= argc) {
        print_usage(); // Print usage instructions
        return 1;
    }

    targetsrc = argv[optind];

    // Set up SIGALRM signal handler
    struct sigaction sa;
    sa.sa_handler = handle_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    // Compile the targetsrc program
    if (fork() == 0) {
        execlp("gcc", "gcc", "-fsanitize=address", targetsrc, "-o", "targetsrc_exec", NULL);
        exit(0);
    } else {
        wait(NULL);
    }

    // Check if compilation was successful
    if (access("targetsrc_exec", X_OK) != 0) {
        perror("Compile Error"); // Print compilation error
        return 1;
    }

    // Open input directory
    DIR *input_dp = opendir(inputdir);
    if (input_dp == NULL) {
        printf("Failed to open input directory\n"); // Print error opening input directory
        return 1;
    }

    // Open answer directory
    DIR *answer_dp = opendir(answerdir);
    if (answer_dp == NULL) {
        printf("Failed to open answer directory\n"); // Print error opening answer directory
        return 1;
    }

    struct dirent *input_entry;
    struct dirent *answer_entry;

    // Traverse input directory
    while ((input_entry = readdir(input_dp)) != NULL && fileCount < MAX_TESTS ) {
        // Regular file and not .DS_Store
        if (input_entry->d_type == DT_REG && strcmp(input_entry->d_name, ".DS_Store") != 0) { 
            char input_file[MAX_FILE_NAME];
            char answer_file[MAX_FILE_NAME];
            sprintf(input_file, "%s/%s", inputdir, input_entry->d_name);

            // Find corresponding answer file
            char *dot = strrchr(input_entry->d_name, '.');
            if (dot != NULL) {
                *dot = '\0'; // Remove extension
            }
            sprintf(answer_file, "%s/%s.txt", answerdir, input_entry->d_name);
        
            // Run and compare
            run("targetsrc_exec", input_file, answer_file, timelimit); 

            fileCount++;
        }
    }


    closedir(input_dp);
    closedir(answer_dp);

    printResult();
    return 0;
}

void print_usage() {
    printf("Usage: ./autojudge -i <inputdir> -a <answerdir> -t <timelimit> <targetsrc>\n");
}

void run(char *targetsrc, char *inputfile, char *answerfile, int timelimit) {
    // Run the targetsrc program
    struct timeval start, end;
    char outputfile[MAX_FILE_NAME];

    // Construct output file path in the current directory
    char *filename = strrchr(inputfile, '/');
    if (filename != NULL) {
        filename++; // Extract filename only
    } else {
        filename = inputfile; // Use as is if no path
    }
    snprintf(outputfile, sizeof(outputfile), "%s_output.txt", filename);

    // Create the output file
    FILE *output_fp = fopen(outputfile, "w");
    if (output_fp == NULL) {
        printf("Failed to create output file %s\n", outputfile);
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fclose(output_fp); // Close the file immediately after creation

    // Create pipes for capturing input and output
    int inputPipe[2];
    int outputPipe[2];
    if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Execute the targetsrc program
    int pid = fork();
    if (pid == 0) { // child process
        // Close read end of the input pipe in the child process
        close(inputPipe[1]);
        // Close write end of the output pipe in the child process
        close(outputPipe[0]);

        // Redirect stdin to the read end of the input pipe
        if (dup2(inputPipe[0], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        // Redirect stdout and stderr to the write end of the output pipe
        if (dup2(outputPipe[1], STDOUT_FILENO) == -1 || dup2(outputPipe[1], STDERR_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // Close unused file descriptors
        close(inputPipe[0]);
        close(outputPipe[1]);

        // Execute the targetsrc program
        if (execl("./targetsrc_exec", "targetsrc_exec", NULL) == -1) {
            perror("execl");
            exit(EXIT_FAILURE); // Failed to execute the targetsrc program
        }
    } else if (pid > 0) {   // parent process
        // Close read end of the input pipe in the parent process
        close(inputPipe[0]);
        // Close write end of the output pipe in the parent process
        close(outputPipe[1]);

        // Set up an alarm for timeout
        alarm(timelimit);

        // Record the start time
        gettimeofday(&start, NULL);

        // Write input file content to the input pipe
        FILE *input_fp = fopen(inputfile, "r");
        if (input_fp == NULL) {
            printf("Failed to open input file %s\n", inputfile);
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        char buffer[MAX_BUF_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_fp)) > 0) {
            if (write(inputPipe[1], buffer, bytes_read) == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }
        fclose(input_fp);

        // Close write end of the input pipe
        close(inputPipe[1]);

        // Wait for the child process to finish
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            if (errno == EINTR) {
                printf("%s: Timeout (Limited execution time: %d s)\n", inputfile, timelimit);
                timeoutCnt++;
                timeoutArr[timeoutCnt - 1] = strdup(inputfile);
            }
            return;
        }

        // Calculate the execution time
        gettimeofday(&end, NULL);
        double execution_time = ((end.tv_sec - start.tv_sec) * 1000.0) + ((end.tv_usec - start.tv_usec) / 1000.0);

        // Check if a runtime error occurred
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            printf("%s: Runtime Error\n", inputfile);
            runtimeCnt++;
            runtimeArr[runtimeCnt - 1] = strdup(inputfile);
            // Read error message from the output pipe and write to the output file
            FILE *output_fp = fopen(outputfile, "w");
            if (output_fp == NULL) {
                printf("Failed to create output file %s\n", outputfile);
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            while ((bytes_read = read(outputPipe[0], buffer, sizeof(buffer))) > 0) {
                fwrite(buffer, 1, bytes_read, output_fp);
            }
            fclose(output_fp);
            
            return;
        }

        // Open the output file in write mode
        output_fp = fopen(outputfile, "w");
        if (output_fp == NULL) {
            printf("Failed to open output file %s\n", outputfile);
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        // Read output from the output pipe and write to the new output file
        while ((bytes_read = read(outputPipe[0], buffer, sizeof(buffer))) > 0) {
            fwrite(buffer, 1, bytes_read, output_fp);
        }
        fclose(output_fp);

        // Close read end of the output pipe
        close(outputPipe[0]);

        // Compare the output with the answer
        if (compareFiles(outputfile, answerfile) == 0) {
            printf("%s: Correct (Execution Time: %.2f ms)\n", inputfile, execution_time);
            correctCnt++;
            correctArr[correctCnt - 1] = strdup(inputfile);
        } else {
            printf("%s: Wrong Answer (Execution Time: %.2f ms)\n", inputfile, execution_time);
            wrongCnt++;
            wrongArr[wrongCnt - 1] = strdup(inputfile);
        }
    } else { // Failed to fork
        perror("fork");
        exit(EXIT_FAILURE); 
    }
}


int compareFiles(char *file1, char *file2) {
    FILE *f1 = fopen(file1, "r");
    FILE *f2 = fopen(file2, "r");
    int state1, state2;
    char a, b;

    if (f1 == NULL || f2 == NULL) {
        return 1; // Error opening files
    }

    while (1) {
        // Both files haven't reached end
        if (feof(f1) == 0 && feof(f2) == 0) {
            a = fgetc(f1);
            b = fgetc(f2);

            if (a != b) {   // Mismatch
                return 1;
            }
        }

        // Only one file reached end
        else if (feof(f1) != 0 && feof(f2) == 0) {
            return 1;
        }

        // Only one file reached end
        else if (feof(f1) == 0 && feof(f2) != 0) {
            return 1;
        }

        // Both files reached end.
        else {
            // Match
            return 0;
        }
    }

    // fclose returns non-zero on error
    state1 = fclose(f1);
    state2 = fclose(f2);
    if (state1 != 0 || state2 != 0) {
        printf("Error occurred while closing streams\n");
        return 1;
    }

    return 0;
}


void printResult() {
    int totalCnt =  correctCnt+wrongCnt+timeoutCnt+runtimeCnt;
    printf("\n--------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    printf(" - Total: %d\n", totalCnt);
    printf(" - Correct: %d (", correctCnt);
    for (int i = 0; i < correctCnt; i++) {
        printf(" %s", correctArr[i]);
    }
    printf(" )\n");

    printf(" - Wrong Answer: %d (", wrongCnt);
    for (int i = 0; i < wrongCnt; i++) {
        printf(" %s", wrongArr[i]);
    }
    printf(" )\n");

    printf(" - Timeout: %d (", timeoutCnt);
    for (int i = 0; i < timeoutCnt; i++) {
        printf(" %s", timeoutArr[i]);
    }
    printf(" )\n");

    printf(" - Runtime Error: %d (", runtimeCnt);
    for (int i = 0; i < runtimeCnt; i++) {
        printf(" %s", runtimeArr[i]);
    }
    printf(" )\n");

    if (runtimeCnt) 
        printf("\n*** Check Error Content *** \n");

    for (int i = 0; i < runtimeCnt; i++) {
        char *filename = strrchr(runtimeArr[i], '/');
        if (filename != NULL) {
            filename++; 
        } else {
            filename = runtimeArr[i];
        }

        char outputFilename[MAX_FILE_NAME];
        snprintf(outputFilename, sizeof(outputFilename), "%s_output.txt", filename);
        printf("-> Runtime Error content in file %s (details in %s)\n", outputFilename, outputFilename);

        FILE *fp = fopen(outputFilename, "r");
        if (fp == NULL) {
            printf("Failed to open error output file %s\n", outputFilename);
            continue;
        }

        // read summary line
        char buf[MAX_BUF_SIZE];
        while (fgets(buf, sizeof(buf), fp) != NULL) {
            if ((strstr(buf, "SUMMARY:") != NULL) || strstr(buf, "ERROR")) {
                printf("%s", buf);
            }
        }
        printf("\n");

        fclose(fp); 
    }
    printf("--------------------------------------------------------------------------------------------------------------------------------------------------------\n");
}
