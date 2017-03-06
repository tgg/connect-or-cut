/* tcpcontest -- simple connect(2) test program
 *
 * Copyright Ⓒ 2017  Thomas Girard <thomas.g.girard@free.fr>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *  * Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

static const char *me;

static int
usage (int retcode)
{
  FILE *out = retcode ? stderr : stdout;
  fprintf (out, "Usage: %s HOST PORT\n", me);
  fprintf (out, "Invoke connect() on HOST:PORT and return call value\n");
  exit (retcode);
}

int
main (int argc, char *argv[])
{
  me = argv[0];

  if (argc != 3)
    {
      usage (EXIT_FAILURE);
    }

  struct addrinfo hints;
  struct addrinfo *result, *rp;

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = IPPROTO_TCP;

  int addr = getaddrinfo (argv[1], argv[2], &hints, &result);
  int rc = EXIT_FAILURE;

  if (addr != 0)
    {
      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (addr));
      exit (rc);
    }

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      int s = socket (rp->ai_family, rp->ai_socktype,
		      rp->ai_protocol);
      if (s == -1)
	{
	  continue;
	}

      char str[INET6_ADDRSTRLEN];
      void *sinaddr = NULL;
      if (rp->ai_family == AF_INET)
	{
	  struct sockaddr_in *sinp = (struct sockaddr_in *) rp->ai_addr;
	  sinaddr = &sinp->sin_addr;
	}
      else
	{
	  struct sockaddr_in6 *sin6p = (struct sockaddr_in6 *) rp->ai_addr;
	  sinaddr = &sin6p->sin6_addr;
	}

      if (inet_ntop(rp->ai_family, sinaddr, str, INET6_ADDRSTRLEN) == NULL) {
	perror("inet_ntop");
	exit(EXIT_FAILURE);
      }

      rc = connect (s, rp->ai_addr, rp->ai_addrlen);

      if (rc == 0)
	{
	  close (s);
	  printf ("connect to %s is OK\n", str);
	  break;
	}
      else if (rc == -1)
	{
	  printf ("connect to %s is KO: errno is %d (%s)\n", str, errno, strerror(errno));
	  close (s);
	}
      else
	{
	  printf ("connect to %s is OK?: return code is %d\n", str, rc);
	  close (s);
	}
    }

  freeaddrinfo (result);

  exit (rc);
}
