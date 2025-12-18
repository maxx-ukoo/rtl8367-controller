/*
 * sw_menu.h
 *
 *  Created on: Nov 27, 2025
 *      Author: maxx
 */

#ifndef INC_SW_MENU_H_
#define INC_SW_MENU_H_

#define RX_BUFFER_SIZE 64
#define MAX_PORTS 10
#define MAX_TRUNK_SIZE 16

// --- Data structure for JSON ---
typedef struct {
    int vlan;                  // -1 if not set
    int trunk[MAX_TRUNK_SIZE]; // trunk ports
    int trunkCount;
} PortConfig;

void Menu_Init(void);
void ShowMainMenu(void);

void USB_MenuRXHandler(uint8_t* Buf, uint32_t Len);


#endif /* INC_SW_MENU_H_ */
