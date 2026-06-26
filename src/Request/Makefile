NAME = TestReqParssing

CXX = c++ -g

CFLAGS = -Wall -Wextra -Werror -std=c++98

SRC = main.cpp httprequest_utl.cpp httpRequest.cpp

OBJ = $(SRC:.cpp=.o)

all : $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp httprequest.hpp
	$(CXX) $(CFLAGS) -c $< -o $@


fclean:
	rm -rf $(OBJ)
	rm -rf $(NAME)

re : fclean all


.PHONY: all flcean re
