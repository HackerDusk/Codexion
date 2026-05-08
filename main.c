#include "codexion.h"

int compteur = 0;

void    *incrementer(void *arg)
{
    t_data *data = (t_data *)arg;
    int i = 0;
    while (i < 100000)
    {
        pthread_mutex_lock(data->mutex);
        compteur++;
        pthread_mutex_unlock(data->mutex);
        i++;
    }
    return (NULL);
}

int main(void)
{
    pthread_t   t[3];
    pthread_mutex_t mutex;
    t_data  data;
    data.mutex = &mutex;
    int         i = 0;
    pthread_mutex_init(&mutex, NULL);
    while (i < 3)
    {
        pthread_create(&t[i], NULL, incrementer, &data);
        i++;
    }
    i = 0;
    while (i < 3)
    {
        pthread_join(t[i], NULL);
        i++;
    }
    pthread_mutex_destroy(&mutex);
    printf("Résultat : %d\n", compteur);
    return (0);
}