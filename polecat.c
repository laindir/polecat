/*

Copyright 2012 Carl D Hamann

This file is part of polecat.

polecat is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

polecat is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with polecat.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zmq.h>
#include "fifo.h"
#include "cleanup.h"

enum level
{
	DEBUG = 0,
	INFO = 1,
	WARNING = 2,
	NONFATAL = 3,
	FATAL = 4
};

static enum level loglevel = FATAL;

int
try(const char* trace, int success, enum level level)
{
	if(loglevel == DEBUG || (!success && level >= loglevel))
	{
		perror(trace);
	}

	if(!success && level >= FATAL)
	{
		exit(EXIT_FAILURE);
	}

	return success;
}

void
cu_zmq_context(void *data)
{
	zmq_term(data);
}

void
cu_zmq_socket(void *data)
{
	zmq_close(data);
}

void
cu_zmq_msg(void *data)
{
	zmq_msg_close((zmq_msg_t *)data);
}

void
cu_fifo(void *data)
{
	fifo_destroy((struct fifo *)data);
}

int
produce(zmq_pollitem_t *poll, struct fifo *fifo)
{
	int i;
	zmq_msg_t msg;

	if(is_full(fifo))
	{
		return -1;
	}

	cleanup_save();

	if(poll->socket)
	{
		if(try("zmq_msg_init", zmq_msg_init(&msg) == 0, NONFATAL))
		{
			try("cleanup_push zmq_msg_close", cleanup_push(cu_zmq_msg, &msg) == 0, NONFATAL);
		}
		try("zmq_recv", zmq_recv(poll->socket, &msg, 0) == 0, NONFATAL);

		if(zmq_msg_size(&msg) > (unsigned int)space_left(fifo))
		{
			cleanup_rewind();
			return -1;
		}

		i = zmq_msg_size(&msg);
		memcpy(fifo->buffer + produce_index(fifo), zmq_msg_data(&msg), i);
	}
	else
	{
		try("read", (i = read(poll->fd, fifo->buffer + produce_index(fifo), space_left(fifo))) >= 0, NONFATAL);
	}

	if(i > 0)
	{
		fifo->produced += i;
	}

	cleanup_rewind();
	return i;
}

int
consume(zmq_pollitem_t *poll, struct fifo *fifo)
{
	int i;
	zmq_msg_t msg;

	if(is_empty(fifo))
	{
		return -1;
	}

	cleanup_save();

	if(poll->socket)
	{
		i = space_filled(fifo);
		if(try("zmq_msg_init_size", zmq_msg_init_size(&msg, i) == 0, NONFATAL))
		{
			try("cleanup_push zmq_msg_close", cleanup_push(cu_zmq_msg, &msg) == 0, NONFATAL);
		}

		memcpy(zmq_msg_data(&msg), fifo->buffer + consume_index(fifo), i);

		try("zmq_send", zmq_send(poll->socket, &msg, 0) == 0, NONFATAL);
	}
	else
	{
		try("write", (i = write(poll->fd, fifo->buffer + consume_index(fifo), space_filled(fifo))) >= 0, NONFATAL);
	}

	if(i > 0)
	{
		fifo->consumed += i;
	}

	cleanup_rewind();
	return i;
}

int
zmq_eof(zmq_pollitem_t *poll)
{
	zmq_msg_t msg;

	if(poll->socket)
	{
		cleanup_save();
		if(try("zmq_msg_init", zmq_msg_init(&msg) == 0, NONFATAL))
		{
			try("cleanup_push zmq_msg_clse", cleanup_push(cu_zmq_msg, &msg) == 0, NONFATAL);
		}
		try("zmq_send", zmq_send(poll->socket, &msg, 0) == 0, NONFATAL);
		cleanup_rewind();
	}

	return 0;
}

int
poll_loop(zmq_pollitem_t* polls)
{
	int check;
	struct fifo *up_fifo;
	struct fifo *dn_fifo;

	if(try("fifo_init", (up_fifo = fifo_init(1024)) != NULL, FATAL))
	{
		try("cleanup_push fifo_destroy", cleanup_push(cu_fifo, up_fifo) == 0, NONFATAL);
	}
	if(try("fifo_init", (dn_fifo = fifo_init(1024)) != NULL, FATAL))
	{
		try("cleanup_push fifo_destroy", cleanup_push(cu_fifo, dn_fifo) == 0, NONFATAL);
	}

	while(polls[0].events || polls[1].events || polls[2].events || polls[3].events)
	{
		check = zmq_poll(polls, 4, -1);

		if(polls[0].revents & ZMQ_POLLIN)
		{
			check = produce(&polls[0], up_fifo);
			if(check == 0)
			{
				polls[0].events = 0;
			}
		}

		if(polls[3].revents & ZMQ_POLLIN)
		{
			check = produce(&polls[3], dn_fifo);
			if(check == 0)
			{
				polls[3].events = 0;
			}
		}

		if(polls[1].revents & ZMQ_POLLOUT)
		{
			check = consume(&polls[1], dn_fifo);
		}

		if(polls[2].revents & ZMQ_POLLOUT)
		{
			check = consume(&polls[2], up_fifo);
		}

		polls[1].events = is_empty(dn_fifo) ? 0 : ZMQ_POLLOUT;
		polls[2].events = is_empty(up_fifo) ? 0 : ZMQ_POLLOUT;

		if(is_empty(up_fifo) && polls[0].events == 0)
		{
			zmq_eof(&polls[2]);
		}
	}

	return 0;
}

int
setup_sockets(zmq_pollitem_t *polls, void *zmq_context, int argc, char *argv[])
{
	const char *options = "bu:d:t:i:";
	int opt;
	int bind = 0;
	int sock_type = ZMQ_PUB;
	char *ident = NULL;

	opt = getopt(argc, argv, options);

	while(opt != -1)
	{
		switch(opt)
		{
		case 'b':
			bind = 1;
			break;
		case 'u':
			if(try("zmq_socket", (polls[2].socket = zmq_socket(zmq_context, sock_type)) != NULL, FATAL))
			{
				try("cleanup_push zmq_close", cleanup_push(cu_zmq_socket, polls[2].socket) == 0, FATAL);
			}
			try("zmq_setsockopt", (ident ? zmq_setsockopt(polls[2].socket, ZMQ_IDENTITY, (void *)ident, strlen(ident)) : 0) == 0, FATAL);
			try(bind ? "zmq_bind" : "zmq_connect", (bind ? zmq_bind(polls[2].socket, optarg) : zmq_connect(polls[2].socket, optarg)) == 0, FATAL);
			sock_type = ZMQ_PULL;
			bind = 0;
			break;
		case 'd':
			if(try("zmq_socket", (polls[3].socket = zmq_socket(zmq_context, sock_type)) != NULL, FATAL))
			{
				try("cleanup_push zmq_close", cleanup_push(cu_zmq_socket, polls[3].socket) == 0, FATAL);
			}
			try("zmq_setsockopt", (ident ? zmq_setsockopt(polls[3].socket, ZMQ_IDENTITY, (void *)ident, strlen(ident)) : 0) == 0, FATAL);
			try(bind ? "zmq_bind" : "zmq_connect", (bind ? zmq_bind(polls[3].socket, optarg) : zmq_connect(polls[3].socket, optarg)) == 0, FATAL);
			sock_type = ZMQ_PUB;
			bind = 0;
			break;
		case 't':
			if(strcmp(optarg, "REQ") == 0)
			{
				sock_type = ZMQ_REQ;
			}
			else if(strcmp(optarg, "REP") == 0)
			{
				sock_type = ZMQ_REP;
			}
			else if(strcmp(optarg, "DEALER") == 0)
			{
				sock_type = ZMQ_DEALER;
			}
			else if(strcmp(optarg, "ROUTER") == 0)
			{
				sock_type = ZMQ_ROUTER;
			}
			else if(strcmp(optarg, "PUB") == 0)
			{
				sock_type = ZMQ_PUB;
			}
			else if(strcmp(optarg, "SUB") == 0)
			{
				sock_type = ZMQ_SUB;
			}
			else if(strcmp(optarg, "PUSH") == 0)
			{
				sock_type = ZMQ_PUSH;
			}
			else if(strcmp(optarg, "PULL") == 0)
			{
				sock_type = ZMQ_PULL;
			}
			break;
		case 'i':
			ident = optarg;
			break;
		}

		opt = getopt(argc, argv, options);
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	void *context;

	zmq_pollitem_t polls[4] = {
		{NULL, 0, ZMQ_POLLIN, 0},
		{NULL, 1, 0, 0},
		{NULL, 0, 0, 0},
		{NULL, 0, ZMQ_POLLIN, 0}
	};

	try("atexit", atexit(cleanup_rewind) == 0, FATAL);

	if(try("zmq_init", (context = zmq_init(1)) != NULL, FATAL))
	{
		try("cleanup_push zmq_term", cleanup_push(cu_zmq_context, context) == 0, FATAL);
	}

	setup_sockets(polls, context, argc, argv);

	poll_loop(polls);

	exit(EXIT_SUCCESS);
}
