#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LENGTH 50
#define MAX_WORDS 17500000 // Reduced to avoid memory issues

typedef struct {
    char word[MAX_LENGTH];
    int count;
} WordCount;

// Function to read words from the file and store them in an array
int readWords(const char *filename, char **words, int maxWords) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file.\n");
        return -1;
    }

    int wordCount = 0;
    while (wordCount < maxWords && fscanf(file, "%49s", words[wordCount]) == 1) {
        wordCount++;
    }

    fclose(file);
    return wordCount;
}

// Function to process the array of words, calculate frequencies, and store them in another array
int processWords(char **words, int totalWords, WordCount *wordCounts, int maxWords) {
    int wordCount = 0;

    for (int i = 0; i < totalWords; i++) {
        // Check if the word is already in the wordCounts array
        int found = 0;
        for (int j = 0; j < wordCount; j++) {
            if (strcmp(wordCounts[j].word, words[i]) == 0) {
                wordCounts[j].count++;
                found = 1;
                break;
            }
        }

        // Add the word to the wordCounts array if not found
        if (!found) {
            if (wordCount >= maxWords) {
                printf("Exceeded maximum unique word count.\n");
                return wordCount;
            }
            strcpy(wordCounts[wordCount].word, words[i]);
            wordCounts[wordCount].count = 1;
            wordCount++;
        }
    }

    return wordCount;
}

// Comparison function for qsort
int compare(const void *a, const void *b) {
    return ((WordCount *)b)->count - ((WordCount *)a)->count;
}

// Function to list the top 10 most frequent words
void listTopWords(WordCount *wordCounts, int wordCount) {
    qsort(wordCounts, wordCount, sizeof(WordCount), compare);

    printf("Top 10 most frequent words:\n");
    for (int i = 0; i < 10 && i < wordCount; i++) {
        printf("%s: %d\n", wordCounts[i].word, wordCounts[i].count);
    }
}

int main(void) {
    clock_t start, end;
    double time_spent;

    start = clock();

    // Dynamically allocate memory for word storage
    char **words = (char **)malloc(MAX_WORDS * sizeof(char *));
    if (words == NULL) {
        printf("Memory allocation failed for words.\n");
        return 1;
    }

    for (int i = 0; i < MAX_WORDS; i++) {
        words[i] = (char *)malloc(MAX_LENGTH * sizeof(char));
        if (words[i] == NULL) {
            printf("Memory allocation failed for word %d.\n", i);
            for (int j = 0; j < i; j++) { // Free previously allocated memory
                free(words[j]);
            }
            free(words);
            return 1;
        }
    }

    int totalWords = readWords("text8.txt", words, MAX_WORDS);
    if (totalWords == -1) {
        for (int i = 0; i < MAX_WORDS; i++) {
            free(words[i]);
        }
        free(words);
        return 1;
    }
    printf("Total words: %d\n", totalWords);

    // Dynamically allocate memory for word frequencies
    WordCount *wordCounts = (WordCount *)malloc(MAX_WORDS * sizeof(WordCount));
    if (wordCounts == NULL) {
        printf("Memory allocation failed for wordCounts.\n");
        for (int i = 0; i < MAX_WORDS; i++) {
            free(words[i]);
        }
        free(words);
        return 1;
    }

    int wordCount = processWords(words, totalWords, wordCounts, MAX_WORDS);

    listTopWords(wordCounts, wordCount);

    // Free dynamically allocated memory
    for (int i = 0; i < MAX_WORDS; i++) {
        free(words[i]);
    }
    free(words);
    free(wordCounts);

    end = clock();
    time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("EXECUTION TIME: %f seconds --- FIXED APPROACH ---\n", time_spent);

    return 0;
}

