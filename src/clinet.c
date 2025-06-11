#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>

#define STM_PORT 1234
#define MAX_COMMAND_LEN 100
#define RESPONSE_BUFFER_SIZE 256
#define TIMEOUT_SECONDS 3

const char* commands[] = {
    "LED_ON",
    "LED_OFF",
    "READ_TEMP",
    "RESET",
    "EXIT"
};

#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))

// CRC-8 (פשוט, פולינום 0x07)
unsigned char calculate_crc8(const char* data) {
    unsigned char crc = 0;
    while (*data) {
        crc ^= *data++;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

void print_menu() {
    printf("\n--- Available Commands ---\n");
    for (int i = 0; i < (int)NUM_COMMANDS; ++i) {
        printf("%d. %s\n", i + 1, commands[i]);
    }
    printf("--------------------------\n");
    printf("Select a command number: ");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <STM_IP>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* stm_ip = argv[1];

    int sock;
    struct sockaddr_in stm_addr, from_addr;
    socklen_t from_len = sizeof(from_addr);
    char response[RESPONSE_BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    struct timeval timeout = {TIMEOUT_SECONDS, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memset(&stm_addr, 0, sizeof(stm_addr));
    stm_addr.sin_family = AF_INET;
    stm_addr.sin_port = htons(STM_PORT);
    if (inet_pton(AF_INET, stm_ip, &stm_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address: %s\n", stm_ip);
        close(sock);
        return EXIT_FAILURE;
    }

    while (1) {
        print_menu();

        int choice;
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // flush input
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        getchar(); // Consume newline

        if (choice < 1 || choice > (int)NUM_COMMANDS) {
            printf("Invalid selection.\n");
            continue;
        }

        const char* cmd = commands[choice - 1];

        if (strcmp(cmd, "EXIT") == 0) {
            printf("Exiting program.\n");
            break;
        }

        // Prepare message with CRC
        unsigned char crc = calculate_crc8(cmd);
        char message[MAX_COMMAND_LEN];
        snprintf(message, sizeof(message), "%s:%02X", cmd, crc);

        // זמן התחלה
        struct timeval start, end;
        gettimeofday(&start, NULL);

        // Send command
        ssize_t sent = sendto(sock, message, strlen(message), 0,
                              (struct sockaddr*)&stm_addr, sizeof(stm_addr));
        if (sent < 0) {
            perror("Failed to send command");
            continue;
        }

        printf("Sent command: \"%s\" (CRC: 0x%02X)\n", cmd, crc);

        int retries = 3;
        int success = 0;
        while (retries-- > 0) {
            ssize_t received = recvfrom(sock, response, sizeof(response) - 1, 0,
                                        (struct sockaddr*)&from_addr, &from_len);
            if (received < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    printf("Timeout waiting for response. Retrying... (%d left)\n", retries);
                    continue;
                } else {
                    perror("recvfrom failed");
                    break;
                }
            } else {
                gettimeofday(&end, NULL); // זמן סיום

                long elapsed_us = (end.tv_sec - start.tv_sec) * 1000000L +
                                  (end.tv_usec - start.tv_usec);

                response[received] = '\0';
                printf("STM32 Response: %s\n", response);
                printf("Response time: %ld µs (%.2f ms)\n", elapsed_us, elapsed_us / 1000.0);
                success = 1;
                break;
            }
        }

        if (!success) {
            printf("No response from STM32 after retries.\n");
        }
    }

    close(sock);
    return EXIT_SUCCESS;
}
