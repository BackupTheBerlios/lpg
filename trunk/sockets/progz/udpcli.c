/*
   udpcli.c - przyklad klienta UDP

   by Marcin Dawcewicz

*/

#define _XOPEN_SOURCE
#include <unistd.h>		/* crypt()                                  */
#include <sys/types.h>
#include <sys/socket.h>		/* socket(), connect(), recv(), send()      */
#include <linux/in.h>		/* struct sockaddr_in                       */
#include <stdio.h>		/* perror()                                 */
#include <netdb.h>		/* gethostbyname()                          */

#define SRVPORT 1111		/* numer portu serwera                      */
#define CLIPORT 31337		/* numer portu gniazda klienta              */
#define SRVADDR "localhost"	/* adres serwera                            */
#define PASSWD  "sikret"	/* haslo wymagane do polaczenia z serwerem  */
#define CSALT   "Zz"		/* dwuznakowy tzw. salt dla funkcji crypt() */

main ()
{
  int sd, len;
  struct sockaddr_in saddr, caddr;
  struct hostent *sent;
  char buf[1024];

  sd = socket (PF_INET, SOCK_DGRAM, 0);
  if (sd < 0)
    {
      perror ("socket()");
      exit (1);
    }

  caddr.sin_family = PF_INET;
  caddr.sin_port = htons (CLIPORT);
  caddr.sin_addr.s_addr = INADDR_ANY;

  if (bind (sd, (struct sockaddr *) &caddr, sizeof (caddr)) < 0)
    {
      perror ("bind()");
      exit (1);
    }

  printf ("Szukam adresu IP serwera %s ...\n", SRVADDR);
  sent = gethostbyname (SRVADDR);
  if (!sent)
    {
      herror ("gethostbyname()");
      exit (1);
    }

  saddr.sin_family = PF_INET;
  saddr.sin_port = htons (SRVPORT);
  bcopy (sent->h_addr, (char *) &saddr.sin_addr, sent->h_length);

  bzero (buf, sizeof (buf));
  sprintf (buf, "%s", crypt (PASSWD, CSALT));

//connect (sd, (struct sockaddr *)&saddr,sizeof(saddr));

  printf ("Wysylam haslo do %s:%i ...\n", inet_ntoa (saddr.sin_addr.s_addr),
	  SRVPORT);
  sendto (sd, buf, strlen (buf), 0, (struct sockaddr *) &saddr,
	  sizeof (saddr));

  printf ("Czekam na odpowiedz ...\n");
  do
    {
      bzero (buf, sizeof (buf));
      len = sizeof (caddr);
      recvfrom (sd, buf, sizeof (buf), 0, (struct sockaddr *) &caddr, &len);
    }
  while (caddr.sin_addr.s_addr != saddr.sin_addr.s_addr);

  if (strncmp (buf, "OK", 2))
    {
      fprintf (stderr,
	       "Nieprawidlowe haslo albo zly port zrodlowy. Koncze ...\n");
      exit (1);
    }
  else
    printf ("Polaczenie przyjete.\n\n");

  bzero (buf, sizeof (buf));
  len = sizeof (caddr);
  while (recvfrom (sd, buf, sizeof (buf), 0, (struct sockaddr *) &caddr, &len)
	 > 0)
    {
      if (caddr.sin_addr.s_addr == saddr.sin_addr.s_addr)
	{
	  if (!strncmp (buf, "END", 3))
	    break;
	  else if (!strncmp (buf, "Blad:", 5))
	    {
	      printf ("%s", buf);
	      break;
	    }
	  else
	    printf ("%s", buf);
	}
      bzero (buf, sizeof (buf));
      len = sizeof (buf);
    }

  return 0;
}
