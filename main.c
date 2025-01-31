#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>


int input_character_limit = 100;
int output_character_limit = 200;
int port_number = 60000;
int Levenshtein_list_limit = 5;
char dictionary[3000][50];
int dictionary_size = 0;

void readFile();
size_t levenshtein_n(const char *a, const size_t length, const char *b, const size_t bLength);
void start_server();
void *thread_function(void *arg);
char to_lower(char c);


struct thread_data{
    char word[50];
    int socket;
};

int main(void) {
    start_server();
    return 0;
}


void start_server(){
    int flag = 0;
    readFile((char*) dictionary);
    int server_fd , new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer [101] = {0};
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_number);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Text Analysis Server is running on port %d...\n", port_number);

    while (1){

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
        while (flag == 0){

            send(new_socket, "Hello, this is Text Analysis Server!\nPlease enter your input string:\n", 72, 0);
            read(new_socket, buffer, 1000);
            char input[101];
            strcpy(input , buffer);
            input[strcspn(input,"\n")] = 0;
            buffer[strcspn(buffer, "\n")] = 0;
            if (strlen(buffer) > input_character_limit) {
                send(new_socket, "Input string is longer than 100 (INPUT_CHARACTER_LIMIT) variable! \n", 70, 0);
                close(new_socket);
                flag = 1;
                continue;
            }

            for (int i = 0; i < strlen(buffer); ++i) {
                if (!((buffer[i] >= 'a' && buffer[i] <= 'z') || (buffer[i] >= 'A' && buffer[i] <= 'Z') ||
                      buffer[i] == ' ')) {
                    send(new_socket, "Input string contains unsupported characters!\n", 45, 0);
                    close(new_socket);
                    flag = 1;
                    continue;
                }
            }
            for (int i = 0; i < strlen(buffer); ++i) {
                buffer[i] = to_lower(buffer[i]);
            }

            pthread_t threads[input_character_limit];
            struct thread_data thread_data[input_character_limit];
            int threadCount = 0;
            char *token = strtok(buffer, " ");
            char corrected_sentence[1000] = {0};  // Store the corrected sentence
            while (token) {
                if (strlen(token) > 50) {
                    send(new_socket, "Word is too long\n", 18, 0);
                    close(new_socket);
                    break;
                }
                strcpy(thread_data[threadCount].word, token);
                thread_data[threadCount].socket = new_socket;

                pthread_create(&threads[threadCount], NULL, thread_function, (void *)&thread_data[threadCount]);
                pthread_join(threads[threadCount], NULL); // Ensure word is processed before moving to the next

                strcat(corrected_sentence, thread_data[threadCount].word);
                strcat(corrected_sentence, " ");
                threadCount++;
                token = strtok(NULL, " ");
            }

            if (strlen(corrected_sentence) > output_character_limit) {
                send(new_socket, "Error: Corrected sentence exceeds 200 characters. Closing program.\n", 67, 0);
                close(new_socket);
                exit(EXIT_FAILURE);
            }

            char response_output[2000];
            char response_input[2000];
            sprintf(response_input , "INPUT: %s\n" , input);
            send(new_socket ,response_input , strlen(response_input) , 0);
            sprintf(response_output, "OUTPUT: %s\n", corrected_sentence);
            send(new_socket, response_output, strlen(response_output), 0);
            // Ask the client if they want to finish or continue
            send(new_socket, "Do you want to finish or continue? (Y/N): ", 41, 0);
            char user_input[10] = {0};
            read(new_socket, user_input, sizeof(user_input) - 1);
            user_input[strcspn(user_input, "\n")] = 0;

            // Convert to lowercase for case-insensitive comparison
            for (int i = 0; i < strlen(user_input); ++i) {
                user_input[i] = to_lower(user_input[i]);
            }

            if (strcmp(user_input, "y") == 0 || strcmp(user_input, "yes") == 0) {
                // Client wants to continue; wait for new input
                continue;
            } else {
                // Client wants to finish; close the connection and exit the loop
                send(new_socket, "Thank you for using Text Analysis Server! Good Bye!\n", 50, 0);
                close(new_socket);
                break;
            }
        }
        break;
    }
}

size_t levenshtein_n(const char *a, const size_t length, const char *b, const size_t bLength) {
    // Shortcut optimizations for edge cases.
    if (a == b) {
        return 0;
    }

    if (length == 0) {
        return bLength;
    }

    if (bLength == 0) {
        return length;
    }

    // Allocate memory for the cache.
    size_t *cache = (size_t *)calloc(length, sizeof(size_t));
    if (cache == NULL) {
        // Handle memory allocation failure.
        return SIZE_MAX;
    }

    // Initialize the first row of the cache.
    for (size_t index = 0; index < length; ++index) {
        cache[index] = index + 1;
    }

    size_t result = 0; // Final result for the Levenshtein distance.

    // Main algorithm to compute the distance.
    for (size_t bIndex = 0; bIndex < bLength; ++bIndex) {
        char currentBChar = b[bIndex];
        size_t previousDistance = bIndex; // Tracks the value diagonally above-left in the grid.
        result = bIndex + 1;              // Tracks the value directly above in the grid.

        for (size_t index = 0; index < length; ++index) {
            size_t currentDistance = cache[index];

            // Determine the minimum of the three neighboring cells.
            size_t substitutionCost = (currentBChar == a[index]) ? previousDistance : previousDistance + 1;
            previousDistance = currentDistance;
            cache[index] = result = (currentDistance > result)
                                    ? (substitutionCost > result ? result + 1 : substitutionCost)
                                    : (substitutionCost > currentDistance ? currentDistance + 1 : substitutionCost);
        }
    }

    // Free the allocated memory for cache.
    free(cache);

    return result;
}


