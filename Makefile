NAME = webserv

CXX = c++ -g
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 

SRC_DIR = src
INCLUDES = include
REQ_DIR = src/Request
RES_DIR = src/Response

SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/Config.cpp $(SRC_DIR)/Server.cpp $(SRC_DIR)/Logger.cpp $(SRC_DIR)/Cgi.cpp $(SRC_DIR)/Utils.cpp    \
					$(REQ_DIR)/Request.cpp $(REQ_DIR)/httprequest_utl.cpp  \
					$(RES_DIR)/ProcessRequest.cpp  $(RES_DIR)/ServeStaticRq.cpp $(RES_DIR)/ProcessCgi.cpp $(RES_DIR)/Response.cpp \

OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = $(INCLUDES)/WebServ.hpp \
					$(INCLUDES)/Server.hpp \
					$(INCLUDES)/Cgi.hpp \
					$(INCLUDES)/Config.hpp \
					$(INCLUDES)/Logger.hpp \
					$(INCLUDES)/ProcessRequest.hpp  $(INCLUDES)/Response.hpp  $(INCLUDES)/ServeStaticRq.hpp \
					$(INCLUDES)/ProcessCgi.hpp  $(INCLUDES)/Request.hpp \


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
