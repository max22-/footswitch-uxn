all:
	make -C src
	pio run

run:
	make -C src
	pio run -t upload
	pio device monitor
