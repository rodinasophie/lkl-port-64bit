#ifndef _LKL_ETH_H
#define _LKL_ETH_H

int _lkl_add_eth(const char *native_dev, const char *mac, int rx_ring_len);
int _lkl_del_eth(int ifindex);

int lkl_eth_native_xmit(void *handle, const char *data, int len);
void* lkl_eth_native_init(void *netdev, const char *native_dev);
void lkl_eth_native_cleanup(void *handle);

struct lkl_eth_desc {
	char *data;
	int len;
};
struct lkl_eth_desc* lkl_eth_get_rx_desc(void *netdev);

#ifndef __KERNEL__
#include <asm/lkl.h>
static inline int lkl_add_eth(const char *native_dev, const char *mac, int rx_ring_len)
{
	return lkl_sys_call((long)_lkl_add_eth, (long)native_dev, (long)mac,
			    rx_ring_len, 0, 0);
}
static inline int lkl_del_eth(int ifindex)
{
	return lkl_sys_call((long)_lkl_del_eth, ifindex, 0, 0, 0, 0);
}
#endif

#endif
