#ifndef __LINKEDLIST_H
#define __LINKEDLIST_H

#include <iostream>
#include <cstdlib>

using namespace std;

typedef struct val
{
  struct val* prev;
  int sck_val;
}val;

typedef struct lList
{
    pthread_mutex_t access_list;
    val *first;
    val *last;
    int listLength;
}lList;

struct lList* createList();
void lPush(lList* sampList, struct val* new_val);
void pushVal(lList* sampList, int new_val);
void pullSpec(lList* sampList, int pVal);
void pullList(lList* sampList,val* new_val);
struct val* find_next(lList* sampList,int sck_val);
void printList(lList* sampList);
void shutdownList(lList* sampList);

#endif /* __QUEUE_H */
