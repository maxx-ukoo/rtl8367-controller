/*
 * sw_config.c
 *
 *  Created on: Nov 1, 2025
 *      Author: maxx
 */
#include "main.h"
#include <stdio.h>
#include "lwjson/lwjson.h"
#include "sw_config.h"
#include "rtk_switch.h"
#include "rtk_error.h"
#include "vlan.h"
#include "port.h"
#include "rtl8367c_asicdrv_port.h"

/* LwJSON instance and tokens */
static lwjson_token_t tokens[128];
static lwjson_t lwjson;

GPIO_TypeDef *rtkI2C_SCKport;
GPIO_TypeDef *rtkI2C_SDAport;
uint32_t rtkI2C_SCKpin;
uint32_t rtkI2C_SDApin;

#define LINK_PORT		EXT_PORT1
#define RTL_DEVICE_1	1
#define RTL_DEVICE_2	2

static vlan_database_item_t vlan_database[MAX_VLANS] = {
		{ .vid = 0, .vlan_sw1 = { 0 }, .vlan_sw2 = { 0 } },
		{ .vid = 0, .vlan_sw1 = { 0 }, .vlan_sw2 = { 0 } },
		{ .vid = 0, .vlan_sw1 = { 0 }, .vlan_sw2 = { 0 } },
		{ .vid = 0, .vlan_sw1 = { 0 }, .vlan_sw2 = { 0 } },
		{ .vid = 0, .vlan_sw1 = { 0 }, .vlan_sw2 = { 0 } },
		{ .vid = 0, .vlan_sw1 = { 0 }, .vlan_sw2 = { 0 } },
		{ .vid = 0, .vlan_sw1 = { 0 }, .vlan_sw2 = { 0 } },
		{ .vid = 0, .vlan_sw1 = { 0 }, .vlan_sw2 = { 0 } },
		{ .vid = 0, .vlan_sw1 = { 0 }, .vlan_sw2 = { 0 } },
		{ .vid = 0, .vlan_sw1 = { 0 }, .vlan_sw2 = { 0 } }
};

char* json_config = R"JSON_CONFIG(
{
  "1": {
    "vlan": 11,
    "trunk": [240]
  },
  "2": {
    "vlan": 10
  },
  "3": {
    "vlan": 12
  },
  "4": {
    "vlan": 12
  },
  "5": {
    "vlan": 11
  },
  "6": {
    "vlan": 11,
  },
  "7": {
    "vlan": 11
  },
  "8": {
    "vlan": 11
  },  
  "9": {
    "trunk": [10, 11, 12, 240]
  },
  "10": {
    "trunk": [10, 11, 12, 240]
  }
}
)JSON_CONFIG";

static uint8_t check_for_error(rtk_api_ret_t status, char *message) {
	if (status == RT_ERR_OK) {
		printf("%s successfully.\n", message);
		return 0;
	} else {
		printf("%s with error: %ld\n", message, status);
	}
	return 1;
}

static void configure_rgmii_link(void) {
	rtk_port_mac_ability_t rgmii_config;

	rgmii_config.forcemode = MAC_FORCE;
	rgmii_config.speed = PORT_SPEED_1000M;
	rgmii_config.duplex = PORT_FULL_DUPLEX;
	rgmii_config.link = PORT_LINKUP;
	rgmii_config.nway = RTK_DISABLED;
	rgmii_config.txpause = RTK_ENABLED;
	rgmii_config.rxpause = RTK_ENABLED;

	rtk_api_ret_t status = rtk_port_macForceLinkExt_set(LINK_PORT, MODE_EXT_RGMII, &rgmii_config);
	check_for_error(status, "rtk_port_macForceLinkExt_set executed");
	status = rtk_port_rgmiiDelayExt_set(LINK_PORT, 1, 3);
	check_for_error(status, "rtk_port_rgmiiDelayExt_set executed");
	status = rtk_port_phyEnableAll_set(RTK_ENABLED);
	check_for_error(status, "rtk_port_phyEnableAll_set executed");
}

