/*$T temp.c GC 1.150 2013-09-13 13:41:37 */

/*
** selectserver.c -- a cheezy multiperson chat server
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <setjmp.h>

#define PORT	"31000"	// port we're listening on
// specific pocket sphinx includes
#include <sphinxbase/err.h>
#include <sphinxbase/ad.h>
#include <sphinxbase/cont_ad.h>
#include "pocketsphinx.h"

// global vars
fd_set					master;		// master file descriptor list
fd_set					read_fds;	// temp file descriptor list for select()
int						fdmax;		// maximum file descriptor number
int						listener;	// listening socket descriptor
int	openhab_fd = -1; // openhab fd to which we write back
static ps_decoder_t *pocket_sphinx;				// pocketsphinx decoder
static jmp_buf		jbuf;
static cmd_ln_t		*config;					// pocketsphin config?? -> CHECK

/* dunno what this does */
static void sighandler(int signo)
{
	longjmp(jbuf, 1);
}

static const arg_t	cont_args_def[] =
{
	POCKETSPHINX_OPTIONS,

	/* Argument file. */
	{ "-argfile", ARG_STRING, NULL, "Argument file giving extra arguments." },
	{ "-adcdev", ARG_STRING, NULL, "Name of audio device to use for input." },
	{ "-infile", ARG_STRING, NULL, "Audio file to transcribe." },
	{ "-time", ARG_BOOLEAN, "no", "Print word times in file transcription." },
	CMDLN_EMPTY_OPTION
};

	
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if(sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

/* Sleep for specified msec */
void sleep_msec(int ms)
{
	struct timeval	tmo;
	tmo.tv_sec = 0;
	tmo.tv_usec = ms * 1000;
	select(0, NULL, NULL, NULL, &tmo);
}

void setup_listening_socket() {
	struct addrinfo			hints, *ai, *p;
	int						yes = 1;	// for setsockopt() SO_REUSEADDR, below
	int rv;
	
	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	if((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0)
	{
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}

	for(p = ai; p != NULL; p = p->ai_next)
	{
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(listener < 0)
		{
			continue;
		}

		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if(bind(listener, p->ai_addr, p->ai_addrlen) < 0)
		{
			close(listener);
			continue;
		}
		break;
	}

	// if we got here, it means we didn't get bound
	if(p == NULL)
	{
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai);					// all done with this

	// listen
	if(listen(listener, 10) == -1)
	{
		perror("listen");
		exit(3);
	}

	// add the listener to the master set
	FD_SET(listener, &master);

	// keep track of the biggest file descriptor
	fdmax = listener;					// so far, it's this one
}

void check_incoming_messages() {
	
	int						newfd;		// newly accept()ed socket descriptor
	struct sockaddr_storage remoteaddr; // client address
	socklen_t				addrlen;
	char					buf[256];	// buffer for client data
	int						nbytes;
	char					remoteIP[INET6_ADDRSTRLEN];
	struct timeval tv; // this is the timeout we need to rpevent select() blocking
	char *src, *dst;	// string pointers to clean up incoming msg (strip LF/CR)
	int i, j;
	
	// setup timeout select, 2.5 sec
	tv.tv_sec = 2;
	tv.tv_usec = 500000;
	
	read_fds = master;				// copy it
		if(select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1)
		{
			perror("select");
			exit(4);
		}

		// run through the existing connections looking for data to read
		for(i = 0; i <= fdmax; i++)
		{
			if(FD_ISSET(i, &read_fds))
			{	// we got one!!
				if(i == listener)
				{
					// handle new connections
					addrlen = sizeof remoteaddr;
					newfd = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);
					if(newfd == -1)
					{
						perror("accept");
					}
					else
					{
						FD_SET(newfd, &master); // add to master set
						if(newfd > fdmax)
						{						// keep track of the max
							fdmax = newfd;
						}

						printf
						(
							"selectserver: new connection from %s on socket %d\n",
							inet_ntop
								(
									remoteaddr.ss_family,
									get_in_addr((struct sockaddr *) &remoteaddr),
									remoteIP,
									INET6_ADDRSTRLEN
								),
							newfd
						);
					}
				}
				else
				{
					// handle data from a client
					if((nbytes = recv(i, buf, sizeof buf, 0)) <= 0)
					{
						// got error or connection closed by client
						if(nbytes == 0)
						{
							// connection closed
							printf("selectserver: socket %d hung up\n", i);
						}
						else
						{
							perror("recv");
						}

						close(i);				// bye!
						FD_CLR(i, &master);		// remove from master set
					}
					else
					{
						// we got some data from a client
						// null terminating & stripping
						// overflow check, and at least 7 bytes
						if (nbytes <= sizeof buf && nbytes > 6) {
							buf[nbytes] = '\0';
							printf("Stripping...\n");
							//printf("%d [%s]\n",strlen(buf), buf);
							for (src = dst = buf; *src != '\0'; src++) {
								*dst = *src;
								if (*dst != '\r' && *dst != '\n') dst++;
							}
							*dst = '\0';
							
							//printf("%d [%s]\n",strlen(buf), buf);
							printf("Received from %d: %s\n", i, buf);
							// is this openhab machine, register it
							if (strcmp(buf, "OPENHAB") == 0) {
								openhab_fd = i;
								printf("Registered openhab on %d\n", i);
							} else {
								say(buf);
							}
						} else {
							printf("Buf too small for received bytes or too few bytes: %d\n", nbytes);
						}
					}
				}	// END handle data from client
			}		// END got new incoming connection
			else // END looping through file descriptors
			{		
				//printf("Select() timed out.\n");
			}
		}
}


/*
 stuff to handle execution of TTS based on string; return 0 if OK, -1 if failed
*/

int say(const char msg[])
{
	char command_line[1024] ="echo ";
	// this can probably be done smarter, but for now bunch of strcat()'s
	// "echo %s | festival_client --server FESTIVAL_SERVER --port FESTIVAL_PORT --ttw | aplay -q",	msg, FESTIVAL_SERVER FESTIVAL_PORT
	
	if (msg != NULL ) 
	{
		strcat(command_line, msg);
		strcat(command_line, "| festival_client --server ");
		strcat(command_line, "192.168.1.30"); // FESTIVAL_SERVER
		strcat(command_line, " --port ");
		strcat(command_line, "1314"); // FESTIVAL_PORT
		strcat(command_line, " --ttw | aplay -q");
		
		//printf("exec: %s\n", command_line);
		//printf("Converting received text to TTS: %s\n", msg);

		int retval = system(command_line);

		if(retval == 0)
		{
			printf("Said: %s\n", msg);
			// clear msg
			msg = NULL;
		}
		else
		{
			return retval;
		}
	} else {
		printf("Nothing to say...\n");
	}
}


/*	
 Try to send the recognized string (msg) to openhab. This is tried MAX_ATTEMPTS times, 
 with WAIT_TIME sleep in between. If failed, return error value	
*/
int send_to_openhab(const char *msg)
{
	int retval, j;
	int bytes_out;
	int attempts;
	int MAX_ATTEMPTS = 5;
	int WAIT_TIME = 200;
	// only try to send if we have an openhab socket; or all - some check on size mightbe good
	
	if(openhab_fd != -1)
	{
		for (attempts = 0; attempts < MAX_ATTEMPTS; attempts++) {
			//printf("msg: %s\n", msg);
			//printf("size: %d\n", sizeof msg);
			bytes_out = send(openhab_fd, msg, strlen(msg), 0);
		
		// some checking on len(msg) == bytes_out?
			if (strlen(msg) != bytes_out) {
				printf("Error sending all bytes, msg:%d vs out:%d\n", strlen(msg), bytes_out);
			}
			if (retval == -1) {
				perror("Send");
				printf("Failure writing to openhab #%d, retry\n", attempts);
				sleep_msec(200);
			} else {
				printf("Successfully send: %s (length:%d)\n", msg, strlen(msg));
				break;
				// clear hyp, commandmode (although we want the option to have a loop here; 
				// command is send, if openhab response with UNKNOWN then say "Please Repeat" and keep cmd mode
				//hyp = "";
				//command_mode = 0;
			}
		}
		// if we get here, failure
		//printf("Unable to send msg to openhab");
		// say it
		//say("Error sending command to openhab server");
	} else {
		printf("No openhab server connected, send to all\n");
		// say("No openhab server connected");
		 // we got some data from a client
		for(j = 0; j <= fdmax; j++) {
			// send to everyone!
			if (FD_ISSET(j, &master)) {
				// except the listener and ourselves
				if (j != listener ) {
					if (send(j, msg, strlen(msg), 0) == -1) {
						printf("Error sending to all\n");
						perror("send");
					}
				}
			}
		}
	}
}


void execute_recognition() {

	// sphinx stuff
	ad_rec_t	*ad;
	int16		adbuf[4096];
	int32		k, ts, rem;
	char const	*hyp;
	char const	*uttid;
	cont_ad_t	*cont;

	char answer_buf[] = "ANSWERING";
	int send_msg = 1; // for testing purposes to send only 1 response
	int sleep_count = 0; // number of sleepcycles before we check incoming stuff again
	int command_mode = 0;
	
	if((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"), (int) cmd_ln_float32_r(config, "-samprate"))) == NULL)
		E_FATAL("Failed to open audio device\n");

	/* Initialize continuous listening module */
	if((cont = cont_ad_init(ad, ad_read)) == NULL) E_FATAL("Failed to initialize voice activity detection\n");
	if(ad_start_rec(ad) < 0) E_FATAL("Failed to start recording\n");
	if(cont_ad_calib(cont) < 0) E_FATAL("Failed to calibrate voice activity detection\n");
	
	for(;;) 
	{
	/* Indicate listening for next utterance */
		printf("READY....\n");

		// fflush(stdout);
		// fflush(stderr);

		/* Wait data for next utterance; check for incoming msg's */
		while((k = cont_ad_read(cont, adbuf, 4096)) == 0) 
		{
			// socket loop for reading; only thing we do is say() this unless
			// we receive specific string values, handled in get_incoming_messages
			// only want to check every X seconds
			if (sleep_count == 100) {
				// printf("Check incoming messages for %d fd's...\n", fdmax);
				check_incoming_messages();
				sleep_count = 0;
			}
			sleep_msec(100);
			sleep_count ++;
		}

		if(k < 0)
		{
			E_FATAL("Failed to read audio\n");
		}

		/*
         * Non-zero amount of data received; start recognition of new utterance.
         * NULL argument to uttproc_begin_utt => automatic generation of utterance-id.
         */
		if(ps_start_utt(pocket_sphinx, NULL) < 0)
		{
			E_FATAL("Failed to start utterance\n");
		}

		ps_process_raw(pocket_sphinx, adbuf, k, FALSE, FALSE);
		printf("Listening...\n");

		// fflush(stdout);

		/* Note timestamp for this first block of data */
		ts = cont->read_ts;

		/* Decode utterance until end (marked by a "long" silence, >1sec) */
		for(;;)
		{
			/* Read non-silence audio data, if any, from continuous listening module */
			if((k = cont_ad_read(cont, adbuf, 4096)) < 0)
			{
				E_FATAL("Failed to read audio\n");
			}

			if(k == 0)
			{
				/*
                 * No speech data available; check current timestamp with most recent
                 * speech to see if more than 1 sec elapsed.  If so, end of utterance.
                 */
				if((cont->read_ts - ts) > DEFAULT_SAMPLES_PER_SEC)
				{
					break;
				}
			}
			else
			{
				/* New speech data received; note current timestamp */
				ts = cont->read_ts;
			}

			/*
             * Decode whatever data was read above.
             */
			rem = ps_process_raw(pocket_sphinx, adbuf, k, FALSE, FALSE);

			
			
			/* If no work to be done, sleep a bit */
			if((rem == 0) && (k == 0))
			{
				sleep_msec(20); 
			}
		}

		/*
         * Utterance ended; flush any accumulated, unprocessed A/D data and stop
         * listening until current utterance completely decoded
         */
		ad_stop_rec(ad);
		while(ad_read(ad, adbuf, 4096) >= 0);
		cont_ad_reset(cont);

		printf("Stopped listening, please wait...\n");

		//fflush(stdout);

		/* Finish decoding, obtain and print result */
		ps_end_utt(pocket_sphinx);
		hyp = ps_get_hyp(pocket_sphinx, NULL, &uttid);
		printf("%s: %s\n", uttid, hyp);

		//fflush(stdout);
		//openhab_string = NULL;			// implement -> how to null a string
		if(hyp)
		{
			printf("Processing hyp...\n");
			// if string == keyword, set command mode; how to define keyword??
			if ( strcmp(hyp, "STEVEN") == 0) {
				command_mode = 1;
				printf("Set command_mode = 1\n");
				say("Yes what can I do");
				// if string contains keyword, strip and execute
			} else if ( strncmp(hyp, "STEVEN", 6) > 0 && openhab_fd != -1) {
				command_mode = 0; // overrules current cmd mode
				printf("Set command_mode = 0\n");
				// printf("Set openhab_string to: %s\n", strippedString);
				//openhab_string = hyp - KEYWORD // IMPLEMENT - how to strip
				send_to_openhab(hyp);
				printf("Send to openhab: %s\n", hyp);
			} else {
				// if we were in command_mode, send it 
				if (command_mode == 1) { 
					printf("In command mode, hyp: %s\n", hyp);
					send_to_openhab(hyp);
				}
			}
		}

		/* Resume A/D recording for next utterance */
		if(ad_start_rec(ad) < 0)
		{
			E_FATAL("Failed to start recording\n");
		}
		
	}

	// clean exit
	cont_ad_close(cont);
	ad_close(ad);
		
				

}
/* */
int main(int argc, char *argv[])
{
	char const	*cfg;
	
	
	// network stuff
	FD_ZERO(&master);					// clear the master and temp sets
	FD_ZERO(&read_fds);
	
	if(argc == 2)
	{
		config = cmd_ln_parse_file_r(NULL, cont_args_def, argv[1], TRUE);
	}
	else
	{
		config = cmd_ln_parse_r(NULL, cont_args_def, argc, argv, FALSE);
	}

	/* Handle argument file as -argfile. */
	if(config && (cfg = cmd_ln_str_r(config, "-argfile")) != NULL)
	{
		config = cmd_ln_parse_file_r(config, cont_args_def, cfg, FALSE);
	}

	if(config == NULL) return 1;

	/* setup listening socket */
	setup_listening_socket();
	
	pocket_sphinx = ps_init(config);
	if(pocket_sphinx == NULL) return 1;

	E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);

	/* Make sure we exit cleanly (needed for profiling among other things) */
	signal(SIGINT, &sighandler);
	if(setjmp(jbuf) == 0)
	{
		execute_recognition();
	}

	// free mem, cleanup etc.
	ps_free(pocket_sphinx);

	// other stuff?
	// memfree(); ??
	return 0;
	
}


