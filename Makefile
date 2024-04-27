#
# Makefile
#
# Targets:
# 	all
#       The default target, if no target is specified. Compiles source files
#       as necessary and links them into the final executable.
#   clean
#       Removes all object files and executables.
#   install
#	   Installs chewie and scripts into the directory specified by the prefix
#	   variable. Defaults to /usr/local.
#	uninstall
#	   Removes chewie from bin, all the scripts from sbin and ~/.cache/chewie
# Variables:
#   CC
#	   The C compiler to use. Defaults to gcc.
#   CFLAGS
#	   Flags to pass to the C compiler. Defaults to -I/usr/include. If libcurl
#	   or libjson-c are installed in a non-standard location, you may need to
#	   add -I/path/to/include to this variable. On Mac OS, for example, I use
#	   MacPorts, so I use make CFLAGS="-I/opt/local/include".
#   debug=1
#       Build chewie with debug info
#   LDFLAGS
#	   Flags to pass to the linker. Defaults to -L/usr/lib. If libcurl or
#	   libjson-c are installed in a non-standard location, you may need to add
#	   -L/path/to/lib to this variable. On Mac OS, for example, I use MacPorts,
#	   so I use make LDFLAGS="-L/opt/local/lib".
#   prefix
#	   The directory to install chewie into. Defaults to /usr/local. chewie is
#	   installed into $(prefix)/bin and chewie-chat and qcg are installed into
#      $(prefix)/sbin.
#

ifeq ($(CC),)
CC = gcc
endif
CFLAGS += -I/usr/include
LDFLAGS += -L/usr/lib
ifdef debug
CFLAGS += -g3 -D DEBUG
else
CFLAGS += -O3 -flto
LDFLAGS += -flto
endif
ifeq ($(prefix),)
prefix = /usr/local
endif

LIBS = -lcurl -ljson-c

OBJS = main.o action.o api.o configure.o context.o file.o groq.o input.o ollama.o openai.o option.o

.PHONY: all clean install uninstall

all: chewie

clean:
	- rm -f chewie
	- rm -f *.o

action.o : chewie.h action.h api.h configure.h context.h file.h setting.h
api.o : chewie.h groq.h api.h ollama.h openai.h
configure.o : chewie.h action.h api.h configure.h context.h file.h option.h
context.o : chewie.h context.h file.h
file.o : chewie.h file.h
groq.o : chewie.h api.h context.h file.h groq.h setting.h
input.o : chewie.h file.h input.h
main.o : chewie.h action.h configure.h context.h file.h groq.h input.h ollama.h openai.h
ollama.o : chewie.h api.h context.h file.h ollama.h setting.h
openai.o : chewie.h api.h context.h file.h openai.h setting.h
option.o : chewie.h api.h configure.h option.h setting.h

chewie : $(OBJS)
	$(CC) $(LDFLAGS) $(LIBS) $^ -o $@
ifndef debug
	strip $@
endif

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: chewie chewie-chat eliza qb qc qg qn
	mkdir -p ~/.cache/chewie
	install -m 755 chewie $(prefix)/bin
	install -m 755 chewie-chat $(prefix)/sbin 
	install -m 755 eliza $(prefix)/sbin
	install -m 755 qb $(prefix)/sbin
	install -m 755 qc $(prefix)/sbin
	install -m 755 qg $(prefix)/sbin
	install -m 755 qn $(prefix)/sbin

uninstall:
	- rm -rf ~/.cache/chewie
	- rm -f $(prefix)/bin/chewie
	- rm -f $(prefix)/sbin/chewie-chat
	- rm -f $(prefix)/sbin/eliza
	- rm -f $(prefix)/sbin/qb
	- rm -f $(prefix)/sbin/qc
	- rm -f $(prefix)/sbin/qg
	- rm -f $(prefix)/sbin/qn

