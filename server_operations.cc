/*
 * In-place string tokenizer, by Stefan Bruda.  Read the comments in
 * the header before using it.
 */

#include "server_operations.h"
#include "peer_operations.h"

void* server_operations(client_t* clnt)
{

  char msg[2*MAX_LEN];  // logger string
  char request[MAX_LEN];  // logger string
  char response[2*BBLINE_LEN];  // logger string
  bool quit_flag=false;

  client_info client;
  client.id = clnt->sd;
  strcpy(client.ip,clnt->ip);
  pushVal(clients_list,client.id);

  long int ssock = client.id;

  snprintf(msg, 2*MAX_LEN, "Client %ld thread allocation successfull.",ssock);
  logger(msg);

  snprintf(response, 2*MAX_LEN, "\n0.0 Welcome to Bullettin Board Server %s\n1.USER username\n2.READ msg_number\n3.WRITE text\n4.REPLACE msg_num/message\n",client.ip);
  send(ssock, response, strlen(response),0);

  // snprintf(response, 2*MAX_LEN, "\n--bb> ");
  // send(ssock, response, strlen(response),0);

  client.uname_avl=false;
  client.user=new char[MAX_LEN];
  strcpy(client.user,"nobody");


  while(readline(ssock,request,MAX_LEN)!=recv_nodata)
  {

        request[strlen(request)-1]='\0';

        snprintf(msg, 2*MAX_LEN, "Client %ld, Request : %s",ssock,request);
        logger(msg);

        char *command[MAX_LEN];
        int t = str_tokenize(request, command, strlen(request));
        char* client_command;

        if(command[0]!=NULL)
        {
            client_command=tolwr(command[0]);
        }
        else
        {
          break;
        }

        if(client_command!=NULL&&strcmp(client_command,"quit")!=0)
        {
          if(strcmp(tolwr(client_command),"user")==0)
          {
            if(t>1)
            {
                if(client.uname_avl==false)
                {
                  client.uname_avl=true;
                  strcpy(client.user,"");

                  for(int i=1;i<t;i++)
                  {
                    sprintf(client.user,"%s %s",client.user,command[i]);
                  }

                  if(strstr(client.user,"/")==NULL)
                  {
                    snprintf(response, 2*MAX_LEN, "1.0 HELLO %s welcome to the Bullettin Board.\n\n",client.user);
                    send(ssock, response, strlen(response),0);

                    // snprintf(response, MAX_LEN, "--bb> ");
                    // send(ssock, response, strlen(response),0);
                  }
                  else
                  {
                    snprintf(response, 2*MAX_LEN, "ERR : The name parameter of USER should not contain the character / in it.\nEx  : USER Michael\n\n");
                    send(ssock, response, strlen(response),0);

                    // snprintf(response, MAX_LEN, "--bb> ");
                    // send(ssock, response, strlen(response),0);
                  }
                }
                else
                {
                  strcpy(client.user,"");

                  for(int i=1;i<t;i++)
                  {
                    sprintf(client.user,"%s %s",client.user,command[i]);
                  }

                  snprintf(response, 2*MAX_LEN, "1.0 HELLO %s welcome to the Bullettin Board.\n\n",client.user);
                  send(ssock, response, strlen(response),0);

                  // snprintf(response, MAX_LEN, "--bb> ");
                  // send(ssock, response, strlen(response),0);
                }
            }
            else
            {
              snprintf(response, 2*MAX_LEN, "ERR : The USER command must accompany a name with it.\nEx  : USER Michael\n\n");
              send(ssock, response, strlen(response),0);

              // snprintf(response, MAX_LEN, "--bb> ");
              // send(ssock, response, strlen(response),0);
            }
          }
          else if(strcmp(tolwr(client_command),"read")==0)
          {
                if(t==2)
                {
                  // cout<<"\n No of Msgs : "<<no_of_msgs(bbfile)<<endl;
                  char temp_line[BBLINE_LEN];
                  int read_return = read_descriptor(command[1], temp_line,0);

                    if(read_return>=0)
                    {
                      snprintf(response, 2*BBLINE_LEN, "2.0 MESSAGE %s %s\n\n",command[1],temp_line);
                      send(ssock, response, strlen(response),0);

                      // snprintf(response, MAX_LEN, "--bb> ");
                      // send(ssock, response, strlen(response),0);
                    }
                    else if(read_return==-2)
                    {
                      snprintf(response, 2*MAX_LEN, "2.2 ERROR READ : Bullettin Board Unavailable\n\n");
                      send(ssock, response, strlen(response),0);

                      // snprintf(response, MAX_LEN, "--bb> ");
                      // send(ssock, response, strlen(response),0);
                    }
                    else
                    {
                      snprintf(response, 2*MAX_LEN, "2.1 UNKNOWN %s message not found.\n\n",command[1]);
                      send(ssock, response, strlen(response),0);

                      // snprintf(response, MAX_LEN, "--bb> ");
                      // send(ssock, response, strlen(response),0);

                    }
                }
                else
                {
                  snprintf(response, 2*MAX_LEN, "2.2 ERROR READ : Improper READ format.\nFORMAT -> READ message-number\nEx  : READ 6\n\n");
                  send(ssock, response, strlen(response),0);

                  // snprintf(response, MAX_LEN, "--bb> ");
                  // send(ssock, response, strlen(response),0);
                }
          }
          else if(strcmp(tolwr(client_command),"write")==0)
          {
            if(t>=2)
            {
              char* message=new char[MAX_LEN];

              if(t>2)
              {
                for(int i=1;i<t;i++)
                {
                  sprintf(message,"%s %s",message,command[i]);
                }
              }
              else
              {
                strcpy(message,command[1]);
              }


              long int gen_num = generate_number();
              char* msg_nbr=new char[MAX_LEN];
              sprintf(msg_nbr,"%ld",gen_num);

              line* syncer_line = new_line(msg_nbr,client.user,message);

              int sync_ret = sync_write(syncer_line);

              if(sync_ret==0)
              {
                    snprintf(response, 2*MAX_LEN, "3.0 WROTE %s\n\n",msg_nbr); //emark
                    send(ssock, response, strlen(response),0);

                    // snprintf(response, MAX_LEN, "--bb> ");
                    // send(ssock, response, strlen(response),0);
              }
              else
              {
                    release_number(gen_num);

                    snprintf(response, 2*MAX_LEN, "3.2 ERROR WRITE : Synchronization error.\n\n");
                    send(ssock, response, strlen(response),0);
                    //
                    // snprintf(response, MAX_LEN, "--bb> ");
                    // send(ssock, response, strlen(response),0);
              }

            }
            else
            {
              snprintf(response, 2*MAX_LEN, "3.2 ERROR WRITE : Improper WRITE format.\nFORMAT -> WRITE message\nEx  : WRITE jack and jill went up the hill.\n\n");
              send(ssock, response, strlen(response),0);
              //
              // snprintf(response, MAX_LEN, "--bb> ");
              // send(ssock, response, strlen(response),0);
            }
          }
          else if(strcmp(tolwr(client_command),"replace")==0)
          {
            if(t>1)
            {
              char repl_request[MAX_LEN];
              repl_request[0]='\0';
              for(int x=1;x<t;x++)
              {
                strcat(repl_request,command[x]);
                strcat(repl_request," ");
              }

              char *repl_tokens[MAX_LEN];
              int replts = bbline_tokenize(repl_request, repl_tokens, strlen(command[1]));
              char message[MAX_LEN];
              message[0]='\0';

              if(replts>2)
              {
                for(int i=1;i<replts;i++)
                {
                  strcat(message,repl_tokens[i]);
                  if(i!=replts-1)
                    strcat(message,"/");
                }
              }
              else
              {
                if(replts==2)
                {
                  strcpy(message,repl_tokens[1]);
                }
                else
                {
                  snprintf(response, 2*MAX_LEN, "3.2 ERROR WRITE : Improper REPLACE format.\nFORMAT -> REPLACE message-number/message\nEx  : REPLACE 6/jack and jill went up the hill.\n\n");
                  send(ssock, response, strlen(response),0);

                  // snprintf(response, MAX_LEN, "--bb> ");
                  // send(ssock, response, strlen(response),0);

                  continue;
                }
              }

              char temp_line[BBLINE_LEN];
              int read_return = read_descriptor(repl_tokens[0], temp_line,1);

              if(read_return==-1)
              {
                snprintf(response, 2*MAX_LEN, "3.1 UNKNOWN %s\n\n",repl_tokens[0]);
                send(ssock, response, strlen(response),0);

                // snprintf(response, MAX_LEN, "--bb> ");
                // send(ssock, response, strlen(response),0);
              }
              else
              {

                  line* syncer_line = new_line(repl_tokens[0],client.user,message);
                  int sync_ret = sync_replace(syncer_line);

                  if(sync_ret==0)
                  {
                        snprintf(response, 2*MAX_LEN, "3.0 WROTE %s\n\n",repl_tokens[0]);
                        send(ssock, response, strlen(response),0);

                        // snprintf(response, MAX_LEN, "--bb> ");
                        // send(ssock, response, strlen(response),0);
                  }
                  else
                  {
                        snprintf(response, 2*MAX_LEN, "3.2 ERROR WRITE : Synchronization error.\n\n");
                        send(ssock, response, strlen(response),0);

                        // snprintf(response, MAX_LEN, "--bb> ");
                        // send(ssock, response, strlen(response),0);
                  }

              }


            }
            else
            {
              snprintf(response, 2*MAX_LEN, "3.2 ERROR WRITE : Improper REPLACE format.\nFORMAT -> REPLACE message-number/message\nEx  : REPLACE 6/jack and jill went up the hill.\n\n");
              send(ssock, response, strlen(response),0);

              // snprintf(response, MAX_LEN, "--bb> ");
              // send(ssock, response, strlen(response),0);
            }

          }

          else
          {
            snprintf(response, 2*MAX_LEN, "ERR : Invalid Command %s. \n\n",request);
            send(ssock, response, strlen(response),0);

            // snprintf(response, MAX_LEN, "--bb> ");
            // send(ssock, response, strlen(response),0);
          }
        }
        else
        {
          if(strcmp(client_command,"quit")==0)
          {
            snprintf(response, 2*MAX_LEN, "4.0 BYE . Hope to see you later.\n\n");
            send(ssock, response, strlen(response),0);
            quit_flag=true;
            break;
          }
          else
          {
            break;
          }

        }
  }

  snprintf(msg, 2*MAX_LEN, "Client : %ld , Connection terminated.",ssock);
  logger(msg);

  pullSpec(clients_list,client.id);

  if(!quit_flag) //using a separate 'send' just for a clean output at client side
  {
    snprintf(response, 2*MAX_LEN, "\n4.0 BYE . Hope to see you later.\n\n");
    send(ssock, response, strlen(response),0);
  }

  shutdown(ssock, SHUT_RDWR);
  close(ssock);

  return 0;   // will never reach this anyway...

}


