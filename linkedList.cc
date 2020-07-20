#include "linkedList.h"
#include "bbserv_utils.h"

#include <iostream>
#include <cstdlib>

using namespace std;


struct lList* createList()
{

  lList* sampList;

  sampList = (struct lList*)malloc(sizeof(struct lList));//memory allocated to create a teamspace

  sampList->listLength=0;
  sampList->first=NULL;
  sampList->last=NULL;

  pthread_mutex_init(&(sampList->access_list),NULL);

  return sampList;
}

void lPush(lList* sampListX, struct val* new_val)
{
  pthread_mutex_lock(&sampListX->access_list);

    new_val->prev = NULL;

    switch(sampListX->listLength)
    {
      case 0:
        sampListX->first=new_val;
        sampListX->last= new_val;
        break;
      default:
        sampListX->last->prev=new_val;
        sampListX->last=new_val;

    }
    sampListX->listLength++;


  pthread_mutex_unlock(&sampListX->access_list);
}

void pushVal(lList* sampList, int new_val)
{
  val* val_a;
  val_a = (struct val*)malloc(sizeof(struct val));

  val_a->sck_val=new_val;

  lPush(sampList,val_a);
}

void pullSpec(lList* sampList, int pVal)
{
  pthread_mutex_lock(&sampList->access_list);

  val* temp_line;
  temp_line = sampList->first;
  int i=0;

  if(temp_line!=0)
  {
    for(i=0;i<sampList->listLength;i++)
    {
      if(temp_line->sck_val==pVal)
      {
        pullList(sampList,temp_line);
        break;
      }
      else
      {
        temp_line=temp_line->prev;
      }
    }
  }

  pthread_mutex_unlock(&sampList->access_list);

}


void pullList(lList* sampList,val* new_val)
{

  val* next_line;

  if(sampList->first->sck_val==new_val->sck_val)
  {
      if(sampList->listLength==1)
      {
        sampList->first=NULL;
        sampList->last=NULL;
        sampList->listLength-=1;
      }
      else
      {
        sampList->first=sampList->first->prev;
        sampList->listLength-=1;
      }
  }
  else if(sampList->last->sck_val==new_val->sck_val)
  {
    if(sampList->listLength==1)
    {
      sampList->last=NULL;
      sampList->first=NULL;
      sampList->listLength-=1;
    }
    else
    {
      next_line = find_next(sampList,sampList->last->sck_val);
      sampList->last=next_line;
      next_line->prev=NULL;
      sampList->listLength-=1;
    }
  }
  else
  {
    next_line = find_next(sampList,new_val->sck_val);
    next_line->prev=new_val->prev;
    sampList->listLength-=1;
  }


}

struct val* find_next(lList* sampList,int sck_val)
{

  val* temp_line;
  temp_line = sampList->first;
  int i=0;

  int ret_number=-1;

  if(temp_line!=0)
  {
    for(i=0;i<sampList->listLength;i++)
    {
      if(temp_line->sck_val==sck_val)
      {
        ret_number=i-1;
        break;
      }
      else
      {
        temp_line=temp_line->prev;
      }
    }
  }

  temp_line = sampList->first;

  if(temp_line!=0)
  {
    for(i=0;i<sampList->listLength;i++)
    {
      if(i==ret_number)
      {
        return temp_line;
        break;
      }
      else
      {
        temp_line=temp_line->prev;
      }
    }
  }

  return NULL;

}

void printList(lList* sampList)
{
  val* temp_line;
  temp_line = sampList->first;
  int i=0;

  int ret_number=-1;

  if(temp_line!=0)
  {
    for(i=0;i<sampList->listLength;i++)
    {
      cout<<"Socket "<<i<<" is : "<<temp_line->sck_val<<"."<<endl;
      temp_line=temp_line->prev;
    }
  }
}

void shutdownList(lList* sampList)
{
  val* temp_line;
  temp_line = sampList->first;
  int i=0;

  int ret_number=-1;

  if(temp_line!=0)
  {
    for(i=0;i<sampList->listLength;i++)
    {
      // cout<<"Element "<<i<<" is : "<<temp_line->sck_val<<"."<<endl;
      shutdown(temp_line->sck_val, SHUT_RD);
      temp_line=temp_line->prev;
    }
  }
}
