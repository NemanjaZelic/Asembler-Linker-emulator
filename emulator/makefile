# ============================================
# Makefile za projekat: Emulator
# ETF Sistem Softver - Emulator (nivo A/B/C)
# ============================================

# Kompajler i opcije
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2

# Naziv izlaznog programa
TARGET = emulator

# Svi .cpp fajlovi u direktorijumu
SRC = $(wildcard *.cpp)

# Svi .o fajlovi koji nastaju iz .cpp fajlova
OBJ = $(SRC:.cpp=.o)

# ============================================
# Pravila
# ============================================

# Podrazumevana meta — prevodi ceo emulator
all: $(TARGET)

# Linkovanje
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Kompajliranje svake .cpp datoteke u .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pokretanje emulatora sa ulaznim fajlom
run: $(TARGET)
	@echo "Pokretanje emulatora..."
	./$(TARGET) mem_content.hex

# Brisanje privremenih fajlova
clean:
	rm -f $(OBJ) $(TARGET)

# Potpuno čišćenje (uključuje sve)
reset: clean
	rm -f *.hex *.out *.log
