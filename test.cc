#include "tsw.hh"

using namespace tsw;

int main()
{
    TSVWriter<int, int, std::string> writer("out.tsv");//{{"i", "j", "k"}};
    writer.SetColumnNames("aa", "bb", "cc");
    std::string f = "ahoj";
    std::tuple<int, int, std::string> x{1, 2, f};
    writer.SetCacheCapacity(10);
    for (int i = 1; i < 100; i++)
    {
        writer.Store(i, i * i, "ahoj");
    }
    // writer.Flush();
    // writer.Write({4, 4, std::string("ahoj")});
}
