/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000,2001 Thiago Macieira <thiago.macieira@kdemail.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 **/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <qglobal.h>

// This is so that, if addrinfo is defined, it doesn't clobber our definition
// It might be defined in the few cases in which we are replacing the system's
// broken getaddrinfo
#include <netdb.h>

#include "config.h"
#include "kdebug.h"
#include "klocale.h"

#ifndef IN6_IS_ADDR_V4MAPPED
#define NEED_IN6_TESTS
#endif
#undef CLOBBER_IN6
#include "netsupp.h"

#if defined(__hpux) || defined(_HPUX_SOURCE)
extern int h_errno;
#endif

#if !defined(kde_sockaddr_in6)
/*
 * kde_sockaddr_in6 might have got defined even though we #undef'ed
 * CLOBBER_IN6. This happens when we are compiling under --enable-final.
 * However, in that case, if it was defined, that's because ksockaddr.cpp
 * had it defined because sockaddr_in6 didn't exist, and so sockaddr_in6
 * exists and is our kde_sockaddr_in6
 */
# define sockaddr_in6	kde_sockaddr_in6
# define in6_addr	kde_in6_addr
#endif

#ifdef offsetof
#undef offsetof
#endif
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/*
 * These constants tell the flags in KDE::resolverFlags
 * The user could (but shouldn't) test the variable to know what kind of
 * resolution is supported
 */
#define KRF_KNOWS_AF_INET6		0x01	/* if present, the code knows about AF_INET6 */
#define KRF_USING_OWN_GETADDRINFO	0x02	/* if present, we are using our own getaddrinfo */
#define KRF_USING_OWN_INET_NTOP		0x04	/* if present, we are using our own inet_ntop */
#define KRF_USING_OWN_INET_PTON		0x08	/* if present, we are using our own inet_pton */
#define KRF_CAN_RESOLVE_UNIX		0x100	/* if present, the resolver can resolve Unix sockets */
#define KRF_CAN_RESOLVE_IPV4		0x200	/* if present, the resolver can resolve to IPv4 */
#define KRF_CAN_RESOLVE_IPV6		0x400	/* if present, the resolver can resolve to IPv6 */


static void dofreeaddrinfo(struct addrinfo *ai)
{
  while (ai)
    {
      struct addrinfo *ai2 = ai;
      if (ai->ai_canonname != NULL)
	free(ai->ai_canonname);

      if (ai->ai_addr != NULL)
	free(ai->ai_addr);

      ai = ai->ai_next;
      free(ai2);
    }
}

void kde_freeaddrinfo(struct kde_addrinfo *ai)
{
  if (ai->origin == KAI_LOCALUNIX)
    {
      struct addrinfo *p, *last = NULL;
      /* We've added one AF_UNIX socket in here, to the
       * tail of the linked list. We have to find it */
      for (p = ai->data; p; p = p->ai_next)
	{
	  if (p->ai_family == AF_UNIX)
	    {
	      if (last)
		{
		  last->ai_next = NULL;
		  freeaddrinfo(ai->data);
		}
	      dofreeaddrinfo(p);
	      break;
	    }
	  last = p;
	}
    }
  else
    freeaddrinfo(ai->data);

  free(ai);
}

static struct addrinfo*
make_unix(const char *name, const char *serv)
{
  const char *buf;
  struct addrinfo *p;
  struct sockaddr_un *_sun;
  int len;

  p = (addrinfo*)malloc(sizeof(*p));
  if (p == NULL)
    return NULL;
  memset(p, 0, sizeof(*p));

  if (name != NULL)
    buf = name;
  else
    buf = serv;

  // Calculate length of the binary representation
  len = strlen(buf) + offsetof(struct sockaddr_un, sun_path) + 1;
  if (*buf != '/')
    len += 5;			// strlen("/tmp/");

  _sun = (sockaddr_un*)malloc(len);
  if (_sun == NULL)
    {
      // Oops
      free(p);
      return NULL;
    }

  _sun->sun_family = AF_UNIX;
# ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
  _sun->sun_len = len;
# endif
  if (*buf == '/')
    *_sun->sun_path = '\0';	// empty it
  else
    strcpy(_sun->sun_path, "/tmp/");
  strcat(_sun->sun_path, buf);

  // Set the addrinfo
  p->ai_family = AF_UNIX;
  p->ai_addrlen = len;
  p->ai_addr = (sockaddr*)_sun;
  p->ai_canonname = strdup(buf);

  return p;
}

