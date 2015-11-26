# Makefile for Josefin 20130129, Lausanne!
# Modified to also include BeagleBone, Aachen 20140214
  
EXEC = Josefin
OBJS-RPI = Main.c KeyboardBut.c OneWireHandlerOWFSFile.c TimeoutHandler.c Watchdog.c crcutil.c SysDef.c SocketServer.c
OBJS-BB = SimpleGPIO.c Main.c KeyboardBut.c OneWireHandlerOWFSFile.c TimeoutHandler.c Watchdog.c crcutil.c SysDef.c SocketServer.c
OBJS-HOST = SimpleGPIO.c Main.c KeyboardBut.c OneWireHandlerOWFSFile.c TimeoutHandler.c Watchdog.c crcutil.c SysDef.c SocketServer.c
#OBJS = Main.o KeyboardBut.o OneWireHandlerHA7S.o TimeoutHandler.o Watchdog.o crcutil.o SysDef.c SocketServer.c
LDLIBS += -lpthread # Support library for pthreads
LDLIBS += -lcurl # Support library for curl
HOME = /home/$(USER)
  
# Settings for stand-alone build(not from top directory)
ifndef UCLINUX_BUILD_USER
	COMPILER = gcc
	CC = $(COMPILER)	

	WIRINGPILIB = /usr/local/include
#	CFLAGS = -g -Wall	# -W all warnings, -g for debugging
	
endif
#LDFLAGS-RPI = -Wl -v

#LDFLAGS-BB = -W1 -v
#LDFLAGS-HOST = -W1 -v
LDFLAGS-RPI	= -L/usr/local/lib
#define LCD present or not
CFLAGS-RPI += -DLCD_PRESENT
CFLAGS-RPI += -DRPI_DEFINED
CFLAGS-RPI += -DOWLCD_PRESENT

CFLAGS-BB = -g -W	# -W all warnings, -g for debugging
CFLAGS-BB += -DLCD_PRESENT
CFLAGS-BB += -DBB_DEFINED
CFLAGS-BB += -DOWLCD_PRESENT

CFLAGS-HOST = -g -W	# -W all warnings, -g for debugging
#CFLAGS-HOST += -DLCD_PRESENT
CFLAGS-HOST += -DHOST_DEFINED
CFLAGS-HOST += -DOWLCD_PRESENT
#CFLAGS-HOST += -DLCD_PRESENT


# Additional options to compiler
CFLAGS-RPI += -I$(WIRINGPILIB)

LDLIBS-RPI += -lwiringPi
LDLIBS-RPI += -lcurl
LDLIBS-RPI += -lpthread # Support library for pthreads
#$(EXEC): $(OBJS)
#$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS$(LDLIBS_$@))
#  echo Default Compiled
beagle:
	echo Beagle Compiled
 
#CFLAGS += -DBB_DEFINED
#CFLAGS += -DLCD_PRESENT
#		gcc -g  -DBB_DEFINED $(CFLAGS-BB) -o Josefin SimpleGPIO.o Main.c KeyboardBut.c OneWireHandlerOWFSFile.c TimeoutHandler.c Watchdog.c crcutil.c SysDef.c SocketServer.o -lpthread 
		gcc  $(CFLAGS-BB) $(LDFLAGS-BB)  -o Josefin $(OBJS-BB) $(LDLIBS$(LDLIBS_$@))

rpi:
	echo Raspberry Compiled
 
#CFLAGS += -DBB_DEFINED
#CFLAGS += -DLCD_PRESENT
#		gcc -g  -DBB_DEFINED $(CFLAGS-BB) -o Josefin SimpleGPIO.o Main.c KeyboardBut.c OneWireHandlerOWFSFile.c TimeoutHandler.c Watchdog.c crcutil.c SysDef.c SocketServer.o -lpthread 
		gcc  $(CFLAGS-RPI) $(LDFLAGS-RPI)  -o Josefin $(OBJS-RPI) $(LDLIBS-RPI$(LDLIBS_$@))



host:
# Additional options to compiler
	echo "Host entered"

	
		gcc  $(CFLAGS-HOST) $(LDFLAGS-HOST)  -o Josefin $(OBJS-HOST) $(LDLIBS$(LDLIBS_$@))


debug: $(EXEC) install
	rdebug $(EXEC)

install:
	cp $(EXEC) $(VPXDIR)/samba #romfs/usr/bin/
	cp StrtJos $(VPXDIR)/samba #romfs/usr/bin/

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	rm -rf $(EXEC) *.elf *.gdb *.o *.c~


One:
		gcc -g   -DHOST -llibwiringpi.so.1 -LC:/MinGW/lib  $(CFLAGS-HOST) -o Josefin OneWireHandlerHA7S.c  -lpthread 

