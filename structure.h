#pragma once
#include "thread_safe_list.h"
#include "global.h"
#include<queue>
#include<string>
#include<unordered_map>
#include<mutex>
#include<vector>
using namespace std;

/**/
struct LockEntry {
	lock_t type;
	std::thread::id tid;
};

struct Data {
	time_t timestamp;
	int value; // 数据项的值
	LockEntry owner; // 当前数据项上的锁属于谁
    thread_safe::list<LockEntry*> waitlist; // 线程安全的列表，记录目前数据项锁上的等待列表
	bool deleted;
	std::mutex latch; // 互斥量，用于线程对数据项加锁

	Data() {
		value = -1;
		owner.type = LOCK_NONE;
		deleted = false;
	}
	
	void set(Data* data1, Data* data2) {
		data1->deleted = data2->deleted;
		data1->owner = data2->owner;
		data1->timestamp = data2->timestamp;
		data1->value = data2->value;
		data1->waitlist = data2->waitlist;
		data1->waitlist = data2->waitlist;
	}

	Data& operator=(const Data& d) {
		set(this, (Data*)&d);
		return *this;
	}

	Data(const Data& d) {
		*this = d;
	}
};

class Engine {
public:
	Engine() {
		cc = CC_LOCK;
	}
	Engine(cc_type type) {
		cc = type;
	}
	RC find();
	RC update();
	RC delete_();

	std::unordered_map<std::string, Data> data_map;  //hash map，存储数据的结构
private:
	cc_type cc;
};