// Ugh. I hate #ifdefs
// Anyways, here's what this does:
// KDE_IPV6_LOOKUP_MODE != 1, this function doesn't exist
// AF_INET6 not defined, we say there is no IPv6 stack
// otherwise, we try to create a socket.
// returns: 1 for IPv6 stack available, 2 for not available
#if KDE_IPV6_LOOKUP_MODE == 1
static int check_ipv6_stack()
{
# ifndef AF_INET6
  return 2;			// how can we check?
# else
  if (getenv("KDE_NO_IPV6"))
     return 2;
  int fd = ::socket(AF_INET6, SOCK_STREAM, 0);
  if (fd == -1)
     return 2;
     
  ::close(fd);
  return 1;
# endif
}
#endif


/*
 * Reason for using this function: kde_getaddrinfo
 *
 * I decided to add this wrapper function for getaddrinfo
 * and have this be called by KExtendedSocket instead of
 * the real getaddrinfo so that we can make sure that the
 * behavior is the desired one.
 *
 * Currently, the only "undesired" behavior is getaddrinfo
 * not returning PF_UNIX sockets in some implementations.
 *
 * getaddrinfo and family are defined in POSIX 1003.1g
 * (Protocol Independent Interfaces) and in RFC 2553
 * (Basic Socket Interface for IPv6). Whereas the RFC is ambiguosly
 * vague whether this family of functions should return Internet
 * sockets only or not, the name of the POSIX draft says
 * otherwise: it should be independent of protocol.
 *
 * So, my interpretation is that they should return every
 * kind of socket available and known and that's how I
 * designed KExtendedSocket on top of it.
 *
 * That's why there's this wrapper, to make sure PF_UNIX
 * sockets are returned when expected.
 */

int kde_getaddrinfo(const char *name, const char *service,
		    const struct addrinfo* hint,
		    struct kde_addrinfo** result)
{
  struct kde_addrinfo* res;
  struct addrinfo* p;
  int err = EAI_SERVICE;
#if KDE_IPV6_LOOKUP_MODE == 1
  // mode 1: do a check on whether we have an IPv6 stack
  static int ipv6_stack = 0;	// 0: unknown, 1: yes, 2: no
#endif

  // allocate memory for results
  res = (kde_addrinfo*)malloc(sizeof(*res));
  if (res == NULL)
    return EAI_MEMORY;
  res->data = NULL;
  res->origin = KAI_SYSTEM;	// at first, it'll be only system data

  struct addrinfo* last = NULL;
  
  // Skip the getaddrinfo call and the ipv6 check for a UNIX socket.
  if (hint && (hint->ai_family == PF_UNIX))
  {
     if (service == NULL || *service == '\0')
       goto out;		// can't be Unix if no service was requested

     // Unix sockets must be localhost
     // That is, either name is NULL or, if it's not, it must be empty,
     // "*" or "localhost"
     if (name != NULL && !(name[0] == '\0' || (name[0] == '*' && name[1] == '\0') ||
		strcmp("localhost", name) == 0))
       goto out;		// isn't localhost

     goto do_unix;
  }
  
#if KDE_IPV6_LOOKUP_MODE != 0
# if KDE_IPV6_LOOKUP_MODE == 1
  // mode 1: do a check on whether we have an IPv6 stack
  if (ipv6_stack == 0)
    ipv6_stack = check_ipv6_stack();

  if (ipv6_stack == 2)
    {
# endif
      // here we have modes 1 and 2 (no lookups)
      // this is shared code
      struct addrinfo our_hint;
      if (hint != NULL)
	{
	  memcpy(&our_hint, hint, sizeof(our_hint));
	  if (our_hint.ai_family == AF_UNSPEC)
	    our_hint.ai_family = AF_INET;
	}
      else
	{
	  memset(&our_hint, 0, sizeof(our_hint));
	  our_hint.ai_family = AF_INET;
	}

      // do the actual resolution
      err = getaddrinfo(name, service, &our_hint, &res->data);
# if KDE_IPV6_LOOKUP_MODE == 1
    }
  else
# endif
#endif
#if KDE_IPV6_LOOKUP_MODE != 2
      // do the IPV6 resolution
      err = getaddrinfo(name, service, hint, &res->data);
#endif

  // Now we have to check whether the user could want a Unix socket

  if (service == NULL || *service == '\0')
    goto out;			// can't be Unix if no service was requested

  // Unix sockets must be localhost
  // That is, either name is NULL or, if it's not, it must be empty,
  // "*" or "localhost"
  if (name != NULL && !(name[0] == '\0' || (name[0] == '*' && name[1] == '\0') ||
			strcmp("localhost", name) == 0))
    goto out;			// isn't localhost

  // Unix sockets can only be returned if the user asked for a PF_UNSPEC
  // or PF_UNIX socket type or gave us a NULL hint
  if (hint != NULL && (hint->ai_family != PF_UNSPEC && hint->ai_family != PF_UNIX))
    goto out;			// user doesn't want Unix

