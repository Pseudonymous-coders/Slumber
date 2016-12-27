# Use the builtin bash for file io
SHELL = /bin/sh

# Setup for cross compilation
ARCH=arm

# Use the provided cross compiler
CXX = cross_compiler/bin/arm-linux-gnueabihf-g++
CC = cross_compiler/bin/arm-linux-gnueabihf-gcc

# To Use the standard compiler
# Comment out the above line and uncomment below
#CC = g++
# Above for standard system g++

# g++ flags to be placed after each src file
CXXFLAGS = -Iinclude

# Static libs to add
LDFLAGS = -lpthread

# Remove files
RM = rm -f

# Final target library
TARGET = slumberhub

SRCS = $(shell echo src/*.cpp)
HEADS = $(shell echo include/*.h)
OBJS = $(SRCS:.cpp=.o)
LIBS = lib/
OBJF = obj/
SRCF = src/
BINF = bin/

.PHONY: all
all: ${TARGET}

$(TARGET): $(OBJS)
	@echo Creating single static lib $(TARGET)
	$(CXX) -o $(BINF)$@ $^ $(CXXFLAGS) $(LDFLAGS)
	mv $(OBJS) $(OBJF)
	@echo Finished building! Find the compiled version in $(BINF)

$(OBJF)%.o: %.cpp $(HEADS)
	@echo Compiling... $<
	$(CXX) -o $@ $< $(CXXFLAGS)

.PHONY: clean
clean:
	@echo Removing singular compiles!
	-${RM} ${OBJS}
