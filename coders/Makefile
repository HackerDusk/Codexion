NAME = codexion

SRCS =	main.c	argument_checker.c	init.c \
		coder_routine.c	utils.c	time_tools.c \
		deadlock_breaker.c	dongle_access.c	heap.c \
		simulation_tools.c	scheduler.c	coder_actions.c
		

OBJS = $(SRCS:.c=.o)

CC = cc
FLAGS = -Wall -Wextra -Werror -pthread
RM = rm -f

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) $^ -o $@

clean:
	$(RM) $(OBJS)

fclean:	clean
	$(RM) $(NAME)

re: fclean all

.PHONY: clean fclean re all