vlan_database_item_t* get_vlan_config(rtk_vlan_t vid) {
	int current_vlan = 0;
	for (current_vlan=0; current_vlan<MAX_VLANS; current_vlan++) {
		if (vlan_database[current_vlan].vid == 0) {
			break;
		}
		if (vlan_database[current_vlan].vid ==vid) {
			return &vlan_database[current_vlan];
		}
	}
	vlan_database[current_vlan].vid = vid;
	return &vlan_database[current_vlan];
}

void parse_vlan_config(char *json_config) {
	lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));
	lwjsonr_t status = lwjson_parse(&lwjson, json_config);
	if (status == lwjsonOK) {
		lwjson_token_t *t;
		printf("JSON config has valid JSON format...\r\n");
		HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_SET);

		/* Get very first token as top object */
		t = lwjson_get_first_token(&lwjson);
		for (const lwjson_token_t *tkn = lwjson_get_first_child(t); tkn != NULL; tkn = tkn->next) {
			if (tkn->type == LWJSON_TYPE_OBJECT) {
				char tmp_str[256];
				tmp_str[0] = '\0';
				strncpy(tmp_str, tkn->token_name, tkn->token_name_len);
				tmp_str[tkn->token_name_len] = '\0';
				int user_sw_port = atoi(tmp_str);
				rtk_port_t  rtl_port = user_sw_port-1;
				if (user_sw_port > 5) {
					rtl_port = user_sw_port-6;
				}

				printf("SW port: %d, hardware port %d\n\r", user_sw_port, rtl_port);
				rtk_vlan_t vlan_id = 0;

				/* Now print all keys in the object */
				for (const lwjson_token_t *tkn2 = lwjson_get_first_child(tkn); tkn2 != NULL; tkn2 = tkn2->next) {

					if (tkn2->type == LWJSON_TYPE_NUM_INT) {
						vlan_id = (int) lwjson_get_val_int(tkn2);
						vlan_database_item_t* vlan = get_vlan_config(vlan_id);

						if (user_sw_port < 6) {
							printf("Set port %d as member of vlan: %ld \n\r", user_sw_port, vlan_id);
							RTK_PORTMASK_PORT_SET(vlan->vlan_sw1.mbr, rtl_port);
							RTK_PORTMASK_PORT_SET(vlan->vlan_sw1.mbr, LINK_PORT);
							RTK_PORTMASK_PORT_SET(vlan->vlan_sw1.untag, rtl_port);
							RTK_PORTMASK_PORT_SET(vlan->vlan_sw2.mbr, LINK_PORT);
							//set_current_device(RTL_DEVICE_1);
							printf("Set portPvid %ld for port: %d \n\r", vlan_id, rtl_port);
							//rtk_api_ret_t rtl_status = rtk_vlan_portPvid_set(rtl_port, vlan_id, 0);
							//check_for_error(rtl_status, "SW1 rtk_vlan_portPvid_set");
						} else {
							printf("Set port %d as member of vlan: %ld \n\r", user_sw_port, vlan_id);
							RTK_PORTMASK_PORT_SET(vlan->vlan_sw2.mbr, rtl_port);
							RTK_PORTMASK_PORT_SET(vlan->vlan_sw2.mbr, LINK_PORT);
							RTK_PORTMASK_PORT_SET(vlan->vlan_sw2.untag, rtl_port);
							RTK_PORTMASK_PORT_SET(vlan->vlan_sw1.mbr, LINK_PORT);
							//set_current_device(RTL_DEVICE_2);
							printf("Set portPvid %ld for port: %d \n\r", vlan_id, rtl_port);
							//rtk_api_ret_t rtl_status = rtk_vlan_portPvid_set(rtl_port, vlan_id, 0);
							//check_for_error(rtl_status, "SW2 rtk_vlan_portPvid_set");
						}
					}
					if (tkn2->type == LWJSON_TYPE_ARRAY) {
						//printf(": Token is array or object...check children tokens if any, in recursive mode..");
						/* Get first child of token */
						for (const lwjson_token_t *array = lwjson_get_first_child(tkn2); array != NULL; array = array->next) {
							rtk_vlan_t trunk_vlan_id = (rtk_vlan_t) lwjson_get_val_int(array);
							vlan_database_item_t* vlan = get_vlan_config(trunk_vlan_id);
							printf("Set vlan : %ld as trunk member of port: %d\n\r", trunk_vlan_id, user_sw_port);
							RTK_PORTMASK_PORT_SET(vlan->vlan_sw1.mbr, LINK_PORT);
							RTK_PORTMASK_PORT_SET(vlan->vlan_sw2.mbr, LINK_PORT);
							if (user_sw_port < 6) {
								RTK_PORTMASK_PORT_SET(vlan->vlan_sw1.mbr, rtl_port);
							} else {
								RTK_PORTMASK_PORT_SET(vlan->vlan_sw2.mbr, rtl_port);
							}
						}
					}
				}
			}
		}
		printf("Configuring vlans...\n\r");
		for (int current_vlan=0; current_vlan<MAX_VLANS; current_vlan++) {
			if (vlan_database[current_vlan].vid == 0) {
				break;
			}
			set_current_device(RTL_DEVICE_1);
			rtk_api_ret_t rtl_status = rtk_vlan_set(vlan_database[current_vlan].vid, &vlan_database[current_vlan].vlan_sw1);
			check_for_error(rtl_status, "SW1 rtk_vlan_set");
			for (int port = 0; port < 5; port++) {
				if (RTK_PORTMASK_IS_PORT_SET(vlan_database[current_vlan].vlan_sw1.untag, port)) {
					 rtl_status = rtk_vlan_portPvid_set(port, vlan_database[current_vlan].vid, 0);
				     check_for_error(rtl_status, "SW1 rtk_vlan_portPvid_set");
				 }
			}
			set_current_device(RTL_DEVICE_2);
			rtl_status = rtk_vlan_set(vlan_database[current_vlan].vid, &vlan_database[current_vlan].vlan_sw2);
			check_for_error(rtl_status, "SW2 rtk_vlan_set");
			for (int port = 0; port < 5; port++) {
				if (RTK_PORTMASK_IS_PORT_SET(vlan_database[current_vlan].vlan_sw2.untag, port)) {
					 rtl_status = rtk_vlan_portPvid_set(port, vlan_database[current_vlan].vid, 0);
				     check_for_error(rtl_status, "SW2 rtk_vlan_portPvid_set");
				 }
			}
		}
	}
}

