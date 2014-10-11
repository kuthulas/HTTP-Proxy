client.o: proxy.o
	gcc -Wall client.c -o client
proxy.o: 
	gcc -Wall proxy.c -o proxy
clean:
	rm -rf client
	rm -rf proxy