void *thread_function(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    char closest_words[Levenshtein_list_limit][50];
    int distances[Levenshtein_list_limit];

    // Initialize distances
    for (int i = 0; i < Levenshtein_list_limit; i++) distances[i] = 9999;

    // Check if the word exists in the dictionary
    int word_found = 0;
    for (int i = 0; i < dictionary_size; i++) {
        if (strcmp(data->word, dictionary[i]) == 0) {
            word_found = 1;
            break;
        }
    }
    if (!word_found) {
        // Find closest matches
        for (int i = 0; i < dictionary_size; i++) {
            int distance = levenshtein_n(data->word, strlen(data->word), dictionary[i], strlen(dictionary[i]));

            for (int j = 0; j < Levenshtein_list_limit; j++) {
                if (distance < distances[j]) {
                    for (int k = Levenshtein_list_limit - 1; k > j; k--) {
                        distances[k] = distances[k - 1];
                        strcpy(closest_words[k], closest_words[k - 1]);
                    }
                    distances[j] = distance;
                    strcpy(closest_words[j], dictionary[i]);
                    break;
                }
            }
        }

        // Prepare response for unmatched word
        char response[output_character_limit];
        sprintf(response, "\nWORD: %s\nClosest Matches:", data->word);
        for (int i = 0; i < Levenshtein_list_limit; i++) {
            if (distances[i] != 9999) {
                char temp[100];
                sprintf(temp, "\n%s (Distance: %d)", closest_words[i], distances[i]);
                strcat(response, temp);
            }
        }
        strcat(response, "\n");
        send(data->socket, response, strlen(response), 0);

        sprintf(response, "WORD '%s' is not present in the dictionary.\nDo you want to add this word to the dictionary? (Y/N): ", data->word);
        send(data->socket, response, strlen(response), 0);

        char user_input[10] = {0};
        read(data->socket, user_input, sizeof(user_input) - 1);
        user_input[strcspn(user_input, "\n")] = 0;
        for (int i = 0; i < strlen(user_input); ++i) {
            user_input[i] = to_lower(user_input[i]);
        }

        if (strcmp(user_input, "y") == 0 || strcmp(user_input, "yes") == 0) {
            // Add the word to the dictionary array
            strcpy(dictionary[dictionary_size], data->word);
            dictionary_size++;

            // Append the new word to the dictionary file
            FILE *file = fopen("basic_english_2000.txt", "a");
            if (file == NULL) {
                perror("Error opening dictionary file for appending");
            } else {
                fprintf(file, "%s\n", data->word);
                fclose(file);
                printf("Word '%s' written to file.\n", data->word);

            }
        } else {
            // Replace the word with the closest match
            strcpy(data->word, closest_words[0]);
        }
    } else{
        // Find closest matches
        for (int i = 0; i < dictionary_size; i++) {
            int distance = levenshtein_n(data->word, strlen(data->word), dictionary[i], strlen(dictionary[i]));

            for (int j = 0; j < Levenshtein_list_limit; j++) {
                if (distance < distances[j]) {
                    for (int k = Levenshtein_list_limit - 1; k > j; k--) {
                        distances[k] = distances[k - 1];
                        strcpy(closest_words[k], closest_words[k - 1]);
                    }
                    distances[j] = distance;
                    strcpy(closest_words[j], dictionary[i]);
                    break;
                }
            }
        }
        // Prepare response for unmatched word
        char response[output_character_limit];
        sprintf(response, "\nWORD: %s\nClosest Matches:", data->word);
        for (int i = 0; i < Levenshtein_list_limit; i++) {
            if (distances[i] != 9999) {
                char temp[100];
                sprintf(temp, "\n%s (Distance: %d)", closest_words[i], distances[i]);
                strcat(response, temp);
            }
        }
        strcat(response, "\n");
        send(data->socket, response, strlen(response), 0);
    }

    pthread_exit(NULL);
}



void readFile() {
    FILE *file = fopen("basic_english_2000.txt", "r");  // Open file in read mode
    if (file == NULL) {
        perror("Dctionary file “basic_english_2000.txt” not found!");
        exit(EXIT_FAILURE);
    }

    dictionary_size = 0;  // Initialize dictionary size to zero
    while (dictionary_size < 2009 && fgets(dictionary[dictionary_size], sizeof(dictionary[0]), file)) {
        dictionary[dictionary_size][strcspn(dictionary[dictionary_size], "\n")] = '\0'; // Trim newline

        // Skip empty lines or words containing spaces
        if (strlen(dictionary[dictionary_size]) == 0 || strchr(dictionary[dictionary_size], ' ') != NULL) {
            continue; // Ignore blank or invalid words
        }

        dictionary_size++; // Only count valid words
    }

    fclose(file);
}


char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return tolower(c);
    } else {
        return c;
    }
}