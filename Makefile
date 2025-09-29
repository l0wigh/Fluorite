# Fluorite Window Manager
# Master Layout done right

BLU			= \033[0;34m
GRN			= \033[0;32m
RED			= \033[0;31m
YEL			= \033[1;33m
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
CC_FLAGS	= -Wall -Werror -Wextra -O2 -pipe -mtune=native -march=native -I/usr/include/freetype2
LIBS		= -lX11 -lXcursor -lXrandr -lxdo -lconfuse

$(OBJS_DIR)%.o : %.c $(PROJECT_H)
	@mkdir -p $(OBJS_DIR)
	@mkdir -p $(OBJS_DIR)srcs
	@$(CC) $(CC_FLAGS) -c $< -o $@
	@printf	"\033[2K\r${BLU}${TOTEM} [BUILD]${RST} '$<' $(END)"

$(NAME): $(OBJECTS_PREFIXED)
	@$(CC) -o $(NAME) $(OBJECTS_PREFIXED) $(CC_FLAGS) $(LIBS)
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
	@sudo cp -f ./Fluorite /usr/bin/
	@printf "\033[2K\r${GRN}${TOTEM} [INSTALL]${RST} Binary installed to /usr/bin/$(END)\n"
	@bash -c ' \
		read -p "$$(echo -e "$(TOTEM) ${BLU}[INFO]${RST} Do you want to install a default configuration (y/n) ? ")" -r response; \
		response=$$(echo "$$response" | tr "[:upper:]" "[:lower:]"); \
		if [[ "$$response" =~ ^(oui|o|y|yes)$$ ]]; then \
			printf "\033[2K\r$(TOTEM) ${BLU}[INFO]${RST} Installing the Standard configuration$(END)\n"; \
			mkdir -p ~/.config/fluorite 2> /dev/null; \
			cp ./config/example.conf ~/.config/fluorite/fluorite.conf; \
			printf "\033[2K\r${GRN}$(TOTEM) [OK]${RST} Configuration installed at ~/.config/fluorite/fluorite.conf successfully$(END)\n"; \
		elif [[ "$$response" =~ ^(dev|d)$$ ]]; then \
			printf "\033[2K\r$(TOTEM) ${BLU}[INFO]${RST} Installing the L0Wigh configuration$(END)\n"; \
			mkdir -p ~/.config/fluorite 2> /dev/null; \
			cp ./config/l0wigh.conf ~/.config/fluorite/fluorite.conf; \
			printf "\033[2K\r${GRN}$(TOTEM) [OK]${RST} Configuration installed at ~/.config/fluorite/fluorite.conf successfully$(END)\n"; \
		else \
			printf "\033[2K\r${YEL}$(TOTEM) [ATTENTION]${RST} No configuration installed.$(END)\n"; \
			printf "\033[2K\r${YEL}$(TOTEM) [ATTENTION]${RST} Make sure you create one before starting Fluorite.$(END)\n"; \
		fi \
	'

.PHONY:		all clean fclean re install
