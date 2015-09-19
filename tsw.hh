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
#include <fstream>
#include <iomanip>

#ifdef TSW_USE_POSIX_THREADS
    #include <pthread.h>
    #include <stdexcept>
    #define TSW_MUTEX_DECLARATION pthread_mutex_t _mutex
    #define TSW_MUTEX_INITIALIZATION pthread_mutexattr_t attr;\
        pthread_mutexattr_init(&attr);\
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);\
        pthread_mutex_init(&_mutex, &attr)
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
    #ifdef TSW_NO_THREADS
        #define TSW_MUTEX_INITIALIZATION
        #define TSW_MUTEX_DECLARATION
        #define TSW_LOCK
        #define TSW_UNLOCK
    #else
        #define TSW_USE_CPPT11_THREADS
        #include <mutex>
        #define TSW_MUTEX_INITIALIZATION
        #define TSW_MUTEX_DECLARATION std::recursive_mutex _mutex
        #define TSW_LOCK { std::lock_guard<std::recursive_mutex> lock(_mutex)
        #define TSW_UNLOCK }
    #endif
#endif

namespace tsw
{
    /**
      * @short Base class of all thread safe writers.
      *
      * The implementation is defined using templates (including most methods)
      * Only non-templated abstract methods can be declared (as interface) here.
      */
    class ThreadSafeWriter
    {
    public:
        virtual void SetCacheCapacity(size_t capacity) = 0;

        virtual bool IsFlushRequired() = 0;

        virtual void Flush() = 0;

        virtual ~ThreadSafeWriter() = default;     // Enable polymorphism
    };

    template <class... Ts> class BaseThreadSafeWriter : public ThreadSafeWriter { };

    /**
      * @short Base templated class implementing the cache and thread-safety.
      *
      * The particular method for writing is to be implemented in child classes.
      */
    template <class U, class... Ts> class BaseThreadSafeWriter<U, Ts...> : public BaseThreadSafeWriter<>
    {
    public:
        BaseThreadSafeWriter() : _columnNames(nullptr)
        {
            TSW_MUTEX_INITIALIZATION;
        }

        constexpr static size_t itemDim = sizeof...(Ts) + 1;

        using nameCollectionT = std::array<std::string, itemDim>;

        using itemT = std::tuple<U, Ts...> ;

        void SetColumnNames(const nameCollectionT& columnNames)
        {
            // TODO: Prohibit once first data item is stored
            _columnNames = new nameCollectionT(columnNames);
        }

        template <class... Vs> void SetColumnNames(const std::string& name1, const Vs&... names)
        {
            // TODO: Prohibit once first data item is stored
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

        bool IsFlushRequired() override
        {
            return _data.size() == _cacheCapacity;
        }

        void SetCacheCapacity(size_t capacity) override
        {
            TSW_LOCK;
            _cacheCapacity = capacity;
            if (IsFlushRequired())
            {
                Flush();
            }
            else
            {
                _data.reserve(_cacheCapacity);
            }
            TSW_UNLOCK;
        }

        void Flush() override
        {
            TSW_LOCK;
            StartFlush();
            for (auto item : _data) {
                Write(item);
            }
            FinishFlush();
            _data.clear();
            _data.reserve(_cacheCapacity);
            TSW_UNLOCK;
        }

    protected:
        virtual void StartFlush() { }

        virtual void FinishFlush() { }

        virtual void Write(const std::tuple<U, Ts...>& item) = 0;

        size_t _cacheCapacity = 1000;

        std::vector<itemT> _data;

        TSW_MUTEX_DECLARATION;

        nameCollectionT* _columnNames;
    };

    template <class... Ts> class TSVWriter : public BaseThreadSafeWriter<> { };

    /**
      * @short Writer for tabular text formats (TSV, CSV, ...)
      *
      * The default variant uses \n for line separation, \t for column separation.
      *
      * Warning: strings are not escaped in any way
      */
    template <class U, class... Ts> class TSVWriter<U, Ts...> : public BaseThreadSafeWriter<U, Ts...>
    {
    protected:
        using BaseThreadSafeWriter<U, Ts...>::_columnNames;

        using BaseThreadSafeWriter<U, Ts...>::Flush;

    public:
        using BaseThreadSafeWriter<U, Ts...>::itemDim;

        TSVWriter(const std::string& fileName) : _fileName(fileName), _stream(nullptr),
            _columnSeparator("\t"), _lineSeparator("\n"), _precision(6)
        {
        }

        TSVWriter(const TSVWriter& other) = delete;

        TSVWriter& operator=(const TSVWriter& other) = delete;

        virtual ~TSVWriter()
        {
            Flush();
            if (_opened)
            {
                _stream->close();
                delete _stream;
                _stream = nullptr;
            }
        }

        void SetColumnSeparator(const std::string& sep)
        {
            _columnSeparator = sep;
        }

        void SetLineSeparator(const std::string& sep)
        {
            _lineSeparator = sep;
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
                    *_stream << (*_columnNames)[i] << _columnSeparator;
                }
                *_stream << (*_columnNames)[itemDim - 1] << _lineSeparator;
                _stream->flush();
            }
            _opened = true;
        }

        bool _opened = false;

        std::string _fileName;

        std::ofstream* _stream;

        std::string _columnSeparator;

        std::string _lineSeparator;

        int _precision;

        void StartFlush() override
        {
            if (!_opened)
            {
                Open();
            }
        }

        void FinishFlush() override
        {
            _stream->flush();
        }

        void Write(const std::tuple<U, Ts...>& item) override
        {
            WriteItem(item);
        }

        template<std::size_t I = 0, typename... Vs> inline typename std::enable_if<I == sizeof...(Vs), void>::type WriteItem(const std::tuple<Vs...>&)
        {
            *_stream << _lineSeparator;
        }

        template<std::size_t I = 0, typename... Vs> inline typename std::enable_if<I < sizeof...(Vs), void>::type WriteItem(const std::tuple<Vs...>& t)
        {
            *_stream << std::get<I>(t) << _columnSeparator;
            WriteItem<I + 1, Vs...>(t);
        }
    };
}
