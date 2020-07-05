#ifndef __THRD_MGMT_
#define __THRD_MGMT_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include <sys/types.h>
#include <sys/syscall.h>

//A simple thread pool management library.
//Created as part of the CS564 course.

//A "Team" refers to a collection of threads, where all the data about the team is available on it "team" struct.
//A "Employee" refers to a child thread which is managed under a "Team".

//A "Work Queue" is a queue where "function pointers" and "arguments" can be fetched to it.
// The "Employee"s contantly watches the work queue and picks up any work if available.

// "sem" functions are used to manage the waits and activation of employees.

/*-------------------------------DECLARATIONS---------------------------------*/
typedef struct sem
{
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int pol;
}sem;

typedef struct work
{
  struct work* prev;
  void (*function)(void* args);
  void* args;
}work;

typedef struct work_queue
{
    pthread_mutex_t access_work_queue;
    work *first;
    work *last;
    sem *work_pending;
    int work_to_be_done;
}work_queue;

typedef struct employee
{
    int id;
    pthread_t pthread;
    struct team* team_a;
}employee;

typedef struct team
{
  pthread_mutex_t team_lock;
  pthread_cond_t team_meet;
  work_queue work_queue_a;
  volatile int no_of_employees_available;
  volatile int no_of_employees_working;
  volatile bool kill_all;
  volatile int permission_to_quit;
  employee** employees;
  char employee_type[50];
}team;

extern int n_working;
extern int n_available;

extern pthread_mutex_t counter_mutex;

extern volatile int threads_keepalive;
extern volatile int threads_on_hold;


struct team* create_team(int num_of_emps,char type[50]);
int add_work_to_team(team* team_a, void(*functions_a)(void*),void* args_a);
int hire_new_employees(team* team_a, int num_of_emps);
void team_wait(team* team_a);
int is_working(team* team_a);
int is_available(team* team_a);
int wait_for_ready(team* team_a,int num_of_emps);
void terminate_team(team* team_a);

int create_employee(team* team_a, struct employee** employee_a, int id);
void* activate_employee(struct employee* employee_a);
void terminate_employee(employee* employee_a);

int create_work_queue(work_queue* work_queue_a);
void clear_work_queue(work_queue* work_queue_a);
void push_work(work_queue* work_queue_a, struct work* new_work);
struct work* pull_work(work_queue* work_queue_a);
void terminate_work_queue(work_queue* work_queue_a);

void create_sem(sem* sem_a, int val);
void reset_sem(sem* sem_a);
void sem_post(sem* sem_a);
void bsem_post_all(sem* sem_a);
int bsem_wait(sem* sem_a);


#endif
