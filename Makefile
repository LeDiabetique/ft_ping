NAME        := ft_ping

CC			:= gcc
FLAGS		:= -Wall -Wextra 
RM		    := rm -f

SRC_DIR		:= srcs/
SRCS		:= $(addprefix $(SRC_DIR), ft_ping.c)
INC_DIR		:= includes/

OBJ_DIR		:= ./obj/
OBJS 		:= $(SRCS:$(SRC_DIR)%.c=$(OBJ_DIR)%.o)

GREEN		:= \033[92m
YELLOW		:= \033[93m
PURPLE 		:= \033[95m
WHITE		:= \033[97m

$(OBJ_DIR)%.o: $(SRC_DIR)%.c
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(FLAGS) -I$(INC_DIR) -c $< -o $@

${NAME}:	${OBJS}
	@${CC} ${FLAGS} -o ${NAME} ${OBJS}
	@echo "$(PURPLE)Executable $(GREEN)${NAME} created$(WHITE)"

all:		${NAME}

clean:
	@$(RM) $(OBJ_DIR)*.o
	@[ -d $(OBJ_DIR) ] && rm -rf $(OBJ_DIR) || true
	@echo "$(YELLOW)All .o files inside ft_ping directory deleted$(WHITE)"

fclean: clean
	@$(RM) ${NAME}
	@echo "$(PURPLE)$(NAME) $(YELLOW)and .o files inside ft_ping directory deleted$(WHITE)"

re:	fclean all

.PHONY: all clean fclean re