int sync_write(line* line_a)
{
  char msg[MAX_LEN];

  peer_stat* comms_info = new peer_stat;

  comms_info->pos_count=0;
  comms_info->neg_count=0;
  comms_info->peer_addr=new peer_stat_address[server_config.NO_OF_PEERS];
  comms_info->line_a=line_a;

  for(int i=0;i<server_config.NO_OF_PEERS;i++)
  {
    comms_info->peer_addr[i].port=peer_info[i].port;
    strcpy(comms_info->peer_addr[i].ip,peer_info[i].ip);
    comms_info->peer_addr[i].marked=false;
  }

  pthread_mutex_init(&(comms_info->access_peer_stat),NULL);

  for(int i=0;i<server_config.NO_OF_PEERS;i++)
  {

      // cout<<"Pre-Commit Initialize : "<<i<<endl;
      if ( add_work_to_team(peer_sender_team,(void (*) (void*))send_precommit,(void*)comms_info) != 0 )
      {
          terminate_team(peer_sender_team);
          return -1;
      }
  }

  pthread_mutex_lock(&comms_info->access_peer_stat);

  if(comms_info->pos_count!=server_config.NO_OF_PEERS||comms_info->neg_count!=server_config.NO_OF_PEERS||(comms_info->pos_count+comms_info->neg_count)!=server_config.NO_OF_PEERS)
  {
    pthread_cond_wait(&comms_info->peer_stat_cond,&comms_info->access_peer_stat);
  }

  pthread_mutex_unlock(&comms_info->access_peer_stat);

  snprintf(msg, 2*MAX_LEN, "Positive Precommits : %d , Negative Precommits : %d",comms_info->pos_count,comms_info->neg_count);
  logger(msg);

  if(comms_info->pos_count==server_config.NO_OF_PEERS)
  {
    comms_info->writer=true;
    comms_info->replacer=false;

    comms_info->pos_count=0;
    comms_info->neg_count=0;

    for(int i=0;i<server_config.NO_OF_PEERS;i++)
    {
      comms_info->peer_addr[i].marked=false;
    }

    for(int i=0;i<server_config.NO_OF_PEERS;i++)
    {
        if ( add_work_to_team(peer_sender_team,(void (*) (void*))send_commit,(void*)comms_info) != 0 )
        {
            terminate_team(peer_sender_team);
            return -1;
        }
    }
  }
  else
  {
    return -1;
  }

  pthread_mutex_lock(&comms_info->access_peer_stat);

  if(comms_info->pos_count!=server_config.NO_OF_PEERS||comms_info->neg_count!=server_config.NO_OF_PEERS||(comms_info->pos_count+comms_info->neg_count)!=server_config.NO_OF_PEERS)
  {
    pthread_cond_wait(&comms_info->peer_stat_cond,&comms_info->access_peer_stat);
  }

  pthread_mutex_unlock(&comms_info->access_peer_stat);

  snprintf(msg, 2*MAX_LEN, "Positive Commits : %d, Negative Commits : %d",comms_info->pos_count,comms_info->neg_count);
  logger(msg);


  if(comms_info->pos_count==server_config.NO_OF_PEERS)
  {
    int write_return=write_descriptor(line_a->msg_nmbr,line_a->poster,line_a->message,false);

    if(write_return<0)
    {
      comms_info->pos_count=0;
      comms_info->neg_count=0;

      for(int i=0;i<server_config.NO_OF_PEERS;i++)
      {
        comms_info->peer_addr[i].marked=false;
      }

      for(int i=0;i<server_config.NO_OF_PEERS;i++)
      {
          if ( add_work_to_team(peer_sender_team,(void (*) (void*))send_del,(void*)comms_info) != 0 )
          {
              terminate_team(peer_sender_team);
              return -1;
          }
      }

      return -1;

    }

    char read_fetch[MAX_LEN];
    int read_return = read_descriptor(line_a->msg_nmbr,read_fetch,1);

    if(read_return<0)
    {
      comms_info->pos_count=0;
      comms_info->neg_count=0;

      for(int i=0;i<server_config.NO_OF_PEERS;i++)
      {
        comms_info->peer_addr[i].marked=false;
      }

      for(int i=0;i<server_config.NO_OF_PEERS;i++)
      {
          if ( add_work_to_team(peer_sender_team,(void (*) (void*))send_del,(void*)comms_info) != 0 )
          {
              terminate_team(peer_sender_team);
              return -1;
          }
      }

      return -1;
    }
    else
    {
      comms_info->pos_count=0;
      comms_info->neg_count=0;

      for(int i=0;i<server_config.NO_OF_PEERS;i++)
      {
        comms_info->peer_addr[i].marked=false;
      }

      for(int i=0;i<server_config.NO_OF_PEERS;i++)
      {
          if ( add_work_to_team(peer_sender_team,(void (*) (void*))send_quit,(void*)comms_info) != 0 )
          {
              terminate_team(peer_sender_team);
              return -1;
          }
      }

      return 0;
    }
  }
  else
  {
    comms_info->pos_count=0;
    comms_info->neg_count=0;

    for(int i=0;i<server_config.NO_OF_PEERS;i++)
    {
      comms_info->peer_addr[i].marked=false;
    }

    for(int i=0;i<server_config.NO_OF_PEERS;i++)
    {
        if ( add_work_to_team(peer_sender_team,(void (*) (void*))send_del,(void*)comms_info) != 0 )
        {
            terminate_team(peer_sender_team);
            return -1;
        }
    }

    return -1;
  }

  return 0;
}

