/*
   udpsrv.c - przyklad serwera UDP

   by Marcin Dawcewicz
*/

#define _XOPEN_SOURCE
#include <unistd.h>		/* crypt()                                    */
#include <sys/types.h>
#include <sys/socket.h>		/* socket(), connect(), recv(),send(), bind() */
#include <linux/in.h>		/* struct sockaddr_in                         */
#include <stdio.h>		/* perror()                                   */
#include <sys/stat.h>
#include <fcntl.h>		/* open()                                     */

#define SRVPORT 1111		/* numer portu serwera                        */
#define CLIPORT 31337		/* numer portu gniazda klienta                */
#define PASSWD  "sikret"	/* haslo wymagane do polaczenia z serwerem    */
#define CSALT   "Zz"		/* dwuznakowy tzw. salt dla funkcji crypt()   */
#define PLIK    "/proc/meminfo"	/* plik, ktory wyslemy klientowi              */

main ()
{
  int sd, fd, len;
  struct sockaddr_in saddr, caddr;
  struct hostent *sent;
  char buf[1024], hash[20];

  sd = socket (PF_INET, SOCK_DGRAM, 0);
  if (sd < 0)
    {
      perror ("socket()");
      exit (1);
    }

  saddr.sin_family = PF_INET;
  saddr.sin_port = htons (SRVPORT);
  saddr.sin_addr.s_addr = INADDR_ANY;

  if (bind (sd, (struct sockaddr *) &saddr, sizeof (saddr)) < 0)
    {
      perror ("bind()");
      exit (1);
    }

  sprintf (hash, "%s", crypt (PASSWD, CSALT));
  printf ("Czekam na datagramy ...\n");

  while (1)
    {
      bzero (buf, sizeof (buf));
      len = sizeof (caddr);
      recvfrom (sd, buf, sizeof (buf), 0, (struct sockaddr *) &caddr, &len);

      printf ("Datagram od %s:%i ... ", inet_ntoa (caddr.sin_addr.s_addr),
	      ntohs (caddr.sin_port));

      if (!strncmp (buf, hash, strlen (hash))
	  && ntohs (caddr.sin_port) == CLIPORT)
	{
	  printf ("Przyjety.\n");
	  sendto (sd, "OK\n", 3, 0, (struct sockaddr *) &caddr,
		  sizeof (caddr));

	  fd = open (PLIK, O_RDONLY);
	  if (fd < 1)
	    {
	      perror ("open()");
	      sendto (sd, "Blad: open()\n", 13, 0, (struct sockaddr *) &caddr,
		      sizeof (caddr));
	    }

	  bzero (buf, sizeof (buf));
	  while (read (fd, buf, sizeof (buf)) > 0)
	    {
	      sendto (sd, buf, strlen (buf), 0, (struct sockaddr *) &caddr,
		      sizeof (caddr));
	      bzero (buf, sizeof (buf));
	    }
	  close (fd);

	  sendto (sd, "END\n", 4, 0, (struct sockaddr *) &caddr,
		  sizeof (caddr));
	}
      else
	{
	  printf ("Odrzucony.\n");
	  sendto (sd, "Blad: zle haslo albo port zrodlowy\n", 4, 0,
		  (struct sockaddr *) &caddr, sizeof (caddr));
	}
    }
}
