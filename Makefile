test: test.cc tsw.hh
	g++ -o test -std=c++11 test.cc

run: test
	./test
	cat out.tsv