  // If we got here, then it means that the user might be expecting Unix
  // sockets. The user wants a local socket, with a non-null service and
  // has told us that they accept PF_UNIX sockets
  // Check whether the system implementation returned Unix
  if (err == 0)
    for (p = res->data; p; p = p->ai_next)
      {
	last = p;			// we have to find out which one is last anyways
	if (p->ai_family == AF_UNIX)
	  // there is an Unix node
	  goto out;
      }

 do_unix:
  // So, give the user a PF_UNIX socket
  p = make_unix(NULL, service);
  if (p == NULL)
    {
      err = EAI_MEMORY;
      goto out;
    }
  if (hint != NULL)
    p->ai_socktype = hint->ai_socktype;
  if (p->ai_socktype == 0)
    p->ai_socktype = SOCK_STREAM; // default

  if (last)
    last->ai_next = p;
  else
    res->data = p;
  res->origin = KAI_LOCALUNIX;
  *result = res;
  return 0;

 out:
  // Normal exit of the function
  if (err == 0)
    *result = res;
  else
    {
      if (res->data != NULL)
	freeaddrinfo(res->data);
      free(res);
    }
  return err;
}

#if defined(HAVE_GETADDRINFO) && !defined(HAVE_BROKEN_GETADDRINFO)

#define KRF_getaddrinfo		0
#define KRF_resolver		0

#else  // !defined(HAVE_GETADDRINFO) || defined(HAVE_BROKEN_GETADDRINFO)

#define KRF_getaddrinfo			KRF_USING_OWN_GETADDRINFO
#define KRF_resolver			KRF_CAN_RESOLVE_UNIX | KRF_CAN_RESOLVE_IPV4

/*
 * No getaddrinfo() in this system.
 * We shall provide our own
 */

/** TODO
 * Try and use gethostbyname2_r before gethostbyname2 and gethostbyname
 */
static int inet_lookup(const char *name, int portnum, int protonum,
		       struct addrinfo *p, const struct addrinfo *hint,
		       struct addrinfo** result)
{
  struct addrinfo *q;
  struct hostent *h;
  struct sockaddr **psa = NULL;
  int len;

  // TODO
  // Currently, this never resolves IPv6 (need gethostbyname2, etc.)
# ifdef AF_INET6
  if (hint->ai_family == AF_INET6)
    {
      if (p != NULL)
	{
	  *result = p;
	  return 0;
	}
      return EAI_FAIL;
    }
# endif

  q = (addrinfo*)malloc(sizeof(*q));
  if (q == NULL)
    {
      freeaddrinfo(p);
      return EAI_MEMORY;
    }

  h = gethostbyname(name);
  if (h == NULL)
    {
      if (p != NULL)
	{
	  // There already is a suitable result
	  *result = p;
	  return 0;
	}

      switch (h_errno)
	{
	case HOST_NOT_FOUND:
	  return EAI_NONAME;
	case TRY_AGAIN:
	  return EAI_AGAIN;
	case NO_RECOVERY:
	  return EAI_FAIL;
	case NO_ADDRESS:
	  return EAI_NODATA;
	default:
	  // EH!?
	  return EAI_FAIL;
	}
    }

  // convert the hostent to addrinfo
  if (h->h_addrtype == AF_INET && (hint->ai_family == AF_INET || hint->ai_family == AF_UNSPEC))
    len = sizeof(struct sockaddr_in);
# ifdef AF_INET6
  else if (h->h_addrtype == AF_INET6 && (hint->ai_family == AF_INET6 ||
					 hint->ai_family == AF_UNSPEC))
    len = sizeof(struct sockaddr_in6);
# endif
  else
    {
      // We don't know what to do with these addresses
      // Or gethostbyname returned information we don't want
      if (p != NULL)
	{
	  *result = p;
	  return 0;
	}
      return EAI_NODATA;
    }

  q->ai_flags = 0;
  q->ai_family = h->h_addrtype;
  q->ai_socktype = hint->ai_socktype;
  q->ai_protocol = protonum;
  q->ai_addrlen = len;

  q->ai_addr = (sockaddr*)malloc(len);
  if (q->ai_addr == NULL)
    {
      free(q);
      freeaddrinfo(p);
      return EAI_MEMORY;
    }
  if (h->h_addrtype == AF_INET)
    {
      struct sockaddr_in *sin = (sockaddr_in*)q->ai_addr;
      sin->sin_family = AF_INET;
# ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
      sin->sin_len = sizeof(*sin);
# endif
      sin->sin_port = portnum;
      memcpy(&sin->sin_addr, h->h_addr, h->h_length);
    }
# ifdef AF_INET6
  else if (h->h_addrtype == AF_INET6)
    {
      struct sockaddr_in6 *sin6 = (sockaddr_in6*)q->ai_addr;
      sin6->sin6_family = AF_INET6;
#  ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
      sin6->sin6_len = sizeof(*sin6);
#  endif
      sin6->sin6_port = portnum;
      sin6->sin6_flowinfo = 0;
      memcpy(&sin6->sin6_addr, h->h_addr, h->h_length);
      sin6->sin6_scope_id = 0;
    }
# endif

