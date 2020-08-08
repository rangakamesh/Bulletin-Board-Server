#include <stdio.h>
#include "tcp-utils.h"
#include <sys/syscall.h>
#include <stdio.h>
#include <iostream>
#include "tokenize.h"
#include "descriptor.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


char source_file[MAX_LEN];
num_gen gen_on_queue;
file_queue file_queue_a;

void add_trailing_spaces(char *dest, int size, int num_of_spaces)
{
    int len = strlen(dest);

    if( len + num_of_spaces >= size )
    {
        num_of_spaces = size - len - 1;
    }

    memset( dest+len, ' ', num_of_spaces );
    dest[len + num_of_spaces] = '\0';
}

int create_descriptor(char* filename)
{
  int fd = open(filename,O_RDWR | O_CREAT,S_IRWXU);
  close(fd);
  if(fd!=-1)
  {
    return 0;
  }
  else
  {
    return -1;
  }
}

int initiate_descriptor(char* filename)
{
  gen_on_queue.temp_list=new long int[MAX_LEN];
  gen_on_queue.count=0;

  pthread_mutex_init(&(file_queue_a.access_file_queue),NULL);

  int source = open(filename,O_RDONLY);

  if(source!=-1)
  {
    close(source);
    strcpy(source_file,filename);
    return 0;
  }
  else
  {
    close(source);

    int cre_ret = create_descriptor(filename);

    if(cre_ret==0)
    {
      strcpy(source_file,filename);
      return 0;
    }
    else
    {
      return -1;
    }
  }
}

int write_descriptor(char* msg_number,char* user,char* message,bool peer_work)
{

  char msg[MAX_LEN];

  pthread_mutex_lock(&file_queue_a.access_file_queue);

  if(file_queue_a.reads!=0)
  {
    snprintf(msg, MAX_LEN, "%s: Halting write until 0 reads.\n", __FILE__);
    logger(msg);
  }

  while (file_queue_a.reads != 0)
  {
    // release the mutex while waiting...
    pthread_cond_wait(&file_queue_a.can_write, &file_queue_a.access_file_queue);
  }

  if(server_config.DEBUG_DELAY&&!peer_work)
  {
    snprintf(msg, MAX_LEN, "%s: debug write delay 6 seconds begins\n", __FILE__);
    logger(msg);
    sleep(6);
    snprintf(msg, MAX_LEN, "%s: debug write delay 6 seconds ends\n", __FILE__);
    logger(msg);
  }

  // --------------->


  if(msg_number!=NULL&&user!=NULL&&message!=NULL)
  {
    int fd = open(source_file, O_RDONLY);
    if(fd<0)
    {
      pthread_cond_broadcast(&file_queue_a.can_write);
      pthread_mutex_unlock(&file_queue_a.access_file_queue);

      return -2;
    }
    close(fd);

    int destination = open(source_file, O_RDWR | O_APPEND);
    if(destination<0)
    {
      pthread_cond_broadcast(&file_queue_a.can_write);
      pthread_mutex_unlock(&file_queue_a.access_file_queue);

      return -2;
    }

    char cfp[3*MAX_LEN+4];
    sprintf(cfp,"%s/%s/%s\n",msg_number,user,message);
    write(destination,cfp,strlen(cfp));
    close(destination);

    pthread_cond_broadcast(&file_queue_a.can_write);
    pthread_mutex_unlock(&file_queue_a.access_file_queue);

    return 0;
  }
  else
  {
    pthread_cond_broadcast(&file_queue_a.can_write);
    pthread_mutex_unlock(&file_queue_a.access_file_queue);

    return -1;
  }

  // file_queue_a.msg_count++;
  pthread_cond_broadcast(&file_queue_a.can_write);
  pthread_mutex_unlock(&file_queue_a.access_file_queue);

  return 0;
}

