#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <pthread.h>

using namespace std;

struct Node {
    int data_;
    Node *next_;
    mutex mutex_;
    Node():next_(nullptr){};
    Node(int data):data_(data), next_(nullptr){};
};

void Add(int data, Node *head) {
    // TODO 加锁的插入操作
    // 空指针不能加锁 记得异常处理
    // 分三种情况判断：已经存在、已经位于队尾、正常插入（位于队中）

  head->mutex_.lock();
  if(head->next_!= nullptr){
    head->mutex_.unlock();
    Add(data,head->next_);
  } else{
    head->next_=new Node(data);
    head->mutex_.unlock();
  }

}

void Add_nolock(int data, Node *head) {
    // TODO 不加锁的插入操作

  head->next_=new Node(data);
  cout<<head->data_<<" next "<<data<<endl;

}

void Contain(int data, Node *head) {
    // TODO 查找列表中是否含有该数据

  Node* now=head;
  while (1){
    if(now->data_==data){
      head=now;
      break;
    }else {
      if(now->next_!= nullptr){
        now=now->next_;
      }else{
        head= nullptr;
        break;
      }
    }
  }

}

void Remove(int data, Node *head) {
    // TODO 移除列表中特定数据

  Node* now=head;
  while (1){
    now->mutex_.lock();
    if(now->next_== nullptr){
      now->mutex_.unlock();
      break;
    }
    if(now->next_->data_==data){
      Node *tmp=now->next_;
      tmp->mutex_.lock();
      now->next_=tmp->next_;
      tmp->mutex_.unlock();
      delete tmp;
      now->mutex_.unlock();
      break;
    } else{
      now->mutex_.unlock();
      now=now->next_;
    }
  }

}

void Print(Node *head) {
    // TODO  遍历打印
    // 只需要锁头节点

  Node *now=head;
  head->mutex_.lock();
  while(1){
    cout<<now->data_;
    if(now->next_== nullptr){
      break;
    }
    cout<<"->";
    now=now->next_;
  }
  cout<<endl;
  head->mutex_.unlock();
}

int main() {
    Node *head = new Node();
    // Test Case 1
//    vector<thread> addthreads;
//    for (int i = 0; i < 100; i++)
//        addthreads.emplace_back(Add_nolock, i, head);
//    for (auto &thread1 : addthreads)
//        thread1.join();
//    Print(head);
    // Test Case 2
//    vector<thread> addthreads;
//    for (int i = 0; i < 100; i++)
//        addthreads.emplace_back(Add, i, head);
//    for (auto &thread1 : addthreads)
//        thread1.join();
//    Print(head);
//
//    vector<thread> remthreads;
//    for (int i = 0; i < 100; i++)
//        remthreads.emplace_back(Remove, i, head);
//    for (auto &thread2 : remthreads)
//        thread2.join();
//    Print(head);

    // Test case 3
    vector<thread> addthreads;
    for (int i = 0; i < 100; i = i + 2)
        addthreads.emplace_back(Add, i, head);
    for (auto &thread1 : addthreads)
        thread1.join();
    Print(head);
    vector<thread> addremthreads;
    for (int i = 0; i < 100; i = i + 2) {
        addremthreads.emplace_back(Add, i+1, head);
        addremthreads.emplace_back(Remove, i, head);
    }
    for (auto &thread3 : addremthreads)
        thread3.join();
    Print(head);
    // Test case 4
//    vector<thread> addthreads;
//    for (int i = 0; i < 100; i++)
//        addthreads.emplace_back(Add, i, head);
//    for (auto &thread1 : addthreads)
//        thread1.join();
//    Print(head);
//    vector<thread> conremthreads;
//    for (int i = 0; i < 97; i = i + 2) {
//        conremthreads.emplace_back(Contain, i+3, head);
//        conremthreads.emplace_back(Remove, i, head);
//    }
//    for (auto &thread4 : conremthreads)
//        thread4.join();
//    Print(head);
    delete head;
    return 0;
}
