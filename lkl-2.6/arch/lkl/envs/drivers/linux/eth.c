#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <poll.h>
#include <net/if.h>

#include <asm/env.h>
#include <asm/eth.h>
#include <asm/callbacks.h>

struct handle {
	pthread_t thread;
	int ifindex, sock;
	int pod[2]; /* pipe of death */
	void *netdev;
};

static void* rx_thread(void *handle)
{
	struct handle *h=(struct handle*)handle;
	struct lkl_eth_desc *led=NULL;
	struct pollfd pfd[] = {
		{ .fd = h->sock, .events = POLLIN, .revents = 0 },
		{ .fd = h->pod[0], .events = POLLIN, .revents = 0 }
	};

	lkl_printf("lkl eth: starting rx thread\n");

	while (1) {
		int len;

		poll(pfd, 2, -1);

		if (pfd[1].revents)
			break;


		if (!led) {
			led=lkl_eth_get_rx_desc(h->netdev);
			if (!led) {
				/* Drop the packet. FIXME: It would be nice
				 * to have a counter for this, maybe part of the
				 * Linux interface stats?  */
				recv(h->sock, NULL, 0, 0);
				continue;
			}
		}

		len=recv(h->sock, led->data, led->len, 0);

		if (len > 0) {
			led->len=len; 
			lkl_trigger_irq_with_data(ETH_IRQ, led);
			led=NULL;
		} else 
			lkl_printf("lkl eth: failed to receive: %s\n", strerror(errno));
	}

	lkl_printf("lkl eth: stopping rx thread\n");

	return NULL;
}

void* lkl_eth_native_init(void *netdev, const char *native_dev)
{
	struct sockaddr_ll saddr = {
		.sll_family = AF_PACKET,
		.sll_ifindex = 	if_nametoindex(native_dev),
		.sll_protocol = htons(ETH_P_ALL)
	};
	struct handle *h;

	if (!(h=malloc(sizeof(*h)))) {
		lkl_printf("lkl eth: failed to allocate memory\n");
		goto out;
	}

	h->netdev=netdev;

	if (!(h->ifindex=saddr.sll_ifindex)) {
		lkl_printf("lkl eth: bad interface %s\n", native_dev);
		goto out_free_h;
	}

	h->sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (h->sock < 0) {
                lkl_printf("lkl eth: failed to create socket: %s\n", strerror(errno));
		goto out_free_h;
        }

        if (bind(h->sock, (const struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
                lkl_printf("lkl eth: failed to bind socket: %s\n", strerror(errno));
		goto out_close_sock;
        }

	if (pipe(h->pod) < 0) {
                lkl_printf("lkl eth: pipe() failed: %s\n", strerror(errno));
		goto out_close_sock;
	}

	if (pthread_create(&h->thread, NULL, rx_thread, h) < 0) {
		lkl_printf("lkl eth: failed to create thread\n");
		goto out_close_pod;
	}

	return h;

out_close_pod:
	close(h->pod[0]); close(h->pod[1]);
out_close_sock:
	close(h->sock);
out_free_h:
	free(h);
out:
	return NULL;

	return 0;
}

void lkl_eth_native_cleanup(void *handle)
{
	struct handle *h=(struct handle*)handle;
	char c;

	write(h->pod[1], &c, 1);

	pthread_join(h->thread, NULL);

	close(h->pod[0]); close(h->pod[1]);

	free(h);
}

int lkl_eth_native_xmit(void *handle, const char *data, int len)
{
	struct handle *h=(struct handle*)handle;
	struct sockaddr_ll saddr = {
		.sll_family = AF_PACKET,
		.sll_halen = 6,
		.sll_ifindex = h->ifindex,
	};
	int err;

	memcpy(saddr.sll_addr, data, saddr.sll_halen);
	err=sendto(h->sock, data, len, 0, (const struct sockaddr*)&saddr, sizeof(saddr));
	if (err >= 0) 
		return 0;

	lkl_printf("lkl eth: failed to send: %s\n", strerror(errno));
	return err;
}




