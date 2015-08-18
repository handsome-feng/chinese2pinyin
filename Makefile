all:
	g++ -g convert.cpp -lsqlite3 -lpthread -o convert
clean:
	rm convert