int sync_replace(line* line_a)
{
  char msg[MAX_LEN];

  peer_stat* comms_info = new peer_stat;

  comms_info->pos_count=0;
  comms_info->neg_count=0;
  comms_info->peer_addr=new peer_stat_address[server_config.NO_OF_PEERS];
  comms_info->line_a=line_a;

  for(int i=0;i<server_config.NO_OF_PEERS;i++)
  {
    comms_info->peer_addr[i].port=peer_info[i].port;
    strcpy(comms_info->peer_addr[i].ip,peer_info[i].ip);
    comms_info->peer_addr[i].marked=false;
  }

  pthread_mutex_init(&(comms_info->access_peer_stat),NULL);

  for(int i=0;i<server_config.NO_OF_PEERS;i++)
  {

      // cout<<"Pre-Commit Initialize : "<<i<<endl;
      if ( add_work_to_team(peer_sender_team,(void (*) (void*))send_precommit,(void*)comms_info) != 0 )
      {
          terminate_team(peer_sender_team);
          return -1;
      }
  }

  pthread_mutex_lock(&comms_info->access_peer_stat);

  if(comms_info->pos_count!=server_config.NO_OF_PEERS||comms_info->neg_count!=server_config.NO_OF_PEERS||(comms_info->pos_count+comms_info->neg_count)!=server_config.NO_OF_PEERS)
  {
    pthread_cond_wait(&comms_info->peer_stat_cond,&comms_info->access_peer_stat);
  }

  pthread_mutex_unlock(&comms_info->access_peer_stat);

  snprintf(msg, 2*MAX_LEN, "Positive Precommits : %d , Negative Precommits : %d",comms_info->pos_count,comms_info->neg_count);
  logger(msg);

  if(comms_info->pos_count==server_config.NO_OF_PEERS)
  {
    comms_info->writer=false;
    comms_info->replacer=true;

    comms_info->pos_count=0;
    comms_info->neg_count=0;

    for(int i=0;i<server_config.NO_OF_PEERS;i++)
    {
      comms_info->peer_addr[i].marked=false;
    }

    for(int i=0;i<server_config.NO_OF_PEERS;i++)
    {
        if ( add_work_to_team(peer_sender_team,(void (*) (void*))send_commit,(void*)comms_info) != 0 )
        {
            terminate_team(peer_sender_team);
            return 0;
        }
    }
  }
  else
  {
    return -1;
  }

  pthread_mutex_lock(&comms_info->access_peer_stat);

  if(comms_info->pos_count!=server_config.NO_OF_PEERS||comms_info->neg_count!=server_config.NO_OF_PEERS||(comms_info->pos_count+comms_info->neg_count)!=server_config.NO_OF_PEERS)
  {
    pthread_cond_wait(&comms_info->peer_stat_cond,&comms_info->access_peer_stat);
  }

  pthread_mutex_unlock(&comms_info->access_peer_stat);

  snprintf(msg, 2*MAX_LEN, "Positive Commits : %d, Negative Commits : %d",comms_info->pos_count,comms_info->neg_count);
  logger(msg);


  if(comms_info->pos_count==server_config.NO_OF_PEERS)
  {
    int replace_return=replace_descriptor(line_a->msg_nmbr,line_a->poster,line_a->message,false);
    if(replace_return<0)
    {
      comms_info->pos_count=0;
      comms_info->neg_count=0;

      for(int i=0;i<server_config.NO_OF_PEERS;i++)
      {
        comms_info->peer_addr[i].marked=false;
      }

      for(int i=0;i<server_config.NO_OF_PEERS;i++)
      {
          if ( add_work_to_team(peer_sender_team,(void (*) (void*))send_del,(void*)comms_info) != 0 )
          {
              terminate_team(peer_sender_team);
              return -1;
          }
      }

      return -1;
    }

    char read_fetch[MAX_LEN];
    int read_return = read_descriptor(line_a->msg_nmbr,read_fetch,1);

    if(read_return==-1)
    {
      comms_info->pos_count=0;
      comms_info->neg_count=0;

      for(int i=0;i<server_config.NO_OF_PEERS;i++)
      {
        comms_info->peer_addr[i].marked=false;
      }

      for(int i=0;i<server_config.NO_OF_PEERS;i++)
      {
          if ( add_work_to_team(peer_sender_team,(void (*) (void*))send_del,(void*)comms_info) != 0 )
          {
              terminate_team(peer_sender_team);
              return -1;
          }
      }

      return -1;
    }
    else
    {
      comms_info->pos_count=0;
      comms_info->neg_count=0;

      for(int i=0;i<server_config.NO_OF_PEERS;i++)
      {
        comms_info->peer_addr[i].marked=false;
      }

      for(int i=0;i<server_config.NO_OF_PEERS;i++)
      {
          if ( add_work_to_team(peer_sender_team,(void (*) (void*))send_quit,(void*)comms_info) != 0 )
          {
              terminate_team(peer_sender_team);
              return -1;
          }
      }

      return 0;
    }
  }
  else
  {
    comms_info->pos_count=0;
    comms_info->neg_count=0;

    for(int i=0;i<server_config.NO_OF_PEERS;i++)
    {
      comms_info->peer_addr[i].marked=false;
    }

    for(int i=0;i<server_config.NO_OF_PEERS;i++)
    {
        if ( add_work_to_team(peer_sender_team,(void (*) (void*))send_del,(void*)comms_info) != 0 )
        {
            terminate_team(peer_sender_team);
            return -1;
        }
    }

    return -1;
  }

  return 0;
}



