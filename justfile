client:
	gcc client.c -o ./output/client -I ./termbox/include ./termbox/lib/libtermbox.a
	./client 239.0.140.1 5000

server:
	gcc server.c -o ./output/server
	./server 239.0.140.1 172.16.0.117