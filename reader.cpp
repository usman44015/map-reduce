#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <cstddef>

using namespace std;

const int MAX_ITEMS = 1000;         // Replace macro
const int BUFFER_SIZE = 4096;       // Replace macro
const int MAX_WORD_SIZE = 200;      // Replace macro
const char* PIPE_READ = "PIPE1";    // Replace macro
const char* PIPE_WRITE = "PIPE2";   // Replace macro

struct KeyValue {
    char word[MAX_WORD_SIZE];
    int count;
};

struct MapData {
    KeyValue pairs[MAX_ITEMS];
    int count;
} mapped, shuffled;

pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shuffle_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t map_done, shuffle_done;
int fd_read, fd_write;

void* map_function(void* arg) {
    char* input = (char*)arg;
    char* word = strtok(input, " \n");
    
    while (word != NULL && mapped.count < MAX_ITEMS) {
        pthread_mutex_lock(&map_mutex);
        strncpy(mapped.pairs[mapped.count].word, word, MAX_WORD_SIZE-1);
        mapped.pairs[mapped.count].word[MAX_WORD_SIZE-1] = '\0';
        mapped.pairs[mapped.count].count = 1;
        mapped.count++;
        pthread_mutex_unlock(&map_mutex);
        word = strtok(NULL, " \n");
    }
    
    sem_post(&map_done);
    return (void*)0;
}

void* shuffle_function(void*) {
    sem_wait(&map_done);
    pthread_mutex_lock(&shuffle_mutex);
    
    char output[BUFFER_SIZE] = "After Shuffle: ";
    
    for (int i = 0; i < mapped.count; i++) {
        bool found = false;
        for (int j = 0; j < shuffled.count; j++) {
            if (strcmp(mapped.pairs[i].word, shuffled.pairs[j].word) == 0) {
                shuffled.pairs[j].count++;
                found = true;
                break;
            }
        }
        if (!found && shuffled.count < MAX_ITEMS) {
            strncpy(shuffled.pairs[shuffled.count].word, mapped.pairs[i].word, MAX_WORD_SIZE-1);
            shuffled.pairs[shuffled.count].word[MAX_WORD_SIZE-1] = '\0';
            shuffled.pairs[shuffled.count].count = 1;
            shuffled.count++;
        }
    }
    
    for (int i = 0; i < shuffled.count; i++) {
        char temp[MAX_WORD_SIZE] = {0};
        char valuesStr[MAX_WORD_SIZE] = {0};
        
        // Build values string
        for (int j = 0; j < shuffled.pairs[i].count - 1; j++) {
            strcat(valuesStr, "1, ");
        }
        strcat(valuesStr, "1");
        
        // Safe string formatting
        int written = snprintf(temp, sizeof(temp), "(\"%s\", [%s]) ", 
            shuffled.pairs[i].word, valuesStr);
        if (written >= 0 && written < sizeof(temp)) {
            if (strlen(output) + strlen(temp) < BUFFER_SIZE - 2) {
                strcat(output, temp);
            }
        }
    }
    
    if (strlen(output) + 2 <= BUFFER_SIZE) {
        strcat(output, "\n");
        write(fd_write, output, strlen(output));
    }
    
    pthread_mutex_unlock(&shuffle_mutex);
    sem_post(&shuffle_done);
    return (void*)0;
}

void* reduce_function(void*) {
    sem_wait(&shuffle_done);

    pthread_mutex_lock(&shuffle_mutex);
    
    char output[BUFFER_SIZE] = "Final Result: ";
    
    for (int i = 0; i < shuffled.count; i++) {
        char temp[MAX_WORD_SIZE] = {0};
        
        int written = snprintf(temp, sizeof(temp), "(%s, %d) ", 
            shuffled.pairs[i].word, shuffled.pairs[i].count);
        if (written >= 0 && written < sizeof(temp)) {
            if (strlen(output) + strlen(temp) < BUFFER_SIZE - 2) {
                strcat(output, temp);
            }
        }
    }
    if (strlen(output) + 2 <= BUFFER_SIZE) {
        strcat(output, "\n");
        cout<<output<<endl;
        write(fd_write, output, strlen(output));
    }
    pthread_mutex_unlock(&shuffle_mutex); 

    return (void*)0;
}

int main() {
    mkfifo(PIPE_READ, 0666);
    mkfifo(PIPE_WRITE, 0666);
    
    fd_read = open(PIPE_READ, O_RDONLY);
    fd_write = open(PIPE_WRITE, O_WRONLY);
    
    sem_init(&map_done, 0, 0);
    sem_init(&shuffle_done, 0, 0);
    
    pthread_t map_tid, shuffle_tid, reduce_tid;
    char buffer[BUFFER_SIZE];
    
    while(1) {
        mapped.count = 0;
        shuffled.count = 0;
        
        read(fd_read, buffer, BUFFER_SIZE);
        
        pthread_create(&map_tid, NULL, map_function, buffer);
        pthread_create(&shuffle_tid, NULL, shuffle_function, NULL);
        pthread_create(&reduce_tid, NULL, reduce_function, NULL);
        
        pthread_join(map_tid, NULL);
        pthread_join(shuffle_tid, NULL);
        pthread_join(reduce_tid, NULL);
    }
    
    close(fd_read);
    close(fd_write);
    return 0;
}

