DataFile?=WikiData.txt

pagerank:
	mkdir build
	g++ -o ./build/pagerank ./source/main.cpp -std=c++11

.PHONY: clean run

run: ./build/pagerank
	./build/pagerank ./dataset/$(DataFile);

clean:
	rm -rf ./build

