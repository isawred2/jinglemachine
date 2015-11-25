#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

#include "play.c"

static snd_seq_t *seq;
static snd_seq_addr_t *ports;
static volatile sig_atomic_t stop = 0;

/* prints an error message to stderr, and dies */
static void fatal(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(EXIT_FAILURE);
}

/* memory allocation error handling */
static void check_mem(void *p)
{
	if (!p)
		fatal("Out of memory");
}


static void check_snd(char *str, int err)
{
	if (err < 0)
	{
		fprintf(stderr, "Error %s\n", str);
	}
}

static void init_seq(void)
{
	int err;

	/* open sequencer */
	err = snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
	check_snd("open sequencer", err);

	/* set our client's name */
	err = snd_seq_set_client_name(seq, "jinglemachine");
	check_snd("set client name", err);

	err = snd_seq_create_simple_port(seq, "jinglemachine",
			SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
			SND_SEQ_PORT_TYPE_APPLICATION);
	check_snd("creating sequencer port", err);

	ports = realloc(ports, sizeof(snd_seq_addr_t));
	check_mem(ports);

	err = snd_seq_parse_address(seq, &ports[0], "20:0");
	check_snd("parse address", err);

	err = snd_seq_connect_from(seq, 0, ports[0].client, ports[0].port);
	check_snd("connect ports", err);
}

uint note_to_pad(uint note)
{
	switch(note) {
		case 36:
			return 1;
		case 37:
			return 2;
		case 54:
			return 3;
		case 49:
			return 4;
		case 67:
			return 5;
		case 69:
			return 6;
		case 71:
			return 7;
		case 77:
			return 8;
		default:
			return 9;
	}
}

void play_note(uint note, uint on)
{
	uint pad = note_to_pad(note);
	fprintf(stderr, "\r%s%i", on ? "On  " : "Off ", pad);
	play("/home/jeena/Downloads/afraid.wav");
}

void midi_action() {

	snd_seq_event_t *ev;

	do {
		snd_seq_event_input(seq, &ev);
		switch (ev->type) {
			case SND_SEQ_EVENT_NOTEON:
				play_note(ev->data.note.note, 1);
				break;
			case SND_SEQ_EVENT_NOTEOFF:
				play_note(ev->data.note.note, 0);
				break;
		}
		snd_seq_free_event(ev);
	} while (snd_seq_event_input_pending(seq, 0) > 0);
}


int main(int argc, char *argv[])
{
	init_seq();

	int npfd;
	struct pollfd *pfd;

	npfd = snd_seq_poll_descriptors_count(seq, POLLIN);
	pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
	check_mem(pfd);
	snd_seq_poll_descriptors(seq, pfd, npfd, POLLIN);

	while(1) {
		if (poll(pfd, npfd, -1) > 0) {
			midi_action();
		}
	}
}