int seek_msg_descriptor(char* msg_number,char* msg_len)
{
  int destination = open(source_file, O_RDWR | O_APPEND);
  char line[1024];

  while(readline(destination,line,BBLINE_LEN)!=recv_nodata)
  {
    char* tokens[MAX_LEN];
    int line_len;
    line_len=strlen(line);
    int num_toks = bbline_tokenize(line,tokens,strlen(line));
    if(strcmp(tokens[0],msg_number)==0)
    {
      sprintf(msg_len,"%d",line_len);
      int pos = lseek(destination, 0, SEEK_CUR);
      close(destination);
      return pos-line_len-1;
    }

  }

  close(destination);
  return 0;
}

int delete_descriptor(char* msg_number)
{
  int ret;
  char msg_len[1024];

  ret = seek_msg_descriptor(msg_number,msg_len);

  int lim = atoi(msg_len);

  int destination = open(source_file, O_RDWR,S_IRWXU);

  if(destination<0)
  {
    return -2;
  }

  if(destination!=-1)
  {
    int pos = lseek(destination, ret, SEEK_CUR);
    char cfp[3*MAX_LEN+4];
    cfp[0]='\0';

    for(int i=0;i<lim;i++)
    {
      strcat(cfp,"\b");
    }
    strcat(cfp,"\n");

    int wrr = write(destination,cfp,strlen(cfp));
    close(destination);
    return wrr;
  }
  else
  {
    return -1;
  }

}

int replace_descriptor(char* msg_number,char* user,char* message,bool peer_comms)
{
  int del_ret = delete_descriptor(msg_number);
  if(del_ret<0)
  {
    return -2;
  }

  del_ret = write_descriptor(msg_number,user,message,peer_comms);
  if(del_ret<0)
  {
    return -2;
  }

  return 0;
}

int read_descriptor(char* msg_number, char* line_ret,int check)
{
  char msg[MAX_LEN];

  pthread_mutex_lock(&file_queue_a.access_file_queue);
  file_queue_a.reads++;
  pthread_mutex_unlock(&file_queue_a.access_file_queue);

  if(server_config.DEBUG_DELAY&&check==0)
  {
    snprintf(msg, MAX_LEN, "%s: debug read delay 3 seconds begins\n", __FILE__);
    logger(msg);
    sleep(3);
    snprintf(msg, MAX_LEN, "%s: debug read delay 3 seconds ends\n", __FILE__);
    logger(msg);
  }


  int fd = open(source_file, O_RDONLY);

  if(fd<0)
  {
    pthread_mutex_lock(&file_queue_a.access_file_queue);
      file_queue_a.reads--;
      if (file_queue_a.reads == 0)
        pthread_cond_broadcast(&file_queue_a.can_write);
    pthread_mutex_unlock(&file_queue_a.access_file_queue);

    return -2;
  }

  char line[BBLINE_LEN];
  char message[MAX_LEN];
  message[0]='\0';
  int msg_len=0;

  while(readline(fd,line,BBLINE_LEN)!=recv_nodata)
  {
    char* tokens[MAX_LEN];
    int num_toks = bbline_tokenize(line,tokens,strlen(line));
    if(strcmp(tokens[0],msg_number)==0)
    {
      if(num_toks>3)
      {
        strcat(message,"");
        strcat(message,tokens[1]);
        strcat(message,"/");

        for(int j=2;j<num_toks;j++)
        {
          if(j>2)
          {
            strcat(message,"/");
          }
          strcat(message,tokens[j]);
        }
      }
      else if(num_toks==3)
      {
        strcat(message,"");
        strcat(message,tokens[1]);
        strcat(message,"/");
        strcat(message,tokens[2]);
      }
      else
      {
        std::cout<<"Error Encountered!"<<std::endl;
      }

      strcpy(line_ret,message);

      pthread_mutex_lock(&file_queue_a.access_file_queue);
        file_queue_a.reads--;
        if (file_queue_a.reads == 0)
          pthread_cond_broadcast(&file_queue_a.can_write);
      pthread_mutex_unlock(&file_queue_a.access_file_queue);

      close(fd);
      return strlen(message);
    }
  }

  close(fd);

  pthread_mutex_lock(&file_queue_a.access_file_queue);
    file_queue_a.reads--;
    if (file_queue_a.reads == 0)
      pthread_cond_broadcast(&file_queue_a.can_write);
  pthread_mutex_unlock(&file_queue_a.access_file_queue);

  return -1;
}

