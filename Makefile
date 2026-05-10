NAME = codexion

SRCS = 

OBJS = $(SRCS=.c=.o)

CC = cc
FLAGS = -Wall -Wextra -Werror
RM = rm -rf

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

all: $(NAME)

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re