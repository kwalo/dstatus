#ifndef _MIXER_H_
#define _MIXER_H_

#include <alsa/asoundlib.h>

int get_normalized_volume(snd_mixer_elem_t *);
snd_mixer_elem_t *find_mixer_elem(snd_mixer_t *mixer);
int setup_mixer(snd_mixer_t **mixer);
int mixer_volume_changed();

#endif
