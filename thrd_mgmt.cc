#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include "shfd.h"
#include "thrd_mgmt.h"

char msg[MAX_LEN];
volatile int threads_keepalive=0;
volatile int threads_on_hold=0;
std::time_t last_kill=std::time(NULL);


/*-----------------------------Manager takes over-------------------------------------*/


struct team* create_team(int num_of_emps)
{
    threads_on_hold=0;
    threads_keepalive=1;

    if(num_of_emps<0)
    { //alright an empty team it is!
      num_of_emps=0;
    }

    // Let's create a team of threads
    team* team_a;
    team_a = (struct team*)malloc(sizeof(struct team));//memory allocated to create a teamspace
    if(team_a==NULL)
    {
        if (debugs[DEBUG_THREAD])
        {
            snprintf(msg, MAX_LEN, "%s: %s ERROR-> Unable to create thread team.\n", __FILE__, __FUNCTION__);
            logger(msg);
        } /* DEBUG_THREAD */

        //There is no space in the office to allocate a new team..(memory allocation problem)
        return NULL;//-1; //-1 no mamory for the team

    }

    //aight we now have a team, lets initialize it
    team_a->no_of_employees_available=0;
    team_a->no_of_employees_working=0;
    team_a->kill_all=false;


    //lets allocate a work_queue for our team
    if(create_work_queue(&team_a->work_queue_a)==-1)
    {
      //if we are here we did have enough space to create work_queue (no memory to create a work_queue)
      //we better release the memory we allocated for our team on the go
      if (debugs[DEBUG_THREAD])
      {
          snprintf(msg, MAX_LEN, "%s: %s ERROR-> Unable to create work queue for team.\n", __FILE__, __FUNCTION__);
          logger(msg);
      } /* DEBUG_THREAD */

      free(team_a);
      return NULL;//-2; not enough memory to create a work_queue for the team
    }

    //lets give a desk to each employee (allocate memory to hold thread information)
    team_a->employees=(struct employee**)malloc(num_of_emps * sizeof(struct employee*));
    if(team_a->employees==NULL)
    {
      //we were not able to hire people, so having a office space and work_queue is a loss
      if (debugs[DEBUG_THREAD])
      {
          snprintf(msg, MAX_LEN, "%s: %s ERROR-> Unable to spawn threads into the thread team.\n", __FILE__, __FUNCTION__);
          logger(msg);
      } /* DEBUG_THREAD */

      clear_work_queue(&team_a->work_queue_a);
      free(team_a);
      return NULL;//-3; not enough memory to hold thread information
    }

    //lets create a lock to secure the team information and activity control
    pthread_mutex_init(&(team_a->team_lock),NULL);
    pthread_cond_init(&(team_a->team_meet),NULL);

    //lets hire
    for(int i=0;i<num_of_emps;i++)
    {
      create_employee(team_a,&team_a->employees[i],i,num_of_emps);
    }

    while(team_a->no_of_employees_available != num_of_emps){}

    return team_a;

}

void terminate_team(team* team_a)
{
    if(team_a == NULL)
    {
      if (debugs[DEBUG_THREAD])
      {
          snprintf(msg, MAX_LEN, "%s: %s ERROR-> Team termination called on NULL.\n", __FILE__, __FUNCTION__);
          logger(msg);
      } /* DEBUG_THREAD */

      return;
    }

    volatile int total_employees = team_a->no_of_employees_available;
    threads_keepalive=0;

    /* Give one second to kill idle threads */
  	double TIMEOUT = 1.0;
  	time_t start, end;
  	double tpassed = 0.0;
  	time (&start);
  	while (tpassed < TIMEOUT && team_a->no_of_employees_available)
    {
  		bsem_post_all(team_a->work_queue_a.work_pending);
  		time (&end);
  		tpassed = difftime(end,start);
  	}

    while(team_a->no_of_employees_available)
    {
      bsem_post_all(team_a->work_queue_a.work_pending);
    }

    terminate_work_queue(&team_a->work_queue_a);

    //lets fire!
    for(int i=0;i<total_employees;i++)
    {
      terminate_employee(team_a->employees[i]);
    }

    free(team_a->employees);
    free(team_a);

}

