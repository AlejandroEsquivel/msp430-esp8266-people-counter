FILENAME = counter

OBJECTS=${FILENAME}.o
DEVICE=msp430g2553
INSTALL_DIR=${HOME}/ti/

GCC_DIR = $(INSTALL_DIR)/bin
SUPPORT_FILE_DIRECTORY = $(INSTALL_DIR)/include

CC      = $(GCC_DIR)/msp430-elf-gcc
GDB     = $(GCC_DIR)/msp430-elf-gdb

CFLAGS = -I $(SUPPORT_FILE_DIRECTORY) -mmcu=$(DEVICE) -Os
LFLAGS = -L $(SUPPORT_FILE_DIRECTORY) -T $(DEVICE).ld

elf: ${OBJECTS}
	$(CC) $(CFLAGS) $(LFLAGS) $? -g -o ${FILENAME}.elf
asm: 
	$(CC) $(CFLAGS) $(LFLAGS) $? -S ${FILENAME}.c

clean: 
	rm *.o *.elf *.s


debug: all
	$(GDB) ${FILENAME}.elf
