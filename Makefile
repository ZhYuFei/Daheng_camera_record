# Makefile for Daheng Camera with OpenCV
.PHONY: all clean

# Program name
NAME := camera_demo

# Source files
SRCS := main.cpp
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

# Compiler and flags
CXX := g++
LD := g++

# Include paths (add OpenCV and current directory)
INCLUDE_PATHS := -I./ -I/usr/include/opencv4

# Library paths
LIB_PATHS := -L/usr/lib -L./ -Wl,-rpath=/usr/lib:./

# Libraries to link
LIBS := -lgxiapi -lpthread $(shell pkg-config --libs opencv4)

# Compiler flags
CPPFLAGS := -w $(INCLUDE_PATHS) $(LIB_PATHS)
LDFLAGS := $(LIBS)

all: $(NAME)

$(NAME): $(OBJS)
	$(LD) -o $@ $^ $(CPPFLAGS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

clean:
	$(RM) *.o $(NAME)
