#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/stat.h>
#include <X11/Xlib.h>

#define BUFSIZE 128
#define MAX_CMD_LEN 32

typedef struct {
        char *command;
        int interval;
} Module;

#include "config.h"
#define LEN(X) (sizeof(X) / sizeof((X)[0]))
#define MODNUM LEN(modules)

static Display *display;
static int interval[MODNUM];
static char modstatus[LEN(modules)][BUFSIZE];
static char status[MODNUM * BUFSIZE + MODNUM * sizeof(SEPARATOR) + 1];

static void
execute(char *command, char *output)
{
        FILE *fp = popen(command, "r");
	if (fp == NULL)
		return;
        fgets(output, BUFSIZE, fp);
        pclose(fp);
}

static void
refreshsb(void)
{
	status[0] = '\0';
        for (size_t i = 0; i < MODNUM; i++) {
                strcat(status, modstatus[i]);
                if(SEPARATOR[0] != '\0' && i < MODNUM-1) {
                        strcat(status, SEPARATOR);
                }
        }

        XStoreName(display, DefaultRootWindow(display), status);
        XSync(display, False);
}

static bool
modrebuild(size_t i)
{
	char output[BUFSIZE] = {0};
        execute(modules[i].command, output);

        bool found = false;
        if (strcmp(output, modstatus[i])) {
		snprintf(modstatus[i], BUFSIZE, "%s", output);
                found = true;
        }

        return found;
}

static void
init(void)
{
        for (size_t i = 0; i < MODNUM; i++)
                modrebuild(i);
        refreshsb();
}

static void
matchcmd(char *command)
{
        for (size_t i = 0; i < MODNUM; i++) {
                if (!strcmp(command, modules[i].command)) {
                        if(modrebuild(i)) {
                                refreshsb();
                        }
                }
        }
}

int
main(void)
{
        if(!(display = XOpenDisplay(NULL))) {
                fprintf(stderr, "modbar: could not open display\n");
		exit(EXIT_FAILURE);
        }

	const char *pipe_path = getenv("MODBAR_PIPE_PATH");
        if (!pipe_path)
                pipe_path = FIFO_PATH;

        if (mkfifo(pipe_path, 0600) == -1) {
		if (errno != EEXIST) {
			perror("mkfifo");
			exit(EXIT_FAILURE);
		}
	}

	int fd;
	if ((fd = open(pipe_path, O_RDWR | O_NONBLOCK)) == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	
        for (size_t i = 0; i < MODNUM; i++)
                interval[i] = (int)modules[i].interval-1;

        init();

	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;

        char buffer[MAX_CMD_LEN];

	while (true) {
		int prt = poll(&pfd, 1, 1000);
		if (prt == 0) {
			bool refresh = false;
			for (size_t i = 0; i < MODNUM; i++) {
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
			}
		} else if (prt > 0) {
			ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
			if (bytes > 0) {
				buffer[bytes] = '\0';
				buffer[strcspn(buffer, "\n")] = '\0';
				matchcmd(buffer);

			}
		}
	}

	close(fd);
        XCloseDisplay(display);

        return 0;
}
