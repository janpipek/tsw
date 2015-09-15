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
#include <iostream>
#include <utility>
#include <array>
#include <mutex>
#include <fstream>
#include <iomanip>

#ifdef TSW_USE_POSIX_THREADS
    #include <pthread.h>
    #include <stdexcept>
    #define TSW_MUTEX_DECLARATION pthread_mutex_t _mutex;
    #define TSW_MUTEX_INITIALIZATION pthread_mutexattr_t attr;\
        pthread_mutexattr_init(&attr);\
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);\
        pthread_mutex_init(&_mutex, &attr);
    #define TSW_LOCK bool ok = true;\
        pthread_mutex_lock(&_mutex);\
        try {
    #define TSW_UNLOCK } \
        catch(...) { ok = false; }\
        pthread_mutex_unlock(&_mutex);\
        if (!ok) {\
            throw std::runtime_error("Error in threaded writing");\
        }
#else
    #define TSW_USE_CPPT11_THREADS
    #include <thread>
    #define TSW_MUTEX_INITIALIZATION ;
    #define TSW_MUTEX_DECLARATION std::recursive_mutex _mutex;
    #define TSW_LOCK { std::lock_guard<std::recursive_mutex> lock(_mutex);
    #define TSW_UNLOCK }
#endif

namespace tsw
{
    class ThreadSafeWriter
    {
    public:
        virtual void Flush() = 0;

        virtual ~ThreadSafeWriter() = default;     // Enable polymorphism
    };

    template <class... Ts> class BaseThreadSafeWriter : public ThreadSafeWriter { };

    template <class U, class... Ts> class BaseThreadSafeWriter<U, Ts...> : public BaseThreadSafeWriter<>
    {
    public:
        BaseThreadSafeWriter() : _columnNames(nullptr)
        {
            TSW_MUTEX_INITIALIZATION;
        }

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

        virtual ~BaseThreadSafeWriter()
        {
            if (_columnNames)
            {
                delete _columnNames;
            }
        }

        void Store(const itemT& item)
        {
            TSW_LOCK;
            _data.push_back(item);
            if (IsFlushRequired())
            {
                Flush();
            }
            TSW_UNLOCK;
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
            _cacheCapacity = capacity;
            if (IsFlushRequired())
            {
                Flush();
            }
        }

        void Flush() override
        {
            TSW_LOCK;
            for (auto item : _data) {
                Write(item);
            }
            FinishFlush();
            _data.clear();
            TSW_UNLOCK;
        }

        virtual void FinishFlush() { }

    protected:
        virtual void Write(const std::tuple<U, Ts...>& item) = 0;

        size_t _cacheCapacity = 1000;

        std::vector<itemT> _data;

        TSW_MUTEX_DECLARATION;

        nameCollectionT* _columnNames;
    };

    template <class... Ts> class TSVWriter : public BaseThreadSafeWriter<> { };

    template <class U, class... Ts> class TSVWriter<U, Ts...> : public BaseThreadSafeWriter<U, Ts...>
    {
    protected:
        using BaseThreadSafeWriter<U, Ts...>::_columnNames;

        // using BaseThreadSafeWriter<U, Ts...>::Flush();

    public:
        using BaseThreadSafeWriter<U, Ts...>::itemDim;

        TSVWriter(const std::string& fileName) : _fileName(fileName), _stream(nullptr), _separator("\t"), _precision(6)
        {
        }

        virtual ~TSVWriter()
        {
            BaseThreadSafeWriter<U, Ts...>::Flush();
            if (_opened)
            {
                _stream->close();
                delete _stream;
            }
        }

        void SetSeparator(const std::string& sep)
        {
            _separator = sep;
        }

        void SetPrecision(int digits)
        {
            _precision = digits;
            if (_opened)
            {
                (*_stream) << std::setprecision(_precision);
            }
        }

    protected:
        void Open()
        {
            _stream = new std::ofstream(_fileName);
            (*_stream) << std::setprecision(_precision);
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

        int _precision;

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