int send_precommit(peer_stat* peers)
{

    char msg[2*MAX_LEN];  // logger string
    msg[0]='\0';
    const int ALEN = 1024;
    char req[ALEN];
    char ans[ALEN];
    char buff[ALEN];
    buff[0]='\0';


    int mark=-1;

    pthread_mutex_lock(&peers->access_peer_stat);

      for(int i=0;i<server_config.NO_OF_PEERS;i++)
      {
        if(peers->peer_addr[i].marked==false)
        {
          mark = i;
          peers->peer_addr[i].marked=true;
          break;
        }
        else
        {
          continue;
        }
      }

    pthread_mutex_unlock(&peers->access_peer_stat);

    if(mark==-1)
    {
      return 0;
    }

    char* port=new char;
    sprintf(port,"%ld",peers->peer_addr[mark].port);

    size_t sd = connectbyport(peers->peer_addr[mark].ip,port);
    peers->peer_addr[mark].peer_port=sd;

    if (sd == err_host)
    {
        printf("\nSYNC FAIL with %s:%s\n",peers->peer_addr[mark].ip,port);
        return -1;
    }

    if (sd < 0)
    {
        printf("\nSYNC FAIL with %s:%s\n",peers->peer_addr[mark].ip,port);
        return -1;
    }

    sprintf(req,"precommit %s ",peers->line_a->poster);
    snprintf(msg, 2*MAX_LEN, "Sending %s to peer %s:%s.",req,peers->peer_addr[mark].ip,port);
    logger(msg);

    if(strlen(req) > 0 && req[strlen(req) - 1] == '\n')
        req[strlen(req) - 1] = '\0';
    send(sd,req,strlen(req),0);
    send(sd,"\n",1,0);

    int n;
    while ((n = recv_nonblock(sd,buff,ALEN-1,10)) != recv_nodata)
    {
              if (n == 0)
              {
                  break;
              }
    }

    buff[strlen(buff)-1]='\0';

    pthread_mutex_lock(&peers->access_peer_stat);

    if(strcmp(buff,"positive")==0)
    {
      peers->pos_count+=1;
    }
    else
    {
      peers->neg_count+=1;
    }

    if(peers->pos_count==server_config.NO_OF_PEERS||peers->neg_count==server_config.NO_OF_PEERS||(peers->pos_count+peers->neg_count)==server_config.NO_OF_PEERS)
    {
        pthread_cond_broadcast(&peers->peer_stat_cond);
    }

    pthread_mutex_unlock(&peers->access_peer_stat);


    snprintf(msg, 2*MAX_LEN, "REPLY from %s:%s is %s",peers->peer_addr[mark].ip,port,buff);
    logger(msg);

    return 0;

}