void load_test_config(char *json_config) {


	rtk_vlan_cfg_t sw1_vlan10 = { 0 }, sw1_vlan11 = { 0 }, sw1_vlan240 = { 0 };
	RTK_PORTMASK_PORT_SET(sw1_vlan10.mbr, UTP_PORT0);
	RTK_PORTMASK_PORT_SET(sw1_vlan10.mbr, UTP_PORT2);
	RTK_PORTMASK_PORT_SET(sw1_vlan10.mbr, LINK_PORT);
	RTK_PORTMASK_PORT_SET(sw1_vlan10.untag, UTP_PORT0);

	RTK_PORTMASK_PORT_SET(sw1_vlan11.mbr, UTP_PORT1);
	RTK_PORTMASK_PORT_SET(sw1_vlan11.mbr, UTP_PORT2);
	RTK_PORTMASK_PORT_SET(sw1_vlan11.mbr, LINK_PORT);
	RTK_PORTMASK_PORT_SET(sw1_vlan11.untag, UTP_PORT1);


	RTK_PORTMASK_PORT_SET(sw1_vlan240.mbr, UTP_PORT0);
	RTK_PORTMASK_PORT_SET(sw1_vlan240.mbr, LINK_PORT);


	rtk_vlan_cfg_t sw2_vlan10 = { 0 }, sw2_vlan11 = { 0 }, sw2_vlan240 = { 0 };
	RTK_PORTMASK_PORT_SET(sw2_vlan10.mbr, UTP_PORT0);
	RTK_PORTMASK_PORT_SET(sw2_vlan10.mbr, UTP_PORT4);
	RTK_PORTMASK_PORT_SET(sw2_vlan10.mbr, LINK_PORT);
	RTK_PORTMASK_PORT_SET(sw2_vlan10.untag, UTP_PORT0);


	RTK_PORTMASK_PORT_SET(sw2_vlan11.mbr, UTP_PORT1);
	RTK_PORTMASK_PORT_SET(sw2_vlan11.mbr, UTP_PORT4);
	RTK_PORTMASK_PORT_SET(sw2_vlan11.mbr, LINK_PORT);
	RTK_PORTMASK_PORT_SET(sw2_vlan11.untag, UTP_PORT1);


	RTK_PORTMASK_PORT_SET(sw2_vlan240.mbr, UTP_PORT4);
	RTK_PORTMASK_PORT_SET(sw2_vlan240.mbr, LINK_PORT);

	set_current_device(RTL_DEVICE_1);

	rtk_api_ret_t status = rtk_vlan_set(10, &sw1_vlan10);
	check_for_error(status, "Set SW1 vlan10 ");
	status = rtk_vlan_set(11, &sw1_vlan11);
	check_for_error(status, "Set SW1 vlan11 ");
	status = rtk_vlan_set(240, &sw1_vlan240);
	check_for_error(status, "Set SW1 vlan240 ");


	set_current_device(RTL_DEVICE_2);
	status = rtk_vlan_set(10, &sw2_vlan10);
	check_for_error(status, "Set SW2 vlan10 ");
	status = rtk_vlan_set(11, &sw2_vlan11);
	check_for_error(status, "Set SW2 vlan11 ");
	status = rtk_vlan_set(240, &sw2_vlan240);
	check_for_error(status, "Set SW2 vlan240 ");


	set_current_device(RTL_DEVICE_1);
	/* set PVID for each port */
	status = rtk_vlan_portPvid_set(UTP_PORT0, 10, 0);
	check_for_error(status, "Set SW1 PID for port 0 ");
	status = rtk_vlan_portPvid_set(UTP_PORT1, 11, 0);
	check_for_error(status, "Set SW1 PID for port 1 ");

	set_current_device(RTL_DEVICE_2);
	/* set PVID for each port */
	status = rtk_vlan_portPvid_set(UTP_PORT0, 10, 0);
	check_for_error(status, "Set SW2 PID for port 0 ");
	status = rtk_vlan_portPvid_set(UTP_PORT1, 11, 0);
	check_for_error(status, "Set SW2 PID for port 1 ");


}

