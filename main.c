#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

// Constants
#define NUM_PASSWORDS 10 // There are 10 passwords at the start
#define PASSWORD_LENGTH 6
#define NUM_READERS 3
#define NUM_WRITERS 2
#define NUM_OPERATIONS 5 // Number of operations per reader/writer

// Function declarations
void* reader_thread(void* arg);
void* writer_thread(void* arg);
void generate_passwords(int passwords[], int size);
int generate_random_password();
int is_correct_password(int thread_id, int password, int is_writer);
int generate_random_integer();
int get_correct_password(int thread_id, int is_writer);
int is_password_in_use(int password);

// Data structure to pass arguments to threads
typedef struct {
    int thread_id;
    int password;
    int is_dummy;
    int is_writer;
} thread_data;

// Global variables
int read_count = 0;
int BUFFER = 0;
int passwords[NUM_PASSWORDS]; // Array to store passwords

// Semaphores
sem_t mutex, write_lock;

int main() {
    srand(time(NULL)); // Seed the random number generator

    pthread_t readers[NUM_READERS]; // Reader threads
    pthread_t writers[NUM_WRITERS]; // Writer threads
    pthread_t dummy_readers[NUM_READERS]; // Dummy reader threads
    pthread_t dummy_writers[NUM_WRITERS]; // Dummy writer threads

    thread_data reader_data[NUM_READERS]; // Data for reader threads
    thread_data writer_data[NUM_WRITERS]; // Data for writer threads
    thread_data dummy_reader_data[NUM_READERS]; // Data for dummy reader threadss
    thread_data dummy_writer_data[NUM_WRITERS]; // Data for dummy writer threads

    // Initialize semaphores
    sem_init(&mutex, 0, 1); // Binary semaphore
    sem_init(&write_lock, 0, 1); // Write lock

    generate_passwords(passwords, NUM_PASSWORDS); // Generate random passwords

    // Create writer threads
    for (int i = 0; i < NUM_WRITERS; i++) {
        writer_data[i].thread_id = i;
        writer_data[i].password = passwords[NUM_READERS + i]; // To avoid duplicate passwords by using the next indices
        writer_data[i].is_dummy = 0;
        writer_data[i].is_writer = 1;
        pthread_create(&writers[i], NULL, writer_thread, &writer_data[i]);
    }

    // Create reader threads
    for (int i = 0; i < NUM_READERS; i++) {
        reader_data[i].thread_id = i;
        reader_data[i].password = passwords[i];
        reader_data[i].is_dummy = 0;
        reader_data[i].is_writer = 0;
        pthread_create(&readers[i], NULL, reader_thread, &reader_data[i]);
    }

    // Create dummy reader threads with random passwords not in the main table
    for (int i = 0; i < NUM_READERS; i++) {
        int dummy_password;
        do {
            dummy_password = generate_random_password();
        } while (is_password_in_use(dummy_password)); // Ensure the dummy password is not already in use

        dummy_reader_data[i].thread_id = NUM_READERS + NUM_WRITERS + i;
        dummy_reader_data[i].password = dummy_password;
        dummy_reader_data[i].is_dummy = 1;
        dummy_reader_data[i].is_writer = 0;
        pthread_create(&dummy_readers[i], NULL, reader_thread, &dummy_reader_data[i]);
    }

    // Create dummy writer threads with random passwords not in the main table
    for (int i = 0; i < NUM_WRITERS; i++) {
        int dummy_password;
        do {
            dummy_password = generate_random_password();
        } while (is_password_in_use(dummy_password)); // Ensure the dummy password is not already in use

        dummy_writer_data[i].thread_id = NUM_READERS + NUM_WRITERS + NUM_READERS + i;
        dummy_writer_data[i].password = dummy_password;
        dummy_writer_data[i].is_dummy = 1;
        dummy_writer_data[i].is_writer = 1;
        pthread_create(&dummy_writers[i], NULL, writer_thread, &dummy_writer_data[i]);
    }

    // Join threads
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(writers[i], NULL);
    }
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(dummy_readers[i], NULL);
    }
    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(dummy_writers[i], NULL);
    }

    sem_destroy(&mutex);
    sem_destroy(&write_lock);

    return 0;
}