int send_commit(peer_stat* peers)
{
  char msg[2*MAX_LEN];  // logger string
  msg[0]='\0';
  const int ALEN = 1024;
  char req[ALEN];
  char ans[ALEN];
  char buff[ALEN];
  buff[0]='\0';


  int mark=-1;

  pthread_mutex_lock(&peers->access_peer_stat);

    for(int i=0;i<server_config.NO_OF_PEERS;i++)
    {
      if(peers->peer_addr[i].marked==false)
      {
        mark = i;
        peers->peer_addr[i].marked=true;
        break;
      }
      else
      {
        continue;
      }
    }

  pthread_mutex_unlock(&peers->access_peer_stat);

  if(mark==-1)
  {
    return 0;
  }

  char* port=new char;
  sprintf(port,"%ld",peers->peer_addr[mark].port);

  size_t sd = peers->peer_addr[mark].peer_port;



  if (sd == err_host)
  {
      printf("\nSYNC FAIL with %s:%s\n",peers->peer_addr[mark].ip,port);
      return -1;
  }

  if (sd < 0)
  {
      printf("\nSYNC FAIL with %s:%s\n",peers->peer_addr[mark].ip,port);
      return -1;
  }

  if(peers->writer==true&&peers->replacer==false)
  {
    // cout<<"COMMIT : Sending write"<<endl;

    sprintf(req,"commit write %s %s ",peers->line_a->msg_nmbr,peers->line_a->message);

    snprintf(msg, 2*MAX_LEN, "Sending %s to peer %s:%s.",req,peers->peer_addr[mark].ip,port);
    logger(msg);

    if(strlen(req) > 0 && req[strlen(req) - 1] == '\n')
        req[strlen(req) - 1] = '\0';
    send(sd,req,strlen(req),0);
    send(sd,"\n",1,0);
  }
  else if(peers->writer==false&&peers->replacer==true)
  {
    // cout<<"COMMIT : Sending replace"<<endl;

    sprintf(req,"commit replace %s/%s ",peers->line_a->msg_nmbr,peers->line_a->message);

    snprintf(msg, 2*MAX_LEN, "Sending %s to peer %s:%s.",req,peers->peer_addr[mark].ip,port);
    logger(msg);

    if(strlen(req) > 0 && req[strlen(req) - 1] == '\n')
        req[strlen(req) - 1] = '\0';
    send(sd,req,strlen(req),0);
    send(sd,"\n",1,0);
  }
  else
  {
    // cout<<"COMMIT : Returning due to bad config"<<endl;
    return -1;
  }

  int n;
  while ((n = recv_nonblock(sd,buff,ALEN-1,10)) != recv_nodata)
  {
            if (n == 0)
            {
                break;
            }
  }

  if(n==-1)
  {
    return -1;
  }

  buff[strlen(buff)-1]='\0';

  snprintf(msg, 2*MAX_LEN, "REPLY for COMMIT from %s:%s is %s",peers->peer_addr[mark].ip,port,buff);
  logger(msg);

  pthread_mutex_lock(&peers->access_peer_stat);

  if(strcmp(buff,"positive")==0)
  {
    peers->pos_count+=1;
  }
  else if(strcmp(buff,"negative_exists")==0)
  {
    peers->neg_count+=1;
    peers->peer_addr[mark].dont_reissue=true;
  }
  else
  {
    peers->neg_count+=1;
  }

  if(peers->pos_count==server_config.NO_OF_PEERS||peers->neg_count==server_config.NO_OF_PEERS||(peers->pos_count+peers->neg_count)==server_config.NO_OF_PEERS)
  {
      pthread_cond_broadcast(&peers->peer_stat_cond);
  }

  pthread_mutex_unlock(&peers->access_peer_stat);

  return 0;


}

