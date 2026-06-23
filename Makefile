NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 

SRC_DIR = src
INCLUDES = include

SOURCES = $(SRC_DIR)/main.cpp \
          $(SRC_DIR)/Config.cpp \
          $(SRC_DIR)/Server.cpp \
          $(SRC_DIR)/Logger.cpp \
          $(SRC_DIR)/Cgi.cpp

OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = $(INCLUDES)/WebServ.hpp \
          $(INCLUDES)/Server.hpp \
          $(INCLUDES)/Cgi.hpp \
          $(INCLUDES)/Config.hpp \
          $(INCLUDES)/Logger.hpp

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(NAME)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

fclean: clean
	rm -f $(NAME)

clean:
	rm -f $(OBJECTS)

re: fclean all

# test_request_size:
# 	$(CXX) $(CXXFLAGS) test/test_request_size.cpp -o test/test_request_size && ./test/test_request_size

.PHONY: all clean re