int read_tokenized_descriptor(char* msg_number, char* msg_user,char* msg_message)
{
  int fd = open(source_file, O_RDONLY);

  char line[BBLINE_LEN];

  int msg_len=0;

  while(readline(fd,line,BBLINE_LEN)!=recv_nodata)
  {
    char* tokens[MAX_LEN];
    int num_toks = bbline_tokenize(line,tokens,strlen(line));
    if(strcmp(tokens[0],msg_number)==0)
    {
      if(num_toks>3)
      {
        strcpy(msg_user,tokens[1]);
        strcpy(msg_message,"");

        for(int j=2;j<num_toks;j++)
        {
          if(j>2)
          {
            strcat(msg_message,"/");
          }
          strcat(msg_message,tokens[j]);
        }
      }
      else if(num_toks==3)
      {
        strcpy(msg_user,tokens[1]);
        strcpy(msg_message,tokens[2]);
      }
      else
      {
        std::cout<<"Error Encountered!"<<std::endl;
      }

      return strlen(msg_message)+strlen(msg_user);
    }
  }

  close(fd);
  return -1;
}

long int generate_number()
{
  long int max=0;
  int fd = open(source_file, O_RDONLY);
  if(fd<0)
  {
    return -2;
  }

  char line[BBLINE_LEN];

  while(readline(fd,line,BBLINE_LEN)!=recv_nodata)
  {
    char* tokens[MAX_LEN];
    int num_toks = bbline_tokenize(line,tokens,strlen(line));
    if(atoi(tokens[0])>max)
    {
      max=atoi(tokens[0]);
    }
  }

  bool max_changed = false;

  if(1)
 {
   for(int j=0;j<gen_on_queue.count;j++)
   {
     int jj=j;

     if(max==gen_on_queue.temp_list[j])
     {
       max+=1;
       max_changed=true;
       j=0;
     }

     if(jj==gen_on_queue.count-1&&j!=0)
     {
       break;
     }

   }

   gen_on_queue.temp_list[gen_on_queue.count]=max;
   gen_on_queue.count+=1;

 }

 if(!max_changed)
 {
   max+=1;
 }

 return max;

}

long int release_number(long int num)
{

  for(int j=0;j<gen_on_queue.count;j++)
  {
    if(num==gen_on_queue.temp_list[j])
    {
      gen_on_queue.temp_list[j]=0;
      return 0;
    }
  }

  return -1;
}

int housekeep_descriptor()
{

  int source = open(source_file, O_RDONLY);
  int destination = open("bbfile_cache",O_WRONLY | O_CREAT,S_IRWXU);

  char buff[BBLINE_LEN];
  int lineno = 0;
  char* com_tok[MAX_LEN];
  int rec_found=0;
  int orig_del=0;
  int cac_rnm=0;

  while(readline(source,buff,BBLINE_LEN)!=recv_nodata)
  {

    int line_length = strlen(buff);
    int n_c = 0;

    for(int i=0;i<line_length;i++)
    {
      if(buff[i]=='\b')
      {
        n_c++;
      }

    }

    if(n_c!=0&&n_c==line_length)
    {
      rec_found+=1;
    }
    else
    {
      char cfp[3*MAX_LEN+4];
      sprintf(cfp,"%s\n",buff);
      write(destination,cfp,strlen(cfp));
    }
    lineno+=1;
  }

  close(source);
  close(destination);
  char msg[MAX_LEN];
  snprintf(msg, MAX_LEN, "%d garbage lines have been housekept from bbfile.\n",rec_found);
  logger(msg);

  if(remove(source_file) == 0)
    orig_del=1;
  else
    orig_del=-1;

  if(rename("bbfile_cache", source_file) == 0)
	{
		cac_rnm=1;
	}
	else
	{
		cac_rnm=-1;
	}

  if(rec_found==1 && orig_del==1 && cac_rnm==1)
  {
    return 0;
  }
  else
  {
    return -1;
  }

}
