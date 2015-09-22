#ifndef TSW_HEADER_INCLUDED
#define TSW_HEADER_INCLUDED
/**
  * (T)hread-(S)afe cached data (W)riter for C++ 11
  *
  * Copyright (c) 2015 Jan Pipek (MIT licence)
  *
  * This file can be used as is without external dependencies,
  * see https://github.com/janpipek/tsw .
  */

// Some hacking to make this work in various gcc older versions
#if __GNUG__ && !defined(__clang_major__)
    #if __GNUG__ < 4 || (__GNUG__ == 4 && __GNUC_MINOR < 4)
        #error Only supported in gcc >= 4.4
    #endif
    #if (__GNUG__ == 4 && __GNUC_MINOR < 5)
        #define noexcept
    #endif
    #if (__GNUG__ == 4 && __GNUC_MINOR < 6)
        #define constexpr const
        #define nullptr 0   // Good style but necessary
    #endif
    #if (__GNUG__ == 4 && __GNUC_MINOR < 7)
        #define override    // Good style but necessary
    #endif
#endif

#include <tuple>
#include <vector>
#include <iostream>
#include <utility>
#include <array>
#include <fstream>
#include <iomanip>
#include <memory>
#include <stdexcept>

// Enable explicitely not to use standard thread API (question is if it has any sense...)
#ifdef TSW_USE_POSIX_THREADS
    #include <pthread.h>
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
        /**
          * @short Set how many rows will be cached at maximum
          *        before the flushing is forced.
          */
        virtual void SetCacheCapacity(size_t capacity) = 0;

        virtual bool IsFlushRequired() const = 0;

        /**
          * @short This method forces data to be written to the file.
          *
          * Automatically triggered when the cache is full and in the destructor.
          */
        virtual void Flush() = 0;

        virtual ~ThreadSafeWriter() = default;     // Enable polymorphism

        virtual size_t GetItemsStored() const noexcept = 0;

        virtual size_t GetItemsWritten() const noexcept = 0;
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
        BaseThreadSafeWriter() : _columnNames(), _cacheCapacity(1000), _itemsStored(0), _itemsWritten(0)
        {
            TSW_MUTEX_INITIALIZATION;
        }

        constexpr static size_t itemDim = sizeof...(Ts) + 1;

        typedef std::array<std::string, itemDim> nameCollectionT;

        typedef std::tuple<U, Ts...> itemT;

        void SetColumnNames(const nameCollectionT& columnNames)
        {
            TSW_LOCK;
            if (_itemsStored)
            {
                throw std::runtime_error("Cannot change column names after items were already written.");
            }            
            _columnNames.reset(new nameCollectionT(columnNames));
            TSW_UNLOCK;
        }

        template <class... Vs> void SetColumnNames(const std::string& name1, const Vs&... names)
        {
            static_assert(sizeof...(Vs) == (itemDim - 1), "Column names must be of the same dimension as data.");
            
            TSW_LOCK;
            if (_itemsStored)
            {
                throw std::runtime_error("Cannot change column names after items were already written.");
            }
            auto temp = nameCollectionT{name1, names...};
            _columnNames.reset(new nameCollectionT(temp));
            TSW_UNLOCK;
        }

        void Store(const itemT& item)
        {
            TSW_LOCK;
            _data.push_back(item);
            if (IsFlushRequired())
            {
                Flush();
            }
            _itemsStored++;
            TSW_UNLOCK;
        }

        void Store(const U& first, const Ts&... args)
        {
            Store(std::make_tuple(first, args...));
        }

        bool IsFlushRequired() const override
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
            int localWritten = 0;
            for (auto item = _data.begin(); item != _data.end(); item++)
            {
            // for (auto item : _data) {
                Write(*item);
                localWritten++;
            }
            _itemsWritten += localWritten;
            FinishFlush();
            _data.clear();
            _data.reserve(_cacheCapacity);
            TSW_UNLOCK;
        }

        virtual size_t GetItemsStored() const noexcept override { return _itemsStored; }

        virtual size_t GetItemsWritten() const noexcept override { return _itemsWritten; }

    protected:
        virtual void StartFlush() { }

        virtual void FinishFlush() { }

        virtual void Write(const std::tuple<U, Ts...>& item) = 0;

        size_t _cacheCapacity;

        size_t _itemsStored;

        size_t _itemsWritten;

        std::vector<itemT> _data;

        TSW_MUTEX_DECLARATION;

        std::unique_ptr<nameCollectionT> _columnNames;
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

        TSVWriter(const std::string& fileName) : _opened(false), _fileName(fileName), _stream(),
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
            _stream.reset(new std::ofstream(_fileName));
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

        bool _opened;

        std::string _fileName;

        std::unique_ptr<std::ofstream> _stream;

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
#endif