  if (hint->ai_flags & AI_CANONNAME)
    q->ai_canonname = strdup(h->h_name);
  else
    q->ai_canonname = NULL;

  q->ai_next = p;
  p = q;

  // cycle through the rest of the hosts;
  for (psa = (sockaddr**)h->h_addr_list + 1; *psa; psa++)
    {
      q = (addrinfo*)malloc(sizeof(*q));
      if (q == NULL)
	{
	  freeaddrinfo(p);
	  return EAI_MEMORY;
	}
      memcpy(q, p, sizeof(*q));

      q->ai_addr = (sockaddr*)malloc(h->h_length);
      if (q->ai_addr == NULL)
	{
	  freeaddrinfo(p);
	  free(q);
	  return EAI_MEMORY;
	}
      if (h->h_addrtype == AF_INET)
	{
	  struct sockaddr_in *sin = (sockaddr_in*)q->ai_addr;
	  sin->sin_family = AF_INET;
# ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
	  sin->sin_len = sizeof(*sin);
# endif
	  sin->sin_port = portnum;
	  memcpy(&sin->sin_addr, *psa, h->h_length);
	}
# ifdef AF_INET6
      else if (h->h_addrtype == AF_INET6)
	{
	  struct sockaddr_in6 *sin6 = (sockaddr_in6*)q->ai_addr;
	  sin6->sin6_family = AF_INET6;
#  ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
	  sin6->sin6_len = sizeof(*sin6);
#  endif
	  sin6->sin6_port = portnum;
	  sin6->sin6_flowinfo = 0;
	  memcpy(&sin6->sin6_addr, *psa, h->h_length);
	  sin6->sin6_scope_id = 0;
	}
# endif

      if (q->ai_canonname != NULL)
	q->ai_canonname = strdup(q->ai_canonname);

      q->ai_next = p;
      p = q;
    }

  *result = p;
  return 0;			// Whew! Success!
}

