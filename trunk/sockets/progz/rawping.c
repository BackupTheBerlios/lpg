/*
   rawping.c - przyklad uzycia gniazd SOCK_RAW

   by Marcin Dawcewicz
*/

#include <sys/types.h>
#include <sys/socket.h>		/* socket()        */
#include <linux/in.h>
#include <linux/icmp.h>		/* struct icmphdr  */
#include <linux/ip.h>		/* struct iphdr    */
#include <netdb.h>		/* gethostbyname() */
#include <stdio.h>
#include <unistd.h>		/* alarm()         */
#include <signal.h>		/* signal()        */

#define TIMEO  2		/* limit czasu na odpowiedz */

static int
in_cksum (u_short * addr, int len)
{
  register int nleft = len;
  register u_short *w = addr;
  register int sum = 0;
  u_short answer = 0;

  /*
   * Our algorithm is simple, using a 32 bit accumulator (sum), we add
   * sequential 16 bit words to it, and at the end, fold back all the
   * carry bits from the top 16 bits into the lower 16 bits.
   */
  while (nleft > 1)
    {
      sum += *w++;
      nleft -= 2;
    }

  /* mop up an odd byte, if necessary */
  if (nleft == 1)
    {
      *(u_char *) (&answer) = *(u_char *) w;
      sum += answer;
    }
  /* add back carry outs from top 16 bits to low 16 bits */
  sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
  sum += (sum >> 16);		/* add carry */
  answer = ~sum;		/* truncate to 16 bits */
  return (answer);
}

int timeout;

void
alrm_handle (int signum)
{
  timeout = 1;
}

main (int argc, char *argv[])
{
  int sd, len, n;
  struct sockaddr_in haddr, raddr;
  struct icmphdr icmphd, *icmphp;
  struct iphdr *iphp;
  struct hostent *hent;
  char pktbuf[65536];

  if (argc != 2)
    {
      printf ("Uzycie:\n%s adres_hosta\n", argv[0]);
      exit (1);
    }

  sd = socket (PF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sd < 0)
    {
      perror ("socket()");
      exit (1);
    }

  hent = gethostbyname (argv[1]);
  if (!hent)
    {
      herror ("gethostbyname()");
      exit (1);
    }

  haddr.sin_family = PF_INET;
  haddr.sin_port = 0;
  bcopy (hent->h_addr, (char *) &haddr.sin_addr, hent->h_length);

  signal (SIGALRM, &alrm_handle);

  for (n = 1; n < 4; n++)
    {
      int resp = 0;

      memset (&icmphd, 0, sizeof (icmphd));
      icmphd.type = ICMP_ECHO;
      icmphd.code = 0;
      icmphd.un.echo.id = htons (666);
      icmphd.un.echo.sequence = htons (n);
      icmphd.checksum = in_cksum ((u_short *) & icmphd, sizeof (icmphd));
      bcopy (&icmphd, pktbuf, sizeof (icmphd));

      printf ("Wysylanie ICMP Echo Request nr. %i ... ", n);
      fflush (stdout);

      sendto (sd, pktbuf, sizeof (struct icmphdr), 0,
	      (struct sockaddr *) &haddr, sizeof (haddr));

      len = sizeof (raddr);

      alarm (TIMEO);
      timeout = 0;

      while (recvfrom
	     (sd, pktbuf, sizeof (pktbuf), 0, (struct sockaddr *) &raddr,
	      &len) > 0)
	{
	  iphp = (struct iphdr *) pktbuf;
	  icmphp = (struct icmphdr *) ((char *) iphp + 4 * iphp->ihl);

	  if (ntohs (icmphp->un.echo.id) == 666
	      && ntohs (icmphp->un.echo.sequence) == n
	      && raddr.sin_addr.s_addr == haddr.sin_addr.s_addr
	      && icmphp->type == ICMP_ECHOREPLY)
	    {
	      printf ("Odpowiedz otrzymana.\n");
	      resp = 1;
	      alarm (0);
	      break;
	    }
	  if (timeout)
	    break;
	}

      if (!resp)
	printf ("Uplynal limit czasu.\n");
    }
}
