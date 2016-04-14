CFLAGS = -O1 -Wall -std=c99 -g

OBJE = entropy.o prng.o massdal.o frequent.o backup_heap.o c_a_heap.o heap.o symtab.o util.o naive.o naivesymtab.o slowentropy.o

TARGETS = automatedentropy entropymain
all: $(TARGETS)

automatedentropy: automatedentropy.o $(OBJE)
	gcc -o $@ $(OBJE) automatedentropy.o -lm

entropymain: entropymain.o $(OBJE)
	gcc -o $@ $(OBJE) entropymain.o -lm

.PHONY: clean depend
clean:
	rm -f $(TARGETS) *.o