#include <tuple>
#include <vector>
#include <thread>
#include <iostream>
#include <utility>
#include <array>
#include <mutex>

namespace tsw
{
    template <class... Ts> class ThreadSafeWriter {  };

    template <class U, class... Ts> class ThreadSafeWriter<U, Ts...>
    {
    public:
        const static size_t itemDim = sizeof...(Ts) + 1;

        typedef std::array<std::string, itemDim> nameCollectionT;

        typedef std::tuple<U, Ts...> itemT;

        ThreadSafeWriter(nameCollectionT columnNames) : _columnNames(columnNames)
        {

        }

        ThreadSafeWriter()
        {

        }

        void SetColumnNames(nameCollectionT columnNames)
        {
            _columnNames = columnNames;
        }

        template <class... Vs> void SetColumnNames(const std::string& name1, const Vs&... names)
        {
            static_assert(sizeof...(Vs) == (itemDim - 1), "Column names must be of the same dimension as data.");
            _columnNames = {name1, names...};
        }

        virtual ~ThreadSafeWriter() = default;

        void Store(const itemT& item)
        {
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            _data.push_back(item);
            if (IsFlushRequired())
            {
                Flush();
            }
        }

        void Store(const U& first, const Ts&... args)
        {
            Store(std::make_tuple(first, args...));
        }

        virtual bool IsFlushRequired()
        {
            return _data.size() == _cacheCapacity;
        }

        virtual void SetCacheCapacity(size_t capacity)
        {
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            _cacheCapacity = capacity;
            if (IsFlushRequired())
            {
                Flush();
            }
        }

        void Flush()
        {
            std::cout << "Flushing..." << std::endl;
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            for (auto item : _data) {
                Write(item);
            }
            _data.clear();
        }

    protected:
        virtual void Write(const std::tuple<U, Ts...>& item) = 0;

        size_t _cacheCapacity = 100;

        std::vector<itemT> _data;

        std::recursive_mutex _mutex;

        nameCollectionT _columnNames;
    };

    template <class... Ts> class TSVWriter { };

    template <class U, class... Ts> class TSVWriter<U, Ts...> : public ThreadSafeWriter<U, Ts...>
    {
    public:
        ~TSVWriter()
        {
            ThreadSafeWriter<U, Ts...>::Flush();
        }

    protected:
        virtual void Write(const std::tuple<U, Ts...>& item) override
        {
            WriteItem(item);
        }

        template<std::size_t I = 0, typename... Vs> inline typename std::enable_if<I == sizeof...(Vs), void>::type WriteItem(const std::tuple<Vs...>& t)
        {
            std::cout << "\n";
        }

        template<std::size_t I = 0, typename... Vs> inline typename std::enable_if<I < sizeof...(Vs), void>::type WriteItem(const std::tuple<Vs...>& t)
        {
            std::cout << std::get<I>(t) << "\t";
            WriteItem<I + 1, Vs...>(t);
        }
    };
}