static int make_inet(const char *name, int portnum, int protonum, struct addrinfo *p,
		     const struct addrinfo *hint, struct addrinfo** result)
{
  struct addrinfo *q;

  do
    {
      // This 'do' is here just so that we can 'break' out of it

      if (name != NULL)
	{
	  // first, try to use inet_pton before resolving
	  // it will catch IP addresses given without having to go to lookup
	  struct sockaddr_in *sin;
	  struct in_addr in;
# ifdef AF_INET6
	  struct sockaddr_in6 *sin6;
	  struct in6_addr in6;

	  if (hint->ai_family == AF_INET6 || (hint->ai_family == AF_UNSPEC &&
					      strchr(name, ':') != NULL))
	    {
	      // yes, this is IPv6
	      if (inet_pton(AF_INET6, name, &in6) != 1)
		{
		  if (hint->ai_flags & AI_NUMERICHOST)
		    {
		      freeaddrinfo(p);
		      return EAI_FAIL;
		    }
		  break;	// not a numeric host
		}

	      sin6 = (sockaddr_in6*)malloc(sizeof(*sin6));
	      if (sin6 == NULL)
		{
		  freeaddrinfo(p);
		  return EAI_MEMORY;
		}
	      memcpy(&sin6->sin6_addr, &in6, sizeof(in6));

	      if (strchr(name, '%') != NULL)
		{
		  errno = 0;
		  sin6->sin6_scope_id = strtoul(strchr(name, '%') + 1, NULL, 10);
		  if (errno != 0)
		    sin6->sin6_scope_id = 0; // no interface
		}

	      q = (addrinfo*)malloc(sizeof(*q));
	      if (q == NULL)
		{
		  freeaddrinfo(p);
		  free(sin6);
		  return EAI_MEMORY;
		}

	      sin6->sin6_family = AF_INET6;
#  ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
	      sin6->sin6_len = sizeof(*sin6);
#  endif
	      sin6->sin6_port = portnum;
	      sin6->sin6_flowinfo = 0;

	      q->ai_flags = 0;
	      q->ai_family = AF_INET6;
	      q->ai_socktype = hint->ai_socktype;
	      q->ai_protocol = protonum;
	      q->ai_addrlen = sizeof(*sin6);
	      q->ai_canonname = NULL;
	      q->ai_addr = (sockaddr*)sin6;
	      q->ai_next = p;

	      *result = q;
	      return 0;		// success!
	    }
# endif // AF_INET6

	  if (hint->ai_family == AF_INET || hint->ai_family == AF_UNSPEC)
	    {
	      // This has to be IPv4
	      if (inet_pton(AF_INET, name, &in) != 1)
		{
		  if (hint->ai_flags & AI_NUMERICHOST)
		    {
		      freeaddrinfo(p);
		      return EAI_FAIL;	// invalid, I guess
		    }
		  break;	// not a numeric host, do lookup
		}

	      sin = (sockaddr_in*)malloc(sizeof(*sin));
	      if (sin == NULL)
		{
		  freeaddrinfo(p);
		  return EAI_MEMORY;
		}

	      q = (addrinfo*)malloc(sizeof(*q));
	      if (q == NULL)
		{
		  freeaddrinfo(p);
		  free(sin);
		  return EAI_MEMORY;
		}

	      sin->sin_family = AF_INET;
# ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
	      sin->sin_len = sizeof(*sin);
# endif
	      sin->sin_port = portnum;
	      sin->sin_addr = in;

	      q->ai_flags = 0;
	      q->ai_family = AF_INET;
	      q->ai_socktype = hint->ai_socktype;
	      q->ai_protocol = protonum;
	      q->ai_addrlen = sizeof(*sin);
	      q->ai_canonname = NULL;
	      q->ai_addr = (sockaddr*)sin;
	      q->ai_next = p;
	      *result = q;
	      return 0;
	    }

	  // Eh, what!?
	  // One of the two above has to have matched
	  kdError() << "I wasn't supposed to get here!";
	}
    } while (false);

  // This means localhost
  if (name == NULL)
    {
      struct sockaddr_in *sin = (sockaddr_in*)malloc(sizeof(*sin));
# ifdef AF_INET6
      struct sockaddr_in6 *sin6;
# endif

      if (hint->ai_family == AF_INET || hint->ai_family == AF_UNSPEC)
	{
	  if (sin == NULL)
	    {
	      free(sin);
	      freeaddrinfo(p);
	      return EAI_MEMORY;
	    }

	  // Do IPv4 first
	  q = (addrinfo*)malloc(sizeof(*q));
	  if (q == NULL)
	    {
	      free(sin);
	      freeaddrinfo(p);
	      return EAI_MEMORY;
	    }

	  sin->sin_family = AF_INET;
# ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
	  sin->sin_len = sizeof(*sin);
# endif
	  sin->sin_port = portnum;
	  if (hint->ai_flags & AI_PASSIVE)
	    *(Q_UINT32*)&sin->sin_addr = INADDR_ANY;
	  else
	    *(Q_UINT32*)&sin->sin_addr = htonl(INADDR_LOOPBACK);
	  q->ai_flags = 0;
	  q->ai_family = AF_INET;
	  q->ai_socktype = hint->ai_socktype;
	  q->ai_protocol = protonum;
	  q->ai_addrlen = sizeof(*sin);
	  q->ai_canonname = NULL;
	  q->ai_addr = (sockaddr*)sin;
	  q->ai_next = p;
	  p = q;
	}

# ifdef AF_INET6
      // Try now IPv6

      if (hint->ai_family == AF_INET6 || hint->ai_family == AF_UNSPEC)
	{
	  sin6 = (sockaddr_in6*)malloc(sizeof(*sin6));
	  q = (addrinfo*)malloc(sizeof(*q));
	  if (q == NULL || sin6 == NULL)
	    {
	      free(sin6);
	      free(q);
	      freeaddrinfo(p);
	      return EAI_MEMORY;
	    }

	  sin6->sin6_family = AF_INET6;
#  ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
	  sin6->sin6_len = sizeof(*sin6);
#  endif
	  sin6->sin6_port = portnum;
	  sin6->sin6_flowinfo = 0;
	  sin6->sin6_scope_id = 0;

      // We don't want to use in6addr_loopback and in6addr_any
	  memset(&sin6->sin6_addr, 0, sizeof(sin6->sin6_addr));
	  if ((hint->ai_flags & AI_PASSIVE) == 0)
	    ((char*)&sin6->sin6_addr)[15] = 1;

	  q->ai_flags = 0;
	  q->ai_family = AF_INET6;
	  q->ai_socktype = hint->ai_socktype;
	  q->ai_protocol = protonum;
	  q->ai_addrlen = sizeof(*sin6);
	  q->ai_canonname = NULL;
	  q->ai_addr = (sockaddr*)sin6;
	  q->ai_next = p;
	  p = q;
	}

# endif // AF_INET6

      *result = p;
      return 0;			// success!
    }

  return inet_lookup(name, portnum, protonum, p, hint, result);
}


