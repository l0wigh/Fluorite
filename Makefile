# Fluorite Window Manager
# Version 1.1
# Master Layout done right

BLU			= \033[0;34m
GRN			= \033[0;32m
RED			= \033[0;31m
RST			= \033[0m
END			= \e[0m
TOTEM 		= 🦁

FILES = fluorite

SRCS = $(FILES:%=%.c)
NAME		= Fluorite
OBJS_DIR	= objs/
OBJS		= $(SRCS:.c=.o)
OBJECTS_PREFIXED = $(addprefix $(OBJS_DIR), $(OBJS))
CC			= gcc
# CC_FLAGS	= -O0 -I/usr/include/freetype2 -g3 -I/usr/include/freetype2
CC_FLAGS	= -O2 -pipe -mtune=native -march=native -I/usr/include/freetype2
LIBS		= -lX11 -lXcursor -lXcomposite -lXft -lXrandr -lxdo

$(OBJS_DIR)%.o : %.c $(PROJECT_H)
	@mkdir -p $(OBJS_DIR)
	@mkdir -p $(OBJS_DIR)srcs
	@$(CC) $(CC_FLAGS) -c $< -o $@
	@printf	"\033[2K\r${BLU}${TOTEM} [BUILD]${RST} '$<' $(END)"

$(NAME): $(OBJECTS_PREFIXED)
	@$(CC) -o $(NAME) $(OBJECTS_PREFIXED) $(CC_FLAGS) $(LIBS) # Program
	@printf "\033[2K\r\033[0;32m${TOTEM} [END]\033[0m $(NAME)$(END)\n"

all: $(NAME)

clean:
	@rm -rf $(OBJS_DIR)
	@printf "\033[2K\r${GRN}${TOTEM} [CLEAN]${RST} done$(END)\n"

fclean: clean
	@rm -f $(NAME)
	@rm -rf $(OBJS_DIR)
	@printf "\033[2K\r${GRN}${TOTEM} [FCLEAN]${RST} done$(END)\n"

re: fclean all

install: re
	sudo cp -f ./Fluorite /usr/bin/

.PHONY:		all clean fclean re install
