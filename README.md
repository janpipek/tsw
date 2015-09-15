# (T)hread-(S)afe cached data (W)riter for C++ 11

## Dependencies

This small library is header-only. Just copy `tsw.hh` it to your project and start
using it. You just need a C++11-supporting compiler.

## Examples

```c++
#include "tsw.hh"

using namespace tsw;

int main() {
    tsw::TSVWriter<double, double> writer("out.tsv");
    writer.SetColumnNames("x", "x^2");
    for (int i = 1; i < 10; i++) {
        writer.Store(i, i * i);
    }
}
```

Output (out.tsv):

```
x       x^2
1       1       
2       4       
3       9       
4       16      
5       25      
6       36      
7       49      
8       64      
9       81      
10      100  
```

## Threading example

**TODO: include later**
