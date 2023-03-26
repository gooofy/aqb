include ../../config.mk

.PHONY: clean all

all: foo

%.o : %.c
	$(CC_AMIGAOS) -c $(CFLAGS_AMIGAOS) -I../../src/lib/_brt $< -o $@

%.asm: %.c
	$(CC_AMIGAOS) -S $(CFLAGS_AMIGAOS) -I../../src/lib/_brt $< -o $@

foo : foo.bas mod2.a mod2.sym
	$(AQB) -L ../../src/lib -d _brt -o $@ foo.bas
	cp foo ${AMIGA_X}/

main : main.c mod2.a
	$(CC_AMIGAOS) $(CFLAGS_AMIGAOS) -I../../src/lib/_brt $< -o $@ mod2.a
	cp main ${AMIGA_X}/

mod2.sym : mod2.bas
	$(AQB) -L ../../src/lib -d _brt -s $@ mod2.bas
	# $(AQB) -L ../../src/lib -d _brt -I -s $@ mod2.bas

mod2.a: mod2.o
	$(AR_AMIGAOS) -crs mod2.a $^
	pushd .. && rm -f mod2.a && ln -s mod2/mod2.a && popd

clean:
	rm -f *.o mod2.a *.sym *.asm aqb.log

