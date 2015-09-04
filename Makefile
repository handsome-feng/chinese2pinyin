all:
	g++ -g chinese2pinyin.cpp main.cpp -lsqlite3 -lpthread -o chinese2pinyin
clean:
	rm chinese2pinyin