void load_switch_config() {

	set_current_device(RTL_DEVICE_1);
	rtk_api_ret_t rtl_status = rtk_switch_init();
	check_for_error(rtl_status, "SW1 rtk_switch_init");
	rtl_status = rtk_vlan_init();
	check_for_error(rtl_status, "SW1 VLANs initialized");
	printf("Configuring SW1 GMAC2 as MODE_EXT_RGMII\n\r");
	configure_rgmii_link();

	set_current_device(RTL_DEVICE_2);
	rtl_status = rtk_switch_init();
	check_for_error(rtl_status, "SW2 rtk_switch_init");
	rtl_status = rtk_vlan_init();
	check_for_error(rtl_status, "SW2 VLANs initialized");
	printf("Configuring SW2 GMAC2 as MODE_EXT_RGMII\n\r");
	configure_rgmii_link();

	parse_vlan_config(json_config);
	//load_test_config(json_config);
}

void set_current_device(uint8_t device) {
	if (RTL_DEVICE_1 == device) {
		printf("********* RTL_DEVICE_1 selected *********\n\r");
		rtkI2C_SCKport = RTK_I2C_SCK_PORT_1;
		rtkI2C_SDAport = RTK_I2C_SDA_PORT_1;
		rtkI2C_SCKpin = RTK_I2C_SCK_PIN_1;
		rtkI2C_SDApin = RTK_I2C_SDA_PIN_1;
	} else {
		printf("********* RTL_DEVICE_2 selected *********\n\r");
		rtkI2C_SCKport = RTK_I2C_SCK_PORT_2;
		rtkI2C_SDAport = RTK_I2C_SDA_PORT_2;
		rtkI2C_SCKpin = RTK_I2C_SCK_PIN_2;
		rtkI2C_SDApin = RTK_I2C_SDA_PIN_2;
	}
}
