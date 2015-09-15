#include "tsw.hh"

using namespace tsw;

int main()
{
    ThreadSafeWriter<int, int, std::string> writer{{"i", "j", "k"}};
    std::string f = "ahoj";
    std::tuple<int, int, std::string> x{1, 2, f};
    writer.Store(1, 2, "ahoj");
    writer.Flush();
    // writer.Write({4, 4, std::string("ahoj")});
}
