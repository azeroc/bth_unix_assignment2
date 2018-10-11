# Tutorial: http://nuclear.mutantstargoat.com/articles/make/
# == == == Configuration logic == == ==
CC = gcc

TARGET = webserver
INCDIR = include
SRCDIR = src
OBJDIR = objects
BINDIR = bin
RESDIR = resources

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

CFLAGS = -I$(INCDIR) -Wall -pthread

# == == == Makefile logic == == ==

# Default make target
all: $(TARGET)

# Compile program to bin dir from object files, copy resources to bin dir
$(TARGET): $(OBJS)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $(OBJS) -o $(BINDIR)/$(TARGET)
	@cp -r -v $(RESDIR)/. $(BINDIR)

# Compile object files (*.o) from sources (*.c)
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Non-file targets (.PHONY)
.PHONY: clean
clean:
	rm -f -v $(OBJS) myprog
	rm -rf -v $(BINDIR)/*