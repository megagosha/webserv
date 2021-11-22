SRC_FILES	= check.cpp \
				HttpRequest.cpp \
				HttpResponse.cpp \
				KqueueEvents.cpp \
				Location.cpp \
				Server.cpp \
				Socket.cpp \
				Utils.cpp \
				VirtualServer.cpp \
				MimeType.cpp \
				Session.cpp \
				FileStats.cpp \
				CgiHandler.cpp

NAME	= webserv

CC		= clang++ -fsanitize=address
RM		= rm -f

CFLAGS	= -Wall -Wextra -Werror -std=c++98 -pedantic

INCLUDES_DIR = ./incl
SRCS_DIR = ./srcs
OBJS_DIR = ./objs

SRCS	= $(addprefix $(SRCS_DIR)/, $(SRC_FILES))
OBJS	= $(patsubst $(SRCS_DIR)/%.cpp,$(OBJS_DIR)/%.o, $(SRCS))
DEPS	= $(OBJS:.o=.d)

all:		$(NAME)

-include $(DEPS)
$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.cpp | $(OBJS_DIR)
	$(CC) -g $(CFLAGS) -I$(INCLUDES_DIR) -MMD -MP -c -o $@ $<

$(OBJS_DIR):
	mkdir -p $(OBJS_DIR)

$(NAME):	$(OBJS)
			$(CC) -g $(CFLAGS) -I$(INCLUDES_DIR) -MMD -MP -o $(NAME) $(OBJS)

clean:
			if [ -d "$(OBJS_DIR)" ]; then rm -rfv $(OBJS_DIR); fi

fclean:		clean
			if [ -f "$(NAME)" ]; then rm -rfv $(NAME); fi
			
re:			fclean all

.PHONY:		all clean fclean re