int add_work_to_team(team* team_a, void (*functions_a)(void*),void* args_a)
{
  work* new_work;
  new_work=(struct work*)malloc(sizeof(struct work));
  if(new_work==NULL)
  {
    if (debugs[DEBUG_THREAD])
    {
        snprintf(msg, MAX_LEN, "%s: %s ERROR-> No space to accept work.\n", __FILE__, __FUNCTION__);
        logger(msg);
    } /* DEBUG_THREAD */

    return -1;// not able to create a work_a
  }

  new_work->function=functions_a;
  new_work->args=args_a;

  push_work(&team_a->work_queue_a,new_work); //x

  return 0;
}

/* Wait until all jobs have finished */
void team_wait(team* team_a)
{
	pthread_mutex_lock(&team_a->team_lock);
	while (team_a->work_queue_a.work_to_be_done || team_a->no_of_employees_working)
  {
		pthread_cond_wait(&team_a->team_meet, &team_a->team_lock);
	}
	pthread_mutex_unlock(&team_a->team_lock);
}

int is_working(team* team_a)
{
  return team_a->no_of_employees_working;
}

int is_available(team* team_a)
{
  return team_a->no_of_employees_available;
}

int wait_for_ready(team* team_a,int num_of_emps)
{
  while(team_a->no_of_employees_available != num_of_emps){}
  return 0;
}

/* -------------------------EMPOYEE functions ------------------------------ */

int create_employee(team* team_a, struct employee** employee_a, int id,int num_of_emps)
{
  *employee_a = (struct employee*)malloc(sizeof(struct employee));

  if(*employee_a==NULL)
  {
    if (debugs[DEBUG_THREAD])
    {
        snprintf(msg, MAX_LEN, "%s: %s ERROR-> No space to occupy a new thread.\n", __FILE__, __FUNCTION__);
        logger(msg);
    } /* DEBUG_THREAD */

    return -1; //-4 unable to create a threads
  }

  (*employee_a)->team_a = team_a;
  (*employee_a)->id = id;

  if(num_of_emps==1)
  {
    pthread_create(&(*employee_a)->pthread,NULL,(void* (*)(void*))activate_employee,*employee_a);
    pthread_detach((*employee_a)->pthread);
  }
  else
  {
    pthread_create(&(*employee_a)->pthread,NULL,(void* (*)(void*))activate_ambassador,*employee_a);
    pthread_detach((*employee_a)->pthread);
  }

  return 0;
}



