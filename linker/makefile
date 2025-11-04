# ime izvršne datoteke
TARGET = linker
# putanje
SRC_DIR = src
INC_DIR = inc
OBJ_DIR = obj

# fajlovi
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# kompajler i opcije
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I$(INC_DIR)

# pravila
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# testiranje: pokretanje linkera na nekom fajlu iz tests/
test: $(TARGET)
	./$(TARGET) tests/example.o

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

runa: all
	cd ./../tests/nivo-a && ./start.sh