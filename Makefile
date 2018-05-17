
WADLINKOBJS=	$(OBJDIR)/wadlink.o \
				$(OBJDIR)/cmdlib.o \
				$(OBJDIR)/scriplib.o \
				$(OBJDIR)/lzlib.o

LUMPYOBJS=		$(OBJDIR)/lumpy.o \
				$(OBJDIR)/grabcmds.o \
				$(OBJDIR)/lbmlib.o \
				$(OBJDIR)/cmdlib.o \
				$(OBJDIR)/scriplib.o \
				$(OBJDIR)/jaggrab.o

all:	unfuck	\
		ulz77 \
		lz77 \
		wadlink \
		removectrlm \
		cmpfile \
		lumpy \
		spitwad \
		multigen \
		dcolors \
		mapcomp

######################	removectrlm

removectrlm:	$(OBJDIR)/removectrlm.o
	$(CC) $(LDFLAGS) $(OBJDIR)/removectrlm.o -o removectrlm $(LIBS)

$(OBJDIR)/removectrlm.o:	removectrlm.c
	$(CC) $(CFLAGS) -c removectrlm.c -o $(OBJDIR)/removectrlm.o

######################	cmpfile

cmpfile:	$(OBJDIR)/cmpfile.o
	$(CC) $(LDFLAGS) $(OBJDIR)/cmpfile.o -o cmpfile $(LIBS)

$(OBJDIR)/cmpfile.o:	cmpfile.c
	$(CC) $(CFLAGS) -c cmpfile.c -o $(OBJDIR)/cmpfile.o

######################	wadlink
				
wadlink:	$(WADLINKOBJS)
	$(CC) $(LDFLAGS) $(WADLINKOBJS) -o wadlink $(LIBS)

$(OBJDIR)/wadlink.o:	wadlink.c cmdlib.h scriplib.h lzlib.h
	$(CC) $(CFLAGS) -c wadlink.c -o $(OBJDIR)/wadlink.o
$(OBJDIR)/cmdlib.o:	cmdlib.c cmdlib.h
	$(CC) $(CFLAGS) -c cmdlib.c -o $(OBJDIR)/cmdlib.o
$(OBJDIR)/scriplib.o:	scriplib.c scriplib.h cmdlib.h
	$(CC) $(CFLAGS) -c scriplib.c -o $(OBJDIR)/scriplib.o
$(OBJDIR)/lzlib.o:	lzlib.c lzlib.h
	$(CC) $(CFLAGS) -c lzlib.c -o $(OBJDIR)/lzlib.o

######################	lumpy

lumpy : $(LUMPYOBJS)
	$(CC) $(LDFLAGS) -o lumpy $(LUMPYOBJS) $(LIBS)

$(OBJDIR)/lumpy.o:	lumpy.c lumpy.h
	$(CC) $(CFLAGS) -c lumpy.c -o $(OBJDIR)/lumpy.o
$(OBJDIR)/grabcmds.o:	grabcmds.c lumpy.h
	$(CC) $(CFLAGS) -c grabcmds.c -o $(OBJDIR)/grabcmds.o
$(OBJDIR)/lbmlib.o:	lbmlib.c cmdlib.h lbmlib.h
	$(CC) $(CFLAGS) -c lbmlib.c -o $(OBJDIR)/lbmlib.o
$(OBJDIR)/jaggrab.o:	jaggrab.c lumpy.h
	$(CC) $(CFLAGS) -c jaggrab.c -o $(OBJDIR)/jaggrab.o

######################	spitwad

spitwad : $(OBJDIR)/spitwad.o $(OBJDIR)/cmdlib.o
	$(CC) $(LDFLAGS) -o spitwad $(OBJDIR)/spitwad.o $(OBJDIR)/cmdlib.o  $(LIBS)

$(OBJDIR)/spitwad.o:	spitwad.c cmdlib.h
	$(CC) $(CFLAGS) -c spitwad.c -o $(OBJDIR)/spitwad.o

######################	multigen

multigen : $(OBJDIR)/multigen.o $(OBJDIR)/cmdlib.o $(OBJDIR)/scriplib.o
	$(CC) $(LDFLAGS) -o multigen $(OBJDIR)/multigen.o $(OBJDIR)/cmdlib.o $(OBJDIR)/scriplib.o $(LIBS)

$(OBJDIR)/multigen.o:	multigen.c cmdlib.h scriplib.h
	$(CC) $(CFLAGS) -c multigen.c -o $(OBJDIR)/multigen.o

######################	dcolors

dcolors : $(OBJDIR)/dcolors.o $(OBJDIR)/cmdlib.o $(OBJDIR)/lbmlib.o
	$(CC) $(LDFLAGS) -o dcolors $(OBJDIR)/dcolors.o $(OBJDIR)/cmdlib.o $(OBJDIR)/lbmlib.o $(LIBS)

$(OBJDIR)/dcolors.o:	dcolors.c cmdlib.h lbmlib.h
	$(CC) $(CFLAGS) -c dcolors.c -o $(OBJDIR)/dcolors.o

######################	mapcomp

mapcomp : $(OBJDIR)/mapcomp.o $(OBJDIR)/cmdlib.o
	$(CC) $(LDFLAGS) -o mapcomp $(OBJDIR)/mapcomp.o $(OBJDIR)/cmdlib.o $(LIBS)

$(OBJDIR)/mapcomp.o:	mapcomp.c cmdlib.h doomdata.h
	$(CC) $(CFLAGS) -c mapcomp.c -o $(OBJDIR)/mapcomp.o

######################	lz77

lz77 : $(OBJDIR)/lz77.o $(OBJDIR)/cmdlib.o $(OBJDIR)/lzlib.o
	$(CC) $(LDFLAGS) -o lz77 $(OBJDIR)/lz77.o $(OBJDIR)/cmdlib.o $(OBJDIR)/lzlib.o $(LIBS)

$(OBJDIR)/lz77.o:	lz77.c cmdlib.h lzlib.h
	$(CC) $(CFLAGS) -c lz77.c -o $(OBJDIR)/lz77.o

######################	ulz77

ulz77 : $(OBJDIR)/ulz77.o $(OBJDIR)/cmdlib.o $(OBJDIR)/lzlib.o
	$(CC) $(LDFLAGS) -o ulz77 $(OBJDIR)/ulz77.o $(OBJDIR)/cmdlib.o $(OBJDIR)/lzlib.o $(LIBS)

$(OBJDIR)/ulz77.o:	ulz77.c cmdlib.h lzlib.h
	$(CC) $(CFLAGS) -c ulz77.c -o $(OBJDIR)/ulz77.o

######################	unfuck

unfuck : $(OBJDIR)/unfuck.o
	$(CC) $(LDFLAGS) -o unfuck $(OBJDIR)/unfuck.o $(LIBS)

$(OBJDIR)/unfuck.o:	unfuck.c
	$(CC) $(CFLAGS) -c unfuck.c -o $(OBJDIR)/unfuck.o

