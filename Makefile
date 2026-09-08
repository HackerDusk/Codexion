NAME = codexion

SRC_DIR = coders

SRCS =	main.c	argument_checker.c	init.c \
		coder_routine.c	coder_actions.c \
		time_tools.c	deadlock_breaker.c dongle_access.c	\
		taking_dongle.c	heap.c monitor_routine.c	\
		monitor_tools.c	simulation_tools.c	scheduler_tools.c	\
		scheduler_routine.c	simulator_tools.c	coder_routine_simulator.c	

OBJS = $(addprefix $(SRC_DIR)/, $(SRCS:.c=.o))

CC = cc
FLAGS = -Wall -Wextra -Werror -pthread
RM = rm -f

$(SRC_DIR)/%.o:$(SRC_DIR)/%.c
	$(CC) $(FLAGS) -c $< -o $@

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) $^ -o $@

clean:
	$(RM) $(OBJS)

fclean:	clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
