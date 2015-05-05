CC=gcc
CFLAGS= -g -Wall

src=cma_example.c
exe=$(subst .c,,$(src))

$(exe):$(src)
	@echo CC $<
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(exe)