int getaddrinfo(const char *name, const char *serv,
		const struct addrinfo* hint,
		struct addrinfo** result)
{
  unsigned short portnum;	// remember to store in network byte order
  int protonum = IPPROTO_TCP;
  const char *proto = "tcp";
  struct addrinfo *p = NULL;

  // Sanity checks:
  if (hint == NULL || result == NULL)
    return EAI_BADFLAGS;
  if (hint->ai_family != AF_UNSPEC && hint->ai_family != AF_UNIX &&
      hint->ai_family != AF_INET
# ifdef AF_INET6
      && hint->ai_family != AF_INET6
# endif
      )
    return EAI_FAMILY;
  if (hint->ai_socktype != 0 && hint->ai_socktype != SOCK_STREAM &&
      hint->ai_socktype != SOCK_DGRAM)
    return EAI_SOCKTYPE;

  // Treat hostname of "*" as NULL, which means localhost
  if (name != NULL && ((*name == '*' && name[1] == '\0') || *name == '\0'))
    name = NULL;
  // Treat service of "*" as NULL, which I guess means no port (0)
  if (serv != NULL && ((*serv == '*' && serv[1] == '\0') || *serv == '\0'))
    serv = NULL;

  if (name == NULL && serv == NULL) // what the hell do you want?
    return EAI_NONAME;

  // This is just to make it easier
  if (name != NULL && strcmp(name, "localhost") == 0)
     name = NULL;

  // First, check for a Unix socket
  // family must be either AF_UNIX or AF_UNSPEC
  // either of name or serv must be set, the other must be NULL or empty
  if (hint->ai_family == AF_UNIX || hint->ai_family == AF_UNSPEC)
    {
      if (name != NULL && serv != NULL)
	{
	  // This is not allowed
	  if (hint->ai_family == AF_UNIX)
	    return EAI_BADFLAGS;
	}
      else
	{
	  p = make_unix(name, serv);
	  if (p == NULL)
	    return EAI_MEMORY;

	  p->ai_socktype = hint->ai_socktype;
	  // If the name/service started with a slash, then this *IS*
	  // only a Unix socket. Return.
	  if (hint->ai_family == AF_UNIX || ((name != NULL && *name == '/') ||
					      (serv != NULL && *serv == '/')))
	    {
	      *result = p;
	      return 0;		// successful lookup
	    }
	}
    }

  // Lookup the service name, if required
  if (serv != NULL)
    {
      char *tail;
      struct servent *sent;

      portnum = htons((unsigned)strtoul(serv, &tail, 10));
      if (*tail != '\0')
	{
	  // not a number. We have to do the lookup
	  if (hint->ai_socktype == SOCK_DGRAM)
	    {
	      proto = "udp";
	      protonum = IPPROTO_UDP;
	    }

	  sent = getservbyname(serv, proto);
	  if (sent == NULL)	// no service?
	    {
	      if (p == NULL)
		return EAI_NONAME;
	      else
		return 0;	// a Unix socket available
	    }

	  portnum = sent->s_port;
	}
    }
  else
    portnum = 0;		// no port number

  return make_inet(name, portnum, protonum, p, hint, result);
}

void freeaddrinfo(struct addrinfo *p)
{
  dofreeaddrinfo(p);
}

char *gai_strerror(int errorcode)
{
  static const char * const messages[] =
  {
    I18N_NOOP("no error"),	// 0
    I18N_NOOP("address family for nodename not supported"), // EAI_ADDRFAMILY
    I18N_NOOP("temporary failure in name resolution"),	// EAI_AGAIN
    I18N_NOOP("invalid value for 'ai_flags'"),	// EAI_BADFLAGS
    I18N_NOOP("non-recoverable failure in name resolution"), // EAI_FAIL
    I18N_NOOP("'ai_family' not supported"),	// EAI_FAMILY
    I18N_NOOP("memory allocation failure"),	// EAI_MEMORY
    I18N_NOOP("no address associated with nodename"),	// EAI_NODATA
    I18N_NOOP("name or service not known"),	// EAI_NONAME
    I18N_NOOP("servname not supported for ai_socktype"), // EAI_SERVICE
    I18N_NOOP("'ai_socktype' not supported"),	// EAI_SOCKTYPE
    I18N_NOOP("system error")			// EAI_SYSTEM
  };

  if (errorcode > EAI_SYSTEM || errorcode < 0)
    return NULL;

  static char buffer[200];
  strcpy(buffer, i18n(messages[errorcode]).local8Bit());
  return buffer;
}

static void findport(unsigned short port, char *serv, size_t servlen, int flags)
{
  if (serv == NULL)
    return;

  if ((flags & NI_NUMERICSERV) == 0)
    {
      struct servent *sent;
      sent = getservbyport(ntohs(port), flags & NI_DGRAM ? "udp" : "tcp");
      if (sent != NULL && servlen > strlen(sent->s_name))
	{
	  strcpy(serv, sent->s_name);
	  return;
	}
    }

  snprintf(serv, servlen, "%u", ntohs(port));
}

