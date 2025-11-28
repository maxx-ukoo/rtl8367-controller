/*
 * sw_menu.c
 *
 *  Created on: Nov 27, 2025
 *      Author: maxx
 */

#include "main.h"
#include "sw_menu.h"
#include <string.h>
#include <stdio.h>

#define FLASH_PAGE_SIZE       1024U
#define FLASH_USER_START_ADDR 0x0801FC00U   // Start of last page (page 127)
#define FLASH_USER_END_ADDR   0x0801FFFFU   // End of last page
// Magic header constant
#define FLASH_MAGIC_HEADER    0xDEADBEEF

extern UART_HandleTypeDef huart1;

uint8_t rxBuffer[RX_BUFFER_SIZE];
uint8_t rxData;
uint16_t rxIndex = 0;

typedef enum {
    MENU_MAIN,
    MENU_PORT,
    MENU_VLAN,
    MENU_TRUNK
} MenuState;

MenuState currentMenu = MENU_MAIN;
int selectedPort = -1;

PortConfig ports[MAX_PORTS+1]; // index 1..10

typedef struct {
    uint32_t magic;            // header marker
    PortConfig ports[MAX_PORTS+1];
} FlashStorage;

// --- Helper: send string over UART ---
void UART_SendString(char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

// --- Show menus ---
void ShowMainMenu(void) {
    UART_SendString("\r\n=== MAIN MENU ===\r\n");
    UART_SendString("Enter port number (1-10)\r\n");
    UART_SendString("Or 'q' to quit\r\n");
    UART_SendString("s. Show current config\r\n");
    UART_SendString("w. Write config to flash\r\n> ");
}

void ShowPortMenu(void) {
    char msg[64];
    sprintf(msg, "\r\n--- Port %d Menu ---\r\n", selectedPort);
    UART_SendString(msg);
    UART_SendString("Select:\r\n");
    UART_SendString("1. VLAN\r\n");
    UART_SendString("2. TRUNK\r\n");
    UART_SendString("Or 'q' to return\r\n> ");
}

void ShowVlanMenu(void) {
    UART_SendString("\r\n*** VLAN Menu ***\r\n");
    UART_SendString("Enter VLAN ID (integer)\r\n");
    UART_SendString("Or 'q' to return\r\n> ");
}

void ShowTrunkMenu(void) {
    UART_SendString("\r\n*** TRUNK Menu ***\r\n");
    UART_SendString("Enter trunk ports (comma separated)\r\n");
    UART_SendString("Or 'q' to return\r\n> ");
}

// --- JSON printer (exact format with trailing commas if trunkCount==0) ---
void PrintJSON(void) {
    UART_SendString("\r\n{");
    for (int i = 1; i <= MAX_PORTS; i++) {
        char msg[256];
        sprintf(msg, "\r\n  \"%d\": {", i);
        UART_SendString(msg);

        if (ports[i].vlan != -1) {
            sprintf(msg, "\r\n    \"vlan\": %d", ports[i].vlan);
            UART_SendString(msg);
            if (ports[i].trunkCount == 0) {
                UART_SendString(",");
            }
        }
        if (ports[i].trunkCount > 0) {
            UART_SendString("\r\n    \"trunk\": [");
            for (int j = 0; j < ports[i].trunkCount; j++) {
                sprintf(msg, "%d", ports[i].trunk[j]);
                UART_SendString(msg);
                if (j < ports[i].trunkCount - 1) UART_SendString(", ");
            }
            UART_SendString("]");
        }
        UART_SendString("\r\n  }");
        if (i < MAX_PORTS) UART_SendString(",");
    }
    UART_SendString("\r\n}\r\n");
}
HAL_StatusTypeDef Flash_WritePorts(void) {
    HAL_StatusTypeDef status;
    uint32_t Address = FLASH_USER_START_ADDR;
    uint32_t data;

    FlashStorage storage;
    storage.magic = FLASH_MAGIC_HEADER;
    memcpy(storage.ports, ports, sizeof(ports));

    HAL_FLASH_Unlock();

    // Erase last page
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t PageError;
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.PageAddress = FLASH_USER_START_ADDR;
    eraseInit.NbPages = 1;
    status = HAL_FLASHEx_Erase(&eraseInit, &PageError);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return status;
    }

    // Program word by word
    size_t len = sizeof(storage);
    uint8_t *pData = (uint8_t*)&storage;
    for (size_t i = 0; i < len; i += 4) {
        data = 0xFFFFFFFF;
        memcpy(&data, pData + i, (len - i >= 4) ? 4 : (len - i));
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, data);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return status;
        }
        Address += 4;
    }

    HAL_FLASH_Lock();
    return HAL_OK;
}

