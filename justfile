client:
	gcc client.c -o client -L./termbox/lib -I./termbox/include
	./client 239.0.140.1 5000

server:
	gcc server.c -o server
	./server 239.0.140.1 172.16.0.117