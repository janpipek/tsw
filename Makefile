test: test.cc tsw.hh
	g++ -o test -std=c++11 test.cc -pthread

test_clang: test.cc tsw.hh
	clang++ test.cc -std=c++11 -o test_clang

run: test
	./test
	cat out.tsv

run_clang: test_clang
	./test_clang
	cat out.tsv
