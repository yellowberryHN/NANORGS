#!/usr/make

SRCS = compiler.cpp contest06.cpp disasm.cpp organism.cpp world.cpp 
HDRS = mycon.h compiler.h constants.h disasm.h mycon.h organism.h settings.h types.h world.h
LIBS = -lcurses
CC = g++
CCOPTS = -O2
#CCOPTS = -g -O0 -DDEBUG
PROG = contest06 

${PROG}: ${SRCS} ${HDRS}
	${CC} ${CCOPTS} ${LIBS} -o ${PROG} ${SRCS}
