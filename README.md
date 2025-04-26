# map-reduce
Efficient Text Processing using Multithreading and IPC in Map reduce
// writer.cpp
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>

using namespace std;

#define PIPE_WRITE "PIPE1"
#define PIPE_READ "PIPE2"
#define BUFFER_SIZE 4096

int main() {
    mkfifo(PIPE_WRITE, 0666);
    mkfifo(PIPE_READ, 0666);

    int fd_write = open(PIPE_WRITE, O_WRONLY);
    int fd_read = open(PIPE_READ, O_RDONLY);

    char input[BUFFER_SIZE];
    char result[BUFFER_SIZE];
    char result2[BUFFER_SIZE];
    while(1) {
        cout << "Enter sentence: ";
        cin.getline(input, BUFFER_SIZE);
        
        write(fd_write, input, strlen(input) + 1);
        
        // Read shuffle result
        read(fd_read, result, BUFFER_SIZE);
        cout << result;
        
        // Read final result
        read(fd_read, result2, BUFFER_SIZE);
        cout << result2;

        memset(input, '\0', sizeof(input));
        memset(result, '\0', sizeof(result));
        memset(result2, '\0', sizeof(result2));
    }
    
    close(fd_write);
    close(fd_read);
    return 0;
}
