/*
   arpreq.c - przyklad uzycia gniazd PF_PACKET

   by Marcin Dawcewicz
*/

#include <sys/types.h>
#include <sys/socket.h>		/* socket()           */
#include <sys/time.h>		/* struct timeval     */
#include <linux/if_ether.h>	/* struct ethhdr      */
#include <linux/if_packet.h>	/* struct sockaddr_ll */
#include <linux/if_arp.h>	/* struct arphdr      */

#define TIMEOUT 2
#define SRCETHADDR "\x00\x80\x48\xc9\x4e\xd8"
#define DSTETHADDR "\xff\xff\xff\xff\xff\xff"


char *
eth_ntoa (unsigned char n[ETH_ALEN])
{
  static char buf[64];

  sprintf (buf, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", n[0], n[1], n[2], n[3], n[4],
	   n[5]);

  return buf;
}

main (int argc, char *argv[])
{
  int sd, ret;
  struct ethhdr ethd, *ethdp;
  struct arphdr arphd, *arphdp;
  char ethsaddr[ETH_ALEN], ethdaddr[ETH_ALEN];
  unsigned long ipaddr;
  struct sockaddr_ll haddr;
  char pktbuf[1024];
  fd_set fds;
  struct timeval tv;

  if (argc != 2)
    {
      printf ("Uzycie:\n%s IP\n", argv[0]);
      exit (1);
    }

  if ((ipaddr = inet_addr (argv[1])) < 0)
    {
      perror ("inet_addr()");
      exit (1);
    }

  bcopy (SRCETHADDR, ethsaddr, ETH_ALEN);
  bcopy (DSTETHADDR, ethdaddr, ETH_ALEN);

  sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ARP));
  if (sd < 0)
    {
      perror ("socket()");
      exit (1);
    }

  memset (pktbuf, 0, sizeof (pktbuf));
  memcpy (ethd.h_dest, ethdaddr, ETH_ALEN);
  memcpy (ethd.h_source, ethsaddr, ETH_ALEN);
  ethd.h_proto = htons (ETH_P_ARP);
  memcpy (pktbuf, &ethd, sizeof (ethd));

  memset (&arphd, 0, sizeof (arphd));
  arphd.ar_hrd = htons (ARPHRD_ETHER);
  arphd.ar_pro = htons (ETH_P_IP);
  arphd.ar_hln = ETH_ALEN;
  arphd.ar_pln = 4;
  arphd.ar_op = htons (ARPOP_REQUEST);
  memcpy ((char *) &arphd + sizeof (arphd), ethsaddr, ETH_ALEN);
  memset ((char *) &arphd + sizeof (arphd) + ETH_ALEN, 0, 4);
  memset ((char *) &arphd + sizeof (arphd) + ETH_ALEN + 4, 0, ETH_ALEN);
  memcpy ((char *) &arphd + sizeof (arphd) + ETH_ALEN + 4 + ETH_ALEN, &ipaddr,
	  4);
  memcpy (pktbuf + 14, &arphd, sizeof (arphd) + 2 * 4 + 2 * ETH_ALEN);

  memset (&haddr, 0, sizeof (haddr));
  haddr.sll_family = AF_PACKET;
  haddr.sll_protocol = htons (ETH_P_ARP);
  haddr.sll_ifindex = 2;

  if (sendto
      (sd, pktbuf, sizeof (ethd) + sizeof (arphd) + 2 * 4 + 2 * ETH_ALEN, 0,
       (struct sockaddr *) &haddr, sizeof (haddr)) < 0)
    {
      perror ("sendto()");
      exit (1);
    }

  FD_ZERO (&fds);
  FD_SET (sd, &fds);

  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;

  ret = select (sd + 1, &fds, NULL, NULL, &tv);
  if (ret < 0)
    {
      perror ("select()");
      exit (1);
    }
  else if (ret == 0)
    printf ("Uplynal limit czasu.\n");
  else
    {
      recv (sd, pktbuf, sizeof (pktbuf), 0);

      ethdp = (struct ethhdr *) pktbuf;
      arphdp = (struct arphdr *) (pktbuf + sizeof (struct ethhdr));

      if (ntohs (arphdp->ar_op) == ARPOP_REPLY
	  && !memcmp ((char *) arphdp + sizeof (struct arphdr) + ETH_ALEN,
		      &ipaddr, 4))
	{
	  printf ("IP\t: %s\n", argv[1]);
	  printf ("MAC\t: %s\n", eth_ntoa (ethdp->h_source));
	}
    }

  exit (0);
}
