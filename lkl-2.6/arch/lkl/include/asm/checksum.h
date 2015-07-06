#ifndef _ASM_LKL_CHECKSUM_H
#define _ASM_LKL_CHECKSUM_H


static inline __sum16 csum_fold(__wsum csum)
{
	u32 sum = (__force u32)csum;
	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	return (__force __sum16)~sum;
}

__wsum csum_partial(const void *buff, int len, __wsum sum);

static inline __sum16 ip_fast_csum(const void *iph, unsigned int ihl)
{
	return csum_fold(csum_partial(iph, ihl*4, 0));
}

/*
 * the same as csum_partial, but copies from src while it
 * checksums
 *
 * here even more important to align src and dst on a 32-bit (or even
 * better 64-bit) boundary
 */
static inline __wsum
csum_partial_copy_nocheck(const void *src, void *dst, int len, __wsum sum)
{
	memcpy(dst, src, len);
	return csum_partial(dst, len, sum);
}


/*
 * the same as csum_partial_copy, but copies from user space.
 *
 * here even more important to align src and dst on a 32-bit (or even
 * better 64-bit) boundary
 */
static inline __wsum
csum_partial_copy_from_user(const void __user *src, void *dst,
			    int len, __wsum sum, int *csum_err)
{
	int rem;

	if (csum_err)
		*csum_err = 0;

	rem = copy_from_user(dst, src, len);
	if (rem != 0) {
		if (csum_err)
			*csum_err = -EFAULT;
		memset(dst + len - rem, 0, rem);
		len = rem;
	}

	return csum_partial(dst, len, sum);
}


static inline 
__wsum csum_tcpudp_nofold(__be32 saddr, __be32 daddr,
				   unsigned short len,
				   unsigned short proto,
				   __wsum sum)
{
	u16 pseudo_header[] = { saddr >> 16, saddr & 0xffff, 
				daddr >> 16, daddr & 0xffff, 
				htons(proto & 0xff), htons(len) };
				
	return csum_partial(pseudo_header, sizeof(pseudo_header), sum);
}


static inline __sum16
csum_tcpudp_magic(__be32 saddr, __be32 daddr, unsigned short len,
		  unsigned short proto, __wsum sum)
{
	return csum_fold(csum_tcpudp_nofold(saddr,daddr,len,proto,sum));
}




/*
 * this routine is used for miscellaneous IP-like checksums, mainly
 * in icmp.c
 */
extern __sum16 ip_compute_csum(const void *buff, int len);



#endif
