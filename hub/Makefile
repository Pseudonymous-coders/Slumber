SHELL = /bin/sh
CC = gcc
GLIBH = $(shell echo `pkg-config --cflags glib-2.0`)
CFLAGS = -fPIC -Wall -Wextra -O2 -g -Iinclude $(GLIBH)
LDFLAGS = -shared
ENDFLAGS = -Iinclude -I/usr/include $(shell echo `pkg-config --libs glib-2.0`) -lpthread
RM = rm -f
TARGET_LIB = libslumber.so

SRCS = $(shell echo src/*.c)
HEADS = $(shell echo include/*.h)
OBJS = $(SRCS:.c=.o)
LIBS = lib/
OBJF = obj/
SRCF = src/

PREFIX = /usr
LIBDIR = $(PREFIX)/lib/
INCDIR = $(PREFIX)/include/

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^ $(GLIBH) $(ENDFLAGS)
	mv $(TARGET_LIB) $(LIBS)
	mv $(OBJS) $(OBJF)
	mv $(SRCS:.c=.d) $(OBJF)
	@echo Finished building! Now run sudo make install

$(SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

.PHONY: install
install:
	# mv $(OBJF)* $(SRCF)
	@echo copying $(TARGET_LIB) into $(LIBDIR)
	cp $(LIBS)$(TARGET_LIB) $(LIBDIR)$(TARGET_LIB)
	@echo copying $(HEADS) into $(INCDIR)
	cp include/*.h $(INCDIR)
	@echo Done installing! You can use $(TARGET_LIB) now!

include $(SRCS:.c=.d)

.PHONY: clean
clean:
	-${RM} ${OBJS} $(SRCS:.c=.d)



