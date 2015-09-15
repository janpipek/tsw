#include <tuple>
#include <vector>
#include <thread>
#include <iostream>
#include <utility>
#include <array>
#include <mutex>

namespace tsw
{
    template <class... Ts> class ThreadSafeWriter
    {  };

    template <class U, class... Ts> class ThreadSafeWriter<U, Ts...>
    {
        public:
            ThreadSafeWriter(std::array<std::string, sizeof...(Ts) + 1> columnNames)
            {

            }

            void Store(const std::tuple<U, Ts...>&& item)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _data.push_back(item);
            }

            void Store(U&& first, Ts&&... args)
            {
                Store(std::make_tuple(first, args...));
            }

            void Flush()
            {
                std::lock_guard<std::mutex> lock(_mutex);
                for (auto item : _data) {
                    Write(item);
                }
                _data.clear();
            }

        protected:
            template<std::size_t I = 0, typename... Vs> inline typename std::enable_if<I == sizeof...(Vs), void>::type Write(const std::tuple<Vs...>& t)
            {
                std::cout << "\n";
            }

            template<std::size_t I = 0, typename... Vs> inline typename std::enable_if<I < sizeof...(Vs), void>::type Write(const std::tuple<Vs...>& t)
            {
                std::cout << std::get<I>(t) << "\t";
                Write<I + 1, Vs...>(t);
            }

        private:
            std::mutex _mutex;

            std::vector<std::tuple<U, Ts...>> _data;
    };
}
