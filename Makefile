# Fluorite Window Manager
# Master Layout done right

BLU             = \033[0;34m
GRN             = \033[0;32m
RED             = \033[0;31m
YEL             = \033[1;33m
RST             = \033[0m
END             = \e[0m
TOTEM           = 🦁

NAME            = Fluorite
CTL_NAME        = fluoritectl

OBJS_DIR        = objs/

CC              = gcc
CC_FLAGS        = -Wall -Werror -Wextra -O2 -pipe -mtune=native -march=native -I/usr/include/freetype2
LIBS            = -lX11 -lXcursor -lXrandr -lxdo -lconfuse -lpthread

SRCS            = fluorite.c
OBJS            = $(addprefix $(OBJS_DIR), $(SRCS:.c=.o))

CTL_SRCS        = fluoritectl.c
CTL_OBJS        = $(addprefix $(OBJS_DIR), $(CTL_SRCS:.c=.o))

all: $(NAME) $(CTL_NAME)

$(OBJS_DIR)%.o: %.c
	@mkdir -p $(OBJS_DIR)
	@$(CC) $(CC_FLAGS) -c $< -o $@
	@printf "\033[2K\r${BLU}${TOTEM} [BUILD]${RST} '$<' $(END)"

$(NAME): $(OBJS)
	@$(CC) -o $(NAME) $(OBJS) $(CC_FLAGS) $(LIBS)
	@printf "\033[2K\r\033[0;32m${TOTEM} [END]\033[0m $(NAME)$(END)\n"

$(CTL_NAME): $(CTL_OBJS)
	@$(CC) -o $(CTL_NAME) $(CTL_OBJS) $(CC_FLAGS)
	@printf "\033[2K\r\033[0;32m${TOTEM} [END]\033[0m $(CTL_NAME)$(END)\n"

clean:
	@rm -rf $(OBJS_DIR)
	@printf "\033[2K\r${GRN}${TOTEM} [CLEAN]${RST} done$(END)\n"

fclean: clean
	@rm -f $(NAME) $(CTL_NAME)
	@printf "\033[2K\r${GRN}${TOTEM} [FCLEAN]${RST} done$(END)\n"

re: fclean all

install: re
	@sudo cp -f ./$(NAME) /usr/bin/
	@sudo cp -f ./$(CTL_NAME) /usr/bin/
	@printf "\033[2K\r${GRN}${TOTEM} [INSTALL]${RST} Binaries installed to /usr/bin/$(END)\n"
	@bash -c ' \
		read -p "$$(echo -e "$(TOTEM) ${BLU}[INFO]${RST} Do you want to install a default configuration (y/n) ? ")" -r response; \
		response=$$(echo "$$response" | tr "[:upper:]" "[:lower:]"); \
		if [[ "$$response" =~ ^(oui|o|y|yes)$$ ]]; then \
			printf "\033[2K\r$(TOTEM) ${BLU}[INFO]${RST} Installing the Standard configuration$(END)\n"; \
			mkdir -p ~/.config/fluorite 2> /dev/null; \
			cp ./config/standard.conf ~/.config/fluorite/fluorite.conf; \
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

.PHONY: all clean fclean re install
