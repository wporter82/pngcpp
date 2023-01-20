CXX = g++
OBJECTS = main.o
FLAGS = -Wall -std=c++17
LDFLAGS = -lz
EXEC = pngcpp

all: $(OBJECTS)
	$(CXX) $(FLAGS) -o $(EXEC) $(OBJECTS) $(LDFLAGS)

.PHONY: all clean
clean:
	rm *.o $(EXEC)
