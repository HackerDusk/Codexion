# include "codexion.h"

void    take_dongle(t_dongle *dongle, t_coder *coder)
{
    pthread_mutex_lock(&dongle->mutex);
    while (!dongle->available && !(!dongle->cooldown_end))
        pthead_cond_wait(&dongle->cond, &dongle->mutex);
    pthread_mutex_unlock(&dongle->mutex);
}