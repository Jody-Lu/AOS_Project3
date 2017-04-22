all: setup server

server: setup bin/server bin/server.config

bin/server: obj/exception.o obj/utils.o obj/config.o obj/tcp_socket.o obj/connection_manager.o obj/callback_bridge.o obj/registry.o obj/tcp_server.o obj/server.o obj/jajodia_mutchler.o
	g++ -Wall -std=c++11 -o bin/server obj/exception.o obj/utils.o obj/config.o obj/tcp_socket.o obj/connection_manager.o obj/callback_bridge.o obj/registry.o obj/tcp_server.o obj/server.o obj/jajodia_mutchler.o

obj/server.o: src/server.cpp
	g++ -Wall -std=c++11 -c src/server.cpp -o obj/server.o

obj/tcp_server.o: src/tcp_server.h src/tcp_server.cpp
	g++ -Wall -std=c++11 -c src/tcp_server.cpp -o obj/tcp_server.o

obj/registry.o: src/registry.h src/registry.cpp
	g++ -Wall -std=c++11 -c src/registry.cpp -o obj/registry.o

obj/callback_bridge.o: src/callback_bridge.h src/callback_bridge.cpp
	g++ -Wall -std=c++11 -c src/callback_bridge.cpp -o obj/callback_bridge.o

obj/connection_manager.o: src/connection_manager.h src/connection_manager.cpp
	g++ -Wall -std=c++11 -c src/connection_manager.cpp -o obj/connection_manager.o

obj/tcp_socket.o: src/tcp_socket.h src/tcp_socket.cpp
	g++ -Wall -std=c++11 -c src/tcp_socket.cpp -o obj/tcp_socket.o

obj/config.o: src/config.h src/config.cpp
	g++ -Wall -std=c++11 -c src/config.cpp -o obj/config.o

obj/exception.o: src/exception.h src/exception.cpp
	g++ -Wall -std=c++11 -c src/exception.cpp -o obj/exception.o

obj/utils.o: src/utils.h src/utils.cpp
	g++ -Wall -std=c++11 -c src/utils.cpp -o obj/utils.o

obj/jajodia_mutchler.o: src/jajodia_mutchler.h src/jajodia_mutchler.cpp
	g++ -Wall -std=c++11 -c src/jajodia_mutchler.cpp -o obj/jajodia_mutchler.o


bin/server.config:
	cp server.config bin/

bin/client.config:
	cp client.config bin/

bin/quorum.config:
	cp quorum.config bin/

setup: bin/ obj/

bin/:
	mkdir -p bin

obj/:
	mkdir -p obj

.PHONY: clean
clean:
	rm -rf bin/ obj/
