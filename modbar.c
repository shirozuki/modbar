#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <X11/Xlib.h>

/* Max command output length */
#define BUFSIZE 128

/* Max cmd length a named pipe can take */
#define MAX_CMD_LEN 32

typedef struct {
	char	*command;
	int 	interval;
} Module;

#include "config.h"

/* Variables */
static Display 	*display 	= NULL;

static int		modnum;
static int		*interval 	= NULL;
static char		*status 	= NULL;
static char		**modstatus = NULL;

/* Statusbar modification / XSetRoot mutex and modrebuild mutex */
pthread_mutex_t mutex0, mutex1;

/* Function declarations */
static void execute(char *, char *);
static void refreshsb(void);
static void initsb(void);
static void matchcmd(char *);
static bool modrebuild(int);
static void *schedule(void *);
static void *npipe(void *);

void
execute(char *command, char *output)
{
	FILE *fp = popen(command, "r");
	fgets(output, BUFSIZE, fp);
	pclose(fp);
}

void
refreshsb(void)
{
	pthread_mutex_lock(&mutex0);
	memset(status, 0, strlen(status));
	for (int i = 0; i < modnum; i++) {
		strcat(status, modstatus[i]);
		if(SEPARATOR[0] != '\0' && i < modnum-1) {
			strcat(status, SEPARATOR);
		}
	}

	XStoreName(display, DefaultRootWindow(display), status);
	XSync(display, False);
	pthread_mutex_unlock(&mutex0);
}

bool
modrebuild(int i)
{
	pthread_mutex_lock(&mutex1);

	char *output = calloc(BUFSIZE, sizeof(char));
	execute(modules[i].command, output);

	bool found = false;
	if (strcmp(output, modstatus[i])) {
		memset(modstatus[i], 0, strlen(modstatus[i]));
		strncpy(modstatus[i], output, strlen(output));
		found = true;
	}

	free(output);
	pthread_mutex_unlock(&mutex1);
	
	return found;
}

void
initsb(void)
{
	for (int i = 0; i < modnum; i++)
		modrebuild(i);
	refreshsb();
}

void
matchcmd(char *command)
{
	for (int i = 0; i < modnum; i++) {
		if (!strcmp(command, modules[i].command)) {
			if(modrebuild(i)) {
				refreshsb();
			}
		}
	}
}

void *
schedule(void *tid) {
	for(bool refresh; true; sleep(1), refresh = false) {
		for (int i = 0; i < modnum; i++) {
			if (interval[i] == -1) {
				continue;
			} 
			if (interval[i] > 0) {
				--interval[i];
				continue;
			} else if (interval[i] == 0) {
				modrebuild(i) && (refresh = true);
				interval[i] = modules[i].interval-1;
			}
		}
		if(refresh) {
			refreshsb();
			refresh = false;
		}
	}
	
	pthread_exit("t_schedule");
}

void *
npipe(void *tid) {
	mkfifo(PIPE_PATH, 0600);
	FILE *fp = fopen(PIPE_PATH, "r+");

	int i, c;
	static char buffer[MAX_CMD_LEN];

	while(true) {
		c = i = 0;
		memset(buffer, 0, strlen(buffer));
		while((c = fgetc(fp)) != '\n' && c != '\0' && c != ' ' && c != '\t') {
			if (i < MAX_CMD_LEN-1)
				buffer[i++] = c;
		}
		buffer[i] = '\0';
		matchcmd(buffer);
	}

	fclose(fp);
	pthread_exit("t_npipe");
}

int
main(void)
{
	/* Open X display */
	if(!(display = XOpenDisplay(NULL))) {
		fprintf(stderr, "modbar: could not open display\n");
		return 1;
	}

	/* Determine number of modules */
	modnum 		= sizeof(modules) / sizeof(modules[0]);
	
	/* Initialize arrays */
	status		= calloc(modnum*BUFSIZE+(modnum-2)*strlen(SEPARATOR), sizeof(char));

	interval	= calloc(modnum, sizeof(int));
	for (int i = 0; i < modnum; i++)
		interval[i] = (int) modules[i].interval-1;

	modstatus 	= calloc(modnum, sizeof(int*));
	for(int i = 0; i < modnum; i++)
		modstatus[i] = calloc(BUFSIZE, sizeof(char));

	/* Build initial Statusbar */
	initsb();

	/* Declare threads */
	pthread_t t_schedule, t_npipe;

	/* Initialize mutex locks */
	pthread_mutex_init(&mutex0, NULL);
	pthread_mutex_init(&mutex1, NULL);

	/* Create threads */
	if(pthread_create(&t_schedule, NULL, schedule, "t_schedule")) {
		fprintf(stderr, "modbar: pthread_create() error\n");
		exit(EXIT_FAILURE);
	}
		
	if(pthread_create(&t_npipe, NULL, npipe, "t_npipe")) {
		fprintf(stderr, "modbar: pthread_create() error\n");
		exit(EXIT_FAILURE);
	}

	/* Wait for threads to finish */
	void *pschedule;
	if (pthread_join(t_schedule, &pschedule)) {
    	perror("modbar: pthread_join() error");
    	exit(EXIT_FAILURE);
  	}

	void *ppipe;
	if (pthread_join(t_npipe, &ppipe)) {
    	perror("modbar: pthread_join() error");
    	exit(EXIT_FAILURE);
  	}
	
	/* Exit sequence */
	for(int i = 0; i < modnum; i++)
		free(modstatus[i]);
	free(modstatus);
	free(status);
	free(interval);
	XCloseDisplay(display);

	return 0;
}
