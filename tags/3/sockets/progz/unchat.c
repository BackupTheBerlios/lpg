/*
   unchat.c - przyklad uzycia gniazd PF_UNIX

   by Marcin Dawcewicz
*/


#include <sys/types.h>
#include <sys/socket.h>		/* socket(), connect(), recv(), send() */
#include <linux/un.h>		/* struct sockaddr_un                  */
#include <stdio.h>		/* fgets()                             */
#include <unistd.h>		/* chdir(), unlink()                   */
#include <signal.h>		/* signal()                            */
#include <sys/stat.h>		/* chmod()                             */

#define CHATDIR       "/tmp/"	/* katalog, w ktorym zakladane beda sockety */

void
sig_handle (int signum)
{
  if (unlink (getlogin ()) < 0)
    {
      perror ("unlink()");
      exit (1);
    }

  exit (0);
}

main (int argc, char *argv[])
{
  int sd, cd, len, ret;
  struct sockaddr_un saddr, caddr;
  char buf[256], buf1[256];
  fd_set fds;

  if (argc != 2)
    {
      printf ("Uzycie:\n%s nazwa_uzytkownika\n", argv[0]);
      exit (1);
    }

  sd = socket (PF_UNIX, SOCK_DGRAM, 0);
  if (sd < 01)
    {
      perror ("socket()");
      exit (1);
    }

  chdir (CHATDIR);

  saddr.sun_family = PF_UNIX;
  strncpy (saddr.sun_path, getlogin (), UNIX_PATH_MAX);

  if (bind
      (sd, (struct sockaddr *) &saddr,
       sizeof (saddr.sun_family) + strlen (saddr.sun_path)) < 0)
    {
      perror ("bind()");
      exit (1);
    }

  if (chmod (saddr.sun_path, S_IRUSR | S_IWUSR | S_IWOTH) < 0)
    {
      perror ("chmod()");
      exit (1);
    }

  signal (SIGINT, &sig_handle);

  caddr.sun_family = PF_UNIX;
  strncpy (caddr.sun_path, argv[1], UNIX_PATH_MAX);

  while (1)
    {
      FD_ZERO (&fds);
      FD_SET (0, &fds);
      FD_SET (sd, &fds);

      ret = select (sd + 1, &fds, NULL, NULL, NULL);

      if (ret < 0)
	{
	  perror ("select()");
	  exit (1);
	}

      if (FD_ISSET (0, &fds))
	{
	  bzero (buf1, sizeof (buf1));
	  fgets (buf1, sizeof (buf1), stdin);
	  len = sizeof (caddr.sun_family) + strlen (caddr.sun_path);
	  sendto (sd, buf1, strlen (buf1), 0, (struct sockaddr *) &caddr,
		  len);
	}

      if (FD_ISSET (sd, &fds))
	{
	  len = sizeof (saddr.sun_family) + strlen (saddr.sun_path);
	  bzero (buf, sizeof (buf));
	  recvfrom (sd, buf, sizeof (buf), 0,
		    (struct sockaddr *) &saddr, &len);
	  printf ("[%s] %s", saddr.sun_path, buf);
	}
    }

  sig_handle (0);
}