int send_del(peer_stat* peers)
{
  char msg[2*MAX_LEN];  // logger string
  msg[0]='\0';

  const int ALEN = 1024;
  char req[ALEN];
  char ans[ALEN];

  char buff[ALEN];
  buff[0]='\0';


  int mark=-1;

  pthread_mutex_lock(&peers->access_peer_stat);

    for(int i=0;i<server_config.NO_OF_PEERS;i++)
    {
      if(peers->peer_addr[i].marked==false)
      {
        mark = i;
        peers->peer_addr[i].marked=true;
        break;
      }
      else
      {
        continue;
      }
    }

  pthread_mutex_unlock(&peers->access_peer_stat);

  if(mark==-1)
  {
    return 0;
  }

  char* port=new char;
  sprintf(port,"%ld",peers->peer_addr[mark].port);

  size_t sd = peers->peer_addr[mark].peer_port;



  if (sd == err_host)
  {
      printf("\nSYNC FAIL with %s:%s\n",peers->peer_addr[mark].ip,port);
      return -1;
  }

  if (sd < 0)
  {
      printf("\nSYNC FAIL with %s:%s\n",peers->peer_addr[mark].ip,port);
      return -1;
  }

  if(!peers->peer_addr[mark].dont_reissue)
  {
    // cout<<"UNDO : Sending Del"<<endl;

    sprintf(req,"abort ");

    snprintf(msg, 2*MAX_LEN, "Sending %s to peer %s:%s.",req,peers->peer_addr[mark].ip,port);
    logger(msg);

    if(strlen(req) > 0 && req[strlen(req) - 1] == '\n')
        req[strlen(req) - 1] = '\0';

    send(sd,req,strlen(req),0);
    send(sd,"\n",1,0);
  }
  else
  {
    // cout<<"UNDO : Returning due to bad config"<<endl;
    return -1;
  }

  shutdown(sd, SHUT_RDWR);
  close(sd);
  return 0;


}

