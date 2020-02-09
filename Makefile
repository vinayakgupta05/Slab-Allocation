#
# File          : Makefile
# Description   : Build file for CMPSC473 project 2, slab allocator
#                 


# Environment Setup
#LIBDIRS=-L. -L/opt/local/lib
#INCLUDES=-I. -I/opt/local/include
CC=gcc 
CFLAGS=-c $(INCLUDES) -g -Wall
# LINK=gcc -g -m32
LINK=gcc -g 
LDFLAGS=$(LIBDIRS)
AR=ar rc
RANLIB=ranlib

# TASK 0
# Update with your student number
# BTW - No spaces after the number
SNUM=66


# Suffix rules
.c.o :
	${CC} ${CFLAGS} $< -o $@

#
# Setup builds

TARGETS=cmpsc473-p2
LIBS=

#
# Project Protections

p2 : $(TARGETS)

cmpsc473-p2 : cmpsc473-p2.o cmpsc473-mm.o cmpsc473-kvs.o cmpsc473-util.o clock.o
	$(LINK) $(LDFLAGS) cmpsc473-p2.o cmpsc473-mm.o cmpsc473-kvs.o cmpsc473-util.o clock.o $(LIBS) -o $@ -lrt

clean:
	rm -f *.o *~ $(TARGETS)

BASENAME=p2-assign
tar: 
	tar cvfz $(BASENAME)-$(SNUM).tgz -C ..\
	    $(BASENAME)/Makefile \
            $(BASENAME)/cmpsc473-p2.c \
            $(BASENAME)/cmpsc473-mm.c \
            $(BASENAME)/cmpsc473-mm.h \
	    $(BASENAME)/cmpsc473-kvs.c \
	    $(BASENAME)/cmpsc473-kvs.h \
	    $(BASENAME)/cmpsc473-util.c \
	    $(BASENAME)/cmpsc473-util.h \
	    $(BASENAME)/cmpsc473-format-$(SNUM).h \
		$(BASENAME)/clock.c \
		$(BASENAME)/clock.h