void* reader_thread(void* arg) {
    thread_data* data = (thread_data*)arg; // Get thread data

    //printf("Reader %d (dummy: %d) with password %d started.\n", data->thread_id, data->is_dummy, data->password);

    for (int i = 0; i < NUM_OPERATIONS; i++) {
        if (is_correct_password(data->thread_id, data->password, data->is_writer)) {
            sem_wait(&mutex); // Lock the mutex
            read_count++; // Increment the read count
            if (read_count == 1) { // If this is the first reader
                sem_wait(&write_lock); // Lock the write lock
            }
            sem_post(&mutex); // Unlock the mutex

            // Read operation
            sleep(1); // Simulate reading
            printf("Reader %d (dummy: %d) read %d from the buffer.\n", data->thread_id, data->is_dummy, BUFFER);

            sem_wait(&mutex); // Lock the mutex
            read_count--; // Decrement the read count
            if (read_count == 0) { // If this is the last reader
                sem_post(&write_lock); // Unlock the write lock
            }
            sem_post(&mutex); // Unlock the mutex
        } else {
            int correct_password = get_correct_password(data->thread_id, data->is_writer);
            printf("Reader %d attempted to read with invalid password %d. Correct password: %d\n", data->thread_id, data->password, correct_password);
        }
    }
    return NULL;
}

void* writer_thread(void* arg) {
    thread_data* data = (thread_data*)arg; // Get thread data

    //printf("Writer %d (dummy: %d) with password %d started.\n", data->thread_id, data->is_dummy, data->password);

    for (int i = 0; i < NUM_OPERATIONS; i++) {
        if (is_correct_password(data->thread_id, data->password, data->is_writer)) {
            sem_wait(&write_lock); // Lock the write lock

            // Write operation
            sleep(1); // Simulate writing
            int random_integer = generate_random_integer(); // Generate a random integer
            BUFFER = random_integer; // Write the random integer to the buffer
            printf("Writer %d (dummy: %d) wrote %d to the buffer.\n", data->thread_id,data->thread_id, BUFFER);

            sem_post(&write_lock); // Unlock the write lock
        } else {
            int correct_password = get_correct_password(data->thread_id, data->is_writer);
            printf("Writer %d attempted to write with invalid password %d. Correct password: %d\n", data->thread_id, data->password, correct_password);
        }
    }

    return NULL;
}

void generate_passwords(int passwords[], int size) { // Generate random passwords
    for (int i = 0; i < size; i++) {
        passwords[i] = generate_random_password();
    }
}

int generate_random_password() { // Generate a random 6-digit password
    return rand() % 900000 + 100000;
}

// Function to check if the password is correct for the given thread ID
int is_correct_password(int thread_id, int password, int is_writer) {
    if (is_writer) {
        int writer_id = thread_id % NUM_WRITERS;
        return passwords[NUM_READERS + writer_id] == password;
    } else {
        int reader_id = thread_id % NUM_READERS;
        return passwords[reader_id] == password;
    }
}

// Function to get the correct password for the given thread ID
int get_correct_password(int thread_id, int is_writer) {
    if (is_writer) {
        int writer_id = thread_id % NUM_WRITERS;
        return passwords[NUM_READERS + writer_id]; // Adding the
    } else {
        int reader_id = thread_id % NUM_READERS;
        return passwords[reader_id];
    }
}

// Function to check if the password is already in use
int is_password_in_use(int password) {
    for (int i = 0; i < NUM_PASSWORDS; i++) {
        if (passwords[i] == password) {
            return 1;
        }
    }
    return 0;
}

// Generate a random integer in the range 0-9999
int generate_random_integer() {
    return rand() % 10000;
}

