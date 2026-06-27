NAME = TestProcessResponse

CXX = c++ -g

CFLAGS = -Wall -Wextra -Werror -std=c++98

SRC = main.cpp ../Config.cpp ../Request/httprequest_utl.cpp ../Request/httpRequest.cpp \
		ProcessRequest.cpp ServeStaticRq.cpp Response.cpp ProcessCgi.cpp

OBJ = $(SRC:.cpp=.o)

all : $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp 
	$(CXX) $(CFLAGS) -c $< -o $@


fclean:
	rm -rf $(OBJ)
	rm -rf $(NAME)

re : fclean all


.PHONY: all flcean re
