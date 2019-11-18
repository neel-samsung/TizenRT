/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#ifndef __TIZENRT_NETMGR_H__
#define __TIZENRT_NETMGR_H__

#define NM_MAX_HWADDR_LEN 6

#define NM_FLAG_BROADCAST    0x02U
/** If set, the netif is an ethernet device using ARP.
 * Set by the netif driver in its init function.
 * Used to check input packet types and use of DHCP. */
#define NM_FLAG_ETHARP       0x08U
/** If set, the netif is an ethernet device. It might not use
 * ARP or TCP/IP if it is used for PPPoE only.
 */
#define NM_FLAG_ETHERNET     0x10U
/** If set, the netif has IGMP capability.
 * Set by the netif driver in its init function. */
#define NM_FLAG_IGMP         0x20U
/** If set, the netif has MLD6 capability.
 * Set by the netif driver in its init function. */
#define NM_FLAG_MLD6         0x40U

typedef enum {
	NM_LOOPBACK,
	NM_WIFI,
	NM_ETHERNET,
	NM_UNKNOWN,
} netdev_type;

typedef enum {
	/** Delete a filter entry */
	NM_DEL_MAC_FILTER = 0,
	/** Add a filter entry */
	NM_ADD_MAC_FILTER = 1
} netdev_mac_filter_action;

struct netdev {
	char ifname[IFNAMSIZ];
	netdev_type type;
	union {
		struct ethernet_ops *eth;
		struct trwifi_ops *wl;
	} t_ops;
	void *ops; /*	stack plane */
	uint8_t *tx_buf; // SET MTU Size
	void *priv;
};

struct nic_io_ops {
	int (*linkoutput)(struct netdev *dev, uint8_t *data, uint16_t len);
	int (*igmp_mac_filter)(struct netdev *netif, const struct in_addr *group, netdev_mac_filter_action action);
};

struct netdev_config {
	struct nic_io_ops *ops;
	int flag;
	int mtu;
	int hwaddr_len;
	uint8_t hwaddr[NM_MAX_HWADDR_LEN];

	int is_default;
	union {
		struct ethernet_ops *eth;
		struct trwifi_ops *wl;
	} t_ops;
	netdev_type type;

	int (*d_ioctl)(struct netdev *dev, int cmd, unsigned long arg); // SIOCSMIIREG
	void *priv;
};

/**
 * Public API
 *
 * Desc: provide these APIs to vendors who are poring NIC
 */

/*
 * desc: register a network device
 * return: return netif registered
 */
struct netdev *netdev_register(struct netdev_config *config);
/*
 * desc: NIC driver should call following fuction to pass the incoming data to network stack.
 */
int netdev_input(struct netdev *dev, uint8_t *data, uint16_t len);
/**
 * Configuration
 */
int netdev_set_hwaddr(struct netdev *dev, uint8_t *hwaddr, uint8_t hwaddr_len);
int netdev_get_hwaddr(struct netdev *dev, uint8_t *hwaddr, uint8_t *hwaddr_len);
int netdev_get_mtu(struct netdev *dev, int *mtu);

/*
 * Deprecate
 * return address of hwaddr
 * this function is provided to support slsi driver which is implemented before netmgr is appear.
 * this function should not be used to new devices
 */
uint8_t *netdev_get_hwaddr_ptr(struct netdev *dev);

#endif // __TIZENRT_NETMGR_H__