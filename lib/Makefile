

all: 
	g++ -ggdb -DPLGDIR='"."' test.c common.c config.c db.c tabledef.c -lrt -lz -lmysqlclient -o t

clean:
	rm -f *.o *.a *~ core
