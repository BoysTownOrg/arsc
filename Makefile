# makefile for ARSC using MinGW compiler

CC = gcc
AR = ar
CFLAGS = -O2 -Wall -Wno-stringop-truncation -DANSI_C -fpack-struct -Dmgw 
WINLIB = -lwinmm -lkernel32 -luser32 -lgdi32
LIBS = $(WINLIB) -lm 
EFILES = tstfm tstlat tstout tstsio wrtnfo devlst arsc_chk
OFILES = arsc_api.o arsc_xfer.o arsc_win.o sio_arsc.o
BINDIR=c:/usr/bin
LIBDIR=c:/usr/lib
INCDIR=c:/usr/include
VSLIBDIR=../lib
VSINCDIR=../include

all: $(EFILES) $(AFILES)

arsc_chk : arsc_chk.o $(OFILES)
	$(CC) -o $@ $^ $(LIBS)

devlst : devlst.o $(OFILES)
	$(CC) -o $@ $^ $(LIBS)

tstfm : tstfm.o $(OFILES)
	$(CC) -o $@ $^ $(LIBS)

tstlat : tstlat.o $(OFILES)
	$(CC) -o $@ $^ $(LIBS)

tstout : tstout.o $(OFILES)
	$(CC) -o $@ $^ $(LIBS)

tstsio : tstsio.o siodll.o $(OFILES)
	$(CC) -o $@ $^ $(LIBS)

wrtnfo : wrtnfo.o $(OFILES)
	$(CC) -o $@ $^ $(LIBS)

libarsc.a: $(OFILES) 
	$(AR) rs libarsc.a $(OFILES)

install : libarsc.a 
#	mkdir -p $(BINDIR) $(LIBDIR) $(INCDIR)
	cp -f libarsc.a $(LIBDIR)
	cp -f arsclib.h $(INCDIR)
	cp -f siodll.h  $(INCDIR)

clean:
	rm -f *.bak *.o  *.log *~ tst *.txt *.a
	rm -f $(EFILES) *.exe

version:
	perl version.pl

arscsc.zip: $(EFILES)
	rm -f $@
	zip arscsc *.mgw *.lnx *.mac *.arm
	zip arscsc *.c *.h *.def *.pdf *.plt *.pl
	zip arscsc *.txt *.reg configure configure.bat
	zip arscsc arsc_asio/*.txt arsc_asio/arsc*.*
	zip arscsc arsc_cpp/*.cc arsc_cpp/Makefile
	zip arscsc arsc_mex/*.c arsc_mex/*.m arsc_mex/*.bat
	zip arscsc arsc_mex/Makefile arsc_mex/_README.txt
	zip arscsc arsc_VS16/*.sln arsc_VS16/*.vcxproj 

arscmex.zip: 
	rm -f $@
	zip -j arscmex arsc_mex/*.c arsc_mex/*.m arsc_mex/_README.txt
	zip -j arscmex arsc_mex/arsc_mex.mexw32 arsc_mex/arsc_mex.mexglx

arscdll.zip: 
	rm -f $@
	zip -j arscdll arsc_VS16/Release/arsc.dll
	zip -j arscdll arsclib.h

sio_arsc.zip: 
	rm -f $@
	zip -j sio_arsc tstsio.c
	zip -j sio_arsc arsc_VS16/Release/tstsio.exe
	zip -j sio_arsc arsc_VS16/Release/sio.dll
	zip -j arscdll siodll.h

arsc-win.zip: version arsc_VS16/Release/arsc_chk.exe
	rm -f $@
	../setup/iscc.exe arsc.iss
	zip -j arsc-win Output/setup.exe arsc_VS16/_README.txt
	rm -rf Output

dist: arscsc.zip arscmex.zip arscdll.zip sio_arsc.zip arsc-win.zip
	mv -f $^ ..\dist

# C file dependencies
# (makedep will replace everything after this line) 

arsc_alsa.o : arscdev.h
arsc_api.o : version.h
arsc_asio.o : arscdev.h
arsc_helper.o : version.h
arsc_win.o : arsclib.h arscdev.h
sio_arsc.o : sio.h arsclib.h
siodll.o : sio.h siodll.h
tstfm.o : arsclib.h
tstlat.o : arsclib.h
tstout.o : arsclib.h
tstsio.o : siodll.h
