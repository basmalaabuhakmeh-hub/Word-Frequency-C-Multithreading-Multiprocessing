#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>

#define WORD_LEN 50           // Maximum length of a word
#define NUM_THREADS 8         // Number of threads to use
#define MAX_WORDS 17500000    // Maximum number of words that can be stored

// Structure to store a word and its count
typedef struct {
    char word[WORD_LEN];
    int count;
} WordCount;

// Global data structure for shared storage of word frequencies
WordCount *global_words;       // Array of global words
int global_word_count = 0;     // Total count of words in the global array
pthread_mutex_t lock;          // Mutex for thread synchronization

// Structure for thread-local storage
typedef struct {
    WordCount *local_words;    // Array of words in thread-local storage
    int local_word_count;      // Count of words in thread-local storage
} ThreadData;

// Structure to pass arguments to threads
typedef struct {
    int thread_id;             // ID of the thread
    int segment_start;         // Start byte of the file segment
    int segment_end;           // End byte of the file segment
    ThreadData *thread_data;   // Pointer to thread-local storage
} ThreadArgs;

// Find the index of a word in a given array
int find_word_index(WordCount *words, int word_count, const char *word) {
    for (int i = 0; i < word_count; i++) {
        if (strcmp(words[i].word, word) == 0) {
            return i; // Word found, return its index
        }
    }
    return -1; // Word not found
}

// Add a word to thread-local storage
void add_word(ThreadData *data, const char *word) {
    int index = find_word_index(data->local_words, data->local_word_count, word);
    if (index >= 0) {
        // If the word already exists, increment its count
        data->local_words[index].count++;
        return;
    }
    if (data->local_word_count >= MAX_WORDS / NUM_THREADS) {
        // Check for thread-local storage overflow
        fprintf(stderr, "Thread-local storage exceeded\n");
        exit(1);
    }
    // Add the new word to the thread-local storage
    strcpy(data->local_words[data->local_word_count].word, word);
    data->local_words[data->local_word_count].count = 1;
    data->local_word_count++;
}

// Function to process a chunk of the file
void *process_chunk(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    FILE *file = fopen("text8.txt", "r");
    if (!file) {
        perror("Failed to open file");
        pthread_exit(NULL);
    }

    fseek(file, args->segment_start, SEEK_SET);

    // Ensure the start position aligns with a word boundary
    if (args->segment_start > 0) {
        char c;
        while (!feof(file)) {
            c = fgetc(file);
            if (isspace(c)) break;
        }
    }

    char word[WORD_LEN];
    // Read words within the segment
    while (ftell(file) < args->segment_end && fscanf(file, "%49s", word) != EOF) {
        add_word(args->thread_data, word);
    }

    fclose(file);
    return NULL;
}

// Merge thread-local results into the global array
void merge_results(ThreadData *thread_data) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < thread_data->local_word_count; i++) {
        int index = find_word_index(global_words, global_word_count, thread_data->local_words[i].word);
        if (index >= 0) {
            // If the word exists in global storage, update its count
            global_words[index].count += thread_data->local_words[i].count;
        } else {
            if (global_word_count >= MAX_WORDS) {
                // Check for global storage overflow
                fprintf(stderr, "Global word storage exceeded\n");
                pthread_mutex_unlock(&lock);
                exit(1);
            }
            // Add the new word to the global array
            strcpy(global_words[global_word_count].word, thread_data->local_words[i].word);
            global_words[global_word_count].count = thread_data->local_words[i].count;
            global_word_count++;
        }
    }
    pthread_mutex_unlock(&lock);
}

// Find and print the top 10 most frequent words
void find_top_10_words() {
    // Sort the global word array in descending order of frequency
    for (int i = 0; i < global_word_count - 1; i++) {
        for (int j = i + 1; j < global_word_count; j++) {
            if (global_words[j].count > global_words[i].count) {
                WordCount temp = global_words[i];
                global_words[i] = global_words[j];
                global_words[j] = temp;
            }
        }
    }

    // Print the top 10 words
    printf("Top 10 words:\n");
    for (int i = 0; i < 10 && i < global_word_count; i++) {
        printf("%s: %d\n", global_words[i].word, global_words[i].count);
    }
}

int main() {
    FILE *file = fopen("text8.txt", "r");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    int file_size = ftell(file); // Get the total size of the file
    fclose(file);

    // Allocate memory for global word storage
    global_words = malloc(MAX_WORDS * sizeof(WordCount));
    if (!global_words) {
        perror("Failed to allocate global memory");
        return 1;
    }

    pthread_mutex_init(&lock, NULL); // Initialize the mutex

    pthread_t threads[NUM_THREADS];
    ThreadArgs thread_args[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];
    int segment_size = file_size / NUM_THREADS;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start); // Start time measurement

    // Create threads to process file segments
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].local_words = malloc((MAX_WORDS / NUM_THREADS) * sizeof(WordCount));
        thread_data[i].local_word_count = 0;

        thread_args[i].thread_id = i;
        thread_args[i].segment_start = i * segment_size;
        thread_args[i].segment_end = (i == NUM_THREADS - 1) ? file_size : (i + 1) * segment_size;
        thread_args[i].thread_data = &thread_data[i];

        pthread_create(&threads[i], NULL, process_chunk, &thread_args[i]);
    }

    // Wait for threads to complete and merge their results
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        merge_results(&thread_data[i]);
        free(thread_data[i].local_words);
    }

    find_top_10_words(); // Find and print the top 10 words

    clock_gettime(CLOCK_MONOTONIC, &end); // End time measurement
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Execution time (multithreading): %.6f seconds\n", elapsed_time);

    pthread_mutex_destroy(&lock); // Destroy the mutex
    free(global_words); // Free the global memory

    return 0;
}
