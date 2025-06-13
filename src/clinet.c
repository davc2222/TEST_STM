#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>

#include "clinet.h"
uint8_t get_peripheral_value(int choice);
cmd_to_stm_t cmd;
#define STM_PORT 1234
#define MAX_COMMAND_LEN 100
#define RESPONSE_BUFFER_SIZE 256
#define TIMEOUT_SECONDS 3
#define ITERATION_NUM 1
#define BIT_PATTERN_LENGTH 8

char bit_pattern[8] = {0x55};

const char *commands[] = {
    "TIMER",
    "UART",
    "SPI",
    "I2C",
    "ADC",
    "EXIT"};

#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0])) // commands number

// CRC-8
uint8_t calculate_crc8(const char *data)
{
    unsigned char crc = 0;
    while (*data)
    {
        crc ^= *data++;
        for (int i = 0; i < 8; ++i)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

uint8_t calculate_struct_crc8(const void *data, size_t length)
{
    const uint8_t *byte_data = (const uint8_t *)data;
    unsigned char crc = 0;
    for (size_t i = 0; i < length; ++i)
    {
        crc ^= byte_data[i];
        for (int j = 0; j < 8; ++j)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

void print_menu()
{
    printf("\n--- Available Commands ---\n");
    for (int i = 0; i < (int)NUM_COMMANDS; ++i)
    {
        printf("%d. %s\n", i + 1, commands[i]);
    }
    printf("--------------------------\n");
    printf("Select a command number: ");
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <STM_IP>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *stm_ip = argv[1];

    int sock;
    struct sockaddr_in stm_addr, from_addr;
    socklen_t from_len = sizeof(from_addr);
    char response[RESPONSE_BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    struct timeval timeout = {TIMEOUT_SECONDS, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memset(&stm_addr, 0, sizeof(stm_addr));
    stm_addr.sin_family = AF_INET;
    stm_addr.sin_port = htons(STM_PORT);
    if (inet_pton(AF_INET, stm_ip, &stm_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid IP address: %s\n", stm_ip);
        close(sock);
        return EXIT_FAILURE;
    }

    while (1)
    {
        print_menu();

        int choice;
        if (scanf("%d", &choice) != 1)
        {
            while (getchar() != '\n')
                ; // flush input
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        getchar(); // Consume newline

        if (choice < 1 || choice > (int)NUM_COMMANDS)
        {
            printf("Invalid selection.\n");
            continue;
        }

        const char *cmd_str = commands[choice - 1];

        if (strcmp(cmd_str, "EXIT") == 0)
        {
            printf("Exiting program.\n");
            break;
        }

        memset(&cmd, 0, sizeof(cmd)); // Reset

        cmd.test_id = 1001;
        cmd.tested_Peripheral = 0; // UART
        cmd.iterations_num = ITERATION_NUM;
        cmd.bit_pattern_length = BIT_PATTERN_LENGTH;
        strcpy(cmd.bit_pattern, bit_pattern);  // מעתיקים את המחרוזת לתוך המערך
         // what to test
         cmd.tested_Peripheral = get_peripheral_value(choice);
        // Calculate CRC on the struct, excluding the last field (cmd_crc)
        cmd.cmd_crc = calculate_struct_crc8(&cmd, sizeof(cmd) - sizeof(cmd.cmd_crc));
       

      
        ssize_t sent = sendto(sock, &cmd, sizeof(cmd), 0, (struct sockaddr *)&stm_addr, sizeof(stm_addr));
        if (sent < 0)
        {
            perror("Failed to send command");
            continue;
        }

        printf("Sent command: \"%s\" (CRC: 0x%02X)\n", cmd_str, cmd.cmd_crc);

        uint8_t success = 0;
        while (cmd.iterations_num-- > 0)
        {
            ssize_t received = recvfrom(sock, response, sizeof(response) - 1, 0,
                                        (struct sockaddr *)&from_addr, &from_len);
            if (received < 0)
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    printf("Timeout waiting for response. Retrying... (%d left)\n", cmd.iterations_num);
                    continue;
                }
                else
                {
                    perror("recvfrom failed");
                    break;
                }
            }
            else
            {
                response[received] = '\0';
                printf("STM32 Response: %s\n", response);
                success = 1;
                break;
            }
        }

        if (!success)
        {
            printf("No response from STM32 after retries.\n");
        }
    }

    close(sock);
    return EXIT_SUCCESS;
}


uint8_t get_peripheral_value(int choice) {
    switch (choice) {
        case 1:
            return 1;  // Timer
        case 2:
            return 2;  // UART
        case 3:
            return 4;  // SPI
        case 4:
            return 8;  // I2C
        case 5:
            return 16; // ADC
        default:
            return 0;  // אם הבחירה לא תקינה
    }
}