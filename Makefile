LIB=-lpthread
CCPP=g++

all: proj4

proj4: proj4.C
	$(CCPP) proj4.C mailboxs.C -o proj4 $(LIB)

clean:
	rm proj4
