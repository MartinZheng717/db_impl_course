#pragma once
#include <iostream>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <ctime>
#include <vector>

using namespace std;

struct Data {
    int value;
    bool deleted;

    std::time_t commitstamp;
    std::time_t readstamp;
    std::time_t writestamp;

    Data() {
        value = -1;
        deleted = false;
        readstamp = -1;
        writestamp = -1;
        commitstamp = -1;
    }

    void set(Data* data1, Data* data2) {
        data1->deleted = data2->deleted;
        data1->readstamp = data2->readstamp;
        data1->writestamp = data2->writestamp;
        data1->commitstamp = data2->commitstamp;
        data1->value = data2->value;
    }
    Data& operator=(const Data& d) {
        set(this, (Data*)& d);
        return *this;
    }
};

class updateEntry {
public:
    Data * data;
    int value;
};

class txn_man {
public:
    txn_man() {
        start_ts = INT64_MAX;
        end_ts = INT64_MAX;
        commit_ts = INT64_MAX;
        tid = this_thread::get_id();

        rd_cnt = 0;
        wr_cnt = 0;
    }

    //begin time of the read phase
    std::time_t start_ts;

    //end time of the read phase
    std::time_t end_ts;

    std::time_t commit_ts;

    thread::id tid;

    //buffer the update data of the write operation
    vector<updateEntry*> wr_list;

    //record the read data
    vector<Data*> rd_list;

    int rd_cnt;
    int wr_cnt;
};

// 返回值，代表操作的结果类型，RCOK 正确，ERROR 错误，NOT_FOUND 数据项不存在， ALREADY_EXIST 数据项已存在
enum RC { RCOK, ABORT };

namespace thread_safe {

    template <class Key, class T>
    class unordered_map {
    public:
        typedef typename std::unordered_map<Key, T>::iterator iterator;
        typedef typename std::unordered_map<Key, T>::const_iterator const_iterator;
        typedef typename std::unordered_map<Key, T>::allocator_type allocator_type;
        typedef typename std::unordered_map<Key, T>::size_type size_type;
        typedef typename std::unordered_map<Key, T>::value_type value_type;

        // Constructors
        unordered_map() = default;
        template <class InputIterator> unordered_map(InputIterator first, InputIterator last) : storage(first, last) { }
        unordered_map(const thread_safe::unordered_map<Key, T>& x) : storage(x.storage) { }

        // Copy
        thread_safe::unordered_map<Key, T>& operator=(const thread_safe::unordered_map<Key, T>& x) { std::lock_guard<std::mutex> lock(mutex); std::lock_guard<std::mutex> lock2(x.mutex); storage = x.storage; return *this; }

        // Destructor
        ~unordered_map(void) { }

        // Iterators
        iterator begin(void) { std::lock_guard<std::mutex> lock(mutex); return storage.begin(); }
        const_iterator begin(void) const { std::lock_guard<std::mutex> lock(mutex); return storage.begin(); }

        iterator end(void) { std::lock_guard<std::mutex> lock(mutex); return storage.end(); }
        const_iterator end(void) const { std::lock_guard<std::mutex> lock(mutex); return storage.end(); }

        // Capacity
        size_type size(void) const { std::lock_guard<std::mutex> lock(mutex); return storage.size(); }

        size_type max_size(void) const { std::lock_guard<std::mutex> lock(mutex); return storage.max_size(); }

        bool empty(void) const { std::lock_guard<std::mutex> lock(mutex); return storage.empty(); }

        void reserve(size_type n) { std::lock_guard<std::mutex> lock(mutex); storage.reserve(n); }

        void rehash(size_type n) { std::lock_guard<std::mutex> lock(mutex); storage.rehash(n); }

        // Element Access
        T& operator[](const Key& x) { std::lock_guard<std::mutex> lock(mutex); return storage[x]; }
        T& at(const Key& x) { std::lock_guard<std::mutex> lock(mutex); return storage.at(x); };
        const T& at(const Key& x) const { std::lock_guard<std::mutex> lock(mutex); return storage.at(x); };

        // Modifiers
        std::pair<iterator, bool> insert(const value_type& x) { std::lock_guard<std::mutex> lock(mutex); return storage.insert(x); }
        iterator insert(iterator position, const value_type& x) { std::lock_guard<std::mutex> lock(mutex); return storage.insert(position, x); }
        template <class InputIterator> void insert(InputIterator first, InputIterator last) { std::lock_guard<std::mutex> lock(mutex); storage.insert(first, last); }

        void erase(iterator pos) { std::lock_guard<std::mutex> lock(mutex); storage.erase(pos); }
        size_type erase(const Key& x) { std::lock_guard<std::mutex> lock(mutex); return storage.erase(x); }
        void erase(iterator begin, iterator end) { std::lock_guard<std::mutex> lock(mutex); storage.erase(begin, end); }

        void swap(thread_safe::unordered_map<Key, T>& x) { std::lock_guard<std::mutex> lock(mutex); std::lock_guard<std::mutex> lock2(x.mutex); storage.swap(x.storage); }

        void clear(void) { std::lock_guard<std::mutex> lock(mutex); storage.clear(); }

        // Operations
        const_iterator find(const Key& x) const { std::lock_guard<std::mutex> lock(mutex); return storage.find(x); }
        iterator find(const Key& x) { std::lock_guard<std::mutex> lock(mutex); return storage.find(x); }

        size_type count(const Key& x) const { std::lock_guard<std::mutex> lock(mutex); return storage.count(x); }

