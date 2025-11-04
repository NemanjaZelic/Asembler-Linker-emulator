
CXX = g++
FLEX = flex
BISON = bison


LEXER = lexer.l
PARSER = parser.y
MAIN_SRC = asembler.cpp
MAIN_OBJ = $(MAIN_SRC:.cpp=.o)


OBJS = parser.tab.o lex.yy.o $(MAIN_OBJ)

TARGET = asembler


all: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^

parser.tab.c parser.tab.h: $(PARSER)
	$(BISON) -d $(PARSER)

lex.yy.c: $(LEXER) parser.tab.h
	$(FLEX) $(LEXER)

%.o: %.c
	$(CXX) -c $< -o $@

%.o: %.cpp
	$(CXX) -c $< -o $@


run: $(TARGET)
	./$(TARGET) $(IN) $(OUT)

clean:
	rm -f $(OBJS) $(TARGET) parser.tab.* lex.yy.c

mjau:
	g++ parser.tab.c lex.yy.c $(IN) -o $(OUT)