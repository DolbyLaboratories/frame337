INCLUDE = -I .
SOURCES = .

OBJPATH = ./Release/$(shell echo $(shell uname))

CC = gcc
CFLAGS = -c 
WFLAGS = -Wall
DFLAGS = -g
DEFFLAGS = -DUNIX

DIR = $(OBJPATH)

NAME = $(OBJPATH)/frame337

cleanbuild: all
	@echo Cleaning object files
	rm -rf $(OBJPATH)/*.o
	@echo Build of frame337 successfully completed

all: frame337

frame337: $(OBJPATH)/data.o $(OBJPATH)/frame337.o
	@echo Linking binary into $(NAME) at $(OBJPATH)
	$(CC) -o $(NAME) $(OBJPATH)/data.o $(OBJPATH)/frame337.o 

$(OBJPATH)/frame337.o: $(DIR) $(SOURCES)/frame337.c
	@echo Compiling frame337.c
	$(CC) $(CFLAGS) $(WFLAGS) $(DFLAGS) $(INCLUDE) $(DEFFLAGS) $(SOURCES)/frame337.c -o $(OBJPATH)/frame337.o

$(OBJPATH)/data.o: $(DIR) $(SOURCES)/data.c
	@echo Compiling data.c
	$(CC) $(CFLAGS) $(WFLAGS) $(DFLAGS) $(INCLUDE) $(DEFFLAGS) $(SOURCES)/data.c -o $(OBJPATH)/data.o

$(DIR):
	@echo Creating build path $(OBJPATH)
	@$(SHELL) -ec 'mkdir -p $(OBJPATH)'