int getnameinfo(const struct sockaddr *sa, ksocklen_t salen,
		char *host, size_t hostlen, char *serv, size_t servlen,
		int flags)
{
  union
    {
      const sockaddr *sa;
      const sockaddr_un *_sun;
      const sockaddr_in *sin;
      const sockaddr_in6 *sin6;
  } s;

  if ((host == NULL || hostlen == 0) && (serv == NULL || servlen == 0))
    return 1;

  s.sa = sa;
  if (s.sa->sa_family == AF_UNIX)
    {
      if (salen < offsetof(struct sockaddr_un, sun_path) + strlen(s._sun->sun_path) + 1)
	return 1;		// invalid socket

      if (servlen && serv != NULL)
	*serv = '\0';
      if (host != NULL && hostlen > strlen(s._sun->sun_path))
	strcpy(host, s._sun->sun_path);

      return 0;
    }
  else if (s.sa->sa_family == AF_INET)
    {
      if (salen < offsetof(struct sockaddr_in, sin_addr) + sizeof(s.sin->sin_addr))
	return 1;		// invalid socket

      if (flags & NI_NUMERICHOST)
	inet_ntop(AF_INET, &s.sin->sin_addr, host, hostlen);
      else
	{
	  // have to do lookup
	  struct hostent *h = gethostbyaddr((const char*)&s.sin->sin_addr, sizeof(s.sin->sin_addr),
					    AF_INET);
	  if (h == NULL && flags & NI_NAMEREQD)
	    return 1;
	  else if (h == NULL)
	    inet_ntop(AF_INET, &s.sin->sin_addr, host, hostlen);
	  else if (host != NULL && hostlen > strlen(h->h_name))
	    strcpy(host, h->h_name);
	  else
	    return 1;		// error
	}

      findport(s.sin->sin_port, serv, servlen, flags);
    }
# ifdef AF_INET6
  else if (s.sa->sa_family == AF_INET6)
    {
      if (salen < offsetof(struct sockaddr_in6, sin6_addr) + sizeof(s.sin6->sin6_addr))
	return 1;		// invalid socket

      if (flags & NI_NUMERICHOST)
	inet_ntop(AF_INET6, &s.sin6->sin6_addr, host, hostlen);
      else
	{
	  // have to do lookup
	  struct hostent *h = gethostbyaddr((const char*)&s.sin->sin_addr, sizeof(s.sin->sin_addr),
					    AF_INET6);
	  if (h == NULL && flags & NI_NAMEREQD)
	    return 1;
	  else if (h == NULL)
	    inet_ntop(AF_INET6, &s.sin6->sin6_addr, host, hostlen);
	  else if (host != NULL && hostlen > strlen(h->h_name))
	    strcpy(host, h->h_name);
	  else
	    return 1;		// error
	}

      findport(s.sin6->sin6_port, serv, servlen, flags);
    }
# endif // AF_INET6

  return 1;			// invalid family
}

#endif // HAVE_GETADDRINFO

#ifndef HAVE_INET_NTOP

#define KRF_inet_ntop	KRF_USING_OWN_INET_NTOP

static void add_dwords(char *buf, Q_UINT16 *dw, int count)
{
  int i = 1;
  sprintf(buf + strlen(buf), "%x", ntohs(dw[0]));
  while (--count)
    sprintf(buf + strlen(buf), ":%x", ntohs(dw[i++]));
}

