## Assignment for ECNU DaSE db_impl_course

***

Before this assignment, I've learned the theory of 2PL(2 Phase Lock) and OCC(Optimistic Concurrency Control) in class. So, in this assignment, I'd try to finish a simple demo of 2PL and OCC during multi-thread concurrency contorl transaction.

***
### To Run this demo:
Run command at the root of this assignment in terminal to compile the file:
```shell
cmake CMakeLists.txt
make
```
Also, run this to get debug version：
```shell
cmake -DDEBUG=ON CMakeLists.txt
make
```

To run and test the file：
```shell
./ccLock
```

You should got the answer:
```shell
Key: 0   Value: 4
Key: 1   Value: 88
Key: 2   Value: 15
Key: 3   Value: 28
Key: 4   Value: 17
Key: 5   has been deleted 
Key: 6   Value: 20
```