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

  Node *pred = head;
  pred->mutex_.lock();
  Node *curr = pred->next_;

  while (curr != nullptr && curr->data_ < data) {
    curr->mutex_.lock();
    pred->mutex_.unlock();
    pred = curr;
    curr = curr->next_;
  }

  if (curr == nullptr || curr->data_ > data) {
    Node *new_node = new Node(data);
    new_node->next_ = curr;
    pred->next_ = new_node;
  }
  pred->mutex_.unlock();
  if (curr != nullptr) {
    curr->mutex_.unlock();
  }
}

void Add_nolock(int data, Node *head) {
  // TODO 不加锁的插入操作

  Node *new_node = new Node(data);
  new_node->next_ = head->next_;
  head->next_ = new_node;
}

void Contain(int data, Node *head) {
  // TODO 查找列表中是否含有该数据

  // TODO 查找列表中是否含有该数据

  Node *curr = head->next_;
  while (curr != nullptr) {
    curr->mutex_.lock();
    if (curr->data_ == data) {
      curr->mutex_.unlock();
      return;
    }
    Node *prev = curr;
    curr = curr->next_;
    prev->mutex_.unlock();
  }
}

void Remove(int data, Node *head) {
  // TODO 移除列表中特定数据

  Node *pred = head;
  pred->mutex_.lock();
  Node *curr = pred->next_;
  if (curr) curr->mutex_.lock();

  while (curr && curr->data_ < data) {
    pred->mutex_.unlock();
    pred = curr;
    curr = curr->next_;
    if (curr) curr->mutex_.lock();
  }

  if (curr && curr->data_ == data) {
    pred->next_ = curr->next_;
    curr->mutex_.unlock();
    delete curr;
  } else {
    if (curr) curr->mutex_.unlock();
  }

  pred->mutex_.unlock();
}

void Print(Node *head) {
  // TODO  遍历打印
  // 只需要锁头节点

  cout << "head: ";
  Node *curr = head->next_;
  while (curr != nullptr) {
    cout << curr->data_ << " ";
    curr = curr->next_;
  }
  cout << endl;
}

int main() {
  Node *head = new Node();
  // Test Case 1
  /*
  由于 Add_nolock 是不加锁的插入操作，多个线程在并发执行插入操作时没有进行同步，导致插入顺序不可预测。因此运行Test Case 1输出的链表内容是乱序的
  */
  cout << "Test case 1" <<endl;
  vector<thread> addthreads;
  for (int i = 0; i < 100; i++)
    addthreads.emplace_back(Add_nolock, i, head);
  for (auto &thread1 : addthreads)
    thread1.join();
  Print(head);


  // Test Case 2
  // cout << "Test case 2" <<endl;
  // vector<thread> addthreads;
  // for (int i = 0; i < 100; i++)
  // 	addthreads.emplace_back(Add, i, head);
  // for (auto &thread1 : addthreads)
  // 	thread1.join();
  // Print(head);

  // vector<thread> remthreads;
  // for (int i = 0; i < 100; i++)
  // 	remthreads.emplace_back(Remove, i, head);
  // for (auto &thread2 : remthreads)
  // 	thread2.join();
  // Print(head);

  // Test case 3
  // cout << "Test case 3" <<endl;
  // vector<thread> addthreads;
  // for (int i = 0; i < 100; i = i + 2)
  //     addthreads.emplace_back(Add, i, head);
  // for (auto &thread1 : addthreads)
  //     thread1.join();
  // Print(head);
  // vector<thread> addremthreads;
  // for (int i = 0; i < 100; i = i + 2) {
  //     addremthreads.emplace_back(Add, i+1, head);
  //     addremthreads.emplace_back(Remove, i, head);
  // }
  // for (auto &thread3 : addremthreads)
  //     thread3.join();
  // Print(head);


  // Test case 4
  // cout << "Test case 4" <<endl;
  // vector<thread> addthreads;
  // for (int i = 0; i < 100; i++)
  // 	addthreads.emplace_back(Add, i, head);
  // for (auto &thread1 : addthreads)
  // 	thread1.join();
  // Print(head);
  // vector<thread> conremthreads;
  // for (int i = 0; i < 97; i = i + 2) {
  // 	conremthreads.emplace_back(Contain, i+3, head);
  // 	conremthreads.emplace_back(Remove, i, head);
  // }
  // for (auto &thread4 : conremthreads)
  // 	thread4.join();
  // Print(head);


  delete head;
  return 0;
}
