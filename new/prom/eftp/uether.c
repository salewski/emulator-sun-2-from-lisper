/* Copyright (C) 1981 by Stanford University */

/* LINTLIBRARY */
/*
 * uether.c
 *
 * micro version of ethernet code for MC68000 ethernet
 * bootstrap loader.
 *
 * Jeffrey Mogul @ Stanford	16 March 1981
 *			revised 21 April 1981
 * Small changes made by Ross Finlayson - March, 1984.
 */

#include <m68enet.h>		/* Include interface bit definitions */
#include <puppacket.h>
#include <pupstatus.h>

#define PCKTMAX		280	/* Word count, not byte count */

/*
 * engethost.c
 *
 * MC68000 version
 *
 * Returns the Ethernet host number of the local machine.
 * Very specific to NON-INTERRUPT DRIVEN 68000.
 *
 * Jeffrey Mogul @ Stanford	2 March 1981
 *
 */

unsigned char engethost(fid)
int	fid;		/* file id of an ethernet device  (IGNORED) */
{
	return(EIAddress);
}

/* enetaddress() is another version of the above, which returns its result in
 * a return parameter. This is for compatibility with 10Mb drivers, which can't
 * return an Ethernet address, since 10Mb addresses are bigger than 32 bits.
 */
enetaddress(enetAddr)
    unsigned char *enetAddr;
  {
    *enetAddr = EIAddress;
  }


/*
 * enopen.c
 *	version for MC68000
 *
 * In the MC68000 version, this just initializes the ethernet
 * interface, if this hasn't been done already.  In this minimal
 * version, it doesn't even check for repeats.
 *
 */


enopen() {

	if ( !ProbeAddress(&(EIStatus)) ) /* Make sure the Ethernet exists. */
	    return (0);
	mc68keninit();
	return(1);	/* caller is expecting a positive number */
}

/*
 * mc68kenetinit
 *
 * initializes MC68000 ethernet interface
 *
 * Jeffrey Mogul @ Stanford	27 February 1981
 *
 * stolen from code written by Mike Nielsen
 */

mc68keninit()
{
	register short address;
	register short wordbucket;

	EIStatus = Init1+IntEnable0+FilterData0+Loopback0;
	for (address=0; address<256; address++)	/* Clear Address Filter */
		EIAddress = address;
	address = EIAddress;			/* Activate Switch Address */
 	EIStatus = Init1+IntEnable0+FilterData1+Loopback0;
	EIAddress = address;
	EIAddress = 0;	/* also accept broadcasts */
	EIStatus = Init0+IntEnable0+Loopback0+IntLevel2;
		/* don't use interrupts in this package */
	wordbucket = EIData0; /* Synchronize data port lookahead */
}

/*
 * enread.c	THIRD VERSION
 *	version for MC68000
 *
 * reads an ether packet from an ethernet file.  The timeout argument
 * is in clock ticks (1/60th of a second), more or less.
 *
 * JKS sometime in early 1980
 * modified by Jeffrey Mogul 23-June-1980
 * again by Jeff Mogul & Dan Kolkowitz	12-Jan-81
 */

enread (fid, packet, filter, timeout)
int	fid;			/* file id of ether */
register struct	EnPacket *packet;	/* packet to read */
register struct	enfilter *filter;	/* filter to use on read */
int	timeout;		/* timeout interval in clock ticks */
{	/* */
	register int done = 0;

	while (!done) {
		if (mc68kenread(packet, &timeout, PCKTMAX) < 0) return(TIMEOUT);

		/* got a packet -- is it a good one? */

		if (filter->mask&ENPTYPE)	/* check enet packet type */
		    if (packet->PacketType != filter->enptype) continue;

		if (filter->mask&DSTSOCK)	/* check Pup dest socket */
		    if (getlong(
	    		*( (unsigned long*)&packet->Pup.PupDst.pk_FSocket))
		    	  != filter->dstsock) continue;

		if (filter->mask&DSTHOST)	/* check Pup dest host */
		    if (packet->Pup.PupDst.pk_Host != filter->dsthost)
				continue;

		done++;	/* got a good one */
		}
	
	return(OK);
}

/* bug - when a packet is discarded, the timer is reset */

/*
 * mc68kenread
 *
 * read a packet from the ethernet - MC68000 version
 *
 * Jeffrey Mogul @ Stanford	27 February 1981
 *
 * stolen from code written by Mike Nielsen
 */

/*#define	CLOCKTICK 825	/* loop this many times for 1/60 second */
#define	CLOCKTICK	512	/* this is more like 1/100 second, but
				 * it saves us having to load the lmul
				 * routine (i.e., several dozen bytes of
				 * code.
				 */

mc68kenread(buffer,timeout, maxShorts)
register unsigned short *buffer;		/* address of buffer */
register int *timeout;		/* timeout for read -- 1/60 sec units */
int maxShorts;			/* maximum number of shorts to read in. */
{
	int retstatus = TIMEOUT;	/* assume timeout */
	register int i;
	register long j;		/* MUST be long */
	register short rxstatus;
	register short dum;
	register short rxcount;
	register unsigned short *tp;


	for (j=(*timeout * CLOCKTICK); j>0 ; j--)
	    if (EIStatus&RxRcvd) {	/* something in queue */
	        rxstatus = EIData0;
	        while (rxstatus&QueueEmptyBit)
		    rxstatus = EIData0;	/* get real header word */

		rxcount = rxstatus&CountMask;	/* get word count */

		if (rxcount > maxShorts)
		    rxcount = maxShorts;
		if ( !(rxstatus&(OverflowBit|CollisionBit|CRCerrorBit))) {
			/* good packet found, copy to user buffer */
			tp = buffer;
			for (i=0; i<rxcount; i++) *tp++ = EIData0;
			retstatus = rxcount;	/* return word count */
			*timeout = j/CLOCKTICK; /* return how much time left. */
			break;	/* out of busy-wait loop */
		}
		else {	/* bad packet or too long, drop it */
			for (i=0; i<rxcount; i++) dum = EIData0;
		}
	}

    return(retstatus);
}

/*
 * enwrite.c
 *	version for MC68000
 *
 * writes an ether packet to an ether host via an ethernet fid.
 * The length argument is the length of the encapsulated
 * part of the packet, not the total length. This had better
 * be an even value.
 *
 * JKS sometime in early 1980
 * Jeffrey Mogul & Dan Kolkowitz	12-Jan-81
 */


enwrite (DstHost, Ptype, fid, packet, len)
register Host	DstHost;			/* destination host */
register ushort	Ptype;				/* packet type */
register int	fid;				/* file id to write to */
register struct	EnPacket *packet;		/* Packet to write */
register int	len;				/* length of encapsulated 
					 * part of packet */
{	/* */
	packet->PacketSrcHost = engethost(fid);
	packet->PacketDstHost = DstHost;
	packet->PacketType = Ptype;

	/* The length had better be even!! */
	mc68kenwrite(packet,(ENPACKOVER+len)>>1);
		/* mc68kenwrite takes a word count */

	return (OK);
}

/*
 * mc68kenwrite
 *
 * write a packet to the ethernet - MC68000 version
 *
 * Jeffrey Mogul @ Stanford	27 February 1981
 *
 * stolen from code written by Mike Nielsen
 */


mc68kenwrite(buffer,count)
register unsigned short *buffer;		/* address of buffer */
register int count;			/* WORD count of buffer */
{
	register int i;

	EIData0 = count;	/* set count word */
	for (i=0; i<count; i++) EIData0 = buffer[i];	/* move data words */
	
	while (!(EIStatus&TSent));	/* busy-wait until done */

}