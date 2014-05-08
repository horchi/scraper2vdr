
BASELIBS = -lrt -lz -lmysqlclient -lcurl  -luuid
BASELIBS += $(shell mysql_config --libs)

CFLAGS   += $(shell mysql_config --include)

all: 
	g++ -ggdb -DPLGDIR='"."' $(CFLAGS) test.c dbdict.c common.c config.c db.c tabledef.c $(BASELIBS) -o t

demo: demo.c
	g++ -ggdb -DUSEUUID -DPLGDIR='"."' $(CFLAGS) demo.c common.c db.c tabledef.c config.c $(BASELIBS) -o demo

clean:
	rm -f *.o *.a *~ core demo
