#include "dav.h"
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int		fd_recv = -1,
				fd_send = -1;
static struct sockaddr_in
				addr_send,
				addr_recv;

typedef struct
	{
	char	*name;
	int		message_id;
	int		time;
	}
MulticastHost;

static SList	*multicast_host_list;

void
multicast_send(char *seq, char *message)
	{
	char	buf[256];

	if (fd_send < 0 || !dav.multicast_enable)
		return;
	if (!seq)
		seq = "";
	if (!message || *message == '\0')
		return;
	snprintf(buf, sizeof(buf), "%s%s %s\n", dav.hostname, seq, message);
	sendto(fd_send, buf, strlen(buf), 0,
				(struct sockaddr *) &addr_send, sizeof(addr_send));
	}

static boolean
hostname_match(char *from, char *to)
	{
	char	*next;

	if (!strcasecmp(to, "all"))
		return TRUE;
	if (!strcasecmp(to, "all!") || !strcasecmp(to, "!all"))
		{
		if (!strcmp(from, dav.hostname))
			return FALSE;
		else
			return TRUE;
		}

	while (*to)
		{
		next = strchr(to, ',');
		if (next)
			*next++ = '\0';
		if (!strcmp(to, dav.hostname))
			return TRUE;
		to = next ? next : "";
		}
	return FALSE;
	}

static MulticastHost *
multicast_host_find(char *hostname)
	{
    MulticastHost	*host;
    SList			*list;

    for (list = multicast_host_list; list; list = list->next)
        {
        host = (MulticastHost *) list->data;
        if (!strcmp(host->name, hostname))
            return host;
        }
    return NULL;

	}

  /* If there is a record of a multicast message from hostname within a
  |  threshold time with this message_id number, then consider that we are
  |  getting a repeat transmission of an already received message.
  */
static boolean
multicast_message_id_repeat(char *hostname, int message_id)
	{
    MulticastHost	*host;
	boolean			result = FALSE;

	host = multicast_host_find(hostname);
	if (!host)
		{
		host = (MulticastHost *) calloc(1, sizeof(MulticastHost));
		host->name = strdup(hostname);
		multicast_host_list = slist_append(multicast_host_list, host);
		}
	else if (   message_id > 0
		     && host->message_id == message_id
		     && dav.t_now - host->time < 3
		    )
			result = TRUE;

	host->message_id = message_id;
	host->time = dav.t_now;
	return result;
	}

static void
multicast_ack(char *to_host, int message_id)
	{
	char	message[200];

	snprintf(message, sizeof(message), "%s ack %d", to_host, message_id);
	multicast_send("", message);
	}

void
multicast_recv(void)
	{
	socklen_t	addrlen;
	int			msg_id, n, nbytes = 0;
	char		*line, *eol, *s, recv_buf[1025];
	char		from[128], to[256], message[256];
	char		msg_type[32], action[256];
	boolean		repeat, match;

	if (fd_recv < 0 || !dav.multicast_enable)
		return;
	ioctl(fd_recv, FIONREAD, &nbytes);
	if (nbytes <= 0)
		return;
	addrlen = sizeof(addr_recv);
	if ((nbytes = recvfrom(fd_recv, recv_buf, sizeof(recv_buf) - 1, 0,
						(struct sockaddr *) &addr_recv, &addrlen)) <= 0)
		return;
	if (recv_buf[nbytes - 1] != '\n')
		recv_buf[nbytes++] = '\n';
	recv_buf[nbytes] = '\0';
	line = recv_buf;
	while (*line)
		{
		eol = strchr(line, '\n');
		if (!eol)
			break;
		*eol++ = '\0';
		msg_id = 0;
		repeat = FALSE;
		from[0] = to[0] = message[0] = '\0';
		n = sscanf(line, "%127s %255s %255[^\n]", from, to, message);
		if (dav.verbose_multicast)
			printf("multicast recv: <%s> <%s> <%s>\n", from, to, message);
		if ((s = strchr(from, ':')) != NULL)
			{
			*s++ = '\0';
			msg_id = atoi(s);
			repeat = multicast_message_id_repeat(from, msg_id);
			}
		if (n == 3)
			{
			if ((match = hostname_match(from, to)) && !repeat)
				{
				if (dav.verbose_multicast)
					printf("  message accepted for %s\n", dav.hostname);
				if (sscanf(message, "%31s %255[^\n]", msg_type, action) == 2)
					{
					dup_string(&dav.multicast_from_hostname, from);
					if (!strcmp(msg_type, "command"))
						exec_no_wait(action, NULL, TRUE);
					else if (!strcmp(msg_type, "pkc-message"))	/* user defined */
						exec_no_wait(dav.on_multicast_message_cmd, action, TRUE);
					/* ack and other message types are ignored */
					}
				else
					if (dav.verbose_multicast)
						printf("  msg_type action parse fail.\n");
				}
			else
				if (dav.verbose_multicast)
					printf("  message rejected: %s\n",
						!match ? "hostname match failed" : "repeated message");
			if (match && msg_id > 0)
				multicast_ack(from, msg_id);
			}
		line = eol;
		}
	}

void
multicast_init(void)
	{
	struct ip_mreq		mreq;
	int					opt = TRUE;

	if (!dav.multicast_enable)
		return;

	/* Receiving multicast needs a UDP socket which must be reusable.
	*/
	if (   (fd_recv = socket(AF_INET, SOCK_DGRAM, 0)) >= 0
	    && setsockopt(fd_recv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) >= 0
	   )
		{
		/* To receive multicast, bind to the multicast group port and
		|  join the multicast group by adding membership to the group IP.
		|  (Membership selects the group IP so the bind addr should be ANY).
		*/
		addr_recv.sin_family = AF_INET;
		addr_recv.sin_addr.s_addr = htonl(INADDR_ANY);
		addr_recv.sin_port = htons(dav.multicast_group_port);

		mreq.imr_multiaddr.s_addr = inet_addr(dav.multicast_group_IP);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);

		if (   bind(fd_recv, (struct sockaddr *) &addr_recv,
						sizeof(addr_recv)) < 0
		    || setsockopt(fd_recv, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
						sizeof(mreq)) < 0
		   )
			{
			close(fd_recv);
			fd_recv = -1;
			}
		}

	/* Sending multicast needs a UDP socket and needs a send address set up
	|  to send to the multicast group IP and group port number.
	*/
	fd_send = socket(AF_INET, SOCK_DGRAM, 0);
	addr_send.sin_family = AF_INET;
	addr_send.sin_addr.s_addr = inet_addr(dav.multicast_group_IP);
	addr_send.sin_port = htons(dav.multicast_group_port);


	}


