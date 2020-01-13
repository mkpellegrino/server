CPP_FLAGS=-arch x86_64 -m64
DEBUG=-g -DDEBUG
OPT=-O2
GCCSTACK=-mpreferred-stack-boundary=32
CLANGSTACK=-fdiagnostics-show-option
BIN_DIR=./
LIB_DIR=./
LST_DIR=./

all:	clean server debug

release:clean server
	rm -fR $(BIN_DIR)*.o
	rm -fR $(BIN_DIR)*.lst

server: server.cpp
	g++ $(OPT) $(CPP_FLAGS) server.cpp -o $(BIN_DIR)server
	strip -no_uuid -A -u -S -X -N -x $(BIN_DIR)server

debug: server.cpp
	g++ $(DEBUG) $(CPP_FLAGS) server.cpp -o $(BIN_DIR)server-dbg
	dsymutil $(BIN_DIR)server-dbg


clean :
	rm -fR $(BIN_DIR)server
	rm -fR $(BIN_DIR)server-dbg
	rm -fR $(BIN_DIR)*.*~
	rm -fR $(BIN_DIR)Makefile~
	rm -fR $(BIN_DIR)*.o
	rm -fR $(BIN_DIR)*.lst
	rm -fR $(BIN_DIR)server-dbg.dSYM
