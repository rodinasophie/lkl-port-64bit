#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <asm/irq_regs.h>
#include <linux/string.h>

#include <asm/eth.h>
#include <asm/callbacks.h>

#define MAX_FRAME_LENGTH 1518

struct netdev_data {
	void *native_handle;
	const char *native_dev;
	struct list_head rx_ring_ready, rx_ring_inuse;
	void *rx_ring_lock;
};

struct rx_ring_entry {
	struct lkl_eth_desc led;
	struct net_device *netdev;
	struct sk_buff *skb;
	struct list_head list;
};

static int start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct netdev_data *nd=(struct netdev_data*)netdev_priv(netdev);
	int err=lkl_eth_native_xmit(nd->native_handle, skb->data, skb->len);

	if (err == 0)
		dev_kfree_skb(skb);

	return err;
}

static int reload_rx_ring_entry(struct net_device *netdev,
				struct rx_ring_entry *rre)
{
	struct netdev_data *nd=(struct netdev_data*)netdev_priv(netdev);
	struct sk_buff *skb=dev_alloc_skb(MAX_FRAME_LENGTH);

	if (!skb)
		return -ENOMEM;

	rre->led.data=skb->data;
	rre->led.len=MAX_FRAME_LENGTH;
	rre->skb=skb;

	lkl_nops->sem_down(nd->rx_ring_lock);
	/* remove it from the inuse list */
	list_del(&rre->list);
	/* and put it on the ready list */
	list_add(&rre->list, &nd->rx_ring_ready);
	lkl_nops->sem_up(nd->rx_ring_lock);
	
	return 0;
}

struct lkl_eth_desc* lkl_eth_get_rx_desc(void *_netdev)
{
	struct net_device *netdev=(struct net_device*)_netdev;
	struct netdev_data *nd=(struct netdev_data*)netdev_priv(netdev);
	struct rx_ring_entry *rre=NULL;

	lkl_nops->sem_down(nd->rx_ring_lock);
	if (!list_empty(&nd->rx_ring_ready)) {
		rre=list_first_entry(&nd->rx_ring_ready, struct rx_ring_entry, list);
		list_del(&rre->list);
		/* don't forget to add it on the inuse list */
		list_add(&rre->list, &nd->rx_ring_inuse);
	}
	lkl_nops->sem_up(nd->rx_ring_lock);

	if (rre)
		return &rre->led;

	return NULL;
}

static irqreturn_t lkl_net_irq(int irq, void *dev_id)
{
	struct pt_regs *regs=get_irq_regs();
	struct lkl_eth_desc *led=(struct lkl_eth_desc*)regs->irq_data;
	struct rx_ring_entry *rre=container_of(led, struct rx_ring_entry, led);

	skb_put(rre->skb, rre->led.len);
	rre->skb->protocol=eth_type_trans(rre->skb, rre->netdev);

	netif_rx(rre->skb);

	reload_rx_ring_entry(rre->netdev, rre);

	return IRQ_HANDLED;
}

static int failed_init=1;

static void cleanup_netdev(struct net_device *netdev)
{
	struct netdev_data *nd=(struct netdev_data*)netdev_priv(netdev);
	struct list_head *i, *aux;

	if (nd->native_handle)
		lkl_eth_native_cleanup(nd->native_handle);

	list_for_each_safe(i, aux, &nd->rx_ring_inuse) {
		struct rx_ring_entry *rre=list_entry(i, struct rx_ring_entry, list);
		list_del(&rre->list);
		kfree(rre);
	}

	list_for_each_safe(i, aux, &nd->rx_ring_ready) {
		struct rx_ring_entry *rre=list_entry(i, struct rx_ring_entry, list);
		list_del(&rre->list);
		kfree(rre);
	}

	if (nd->rx_ring_lock)
		lkl_nops->sem_free(nd->rx_ring_lock);

	kfree(netdev);
}

int _lkl_del_eth(int ifindex)
{
	struct net_device *netdev=dev_get_by_index(&init_net, ifindex);

	if (!netdev)
		return -1;

	dev_put(netdev);

	unregister_netdev(netdev);

	cleanup_netdev(netdev);

	return 0;
}

static int open(struct net_device *netdev)
{
	struct netdev_data *nd=(struct netdev_data*)netdev_priv(netdev);
	
	if (!(nd->native_handle=lkl_eth_native_init(netdev, nd->native_dev)))
		return -EUNATCH;

	return 0;
}

static int stop(struct net_device *netdev)
{
	struct netdev_data *nd=(struct netdev_data*)netdev_priv(netdev);

	lkl_eth_native_cleanup(nd->native_handle);
	nd->native_handle=NULL;

	lkl_purge_irq_queue(ETH_IRQ);

	return 0;
}

const static struct net_device_ops lkl_eth_ops = {
	.ndo_open = open,
	.ndo_stop = stop,
	.ndo_start_xmit = start_xmit
};

int _lkl_add_eth(const char *native_dev, const char *mac, int rx_ring_len)
{
	struct net_device *netdev;
	struct netdev_data *nd;
	int i;

	if (failed_init)
		return -EUNATCH;

	if (!(netdev=alloc_etherdev(sizeof(struct netdev_data)))) 
		return -ENOMEM;

	nd=(struct netdev_data*)netdev_priv(netdev);

	memset(nd, 0, sizeof(*nd));

	INIT_LIST_HEAD(&nd->rx_ring_ready);
	INIT_LIST_HEAD(&nd->rx_ring_inuse);

	if (!(nd->rx_ring_lock=lkl_nops->sem_alloc(1))) 
		goto error;

	for(i=0; i<rx_ring_len; i++) {
		struct rx_ring_entry *rre=kmalloc(sizeof(*rre), GFP_KERNEL);
		if (!rre) 
			goto error;
		rre->netdev=netdev;
		list_add(&rre->list, &nd->rx_ring_inuse);
		if (reload_rx_ring_entry(netdev, rre) < 0) 
			goto error;

	}

	memcpy(netdev->dev_addr, mac, 6);

	nd->native_dev=kstrdup(native_dev, GFP_KERNEL);
	netdev->netdev_ops = &lkl_eth_ops;

	if (register_netdev(netdev)) 
		goto error;

	printk(KERN_INFO "lkl eth: registered device %s / %s\n", netdev->name,
	       native_dev);

	return netdev->ifindex;

error:
	cleanup_netdev(netdev);
	
	return 0;
}

int eth_init(void)
{
	int err=-ENOMEM;

	if ((err=request_irq(ETH_IRQ, lkl_net_irq, 0, "lkl_net", NULL))) {
		printk(KERN_ERR "lkl eth: unable to register irq %d: %d\n",
		       ETH_IRQ, err);
		return err;
	}

	return failed_init=0;
}

late_initcall(eth_init);
