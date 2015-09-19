// g++ -std=c++11 simple.cc -I.. -o simple ; ./simple; cat out.tsv

#define TSW_NO_THREADS    // There will be no thread(t)s.
#include "tsw.hh"

using namespace tsw;

int main() {
    tsw::TSVWriter<double, double> writer("out.tsv");
    writer.SetColumnNames("x", "x^2");
    for (int i = 1; i < 100; i++) {
        writer.Store(i, i*i);
    }
}
