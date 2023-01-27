all: vcube

vcube: vcube.o smpl.o rand.o cis.o
	$(LINK.c) -o $@ -Bstatic vcube.o smpl.o rand.o cis.o -lm

smpl.o: smpl.c smpl.h
	$(COMPILE.c)  -g smpl.c

vcube.o: vcube.c smpl.h
	$(COMPILE.c) -g  vcube.c

rand.o: rand.c
	$(COMPILE.c) -g rand.c

cis.o: cis.c cis.h
	$(COMPILE.c) -g cis.c

clean:
	$(RM) *.o vcube relat saida
