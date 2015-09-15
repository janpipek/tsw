/**
  * (T)hread-(S)afe cached data (W)riter for C++ 11
  *
  * Copyright (c) 2015 Jan Pipek (MIT licence)
  *
  * This file can be used as is without external dependencies,
  * see https://github.com/janpipek/tsw .
  */

#include <tuple>
#include <vector>
#include <thread>
#include <iostream>
#include <utility>
#include <array>
#include <mutex>
#include <fstream>

namespace tsw
{
    template <class... Ts> class ThreadSafeWriter {  };

    template <class U, class... Ts> class ThreadSafeWriter<U, Ts...>
    {
    public:
        ThreadSafeWriter() : _columnNames(nullptr) { }

        const static size_t itemDim = sizeof...(Ts) + 1;

        typedef std::array<std::string, itemDim> nameCollectionT;

        typedef std::tuple<U, Ts...> itemT;

        void SetColumnNames(const nameCollectionT& columnNames)
        {
            _columnNames = new nameCollectionT(columnNames);
        }

        template <class... Vs> void SetColumnNames(const std::string& name1, const Vs&... names)
        {
            static_assert(sizeof...(Vs) == (itemDim - 1), "Column names must be of the same dimension as data.");
            if (_columnNames)
            {
                delete _columnNames;
            }
            auto temp = nameCollectionT{name1, names...};
            _columnNames = new nameCollectionT(temp);
        }

        virtual ~ThreadSafeWriter()
        {
            if (_columnNames)
            {
                delete _columnNames;
            }
        }

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

        virtual void Flush()
        {
            // std::cout << "Flushing..." << std::endl;
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            for (auto item : _data) {
                Write(item);
            }
            FinishFlush();
            _data.clear();
        }

        virtual void FinishFlush() { }

    protected:
        virtual void Write(const std::tuple<U, Ts...>& item) = 0;

        size_t _cacheCapacity = 100;

        std::vector<itemT> _data;

        std::recursive_mutex _mutex;

        nameCollectionT* _columnNames;
    };

    template <class... Ts> class TSVWriter { };

    template <class U, class... Ts> class TSVWriter<U, Ts...> : public ThreadSafeWriter<U, Ts...>
    {
    protected:
        using ThreadSafeWriter<U, Ts...>::_columnNames;

    public:
        using ThreadSafeWriter<U, Ts...>::itemDim;

        TSVWriter(const std::string& fileName) : _fileName(fileName), _stream(nullptr), _separator("\t")
        {
        }

        ~TSVWriter()
        {
            ThreadSafeWriter<U, Ts...>::Flush();
            if (_opened)
            {
                delete _stream;
            }
        }

        void SetSeparator(const std::string& sep)
        {
            _separator = sep;
        }

    protected:
        void Open()
        {
            _stream = new std::ofstream(_fileName);
            if (_columnNames)
            {
                for (size_t i = 0; i < itemDim - 1; i++)
                {
                    *_stream << (*_columnNames)[i] << _separator;
                }
                *_stream << (*_columnNames)[itemDim - 1] << std::endl;
            }
            _opened = true;
        }

        bool _opened = false;

        std::string _separator;

        std::ofstream* _stream;

        std::string _fileName;

        virtual void FinishFlush()
        {
            _stream->flush();
        }

        virtual void Write(const std::tuple<U, Ts...>& item) override
        {
            if (!_opened)
            {
                Open();
            }
            WriteItem(item);
        }

        template<std::size_t I = 0, typename... Vs> inline typename std::enable_if<I == sizeof...(Vs), void>::type WriteItem(const std::tuple<Vs...>& t)
        {
            *_stream << "\n";
        }

        template<std::size_t I = 0, typename... Vs> inline typename std::enable_if<I < sizeof...(Vs), void>::type WriteItem(const std::tuple<Vs...>& t)
        {
            *_stream << std::get<I>(t) << _separator;
            WriteItem<I + 1, Vs...>(t);
        }
    };
}
