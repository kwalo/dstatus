#include "mixer.h"
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <X11/Xlib.h>

#define MAXLEN 30
#define VOLUME_NA     " ♬  n/a | "
#define VOLUME_FORMAT " ♬ %3d%% | "
#define TIME_FORMAT   "%a %b %d %H:%M "

static int done = 0;

static int
fatal_error(const char *msg, int err)
{
        fprintf(stderr, "Error: %s: %s\n", msg, strerror(err));
        return 1;
}

struct tm *
get_localtime()
{
        struct tm *tm;
        time_t t = time(NULL);

        if (t < 0) {
                return NULL;
        }

        tm = localtime(&t);
        if (!tm) {
                return NULL;
        }

        return tm;
}

static void
difftimespec(struct timespec *res, const struct timespec *a, const struct timespec *b)
{
        res->tv_sec = a->tv_sec - b->tv_sec - (a->tv_nsec < b->tv_nsec);
        res->tv_nsec = a->tv_nsec - b->tv_nsec +
                (a->tv_nsec < b->tv_nsec) * 1E9;
}

static void
terminate(const int signo)
{
        (void)signo;

        done = 1;
}

int
main(int argc, char *argv[])
{
        Display *dpy;
        struct sigaction act;
        struct timespec start, current, elapsed, interval, wait;
        char status[MAXLEN];
        struct tm *tm;
        int err;
        int retval = 0;
        int printed = 0;
        int timeout;

        snd_mixer_t *mixer = NULL;
        snd_mixer_elem_t *elem;
        int volume;

        struct pollfd *pollfds = NULL;
        int n, nfds = 0;
        unsigned short revents;

        (void)argc;
        (void)argv;

        interval.tv_nsec = 0;

        memset(&act, 0, sizeof(act));
        act.sa_handler = terminate;
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);

        dpy = XOpenDisplay(NULL);
        if (!dpy) {
                perror("Failed to open display");
                return 1;
        }

        if (XStoreName(dpy, DefaultRootWindow(dpy), "") < 0) {
                retval = fatal_error("XStoreName failed", errno);
                done = 1;
        }
        XFlush(dpy);

        if (!done) {
                err = setup_mixer(&mixer);
                if (err != 0) {
                        retval = fatal_error("setup_mixer", err);
                        done = 1;
                }

                elem = find_mixer_elem(mixer);
                if (elem == NULL) {
                        done = 1;
                }

        }

        while (!done) {
                if (mixer_volume_changed()) {
                        volume = get_normalized_volume(elem);
                        if (volume < 0) {
                                printed = snprintf(status, MAXLEN, VOLUME_NA);
                        } else {
                                printed = snprintf(status, MAXLEN, VOLUME_FORMAT, volume);
                        }
                }

                if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
                        perror("clock_gettime failed");
                        done = retval = 1;
                        break;
                }

                tm = get_localtime();
                if (!tm) {
                        perror("Cannot get localtime");
                        done = retval = 1;
                        break;
                }

                if (!strftime(&status[printed], sizeof(status) - printed, TIME_FORMAT, tm)) {
                        status[0] = '\0';
                }

                if (XStoreName(dpy, DefaultRootWindow(dpy), status) < 0) {
                        perror("XStoreName failed");
                        done = retval = 1;
                        break;
                }
                XFlush(dpy);

                if (done)
                        break;

                n = snd_mixer_poll_descriptors_count(mixer);

                if (n != nfds) {
                        free(pollfds);
                        nfds = n;
                        pollfds = calloc(nfds, sizeof *pollfds);
                }
                if (pollfds == NULL) {
                        n = 0;
                } else {
                        n = snd_mixer_poll_descriptors(mixer, pollfds, nfds);
                }

                if (err != 0) {
                        fprintf(stderr, "Cannot get poll descriptors: %s\n", strerror(err));
                        nfds = 0;
                }

                if (clock_gettime(CLOCK_MONOTONIC, &current) < 0) {
                        perror("cannot get current clock");
                        done = retval = 1;
                        break;
                }

                difftimespec(&elapsed, &current, &start);
                interval.tv_sec = 60 - tm->tm_sec;
                difftimespec(&wait, &interval, &elapsed);


                timeout = wait.tv_sec * 1000 + wait.tv_nsec / 1E6;

                n = poll(pollfds, nfds, timeout);

                if (n < 0 && errno != EINTR) {
                        perror("poll");
                        retval = 1;
                        break;
                }

                if (n == 0) {
                        continue;
                }

                err = snd_mixer_poll_descriptors_revents(mixer, pollfds, nfds, &revents);
                if (err != 0) {
                        perror("Cannot get poll descriptor events");
                        retval = 1;
                        break;
                }

                if (revents & (POLLERR | POLLNVAL)) {
                        fprintf(stderr, "poll error\n");
                        retval = 1;
                        break;
                } else if (revents & POLLIN) {
                        snd_mixer_handle_events(mixer);
                }


        }

        if (XCloseDisplay(dpy) < 0) {
                perror("XCloseDisplay failed");
                retval = 1;
        }

        free(pollfds);
        if (mixer != NULL)
                snd_mixer_close(mixer);

        return retval;
}
