#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// Constants
#define NUM_PASSWORDS 10 // It is stated in the question that there are 10 passwords at start
#define PASSWORD_LENGTH 6
#define NUM_READERS 3
#define NUM_WRITERS 2

// Function declarations
void* reader_thread(void* arg);
void* writer_thread(void* arg);
void generate_passwords(int passwords[]);
int generate_random_password();

// Data structure to pass arguments to threads
typedef struct {
    int thread_id;
    int password;
    int is_dummy;
} thread_data;

// Global variables
int read_count = 0;
int BUFFER = 0;

// Semaphores
sem_t mutex, write_lock;

int main() {
    pthread_t readers[NUM_READERS]; // Reader threads
    pthread_t writers[NUM_WRITERS]; // Writer threads
    pthread_t dummy_readers[NUM_READERS];
    pthread_t dummy_writers[NUM_WRITERS];
    thread_data reader_data[NUM_READERS]; // Data for reader threads
    thread_data writer_data[NUM_WRITERS]; // Data for writer threads
    thread_data dummy_reader_data[NUM_READERS];
    thread_data dummy_writer_data[NUM_WRITERS];

    // Initialize semaphores
    sem_init(&mutex, 0, 1); // Binary semaphore
    sem_init(&write_lock, 0, 1); // Write lock

    int passwords[NUM_PASSWORDS]; // Array to store passwords
    generate_passwords(passwords); // Generate random passwords

    // Create reader threads
    for (int i = 0; i < NUM_READERS; i++) {
        reader_data[i].thread_id = i + 1;
        reader_data[i].password = passwords[i];
        reader_data[i].is_dummy = 0;
        pthread_create(&readers[i], NULL, reader_thread, &reader_data[i]);
    }

    // Create writer threads
    for (int i = 0; i < NUM_WRITERS; i++) {
        writer_data[i].thread_id = i + 1;
        writer_data[i].password = passwords[NUM_READERS + i];
        writer_data[i].is_dummy = 0;
        pthread_create(&writers[i], NULL, writer_thread, &writer_data[i]);
    }

    // Create dummy reader threads with random passwords not in the main table
    for (int i = 0; i < NUM_READERS; i++) {
        int dummy_password;
        do {
            dummy_password = generate_random_password();
        } while (dummy_password == passwords[i]);

        dummy_reader_data[i].thread_id = i + 1;
        dummy_reader_data[i].password = dummy_password;
        dummy_reader_data[i].is_dummy = 1;
        pthread_create(&dummy_readers[i], NULL, reader_thread, &dummy_reader_data[i]);
    }

    // Create dummy writer threads with random passwords not in the main table
    for (int i = 0; i < NUM_WRITERS; i++) {
        int dummy_password;
        do {
            dummy_password = generate_random_password();
        } while (dummy_password == passwords[NUM_READERS + i]);

        dummy_writer_data[i].thread_id = i + 1;
        dummy_writer_data[i].password = dummy_password;
        dummy_writer_data[i].is_dummy = 1;
        pthread_create(&dummy_writers[i], NULL, writer_thread, &dummy_writer_data[i]);
    }

    // Join threads
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(writers[i], NULL);
    }

    return 0;
}

void* reader_thread(void* arg) {
    thread_data* data = (thread_data*)arg; // Get thread data
    printf("Reader %d with password %d started.\n", data->thread_id, data->password);

    sem_wait(&mutex); // Lock the mutex
    read_count++; // Increment the read count
    if (read_count == 1) { // If this is the first reader
        sem_wait(&write_lock); // Lock the write lock
    }
    sem_post(&mutex); // Unlock the mutex

    // Read operation

    sem_wait(&mutex); // Lock the mutex
    read_count--; // Decrement the read count
    if (read_count == 0) { // If this is the last reader
        sem_post(&write_lock); // Unlock the write lock
    }
    sem_post(&mutex); // Unlock the mutex

    sleep(1);
    return NULL;
}

void* writer_thread(void* arg) {
    thread_data* data = (thread_data*)arg; // Get thread data
    printf("Writer %d with password %d started.\n", data->thread_id, data->password);

    sem_wait(&write_lock); // Lock the write lock

    // Write operation

    sem_post(&write_lock); // Unlock the write lock

    sleep(1);
    return NULL;
}

void generate_passwords(int passwords[]) { // Generate random passwords
    for (int i = 0; i < NUM_PASSWORDS; i++) {
        passwords[i] = generate_random_password();
    }
}

int generate_random_password() { // Generate a random 6-digit password
    return rand() % 900000 + 100000;
}

