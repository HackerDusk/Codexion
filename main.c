# include "codexion.h"
# include <unistd.h>

int is_positive_int(char *str)
{
    if (!str || !*str)
        return (0);
    if (*str == '0' && *(str + 1) == '\0')
        return (0);
    while (*str)
    {
        if (*str < '0' || *str > '9')
            return (0);
        str++;
    }
    return (1);
}

int check_args(char **argc)
{
    int i;
    i = 1;
    while (*(argc + i) && i < 8)
    {
        if (!is_positive_int(*(argc + i)))
        {
            write (2, "Error: top 7 arguments must be an integer\n", 42);
            return (0);
        }
        i++;
    }
    int fifo = !strcmp(argc[i], "fifo");
    int edf =  !strcmp(argc[i], "edf");
    if (!edf && !fifo)
    {
        write (2, "Error: 8th argument must be 'fifo' or 'edf'.\n", 44);
        return (0);
    }
    return (1);
}

int main(int argc, char  **argv)
{
    if (argc != 9)
    {
        printf("Error: Got %d/9 arguments \n", argc);
        return (1);
    }
    if (!check_args(argv))
        return (1);
    return (0);
}