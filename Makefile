all:
	g++ -g convert.cpp -lsqlite3 -lpthread -o chinese2pinyin
clean:
	rm chinese2pinyin