const char* inet_ntop(int af, const void *cp, char *buf, size_t len)
{
  char buf2[sizeof "1234:5678:9abc:def0:1234:5678:255.255.255.255" + 1];
  Q_UINT8 *data = (Q_UINT8*)cp;

  if (af == AF_INET)
    {
      sprintf(buf2, "%u.%u.%u.%u", data[0], data[1], data[2], data[3]);

      if (len > strlen(buf2))
	{
	  strcpy(buf, buf2);
	  return buf;
	}

      errno = ENOSPC;
      return NULL;		// failed
    }

# ifdef AF_INET6
  if (af == AF_INET6)
    {
      Q_UINT16 *p = (Q_UINT16*)data;
      Q_UINT16 *longest = NULL, *cur = NULL;
      int longest_length = 0, cur_length;
      int i;

      if (KDE_IN6_IS_ADDR_V4MAPPED(p) || KDE_IN6_IS_ADDR_V4COMPAT(p))
	sprintf(buf2, "::%s%u.%u.%u.%u",
		KDE_IN6_IS_ADDR_V4MAPPED(p) ? "ffff:" : "",
		buf[12], buf[13], buf[14], buf[15]);
      else
	{
	  // find the longest sequence of zeroes
	  for (i = 0; i < 8; i++)
	    if (cur == NULL && p[i] == 0)
	      {
		// a zero, start the sequence
		cur = p + i;
		cur_length = 1;
	      }
	    else if (cur != NULL && p[i] == 0)
	      // part of the sequence
	      cur_length++;
	    else if (cur != NULL && p[i] != 0)
	      {
		// end of the sequence
		if (cur_length > longest_length)
		  {
		    longest_length = cur_length;
		    longest = cur;
		  }
		cur = NULL;		// restart sequence
	      }
	  if (cur != NULL && cur_length > longest_length)
	    {
	      longest_length = cur_length;
	      longest = cur;
	    }

	  if (longest_length > 1)
	    {
	      // We have a candidate
	      buf2[0] = '\0';
	      if (longest != p)
		add_dwords(buf2, p, longest - p);
	      strcat(buf2, "::");
	      if (longest + longest_length < p + 8)
		add_dwords(buf2, longest + longest_length, 8 - (longest - p) - longest_length);
	    }
	  else
	    {
	      // Nope, no candidate
	      buf2[0] = '\0';
	      add_dwords(buf2, p, 8);
	    }
	}

      if (strlen(buf2) < len)
	{
	  strcpy(buf, buf2);
	  return buf;
	}

      errno = ENOSPC;
      return NULL;
    }
# endif

  errno = EAFNOSUPPORT;
  return NULL;			// a family we don't know about
}

#else	// HAVE_INET_NTOP

#define KRF_inet_ntop		0

#endif	// HAVE_INET_NTOP

#ifndef HAVE_INET_PTON

#define KRF_inet_pton		KRF_USING_OWN_INET_PTON
int inet_pton(int af, const char *cp, void *buf)
{
  if (af == AF_INET)
    {
      // Piece of cake
      unsigned p[4];
      unsigned char *q = (unsigned char*)buf;
      if (sscanf(cp, "%u.%u.%u.%u", p, p + 1, p + 2, p + 3) != 4)
	return 0;

      if (p[0] > 0xff || p[1] > 0xff || p[2] > 0xff || p[3] > 0xff)
	return 0;

      q[0] = p[0];
      q[1] = p[1];
      q[2] = p[2];
      q[3] = p[3];

      return 1;
    }

# ifdef AF_INET6
  else if (af == AF_INET6)
    {
      Q_UINT16 addr[8];
      const char *p = cp;
      int n = 0, start = 8;
      bool has_v4 = strchr(p, '.') != NULL;

      memset(addr, 0, sizeof(addr));

      if (*p == '\0' || p[1] == '\0')
	return 0;		// less than 2 chars is not valid

      if (*p == ':' && p[1] == ':')
	{
	  start = 0;
	  p += 2;
	}
      while (*p)
	{
	  if (has_v4 && inet_pton(AF_INET, p, addr + n) != 0)
	    {
	      // successful v4 convertion
	      addr[n] = ntohs(addr[n]);
	      n++;
	      addr[n] = ntohs(addr[n]);
	      n++;
	      break;
	    }
	  if (sscanf(p, "%hx", addr + n++) != 1)
	    return 0;

	  while (*p && *p != ':')
	    p++;
	  if (!*p)
	    break;
	  p++;

	  if (*p == ':')	// another ':'?
	    {
	      if (start != 8)
		return 0;	// two :: were found
	      start = n;
	      p++;
	    }
	}

      // if start is not 8, then a "::" was found at word 'start'
      // n is the number of converted words
      // n == 8 means everything was converted and no moving is necessary
      // n < 8 means that we have to move n - start words 8 - n words to the right
      if (start == 8 && n != 8)
	return 0;		// bad conversion
      memmove(addr + start + (8 - n), addr + start, (n - start) * sizeof(Q_UINT16));
      memset(addr + start, 0, (8 - n) * sizeof(Q_UINT16));

      // check the byte order
      // The compiler should optimise this out in big endian machines
      if (htons(0x1234) != 0x1234)
	for (n = 0; n < 8; n++)
	  addr[n] = htons(addr[n]);

      memcpy(buf, addr, sizeof(addr));
      return 1;
    }
# endif

  errno = EAFNOSUPPORT;
  return -1;			// unknown family
}

#else  // HAVE_INET_PTON

#define KRF_inet_pton		0

#endif // HAVE_INET_PTON

#ifdef AF_INET6
# define KRF_afinet6	KRF_KNOWS_AF_INET6
#else
# define KRF_afinet6	0
#endif

namespace KDE
{
  /** @internal */
  extern const int resolverFlags = KRF_getaddrinfo | KRF_resolver | KRF_afinet6 | KRF_inet_ntop | KRF_inet_pton;
}
