# --------------------------------------------------------
# Custom M2 Makefile
# written by: Jonathan Bohren & Jonathan Fiene
# updated: June 16, 2012
# --------------------------------------------------------

# --------------------------------------------------------
# if you write separate C files to include in main,
# add their .o targets to the CHILDREN line below
# (e.g. "CHILDREN = myfile.o")
#
# to include code supplied by maevarm, add a .o target
# tag to the parents line (e.g. "PARENTS = "m_bus.o")
# --------------------------------------------------------
FILE			= m2-actuation
MAIN			= $(FILE).o m_usb.o
CHILDREN	= #mrf24j40.o
PARENTS		=

# --------------------------------------------------------
# if you want to use one of our pre-compiled libraries,
# add it to the line below (e.g. "LIBRARIES = libsaast.a")
# --------------------------------------------------------

# --------------------------------------------------------
# you shouldn't change anything below here,
# unless you really know what you're doing
# --------------------------------------------------------
DEVICE     = atmega32u4
CLOCK      = 16000000

COMPILE = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

# symbolic targets: 
all:	$(FILE).hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

.c.s:
	$(COMPILE) -S $< -o $@

install: flash 

flash: all
	dfu-programmer $(DEVICE) erase
	dfu-programmer $(DEVICE) flash $(FILE).hex

clean:
	rm -f $(FILE).hex $(FILE).elf $(MAIN) $(CHILDREN)

# file targets:
$(FILE).elf: $(MAIN) $(CHILDREN) $(PARENTS)
	$(COMPILE) -o $(FILE).elf $(MAIN) $(CHILDREN) $(PARENTS) $(LIBRARIES)

$(FILE).hex: $(FILE).elf
	rm -f $(FILE).hex
	avr-objcopy -j .text -j .data -O ihex $(FILE).elf $(FILE).hex

# Targets for code debugging and analysis:
disasm:	$(FILE).elf
	avr-objdump -d $(FILE).elf

cpp:
	$(COMPILE) -E $(FILE).c
