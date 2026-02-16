#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>

#define MAX_TERMS 17500000     // Maximum number of unique terms that can be stored
#define WORD_LIMIT 50          // Maximum length of a word
#define PROCESS_COUNT 8        // Number of processes to use
#define LOCAL_STORAGE 1000000  // Local buffer size for each process

// Structure to store terms and their frequencies
typedef struct {
    char term[WORD_LIMIT];
    int frequency;
} TermFrequency;

// Global shared memory for storing terms and their frequencies
TermFrequency *globalTerms; // Array of terms in shared memory
int *globalTermCount;       // Pointer to the count of terms in shared memory

// Update the frequency of a term in the local buffer
void update_term(TermFrequency *buffer, int *bufferCount, const char *term) {
    for (int i = 0; i < *bufferCount; i++) {
        // If the term already exists in the buffer, increment its frequency
        if (strcmp(buffer[i].term, term) == 0) {
            buffer[i].frequency++;
            return;
        }
    }
    // If the term doesn't exist, add it to the buffer
    strcpy(buffer[*bufferCount].term, term);
    buffer[*bufferCount].frequency = 1;
    (*bufferCount)++;
}

// Analyze a segment of the file
void analyze_segment(const char *filepath, int start, int finish, TermFrequency *buffer, int *bufferCount) {
    FILE *data = fopen(filepath, "r");
    if (!data) {
        perror("Failed to open file");
        exit(1);
    }

    fseek(data, start, SEEK_SET);

    // Ensure the start position aligns with a word boundary
    if (start != 0) {
        char c;
        while (fread(&c, 1, 1, data) && (c != ' ' && c != '\n')) {
            // Skip characters until the start of the next word
        }
    }

    char term[WORD_LIMIT];
    // Read words within the specified segment
    while (ftell(data) < finish && fscanf(data, "%49s", term) != EOF) {
        update_term(buffer, bufferCount, term); // Add the term to the buffer
    }

    fclose(data);
}

// Integrate results from the local buffer into the global shared memory
void integrate_results(TermFrequency *buffer, int bufferCount) {
    for (int i = 0; i < bufferCount; i++) {
        int exists = 0;
        for (int j = 0; j < *globalTermCount; j++) {
            // If the term already exists in global memory, update its frequency
            if (strcmp(globalTerms[j].term, buffer[i].term) == 0) {
                globalTerms[j].frequency += buffer[i].frequency;
                exists = 1;
                break;
            }
        }
        // If the term doesn't exist, add it to the global memory
        if (!exists) {
            if (*globalTermCount < MAX_TERMS) {
                strcpy(globalTerms[*globalTermCount].term, buffer[i].term);
                globalTerms[*globalTermCount].frequency = buffer[i].frequency;
                (*globalTermCount)++;
            } else {
                fprintf(stderr, "Shared memory limit exceeded!\n");
            }
        }
    }
}

// Identify and print the top 10 most frequent terms
void top_10_terms() {
    // Sort the terms by frequency in descending order
    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < *globalTermCount; j++) {
            if (globalTerms[j].frequency > globalTerms[i].frequency) {
                TermFrequency temp = globalTerms[i];
                globalTerms[i] = globalTerms[j];
                globalTerms[j] = temp;
            }
        }
    }

    // Print the top 10 terms
    printf("Top 10 terms:\n");
    for (int i = 0; i < 10 && i < *globalTermCount; i++) {
        printf("%s: %d\n", globalTerms[i].term, globalTerms[i].frequency);
    }
}

int main() {
    const char *filename = "text8.txt"; // File to process
    FILE *inputFile = fopen(filename, "r");
    if (!inputFile) {
        perror("Failed to open file");
        return 1;
    }

    fseek(inputFile, 0, SEEK_END);
    int totalSize = ftell(inputFile); // Get the total size of the file
    int chunkSize = totalSize / PROCESS_COUNT; // Divide the file into equal chunks
    fclose(inputFile);

    // Create shared memory for storing terms and their counts
    globalTerms = mmap(NULL, MAX_TERMS * sizeof(TermFrequency), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    globalTermCount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *globalTermCount = 0; // Initialize the global term count

    struct timespec begin, finish;
    clock_gettime(CLOCK_MONOTONIC, &begin); // Start measuring execution time

    // Create child processes to process file segments
    for (int id = 0; id < PROCESS_COUNT; id++) {
        int pid = fork();
        if (pid == 0) {
            // Child process
            TermFrequency *localBuffer = malloc(LOCAL_STORAGE * sizeof(TermFrequency));
            int bufferCount = 0;

            // Determine the start and end offsets for the segment
            int start_offset = id * chunkSize;
            int end_offset = (id == PROCESS_COUNT - 1) ? totalSize : (id + 1) * chunkSize;
            analyze_segment(filename, start_offset, end_offset, localBuffer, &bufferCount);

            // Save local results to a temporary file
            char tempFile[20];
            sprintf(tempFile, "segment_%d.bin", id);
            FILE *temp = fopen(tempFile, "wb");
            fwrite(&bufferCount, sizeof(int), 1, temp);
            fwrite(localBuffer, sizeof(TermFrequency), bufferCount, temp);
            fclose(temp);

            free(localBuffer);
            exit(0); // Exit the child process
        }
    }

    // Wait for all child processes to complete
    for (int id = 0; id < PROCESS_COUNT; id++) {
        wait(NULL);
    }

    // Parent process consolidates results from temporary files
    for (int id = 0; id < PROCESS_COUNT; id++) {
        char tempFile[20];
        sprintf(tempFile, "segment_%d.bin", id);
        FILE *temp = fopen(tempFile, "rb");

        int bufferCount;
        fread(&bufferCount, sizeof(int), 1, temp);
        TermFrequency *localBuffer = malloc(bufferCount * sizeof(TermFrequency));
        fread(localBuffer, sizeof(TermFrequency), bufferCount, temp);

        integrate_results(localBuffer, bufferCount);

        free(localBuffer);
        fclose(temp);
        remove(tempFile); // Remove the temporary file
    }

    // Identify and print the top 10 terms
    top_10_terms();

    clock_gettime(CLOCK_MONOTONIC, &finish); // End measuring execution time
    double duration = (finish.tv_sec - begin.tv_sec) + (finish.tv_nsec - begin.tv_nsec) / 1e9;
    printf("Execution time: %f seconds\n", duration);

    // Clean up shared memory
    munmap(globalTerms, MAX_TERMS * sizeof(TermFrequency));
    munmap(globalTermCount, sizeof(int));

    return 0;
}
