#************************************************************************************************************
#* Copyright (c) 2017, Dolby Laboratories Inc.
#* All rights reserved.
#* Redistribution and use in source and binary forms, with or without modification, are permitted
#* provided that the following conditions are met:
#* 1. Redistributions of source code must retain the above copyright notice, this list of conditions
#*    and the following disclaimer.
#* 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
#*    and the following disclaimer in the documentation and/or other materials provided with the distribution.
#* 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
#*    promote products derived from this software without specific prior written permission.
#* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
#* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
#* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
#* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
#* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
#* OF THE POSSIBILITY OF SUCH DAMAGE.
#************************************************************************************************************/
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