        const_iterator lower_bound(const Key& x) const { std::lock_guard<std::mutex> lock(mutex); return storage.lower_bound(x); }
        iterator lower_bound(const Key& x) { std::lock_guard<std::mutex> lock(mutex); return storage.lower_bound(x); }

        const_iterator upper_bound(const Key& x) const { std::lock_guard<std::mutex> lock(mutex); return storage.upper_bound(x); }
        iterator upper_bound(const Key& x) { std::lock_guard<std::mutex> lock(mutex); return storage.upper_bound(x); }

        std::pair<const_iterator, const_iterator> equal_range(const Key& x) const { std::lock_guard<std::mutex> lock(mutex); return storage.equal_range(x); }
        std::pair<iterator, iterator> equal_range(const Key& x) { std::lock_guard<std::mutex> lock(mutex); return storage.equal_range(x); }

        // Allocator
        allocator_type get_allocator(void) const { std::lock_guard<std::mutex> lock(mutex); return storage.get_allocator(); }

    private:
        std::unordered_map<Key, T> storage;
        mutable std::mutex mutex;
    };

    template < class Key, class T >
    class unordered_multimap {
    public:
        typedef typename std::unordered_multimap<Key, T>::iterator iterator;
        typedef typename std::unordered_multimap<Key, T>::const_iterator const_iterator;
        typedef typename std::unordered_multimap<Key, T>::allocator_type allocator_type;
        typedef typename std::unordered_multimap<Key, T>::size_type size_type;
        typedef typename std::unordered_multimap<Key, T>::value_type value_type;

        // Constructors
        unordered_multimap() = default;
        template <class InputIterator> unordered_multimap(InputIterator first, InputIterator last) : storage(first, last) { }
        unordered_multimap(const thread_safe::unordered_multimap<Key, T>& x) : storage(x.storage) { }

        // Copy
        thread_safe::unordered_multimap<Key, T>& operator=(const thread_safe::unordered_multimap<Key, T>& x) { std::lock_guard<std::mutex> lock(mutex); std::lock_guard<std::mutex> lock2(x.mutex); storage = x.storage; return *this; }

        // Destructor
        ~unordered_multimap(void) { }

        // Iterators
        iterator begin(void) { std::lock_guard<std::mutex> lock(mutex); return storage.begin(); }
        const_iterator begin(void) const { std::lock_guard<std::mutex> lock(mutex); return storage.begin(); }

        iterator end(void) { std::lock_guard<std::mutex> lock(mutex); return storage.end(); }
        const_iterator end(void) const { std::lock_guard<std::mutex> lock(mutex); return storage.end(); }

        // Capacity
        size_type size(void) const { std::lock_guard<std::mutex> lock(mutex); return storage.size(); }

        size_type max_size(void) const { std::lock_guard<std::mutex> lock(mutex); return storage.max_size(); }

        bool empty(void) const { std::lock_guard<std::mutex> lock(mutex); return storage.empty(); }

        void reserve(size_type n) { std::lock_guard<std::mutex> lock(mutex); storage.reserve(n); }

        // Modifiers
        std::pair<iterator, bool> insert(const value_type& x) { std::lock_guard<std::mutex> lock(mutex); return storage.insert(x); }
        iterator insert(iterator position, const value_type& x) { std::lock_guard<std::mutex> lock(mutex); return storage.insert(position, x); }
        template <class InputIterator> void insert(InputIterator first, InputIterator last) { std::lock_guard<std::mutex> lock(mutex); storage.insert(first, last); }

        void erase(iterator pos) { std::lock_guard<std::mutex> lock(mutex); storage.erase(pos); }
        size_type erase(const Key& x) { std::lock_guard<std::mutex> lock(mutex); return storage.erase(x); }
        void erase(iterator begin, iterator end) { std::lock_guard<std::mutex> lock(mutex); storage.erase(begin, end); }

        void swap(thread_safe::unordered_multimap<Key, T>& x) { std::lock_guard<std::mutex> lock(mutex); std::lock_guard<std::mutex> lock2(x.mutex); storage.swap(x.storage); }

        void clear(void) { std::lock_guard<std::mutex> lock(mutex); storage.clear(); }

        // Operations
        const_iterator find(const Key& x) const { std::lock_guard<std::mutex> lock(mutex); return storage.find(x); }
        iterator find(const Key& x) { std::lock_guard<std::mutex> lock(mutex); return storage.find(x); }

        size_type count(const Key& x) const { std::lock_guard<std::mutex> lock(mutex); return storage.count(x); }

        const_iterator lower_bound(const Key& x) const { std::lock_guard<std::mutex> lock(mutex); return storage.lower_bound(x); }
        iterator lower_bound(const Key& x) { std::lock_guard<std::mutex> lock(mutex); return storage.lower_bound(x); }

        const_iterator upper_bound(const Key& x) const { std::lock_guard<std::mutex> lock(mutex); return storage.upper_bound(x); }
        iterator upper_bound(const Key& x) { std::lock_guard<std::mutex> lock(mutex); return storage.upper_bound(x); }

        std::pair<const_iterator, const_iterator> equal_range(const Key& x) const { std::lock_guard<std::mutex> lock(mutex); return storage.equal_range(x); }
        std::pair<iterator, iterator> equal_range(const Key& x) { std::lock_guard<std::mutex> lock(mutex); return storage.equal_range(x); }

        // Allocator
        allocator_type get_allocator(void) const { std::lock_guard<std::mutex> lock(mutex); return storage.get_allocator(); }

    private:
        std::unordered_multimap<Key, T> storage;
        mutable std::mutex mutex;
    };
}

class Engine {
public:
    Engine() {}

    thread_safe::unordered_map<std::string, Data> data_map;
};


