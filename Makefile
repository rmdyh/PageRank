DataFile?=WikiData.txt

pagerank: source/main.cpp source/pagerank.h
	-mkdir build
	g++ -o ./build/pagerank ./source/main.cpp -std=c++11

.PHONY: clean run

run: pagerank
	./build/pagerank ./dataset/$(DataFile);

clean:
	rm -rf ./build
