all: udp_client4



udp_client4: udp_client4.o
	${CC} -o udp_client4 -g udp_client4.o

udp_client4.o: udp_client4.c
	${CC} -g -c -Wall udp_client4.c

clean:
	rm -f *.o *~ udp_client4