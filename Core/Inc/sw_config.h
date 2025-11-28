/*
 * sw_config.h
 *
 *  Created on: Nov 1, 2025
 *      Author: maxx
 */

#ifndef INC_SW_CONFIG_H_
#define INC_SW_CONFIG_H_

#include "vlan.h"

#define MAX_VLANS	10


typedef struct {
	rtk_vlan_t vid;
	rtk_vlan_cfg_t vlan_sw1;
	rtk_vlan_cfg_t vlan_sw2;
} vlan_database_item_t;

vlan_database_item_t* get_vlan_config(rtk_vlan_t vid);
void parse_vlan_config(char* json_config);
void load_switch_config();
void set_current_device(uint8_t device);

#endif /* INC_SW_CONFIG_H_ */