int send_quit(peer_stat* peers)
{
  char msg[2*MAX_LEN];  // logger string
  msg[0]='\0';

  const int ALEN = 1024;
  char req[ALEN];
  char ans[ALEN];

  char buff[ALEN];
  buff[0]='\0';


  int mark=-1;

  pthread_mutex_lock(&peers->access_peer_stat);

    for(int i=0;i<server_config.NO_OF_PEERS;i++)
    {
      if(peers->peer_addr[i].marked==false)
      {
        mark = i;
        peers->peer_addr[i].marked=true;
        break;
      }
      else
      {
        continue;
      }
    }

  pthread_mutex_unlock(&peers->access_peer_stat);

  if(mark==-1)
  {
    return 0;
  }

  char* port=new char;
  sprintf(port,"%ld",peers->peer_addr[mark].port);

  size_t sd = peers->peer_addr[mark].peer_port;



  if (sd == err_host)
  {
      printf("\nSYNC FAIL with %s:%s\n",peers->peer_addr[mark].ip,port);
      return -1;
  }

  if (sd < 0)
  {
      printf("\nSYNC FAIL with %s:%s\n",peers->peer_addr[mark].ip,port);
      return -1;
  }

  if(!peers->peer_addr[mark].dont_reissue)
  {
    // cout<<"UNDO : Sending Del"<<endl;

    sprintf(req,"quit ");

    snprintf(msg, 2*MAX_LEN, "Sending %s to peer %s:%s.",req,peers->peer_addr[mark].ip,port);
    logger(msg);

    if(strlen(req) > 0 && req[strlen(req) - 1] == '\n')
        req[strlen(req) - 1] = '\0';

    send(sd,req,strlen(req),0);
    send(sd,"\n",1,0);
  }
  else
  {
    // cout<<"UNDO : Returning due to bad config"<<endl;
    return -1;
  }

  shutdown(sd, SHUT_RDWR);
  close(sd);
  return 0;


}
