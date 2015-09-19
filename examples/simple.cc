// g++ -std=c++11 simple.cc -I.. -o simple ; ./simple; cat out.tsv

#define TSW_NO_THREADS    // There will be no thread(t)s.
#include "tsw.hh"
#include <iostream>
#include <cmath>

using namespace tsw;
using namespace std;

int main() {
    tsw::TSVWriter<int, int, double> writer("out.tsv");
    writer.SetColumnNames("x", "x^2", "sqrt(x)");
    writer.SetPrecision(4);
    for (int i = 1; i < 100; i++) {
        writer.Store(i, i*i, sqrt(i));
    }
    cout << writer.GetItemsStored() << " items stored." << endl;  
}
