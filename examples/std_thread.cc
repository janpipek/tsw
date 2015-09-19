// g++ -std=c++11 std_thread.cc -I.. -lpthread -o std_thread ; ./std_thread; cat out.tsv

/**
  * This example starts a lot of threads each of which prints two messages ("hi!" and "bye!")
  * at random times. The resulting file should have both messages from each thread,
  * probably interleaved almost chaotically :-)
  */
#include <thread>
#include <random>
#include <chrono>
#include <iostream>

#define TSW_USE_CPPT11_THREADS
#include "tsw.hh"

using namespace tsw;
using namespace std;

int main() {
    tsw::TSVWriter<int, std::string> writer("out.tsv");
    writer.SetColumnNames("thread-id", "message");
    writer.SetCacheCapacity(37);

    auto thread_func = [&writer](int i){
        writer.Store(i, "hi!");

        // Wait a little bit
        std::mt19937_64 eng{std::random_device{}()};  // or seed however you want
        std::uniform_int_distribution<> dist{0, 1000};
        std::this_thread::sleep_for(std::chrono::microseconds{dist(eng)});

        writer.Store(i, "bye!");
    };

    vector<thread*> threads;

    int numOfThreads = 100; 
    for (int i = 0; i < numOfThreads; i++)
    {
        thread* t = new thread{thread_func, i};
        threads.push_back(t);
    }
    for (auto thread : threads)
    {
        thread->join();
    }
    cout << writer.GetItemsStored() << " items stored." << endl;
}
