/*
   pop3cli.c - przyklad klienta POP3

   by Marcin Dawcewicz
*/


#include <sys/types.h>
#include <sys/socket.h>		/* socket(), connect(), recv(), send() */
#include <linux/in.h>		/* struct sockaddr_in                  */
#include <sys/stat.h>		/* S_IRUSR, S_IWUSR */
#include <fcntl.h>		/* open() */
#include <unistd.h>		/* close(), write() */
#include <stdio.h>		/* perror() */
#include <netdb.h>		/* getservbyname() */
#include <string.h>		/* strncmp() */

#define USERNAME  "zygfryd"	/* nazwa u¿ytkownika POP3 */
#define PASSWORD  "sikret"	/* has³o do skrzynki pocztowej */
#define POPSERV   "serfer.pl"	/* serwer POP3 */
#define MAILFILE  "mailbox"	/* ¶cie¿ka do pliku zawierajacego poczte */
#define SERVICE   "pop-3"	/* korzystamy z POP3 */
#define KEEPM			/* je¶li zdefiniowane to nie kasujemy
				   wiadomo¶ci z serwera; lepiej mieæ to
				   w³±czone podczas eksperymentowania z
				   tym programem ... */

int
vrfy_ans (char *b)
{
  if (!strncmp (b, "+OK", 3))
    return 1;
  else
    {
      printf ("\nBlad! Serwer zwrocil odpowiedz:\n%s\nKoncze ...\n", b);
      exit (2);
    }
}

int
main ()
{
  int fd, sd, ret, nrmsg = 0, n;
  long int msgsize, msize;
  struct sockaddr_in saddr;
  struct servent *srvent;
  struct hostent *sent;
  char msgbuf[1024], buf[256];

  sd = socket (PF_INET, SOCK_STREAM, 0);
  if (sd < 0)
    {
      perror ("socket()");
      exit (1);
    }

  srvent = getservbyname (SERVICE, "tcp");
  if (!srvent)
    {
      perror("getservbyname()");
      exit (1);
    }

  printf ("Probuje znalezc adres IP maszyny %s...\n", POPSERV);
  sent = gethostbyname (POPSERV);
  if (!sent)
    {
      herror ("gethostbyname()");
      exit (1);
    }
  else
    printf ("Adres %s to %s\n", POPSERV, inet_ntoa (*sent->h_addr));

  saddr.sin_family = sent->h_addrtype;
  saddr.sin_port = srvent->s_port;
  bcopy (sent->h_addr, (char *) &saddr.sin_addr, sent->h_length);

  printf ("Lacze sie z %s:%i\n", inet_ntoa (*sent->h_addr),
	  ntohs (srvent->s_port));

  ret = connect (sd, (struct sockaddr *) &saddr, sizeof (saddr));
  if (ret < 0)
    {
      perror ("connect()");
      exit (1);
    }
  else
    printf ("Polaczony.\n");

  bzero (buf, sizeof (buf));
  recv (sd, buf, sizeof (buf), 0);
  vrfy_ans (buf);

  printf ("Loguje sie jako uzytkownik: %s\n", USERNAME);
  sprintf (buf, "USER %s\n", USERNAME);
  send (sd, buf, strlen (buf), 0);

  bzero (buf, sizeof (buf));
  recv (sd, buf, sizeof (buf), 0);
  vrfy_ans (buf);

  sprintf (buf, "PASS %s\n", PASSWORD);
  send (sd, buf, strlen (buf), 0);

  bzero (buf, sizeof (buf));
  recv (sd, buf, sizeof (buf), 0);
  vrfy_ans (buf);
  printf ("Zalogowany.\n", USERNAME);

  printf ("Sprawdzam, czy sa nowe wiadomosci ...\n");
  sprintf (buf, "STAT\n");
  send (sd, buf, strlen (buf), 0);

  bzero (buf, sizeof (buf));
  recv (sd, buf, sizeof (buf), 0);
  vrfy_ans (buf);
  sscanf (buf, "+OK %i %i", &nrmsg, &msgsize);

  if (nrmsg > 0)
    {
      printf ("Nowych wiadomosci: %i . Calkowity rozmiar: %i.\n\n", nrmsg,
	      msgsize);

      fd = open (MAILFILE, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
      if (fd == -1)
	{
	  perror ("open()");
	  exit (1);
	}

      for (n = 1; n <= nrmsg; n++)
	{
	  sprintf (buf, "RETR %i\n", n);
	  send (sd, buf, strlen (buf), 0);

	  bzero (buf, sizeof (buf));
	  recv (sd, buf, sizeof (buf), MSG_PEEK);
	  vrfy_ans (buf);

	  sscanf (buf, "+OK %i", &msize);
	  printf ("Pobieram wiadomosc nr. %i (rozmiar: %i) ... ", n, msize);
	  fflush (stdout);
	  bzero (msgbuf, sizeof (msgbuf));

	  while (recv (sd, msgbuf, sizeof (msgbuf), 0) > 0)
	    {
	      char *str = msgbuf, *end = msgbuf + strlen (msgbuf);
	      int msg_end = 0;

	      if (!strncmp (msgbuf, "+OK", 3))
		str = strchr (msgbuf, '\12') + 1;
	      if (!strncmp (msgbuf + strlen (msgbuf) - 5, "\15\12.\15\12", 5))
		{
		  end = strrchr (msgbuf, '\12') - 4;
		  msg_end = 1;
		}

	      ret = write (fd, str, end - str);
	      if (ret < 0)
		{
		  perror ("write()");
		  exit (3);
		}

	      bzero (msgbuf, sizeof (msgbuf));
	      if (msg_end)
		{
		  write (fd, "\n\n", 2);
		  break;
		}
	    }
	  printf ("OK\n");
#ifndef KEEPM
	  printf ("Usuwam wiadomosc nr. %i ... ", n);
	  fflush (stdout);
	  sprintf (buf, "DELE %i\n", n);
	  send (sd, buf, strlen (buf), 0);
	  recv (sd, buf, sizeof (buf), 0);
	  vrfy_ans (buf);
	  printf ("OK\n");
#endif
	}

      printf ("\nWszystkie wiadomosci pobrane. Koncze ...\n");
      close (fd);
    }
  else
    printf ("Nie ma nowych wiadomosci. Koncze ...\n");
  sprintf (buf, "QUIT\n");
  send (sd, buf, strlen (buf), 0);
  recv (sd, buf, sizeof (buf), 0);
  vrfy_ans (buf);
  return 0;
}
