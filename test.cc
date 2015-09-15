#include "tsw.hh"

#include <cmath>

using namespace tsw;

int main()
{
    TSVWriter<int, int, double> writer("out.tsv");//{{"i", "j", "k"}};
    writer.SetColumnNames("a", "a^2", "sqrt(a)");
    std::string f = "ahoj";
    std::tuple<int, int, std::string> x{1, 2, f};
    writer.SetCacheCapacity(5);
    writer.SetPrecision(3);
    for (int i = 1; i < 11; i++)
    {
        writer.Store(i, i * i, sqrt(i));
    }
    // writer.Flush();
    // writer.Write({4, 4, std::string("ahoj")});
}
