LIBS = -larsc -lasound -lm 
PGMS = devlst tstout

all : $(PGMS)
	./devlst

devlst : devlst.o
	g++ -o devlst devlst.o $(LIBS)

tstout : tstout.o
	g++ -o tstout tstout.o $(LIBS)

clean:
	rm -f *.o *~ $(PGMS)
