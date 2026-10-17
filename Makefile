CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

TARGET = route_shield
SOURCES = route.cpp Intrusion.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all clean rebuild

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

rebuild: clean al