.PHONY: all install clean

CC          = clang
TARGET      = iBootPatcher
INSTALL     = /usr/local/bin
uname_s     = $(shell uname -s)
CFLAGS      = -DDEBUG -O2 -c -I. -g -Wall -Wextra -o

OBJECTS     = iBootPatcher.o

default: all

all: $(TARGET)

%.o: %.c
	@echo "[INFO]: compiling $(TARGET)"
	@echo "CC	$<"
	@$(CC) $< $(CFLAGS) $@

$(TARGET): $(OBJECTS)
	@echo "LD	$(TARGET)"
	@$(CC) $(OBJECTS) -o $(TARGET)
	@echo "OK: built $(TARGET) for $(uname_s)"

install: $(TARGET)
	cp $(TARGET) $(INSTALL)

clean:
	@rm -f *.o $(TARGET)
	@echo "OK: removed unnecessary files"
