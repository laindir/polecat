#include <stdlib.h>
#include <unistd.h>
#include <zmq.h>
#include "fifo.h"

int
produce(zmq_pollitem_t *poll, struct fifo *fifo)
{
	int i;
	zmq_msg_t msg;

	if(is_full(fifo))
	{
		return -1;
	}

	if(poll->socket)
	{
		zmq_msg_init(&msg);
		zmq_recv(poll->socket, &msg, 0);

		if(zmq_msg_size(&msg) > (unsigned int)space_left(fifo))
		{
			return -1;
		}

		for(i = 0; (unsigned int)i < zmq_msg_size(&msg); i++)
		{
			fifo->buffer[produce_index(fifo) + i] = ((char *)zmq_msg_data(&msg))[i];
		}

		zmq_msg_close(&msg);
	}
	else
	{
		i = read(poll->fd, fifo->buffer + produce_index(fifo), space_left(fifo));
	}

	if(i > 0)
	{
		fifo->produced += i;
	}

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

	if(poll->socket)
	{
		zmq_msg_init_size(&msg, space_filled(fifo));

		for(i = 0; i < space_filled(fifo); i++)
		{
			((char *)zmq_msg_data(&msg))[i] = fifo->buffer[consume_index(fifo) + i];
		}

		zmq_send(poll->socket, &msg, 0);
		zmq_msg_close(&msg);
	}
	else
	{
		i = write(poll->fd, fifo->buffer + consume_index(fifo), space_filled(fifo));
	}

	if(i > 0)
	{
		fifo->consumed += i;
	}

	return i;
}

int
zmq_eof(zmq_pollitem_t *poll)
{
	zmq_msg_t msg;

	if(poll->socket)
	{
		zmq_msg_init(&msg);
		zmq_send(poll->socket, &msg, 0);
		zmq_msg_close(&msg);
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	struct fifo *up_fifo;
	struct fifo *dn_fifo;
	void *context;
	void *up_socket;
	void *dn_socket;
	int up_done = 0;
	int dn_done = 0;
	int check;

	zmq_pollitem_t polls[4] = {
		{NULL, 0, ZMQ_POLLIN, 0},
		{NULL, 1, 0, 0},
		{NULL, 0, 0, 0},
		{NULL, 0, ZMQ_POLLIN, 0}
	};

	up_fifo = fifo_init(1024);
	dn_fifo = fifo_init(1024);

	context = zmq_init(1);
	up_socket = zmq_socket(context, ZMQ_PUSH);
	dn_socket = zmq_socket(context, ZMQ_PULL);

	polls[2].socket = up_socket;
	polls[3].socket = dn_socket;

	if(argc < 3)
	{
		exit(EXIT_FAILURE);
	}

	check = zmq_connect(up_socket, argv[1]);
	check = zmq_bind(dn_socket, argv[2]);

	while(!up_done || !dn_done)
	{
		check = zmq_poll(polls, 4, -1);

		if(polls[0].revents & ZMQ_POLLIN)
		{
			check = produce(&polls[0], up_fifo);
			if(check == 0)
			{
				up_done = 1;
				zmq_eof(&polls[2]);
			}
		}

		if(polls[3].revents & ZMQ_POLLIN)
		{
			check = produce(&polls[3], dn_fifo);
			if(check == 0)
			{
				dn_done = 1;
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
	}

	check = zmq_close(dn_socket);
	check = zmq_close(up_socket);
	check = zmq_term(context);

	fifo_destroy(dn_fifo);
	fifo_destroy(up_fifo);

	exit(EXIT_SUCCESS);
}