void Flash_ReadPorts(void) {
    FlashStorage *storage = (FlashStorage*)FLASH_USER_START_ADDR;

    if (storage->magic == FLASH_MAGIC_HEADER) {
        memcpy(ports, storage->ports, sizeof(ports));
    } else {
        // No valid data in Flash → initialize defaults
        for (int i = 0; i <= MAX_PORTS; i++) {
            ports[i].vlan = -1;
            ports[i].trunkCount = 0;
        }
    }
}

// --- Process input ---
void ProcessInput(char *input) {
    if (strcmp(input, "q") == 0) {
        currentMenu = MENU_MAIN;
        ShowMainMenu();
        return;
    }

    switch (currentMenu) {
        case MENU_MAIN: {
            if (strcmp(input, "s") == 0) {
                PrintJSON();
                ShowMainMenu();
            } else if (strcmp(input, "w") == 0) {
            	if (Flash_WritePorts() == HAL_OK) {
            	    UART_SendString("Config written to Flash!\r\n");
            	}
                ShowMainMenu();
            } else {
                int port = atoi(input);
                if (port >= 1 && port <= 10) {
                    selectedPort = port;
                    currentMenu = MENU_PORT;
                    ShowPortMenu();
                } else {
                    UART_SendString("Invalid choice. Try again.\r\n> ");
                }
            }
            break;
        }
        case MENU_PORT: {
            if (strcmp(input, "1") == 0) {
                currentMenu = MENU_VLAN;
                ShowVlanMenu();
            } else if (strcmp(input, "2") == 0) {
                currentMenu = MENU_TRUNK;
                ShowTrunkMenu();
            } else {
                UART_SendString("Invalid choice. Try again.\r\n> ");
            }
            break;
        }
        case MENU_VLAN: {
            int vlan = atoi(input);
            if (vlan == 0) {
                // Reset VLAN configuration
                ports[selectedPort].vlan = -1;
                char msg[64];
                sprintf(msg, "VLAN reset on port %d\r\n", selectedPort);
                UART_SendString(msg);
            } else {
                ports[selectedPort].vlan = vlan;
                char msg[64];
                sprintf(msg, "VLAN %d configured on port %d\r\n", vlan, selectedPort);
                UART_SendString(msg);
            }
            ShowPortMenu();
            currentMenu = MENU_PORT;
            break;
        }
        case MENU_TRUNK: {
            ports[selectedPort].trunkCount = 0;
            char *token = strtok(input, ",");
            int reset = 0;
            while (token != NULL && ports[selectedPort].trunkCount < MAX_TRUNK_SIZE) {
                int val = atoi(token);
                if (val == 0) {
                    // Reset trunk configuration
                    reset = 1;
                    break;
                } else {
                    ports[selectedPort].trunk[ports[selectedPort].trunkCount++] = val;
                }
                token = strtok(NULL, ",");
            }

            if (reset) {
                ports[selectedPort].trunkCount = 0;
                UART_SendString("Trunk reset.\r\n");
            } else {
                UART_SendString("Trunk configured.\r\n");
            }

            ShowPortMenu();
            currentMenu = MENU_PORT;
            break;
        }
    }
}

// --- UART RX complete callback ---
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        HAL_UART_Transmit(&huart1, &rxData, 1, HAL_MAX_DELAY); // echo

        if (rxData == '\r' || rxData == '\n') {
            rxBuffer[rxIndex] = '\0';
            if (rxIndex > 0) {
                ProcessInput((char*)rxBuffer);
                rxIndex = 0;
            }
        } else {
            if (rxIndex < RX_BUFFER_SIZE - 1) {
                rxBuffer[rxIndex++] = rxData;
            }
        }
        HAL_UART_Receive_IT(&huart1, &rxData, 1);
    }
}

// --- Init function ---
void Menu_Init(void) {
    for (int i = 0; i <= MAX_PORTS; i++) {
        ports[i].vlan = -1;
        ports[i].trunkCount = 0;
    }
    Flash_ReadPorts();
    UART_SendString("\r\nWelcome to STM32 switch Menu!\r\n");
    ShowMainMenu();
    HAL_UART_Receive_IT(&huart1, &rxData, 1);
    load_switch_config(ports);
}
