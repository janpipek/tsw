# (T)hread-(S)afe cached data (W)riter for C++ 11

This library was motivated by the need of writing large numerical datasets 
with the following constraints:

* human-readable format (e.g. TSV)
* compile-time-configurable row "tuple" with different data types
* thread-safety (writing from multiple, possibly many threads in parallel)
* output efficiency (cheap caching + fast chunked output)

I hope the goals were reasonably achieved.

## Threading

It can use three threading model for mutex:

* from C++11 standard (by default)
* from pthreads (set macro `TSW_USE_POSIX_THREADS`)
* no threading model, in case you want to use it non-threading application (set macro `TSW_NO_THREADS`)

In any case, the order of stored rows is preserved.

## Dependencies

This small library is header-only. Just copy `tsw.hh` to your project and start
using it. You need:

* a C++11-supporting compiler.
* pthread (needed if used directly, needed in GCC on Linux)

## Limitations

* The I/O operation is done in the thread that arrives at the cache limit (and blocks further
storing). So, in principle, don't expect *each* Store operation to be extremely fast - only most
of them are. A better approach (planned?) would be to dedicate a separate thread to I/O.

* Strings don't yet get the proper treatment. Thus the TSVWriter does not check for
the presence of delimiters in the string. This could be done, for sure...

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

See `examples/std_thread.cc`
