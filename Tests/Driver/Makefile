SOURCE	:= main_test.cpp basic_test.cpp integration_test.cpp
TARGET := tests
OBJS := $(SOURCE:.cpp=.o)
#STATIC_LIBS := /usr/src/gtest/libgtest.a /usr/src/gtest/libgtest_main.a
LIBS := -lpthread -lgtest -lgtest_main
CC := g++
CFLAGS := -std=c++0x

$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LIBS)
	#$(CC) $^ $(STATIC_LIBS) -o $@ $(LIBS)
	

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) *.o *~ *.orig

beautify:
	astyle --style=linux --indent=spaces=4 $(SOURCE)