void* activate_employee(struct employee* employee_a)
{

    team* team_a=employee_a->team_a;

    pthread_mutex_lock(&team_a->team_lock);
    team_a->no_of_employees_available+=1;
          pthread_mutex_lock(&counter_mutex);

              team_a->permission_to_quit=n_available;
              n_available+=1;
              if (debugs[DEBUG_THREAD])
              {
                  snprintf(msg, MAX_LEN, "%s: %s LOG-> File server thread %d activated.\n", __FILE__, __FUNCTION__,n_available);
                  logger(msg);
              } /* DEBUG_THREAD */

          pthread_mutex_unlock(&counter_mutex);
    pthread_mutex_unlock(&team_a->team_lock);
    int rt;

    while(threads_keepalive)
    {
      rt=0;
      rt=bsem_wait(team_a->work_queue_a.work_pending);

      if(rt!=0)
      {
            pthread_mutex_lock(&counter_mutex);
            int diff = last_kill - std::time(NULL);

            if(diff<-10)
            {
              manage_file_server(team_a->permission_to_quit);
            }

            if(!team_a->kill_all)
            {
                pthread_mutex_unlock(&counter_mutex);
                continue;
            }
            else
            {
                if (debugs[DEBUG_THREAD])
                {
                    snprintf(msg, MAX_LEN, "%s: %s LOG-> File server thread %d put to sleep. TIMEOUT.\n", __FILE__, __FUNCTION__,team_a->permission_to_quit);
                    logger(msg);
                } /* DEBUG_THREAD */

                last_kill=std::time(NULL);

                pthread_mutex_lock(&team_a->team_lock);
                team_a->no_of_employees_available--;
                pthread_mutex_unlock(&team_a->team_lock);

                    n_available-=1;
                pthread_mutex_unlock(&counter_mutex);

                return NULL;
            }
            pthread_mutex_unlock(&counter_mutex);//This will never run.
        }

      if(threads_keepalive)
      {

        pthread_mutex_lock(&team_a->team_lock);
        team_a->no_of_employees_working++;
        pthread_mutex_unlock(&team_a->team_lock);

        pthread_mutex_lock(&counter_mutex);
            n_working+=1;
        pthread_mutex_unlock(&counter_mutex);


        void(*thing_to_be_done)(void *);
        void* args;
        work* work_a=pull_work(&team_a->work_queue_a);
        if(work_a)
        {

          thing_to_be_done=work_a->function;
          args=work_a->args;
          thing_to_be_done(args);
          free(work_a);

        }

  			pthread_mutex_lock(&team_a->team_lock);
  			team_a->no_of_employees_working--;
        pthread_mutex_unlock(&team_a->team_lock);

        pthread_mutex_lock(&counter_mutex);
        n_working-=1;
        pthread_mutex_unlock(&counter_mutex);

      }

    }

    pthread_mutex_lock(&team_a->team_lock);
    team_a->no_of_employees_available-=1;
    pthread_mutex_unlock(&team_a->team_lock);

    pthread_mutex_lock(&counter_mutex);
    n_available-=1;
    pthread_mutex_unlock(&counter_mutex);

    if (debugs[DEBUG_THREAD])
    {
        snprintf(msg, MAX_LEN, "%s: %s LOG-> File server thread %d terminated.\n", __FILE__, __FUNCTION__,team_a->permission_to_quit);
        logger(msg);
    } /* DEBUG_THREAD */

    return NULL;
}

void* activate_ambassador(struct employee* employee_a)
{

    team* team_a=employee_a->team_a;
    pthread_mutex_lock(&team_a->team_lock);

        team_a->no_of_employees_available+=1;

        if (debugs[DEBUG_THREAD])
        {
            snprintf(msg, MAX_LEN, "%s: %s LOG-> A sync communicator thread is actived.\n", __FILE__, __FUNCTION__);
            logger(msg);
        } /* DEBUG_THREAD */


    pthread_mutex_unlock(&team_a->team_lock);

    while(threads_keepalive)
    {
      bsem_wait_ambassador(team_a->work_queue_a.work_pending);

      if(threads_keepalive)
      {

        pthread_mutex_lock(&team_a->team_lock);
        team_a->no_of_employees_working++;
        pthread_mutex_unlock(&team_a->team_lock);

        void(*thing_to_be_done)(void *);
        void* args;
        work* work_a=pull_work(&team_a->work_queue_a);
        if(work_a)
        {

          thing_to_be_done=work_a->function;
          args=work_a->args;

          thing_to_be_done(args);
          free(work_a);

        }

  			pthread_mutex_lock(&team_a->team_lock);

    			team_a->no_of_employees_working--;

        pthread_mutex_unlock(&team_a->team_lock);


      }

    }

    pthread_mutex_lock(&team_a->team_lock);
    team_a->no_of_employees_available-=1;
    pthread_mutex_unlock(&team_a->team_lock);

    if (debugs[DEBUG_THREAD])
    {
        snprintf(msg, MAX_LEN, "%s: %s LOG-> A sync communicator thread terminated.\n", __FILE__, __FUNCTION__);
        logger(msg);
    } /* DEBUG_THREAD */

    return NULL;
}

  void terminate_employee(employee* employee_a)
{
  free(employee_a);
}

/*************************************work_queue*******************************/

int create_work_queue(work_queue* work_queue_a)
{
  work_queue_a->work_to_be_done=0;
  work_queue_a->first=NULL;
  work_queue_a->last=NULL;

  work_queue_a->work_pending=(struct sem*)malloc(sizeof(struct sem));
  if(work_queue_a->work_pending==NULL)
  {
    return -1;//-5 unable to allocate space for a work_queue
  }

  pthread_mutex_init(&(work_queue_a->access_work_queue),NULL);
  create_sem(work_queue_a->work_pending, 0);

  return 0;
}

