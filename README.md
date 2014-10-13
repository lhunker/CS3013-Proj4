Lukas Hunker project 4

To compile:
	make all

To run:
	./proj4 filename [size|mmap]

Will either process file in chunks using read system calls, or by memory mapping. Can also run using multiple threads. When multithreading - it will use a mailbox to pass messages amoung threads.