#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "stdlib.h"
#include <sys/socket.h>
#include <unistd.h>

#define FILE_SIZE 3000000

/*
* @brief A random data generator function based on srand() and rand().
* @param size The size of the data to generate (up to 2^32 bytes).
* @return A pointer to the buffer.
*/
char *util_generate_random_data(unsigned int size) {
    char *buffer = NULL;
    // Argument check.
    if (size == 0)
        return NULL;
    buffer = (char *) calloc(size, sizeof(char));
    // Error checking.
    if (buffer == NULL)
        return NULL;
    // Randomize the seed of the random number generator.
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++)
        *(buffer + i) = ((unsigned int) rand() % 256);
    return buffer;
}

void setBits(char *array, long number, int offset) {
    for (int i = 0; i < 32; i++) {
        array[i+offset] = (number >> (31 - i)) & 1;
    }
}

int main(int argc, char *argv[]) {
    char *file = util_generate_random_data(FILE_SIZE);
    puts("Starting Sender...\n");

    char SERVER_IP[20] = {0};
    int port;
    char algo[10] = {0};
    if (argc != 7) {
        puts("invalid command");
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) { port = atoi(argv[i + 1]); }
        else if (strcmp(argv[i], "-ip") == 0) { strcpy(SERVER_IP, argv[i + 1]); }
        else if (strcmp(argv[i], "-algo") == 0) { strcpy(algo, argv[i + 1]); }
    }

    struct sockaddr_in senderAdress;
    memset(&senderAdress, 0, sizeof(senderAdress));
    senderAdress.sin_family = AF_INET;
    senderAdress.sin_port = htons(port);

    int socketfd = -1; // Initializing the socket
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        perror("socket(2)");
        free(file);

        exit(1);
    }

    // Setting the CC algo to the input algo.
    if (setsockopt(socketfd, IPPROTO_TCP, TCP_CONGESTION, algo, strlen(algo)) != 0) {
        perror("setsockopt(2)");
        free(file);

        exit(1);
    }



    if (inet_pton(AF_INET, SERVER_IP, &senderAdress.sin_addr) <= 0) { // setting up the IP address
        perror("inet_pton(3)");
        close(socketfd);
        free(file);

        return 1;
    }
    if (connect(socketfd, (struct sockaddr *) &senderAdress, sizeof(senderAdress)) < 0) { // connecting to receiver
        puts("failed");
        perror("connect(2)");
        close(socketfd);
        free(file);

        return 1;
    }

    // adding start to file
    struct timeval start, end;
    gettimeofday(&start, NULL);
    setBits(file, start.tv_sec, 0);
    setBits(file, start.tv_usec, 32);

    puts("sending file");
    long bytes_sent = send(socketfd, file, FILE_SIZE, 0);
    gettimeofday(&end,NULL);
    if (bytes_sent <= 0) {
        perror("send(2)");
        close(socketfd);
        free(file);

        return 1;
    }
    puts("file sent");
    double timeTaken = (end.tv_sec - start.tv_sec) * 1000.0;
    timeTaken += (end.tv_usec - start.tv_usec) / 1000.0;
    printf("time taken: %0.3lf ms\n",timeTaken);

    int input = -1;
    while (input) {

        puts("Press 1 to resend the file, press 0 to exit.");
        scanf("%d", &input);
        if (input == 1) {
            gettimeofday(&start, NULL);
            setBits(file, start.tv_sec, 0);
            setBits(file, start.tv_usec, 32);

            bytes_sent = send(socketfd, file, FILE_SIZE, 0);
            if (bytes_sent <= 0) {
                perror("send(2)");
                close(socketfd);
                free(file);
                return 1;
            }
            gettimeofday(&end, NULL);
            timeTaken = (end.tv_sec - start.tv_sec) * 1000.0;
            timeTaken += (end.tv_usec - start.tv_usec) / 1000.0;
            printf("time taken: %0.3lf ms\n",timeTaken);
        }
        else if (input != 0) {
            puts("Invalid input.");
        }
    }

    bytes_sent = send(socketfd, "exit", 4, 0);
    if (bytes_sent <= 0)
    {
        perror("send(2)");
        close(socketfd);
        free(file);

        return 1;
    }
    close(socketfd);
    free(file);
    return 0;
}

