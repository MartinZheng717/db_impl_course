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
./cc_occ
```

You should got the answer:
```shell
txn1:140342314673920  s:  1686014515  e:  1686014516  c:  1686014516  his_len: 1
txn2:140342306281216  s:  1686014516  e:  1686014517  c:  1686014517  his_len: 2
txn3:140342297888512  s:  1686014517  e:  1686014518  c:  1686014518  his_len: 3
txn1:140342217144064  s:  1686014518  e:  1686014519  c:  1686014519  his_len: 4
txn2:140342208751360  s:  1686014519  e:  1686014520  c:  1686014520  his_len: 5
txn3:140342200358656  s:  1686014520  e:  1686014521  c:  1686014521  his_len: 6
txn1:140342191965952  s:  1686014521  e:  1686014522  c:  1686014522  his_len: 7
txn2:140342183573248  s:  1686014522  e:  1686014523  c:  1686014523  his_len: 8
txn3:140342175180544  s:  1686014523  e:  1686014524  c:  1686014524  his_len: 9
Key: 0   Value: 88
Key: 1   Value: 808
Key: 2   Value: 108
Key: 3   Value: 118
Key: 4   Value: 110
Key: 5   Value: 135
```