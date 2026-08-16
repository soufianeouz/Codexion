NAME = codexion

CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread

SRC = src/main.c \
      src/actions.c \
      src/cleanup.c \
      src/coder.c \
      src/init.c \
      src/monitor.c \
      src/parsing.c \
      src/queue.c \
	src/scheduler.c \
      src/functions_init.c \
      src/time_utils.c\
      src/lock_unclock.c


OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) -I src -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re