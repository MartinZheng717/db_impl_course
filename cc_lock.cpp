#include <iostream>
#include <string>

#include "cc_lock.h"
#include "global.h"
#include "structure.h"

RC cc_lock::lock_get(lock_t type, thread::id tid, Data &data){
    RC rc = RCOK;
    LockEntry lock1 = data.owner;
    LockEntry lock2;
    lock2.tid = tid;
    lock2.type = type;
    //1. 先判断数据项上的锁与自己加的锁是否冲突
    //2. a. 若不冲突，对数据项上的mutex加锁，并设置数据上已有锁信息（owner）为自己加的锁
    //   b. 若冲突，将自己要加的锁加到该数据的等待队列waitlist中，并对数据mutex加锁，然后被阻塞，等待数据上已有的锁被释放。
    //      阻塞结束后，设置该数据上已有锁信息(owner)为自己加的锁，将自己从waitlist中移除。

    if(conflict_lock(lock1,lock2)){
        data.waitlist.push_back(&lock2);
        data.latch.lock();
        data.owner=lock2;
        data.waitlist.pop_front();
    }else{
        // 先判断加锁是否是同一个事务（线程）
        if(lock1.tid==lock2.tid){
            // 为同一个线程加上更高级别的锁
            data.owner.type = lock1.type<lock2.type?lock1.type:lock2.type;
        }else{
            data.latch.lock();
            data.owner=lock2;
        }
    }

    return rc;

}

RC cc_lock::lock_release(lock_t type, thread::id tid, Data &data){
    RC rc = RCOK;
    //1. 判断数据项上有无阻塞的锁请求（检查waitlist）
    //2. a. 若没有，先释放mutex，再将数据上的owner置为无锁
    //   b. 释放mutex上的锁

    if(data.waitlist.size()==0){
        data.latch.unlock();
        data.owner.type = LOCK_NONE;
        data.owner.tid = tid;
    }
    else{
        LockEntry *firstEntry = data.waitlist.front();
        data.owner.tid = firstEntry->tid;
        data.owner.type = firstEntry->type;
        data.latch.unlock();
    }

    return rc;
}

//lock1是数据上的锁，lock2是需要加的锁
bool cc_lock::conflict_lock(LockEntry lock1, LockEntry lock2) {

    //1.首先判断是否属于同一事务的锁
    //2.如果两个线程都是读锁，或者没有锁，则不冲突；否则都会发生冲突

    if(lock1.tid!=lock2.tid){
        if(lock1.type==LOCK_SH&&lock2.type==LOCK_SH){
            return false;
        }else if(lock1.type==LOCK_NONE||lock2.type==LOCK_NONE){
            return false;
        }
        else return true;
    }

    return false;
}

RC cc_lock::insert(string key, int value, thread::id tid) {
    RC rc = RCOK;
    auto find = engine.data_map.find(key);
    if (find != engine.data_map.end() && !find->second.deleted) {
        rc = ALREADY_EXIST;
    } else {
        Data data;
        data.deleted = false;
        data.value = value;
        data.owner.type = LOCK_NONE;
        engine.data_map[key] = data;
    }
    return rc;
}

/*
 * 先加写锁，再删除
 */
RC cc_lock::delete_(string key, thread::id tid) {
    RC rc = RCOK;
    auto find = engine.data_map.find(key);
    if (find != engine.data_map.end() && !find->second.deleted) {
        rc = lock_get(LOCK_EX, tid, find->second);
        if (rc == RCOK) {
            find->second.deleted = true;
        } else
            return rc;
    } else {
        rc = NOT_FOUND;
    }
    return rc;
}

/*
 * 先加写锁，再修改value
 */
RC cc_lock::update(string key, int value, thread::id tid){
	RC rc = RCOK;
	auto find = engine.data_map.find(key);
	if (find != engine.data_map.end() && !find->second.deleted){
		rc = lock_get(LOCK_EX, tid, find->second);
		if (rc == RCOK) {
			find->second.value = value;
		} else
			return rc;
	} else {
		rc = NOT_FOUND;
	}
	return rc;
}

/*
 * 先加读锁，再获取value
 */
RC cc_lock::get(string key, int &value, thread::id tid){
	RC rc = RCOK;
	auto find = engine.data_map.find(key);
	if (find != engine.data_map.end() && !find->second.deleted){
		rc = lock_get(LOCK_SH, tid, find->second);
		value = find->second.value;
	} else {
		rc = NOT_FOUND;
	}
	return rc;
}
