/*
   arpscan.c - wyszukiwanie wszystkich obslugujacych stos TCP/IP 
               urzadzen w sieci lokalnej

   by Marcin Dawcewicz

   Uzycie: ./arpscan interfejs

*/

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define TIMEOUT  10		/* Limit czasowy na odpowiedzi (sekundy) */

int sd;

/* Pobranie adresu MAC, adresu IP oraz indeksu interfesju */
void
get_if_data (char *ifname, struct sockaddr_ll *hwaddr, struct in_addr *ip)
{
  struct ifreq ifr;

  sprintf (ifr.ifr_name, "%s", ifname);

  if (ioctl (sd, SIOCGIFHWADDR, &ifr))
    {
      perror ("ioctl()");
      exit (1);
    }

  memcpy (hwaddr->sll_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
  ioctl (sd, SIOCGIFINDEX, &ifr);
  hwaddr->sll_ifindex = ifr.ifr_ifindex;
  ioctl (sd, SIOCGIFADDR, &ifr);
  memcpy ((char *) ip, ifr.ifr_addr.sa_data + 2, 4);
}

/* Obsluga nadchodzacych odpowiedzi */
void
io_handle (int a)
{
  char buf[1024];
  struct in_addr ipadd;
  struct ethhdr *ethdp;
  struct arphdr *arphdp;

  recv (sd, buf, sizeof (buf), 0);

  ethdp = (struct ethhdr *) buf;
  arphdp = (struct arphdr *) (buf + sizeof (struct ethhdr));

  if (ntohs (arphdp->ar_op) == ARPOP_REPLY)
    {
      memcpy ((char *) &ipadd, (char *) arphdp + sizeof (struct arphdr) +
	      ETH_ALEN, sizeof (ipadd));
      printf ("%s => %s\n", inet_ntoa (ipadd),
	      ether_ntoa ((struct ether_addr *) &ethdp->h_source));
    }
}

int
main (int argc, char *argv[])
{
  struct in_addr ipaddr, ifaddr, netmask;
  char pktbuf[1024];
  struct sockaddr_ll haddr, saddr;
  struct ethhdr ethd;
  struct arphdr arphd;
  char braddr[ETH_ALEN] = "\xff\xff\xff\xff\xff\xff";

  netmask.s_addr = htonl (0xffffff00);	/* Klasa C */

  if (argc != 2)
    {
      printf ("Uzycie:\n%s interfejs\n", argv[0]);
      exit (1);
    }

  if (signal (SIGRTMIN, &io_handle))
    {
      perror ("sigaction()");
      exit (1);
    }

  sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ARP));
  if (sd < 0)
    {
      perror ("socket()");
      exit (1);
    }

  /* Pobranie informacji o interfejsie                */
  get_if_data (argv[1], &saddr, &ifaddr);

  /* Zadanie asynchronicznego I/O                     */
  fcntl (sd, F_SETFL, O_ASYNC | O_NONBLOCK);
  fcntl (sd, F_SETOWN, getpid ());
  fcntl (sd, 10, SIGRTMIN);

  /* Czas na wyslanie kilku ramek w siec	      */
  for (ipaddr.s_addr = ifaddr.s_addr & netmask.s_addr;
       ipaddr.s_addr < (ifaddr.s_addr | ~netmask.s_addr);
       ipaddr.s_addr = ipaddr.s_addr + htonl (1))
    {
      memset (pktbuf, 0, sizeof (pktbuf));

      /* naglowek Ethernet  */
      memcpy (ethd.h_dest, braddr, ETH_ALEN);
      memcpy (ethd.h_source, saddr.sll_addr, ETH_ALEN);
      ethd.h_proto = htons (ETH_P_ARP);
      memcpy (pktbuf, &ethd, sizeof (ethd));

      /* naglowek ARP  */
      memset (&arphd, 0, sizeof (arphd));
      arphd.ar_hrd = htons (ARPHRD_ETHER);
      arphd.ar_pro = htons (ETH_P_IP);
      arphd.ar_hln = ETH_ALEN;
      arphd.ar_pln = 4;
      arphd.ar_op = htons (ARPOP_REQUEST);
      memcpy ((char *) &arphd + sizeof (arphd), saddr.sll_addr, ETH_ALEN);
      memset ((char *) &arphd + sizeof (arphd) + ETH_ALEN, 0, 4);
      memset ((char *) &arphd + sizeof (arphd) + ETH_ALEN + 4, 0, ETH_ALEN);
      memcpy ((char *) &arphd + sizeof (arphd) + ETH_ALEN + 4 + ETH_ALEN,
	      &ipaddr, 4);
      memcpy (pktbuf + 14, &arphd, sizeof (arphd) + 2 * 4 + 2 * ETH_ALEN);

      /* Adres docelowy */
      memset (&haddr, 0, sizeof (haddr));
      haddr.sll_family = AF_PACKET;
      haddr.sll_protocol = htons (ETH_P_ARP);
      haddr.sll_ifindex = saddr.sll_ifindex;
      sendto (sd, pktbuf,
	      sizeof (ethd) + sizeof (arphd) + 2 * 4 + 2 * ETH_ALEN, 0,
	      (struct sockaddr *) &haddr, sizeof (haddr));
    }

  /* Czekamy na odpowiedzi */
  alarm (TIMEOUT);
  while (1);

  return 0;
}
