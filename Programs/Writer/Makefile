SOURCE	:=	writer.cpp
HEADER	:=	../log.h writer.h
TARGET := writer
OBJS := $(SOURCE:.cpp=.o)
CC := g++
CFLAGS := -std=c++0x -g

$(TARGET): $(OBJS)
	$(CC) $^ -o $@

%.o: %.cpp %.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) *.o *~ *.orig

beautify:
	astyle --style=linux --indent=spaces=4 $(SOURCE) $(HEADER)