void clear_work_queue(work_queue* work_queue_a)
{

  while(work_queue_a->work_to_be_done>0)
  {
    free(pull_work(work_queue_a));
  }

  work_queue_a->first = NULL;
  work_queue_a->last = NULL;
  reset_sem(work_queue_a->work_pending);
  work_queue_a->work_to_be_done=0;
}

void push_work(work_queue* work_queue_a, struct work* new_work)
{
  pthread_mutex_lock(&work_queue_a->access_work_queue);

  new_work->prev = NULL;

  switch(work_queue_a->work_to_be_done)
  {
    case 0:
      work_queue_a->first=new_work;
      work_queue_a->last= new_work;
      break;
    default:
      work_queue_a->last->prev=new_work;
      work_queue_a->last=new_work;

  }
  work_queue_a->work_to_be_done++;

  sem_post(work_queue_a->work_pending);
  pthread_mutex_unlock(&work_queue_a->access_work_queue);
}

struct work* pull_work(work_queue* work_queue_a)
{
  pthread_mutex_lock(&work_queue_a->access_work_queue);

  work* work_a = work_queue_a->first;

  switch (work_queue_a->work_to_be_done)
  {
    case 0:
      break;
    case 1:
      work_queue_a->first = NULL;
      work_queue_a->last = NULL;
      work_queue_a->work_to_be_done=0;
      break;
    default:
      work_queue_a->first=work_a->prev;
      work_queue_a->work_to_be_done--;
      sem_post(work_queue_a->work_pending);
  }

  pthread_mutex_unlock(&work_queue_a->access_work_queue);

  return work_a;
}

void terminate_work_queue(work_queue* work_queue_a)
{
  clear_work_queue(work_queue_a);
  free(work_queue_a->work_pending);
}

/******************************* semaphore operations *************************/

void create_sem(sem* sem_a, int val)
{
  if(val<0 || val>1)
  {
    if (debugs[DEBUG_THREAD])
    {
        snprintf(msg, MAX_LEN, "%s: %s ERROR-> Semaphore initiation error.\n", __FILE__, __FUNCTION__);
        logger(msg);
    } /* DEBUG_THREAD */

    return;//-91 semaphore initiation error
  }

  pthread_mutex_init(&(sem_a->mutex),NULL);
  pthread_cond_init(&(sem_a->cond),NULL);
  sem_a->pol=val;
}

void reset_sem(sem* sem_a)
{
  create_sem(sem_a,0);
}

void sem_post(sem* sem_a)
{
  pthread_mutex_lock(&sem_a->mutex);
  sem_a->pol=1;
  pthread_cond_signal(&sem_a->cond);
  pthread_mutex_unlock(&sem_a->mutex);
}

void bsem_post_all(sem* sem_a)
{
  pthread_mutex_lock(&sem_a->mutex);
    sem_a->pol=1;
    pthread_cond_broadcast(&sem_a->cond);
  pthread_mutex_unlock(&sem_a->mutex);
}

int bsem_wait(sem* sem_a)
{
  struct timespec ts;
  struct timeval now;

  gettimeofday(&now,NULL);
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 60;
  ts.tv_nsec = 1;

  int rt=0;

  pthread_mutex_lock(&sem_a->mutex);

  if(sem_a->pol!=1)
  {
    rt = pthread_cond_timedwait(&sem_a->cond,&sem_a->mutex,&ts);
  }

  sem_a->pol=0;

  pthread_mutex_unlock(&sem_a->mutex);

  return rt;
}

void bsem_wait_ambassador(sem* sem_a)
{

  pthread_mutex_lock(&sem_a->mutex);

  while(sem_a->pol!=1)
  {
    pthread_cond_wait(&sem_a->cond,&sem_a->mutex);
  }

  sem_a->pol=0;

  pthread_mutex_unlock(&sem_a->mutex);

}
