client:
	gcc client.c -o ./output/client -I ./termbox/include ./termbox/lib/libtermbox.a
	./output/client 239.0.140.1 5000

server:
	gcc server.c -o ./output/server
	./output/server 239.0.140.1 192.168.0.4
