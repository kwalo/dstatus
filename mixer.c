#include "mixer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <math.h>

#define exp10(x) (exp((x) * log(10)))


static int update_volume = 1;

int
mixer_volume_changed()
{
        return update_volume;
}

static int
elem_callback(snd_mixer_elem_t *elem, unsigned int mask)
{
        (void)elem;
	if (mask & SND_CTL_EVENT_MASK_VALUE)
		update_volume = 1;

        return 0;
}


static int
mixer_callback(snd_mixer_t *mixer, unsigned int mask, snd_mixer_elem_t *elem)
{
        (void)mixer;
        if (mask & SND_CTL_EVENT_MASK_ADD) {
                snd_mixer_elem_set_callback(elem, elem_callback);
        }
        return 0;
}


int
get_normalized_volume(snd_mixer_elem_t *elem)
{
        int err;
        long min, max, value;
        int mute;
        double normalized, min_norm;
        update_volume = 0;

        err = snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_MONO, &mute);
        if (err != 0 || mute == 0) {
                return -1;
        }

        err = snd_mixer_selem_get_playback_dB_range(elem, &min, &max);
        if (err != 0 || min >= max) {
                return -1;
        }

        err = snd_mixer_selem_get_playback_dB(elem, SND_MIXER_SCHN_MONO, &value);
        if (err != 0) {
                return -1;
        }

        normalized = exp10((value - max) / 6000.0);
        if (min != SND_CTL_TLV_DB_GAIN_MUTE) {
                min_norm = exp10((min - max) / 6000.0);
                normalized = (normalized - min_norm) / (1 - min_norm);
        }

        return (int)(normalized * 100);
}

snd_mixer_elem_t *
find_mixer_elem(snd_mixer_t *mixer)
{
        snd_mixer_elem_t *elem;

        for (elem = snd_mixer_first_elem(mixer);
             elem;
             elem = snd_mixer_elem_next(elem)) {
                if (strcmp(snd_mixer_selem_get_name(elem), "Master") == 0)
                        return elem;
        }
        return NULL;
}

int
setup_mixer(snd_mixer_t **mixer)
{
        int err;

        struct snd_mixer_selem_regopt selem_regopt = {
                .ver = 1,
                .abstract = SND_MIXER_SABSTRACT_NONE,
                .device = "default",
        };

        err = snd_mixer_open(mixer, 0);
        if (err != 0) {
                mixer = NULL;
                return err;
        }

        err = snd_mixer_selem_register(*mixer, &selem_regopt, NULL);
        if (err != 0) {
                mixer = NULL;
                return err;
        }

        snd_mixer_set_callback(*mixer, mixer_callback);

        err = snd_mixer_load(*mixer);
        if (err != 0) {
                mixer = NULL;
                return err;
        }

        return 0;
}

#undef exp10
