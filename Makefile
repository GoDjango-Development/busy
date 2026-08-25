cflags = -I ./include

#Release profile

obj = release/obj/main.o release/obj/busy.o

hdr = include/busy.h

release_bin = release/busy
CCX = gcc -o
CC = gcc -c
release_command = $(CCX) $(release_bin) $(obj) $(cflags)

release: $(obj) $(hdr)
	$(release_command)

release/obj/main.o: src/main.c
	$(CC) src/main.c -o release/obj/main.o $(cflags)

release/obj/busy.o: src/busy.c include/busy.h
	$(CC) src/busy.c -o release/obj/busy.o $(cflags)


run: release
ifneq ("$(wildcard $(release_bin))","")
	$(release_bin)
else
	$(release_command)
	$(release_bin)
endif

.PHONY: rebuild

rebuild:
	$(MAKE) clean
	$(MAKE) release

# Debug profile

dbg = debug/obj/main.o debug/obj/busy.o

debug_bin = debug/busy
CCGX = gcc -g -DDEBUG -o
CCG = gcc -g -DDEBUG -c
debug_command = $(CCGX) $(debug_bin) $(dbg) $(cflags)

debug: $(dbg) $(hdr)
	$(debug_command)

debug/obj/main.o: src/main.c
	$(CCG) src/main.c -o debug/obj/main.o $(cflags)

debug/obj/busy.o: src/busy.c include/busy.h
	$(CCG) src/busy.c -o debug/obj/busy.o $(cflags)

run_debug: debug
ifneq ("$(wildcard $(debug_bin))","")
	gdb $(debug_bin)
else
	$(debug_command)
	gdb $(debug_bin)
endif

.PHONY: rebuild_debug

rebuild_debug:
	$(MAKE) clean
	$(MAKE) debug

#Others rules

.PHONY: prepare clean

prepare:
	mkdir -p debug release debug/obj release/obj
	mkdir -p src include
clean:
	rm -R -f $(obj) $(dbg) release/obj/* debug/obj/*
	rm -R -f $(release_bin) $(debug_bin)

