
# Makefile for Josefin 20130129, Lausanne!
# Modified to also include BeagleBone, Aachen 20140214

EXEC = Josefin
OBJS-RPI = Main.o KeyboardBut.o OneWireHandlerOWFSFile.o TimeoutHandler.o Watchdog.o crcutil.o SysDef.c SocketServer.o
OBJS-BB = Main.o BeagleGPIO/BeagleBone_gpio.o BeagleGPIO/BeagleBone_hd44780.o  KeyboardBut.o OneWireHandlerOWFSFile.o TimeoutHandler.o Watchdog.o crcutil.o SysDef.c SocketServer.o
OBJS-HOST = Main.o KeyboardBut.o OneWireHandlerOWFSFile.o TimeoutHandler.o Watchdog.o crcutil.o SysDef.c SocketServer.o
#OBJS = Main.o KeyboardBut.o OneWireHandlerHA7S.o TimeoutHandler.o Watchdog.o crcutil.o SysDef.c SocketServer.c
LDLIBS += -lpthread # Support library for pthreads
#LDLIBS += -lwiringPi
HOME = /home/$(USER)
  
# Settings for stand-alone build(not from top directory)
ifndef UCLINUX_BUILD_USER
	COMPILER = gcc
	CC = $(COMPILER)	

	WIRINGPILIB = /usr/local/include
	ROOTDIR = $(HOME)/Dropbox/RaspberryPi
	SRCDIR = $(HOME)/Dropbox/RaspBerryPi/Josefin
	LINUXDIR = linux-2.6.x
#	CFLAGS = -g -Wall	# -W all warnings, -g for debugging
	CFLAGS = -g -W	# -W all warnings, -g for debugging
endif

#LDFLAGS = -Wl -v
LDFLAGS = -v
#LDFLAGS	= -L/usr/local/lib

CFLAGS-HOST = -IC:/MinGW/WiringPi/wiringPi/wiringPi \
	-IC:/MinGW/ -IC:/MinGW/Einclude/  \
	-I../include -IC:/MinGW/Einclude/x86_64-linux-gnu/ \
	-IC:\MinGW\Einclude\x86_64-linux-gnu\gnu \
	-I/../Einclude/  

#define LCD present or not
CFLAGS += -DLCD_PRESENT
#CFLAGS += -DRPI_DEFINED
CFLAGS += -DBB_DEFINED


# Additional options to compiler
#CFLAGS += -I$(WIRINGPILIB)

$(EXEC): $(OBJS-BB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS-BB) $(LDLIBS$(LDLIBS_$@))

	
beagle: 
CFLAGS += -DBB_DEFINED

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS$(LDLIBS_$@))
	echo Beagle Compiled
	
all: $(EXEC) install

debug: $(EXEC) install
	rdebug $(EXEC)

install:
	cp $(EXEC) $(VPXDIR)/samba #romfs/usr/bin/
	cp StrtJos $(VPXDIR)/samba #romfs/usr/bin/

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	rm -rf $(EXEC) *.elf *.gdb *.o *.c~

host:
# Additional options to compiler
	echo Host entered
	echo $(CFLAGS-HOST)
		echo test2
	
#$(EXEC): $(OBJS)

#HOME = /home/$(USER)
#gcc $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS$(LDLIBS_$@))

#		gcc -g -DHOST -I../uCES/ -I./src/ -I/$(HOME)/C4-VPX/src/modules/gold_twi_lcd/ -o Josefin Main.c KeyboardBut.c KeyboardKnob.c OneWireHandler.c TimeoutHandler.c Watchdog.c crcutil.c SysDef.c  -lpthread 
#		gcc -g -Wall -DHOST -lwiringpi -LC:/MinGW/lib  -IC:/MinGW/WiringPi/wiringPi/wiringPi -IC:/MinGW/ -IC:/MinGW/Einclude/  -I../include -IC:/MinGW/Einclude/x86_64-linux-gnu/ -IC:\MinGW\Einclude\x86_64-linux-gnu\gnu -I/../Einclude/ -o Josefin Main.c KeyboardBut.c OneWireHandlerHA7S.c TimeoutHandler.c Watchdog.c crcutil.c SysDef.c  -lpthread 

#		gcc -g  -DHOST -lwiringpi -LC:/MinGW/lib  $(CFLAGS-HOST) -o Josefin Main.c KeyboardBut.c OneWireHandlerHA7S.c TimeoutHandler.c Watchdog.c crcutil.c SysDef.c SocketServer.c -lpthread 
		gcc -g  -DHOST -lwiringpi -LC:/MinGW/lib  $(CFLAGS-HOST) -o Josefin Main.c KeyboardBut.c OneWireHandlerOWFSFile.c TimeoutHandler.c Watchdog.c crcutil.c SysDef.c SocketServer.o -lpthread 

#-I$(ROOTDIR)/uClibc/include -I . -I../uCES/ -I./src/

One:
		gcc -g   -DHOST -llibwiringpi.so.1 -LC:/MinGW/lib  $(CFLAGS-HOST) -o Josefin OneWireHandlerHA7S.c  -lpthread 

