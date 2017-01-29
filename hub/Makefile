# Use the builtin bash for file io
SHELL = /bin/sh

ifeq ($(strip $(HOSTBASE)),)
HOSTBASE=x86_64-linux-gnu
endif

ifneq ($(strip $(SLUMBER)),)
# Setup for cross compilation
ARCH=arm

# Use the provided cross compiler
BASE = arm-linux-gnueabihf
CXX = $(BASE)-g++
CC = $(BASE)-gcc

# Set the extras folder
TYPELOCATION = /usr/$(BASE)
OPENSSLLIBS = $(TYPELOCATION)/lib/libcrypto.so \
$(TYPELOCATION)/lib/libssl.so
PREARGS=-static-libstdc++ -static-libgcc # Statically send std lib (@TODO CHANGE THIS SO IT'S DYNAMICALLY LOADED ON THE SLUMBER MACHINE!!!)
HOSTFLAG=SLUMBERMACHINE

# Final target library
TARGET = slumberhub

else

# Use the standard system compiler
CXX = g++
CC = gcc

# Set the extras folder
TYPELOCATION = /usr/local
HOSTBASELOC = /usr/lib/$(HOSTBASE)/lib
EXTRALLIBS = $(HOSTBASELOC)crypto.so $(HOSTBASELOC)ssl.so \
$(HOSTBASELOC)bluetooth.so $(HOSTBASELOC)glib-2.0.so
PREARGS=
HOSTFLAG=HOSTMACHINE

# Final target library
TARGET = slumberhost
endif


# To Use the standard compiler
# Comment out the above line and uncomment below
#CC = g++
# Above for standard system g++

LIBRARY_PATH=$(TYPELOCATION)/lib
INCLUDELOCATION = $(TYPELOCATION)/include
LIBLOCATION = $(TYPELOCATION)/lib
BLOC=$(TYPELOCATION)/lib/lib
LLIBS = $(BLOC)boost_atomic.so $(BLOC)boost_chrono.so \
$(BLOC)boost_thread.so $(BLOC)boost_system.so $(EXTRALLIBS) \
$(BLOC)boost_filesystem.so $(BLOC)cpprest.so $(BLOC)gatt.so

CFLAGS = -I/usr/include/glib-2.0 \
-I/usr/lib/$(HOSTBASE)/glib-2.0/include \
-I/usr/local/include/pstreams \
-D $(HOSTFLAG) -I$(INCLUDELOCATION) -Iinclude -lpthread -lglib-2.0 -lgatt

# g++ flags to be placed after each src file
CXXFLAGS = -std=c++11 $(CFLAGS) # -ggdb

# Static libs to add
LDFLAGS = -lpthread $(LLIBS) -fpermissive

# Remove files
RM = rm -f

SRCS = $(shell echo src/sbluetooth/*.cpp) $(shell echo src/util/*.cpp) \
$(shell echo src/security/*.cpp) $(shell echo src/sbluetooth/*.cpp) $(shell echo src/*.cpp)
CSRCS = $(shell echo csrc/*.c)
HEADS = $(shell echo include/*.h)
OBJS = $(CSRCS:.c=.o) $(SRCS:.cpp=.o)
LIBS = lib/
OBJF = obj/
SRCF = src/
BINF = bin/

.PHONY: all
all: ${TARGET}

$(TARGET): $(OBJS)
	@echo Creating single static lib $(TARGET) $(MVOBJS)
	$(CXX) -o $(BINF)$@ $^ $(PREARGS) $(CXXFLAGS) $(LDFLAGS)
	@echo Finished compiling! Find the executable at $(BINF)$(TARGET)

$(OBJF)%.o: %.cpp $(HEADS)
	@echo Compiling (C++)... $<
	$(CXX) -o $@ $< $(CXXFLAGS)

$(OBJF)%.: %.c $(HEADS)
	@echo Compiling (C)... $<
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	@echo Removing singular compiles!
	-${RM} ${OBJS}
