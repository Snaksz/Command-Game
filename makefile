# Default port number
# Note Adams student id is used
# Default port number if not defined externally
# Executable name

# Compiler flags
PORT = 95201
CFLAGS = -DPORT=$(PORT) -g -Wall

# Executable name
TARGET = battle
FILE = battle.c

all: $(FILE)
	@gcc $(CFLAGS) -o $(TARGET) $(FILE)	

clean:
	@rm -f $(TARGET) *.o *~
