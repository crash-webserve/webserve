NAME        = webserve

CXX         = c++
CXXFLAGS    = #-Wall -Wextra -Werror #-std=c++98
DEBUG       = -g

INC         =	-I .


SRCS        =	ServerConfig.cpp \
				Log.cpp \
				Request.cpp \
				Response.cpp \
				LocationConfig.cpp \
				Server.cpp \
				ServerManager.cpp \
				Socket.cpp \
				main.cpp

#				Request.cpp

OBJS        = $(SRCS:.cpp=.o)
RM          = rm -f

.cpp.o:
				${CXX} ${CXXFLAGS} ${DEBUG} -c $< -o ${<:.cpp=.o}

$(NAME): ${OBJS}
				${CXX} ${CXXFLAGS} ${DEBUG} $(OBJS) $(INC) -o $(NAME)

all: $(NAME)

fclean: clean
				$(RM) $(NAME)

clean:
				$(RM) $(OBJS)

re: fclean all
