all: footswitch_rom.h

footswitch_rom.h: footswitch.tal
	uxnasm footswitch.tal footswitch.rom
	xxd -i footswitch.rom > footswitch_rom.h
	rm footswitch.rom

.PHONY: clean

clean:
	rm -f footswitch.rom footswitch_rom.h