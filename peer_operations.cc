/*
 * In-place string tokenizer, by Stefan Bruda.  Read the comments in
 * the header before using it.
 */

#include "peer_operations.h"


team* peer_control_team;
team* peer_receiver_team;
team* peer_sender_team;

void peer_establisher()
{

  char msg[MAX_LEN];  // logger string

  cout<<endl;
  team* peer_control_team = create_team(1,(char*)"Peer Comms Control");

  if(add_work_to_team(peer_control_team,(void (*) (void*))peer_receiver_control,(void*)NULL) != 0)
  {
      snprintf(msg,MAX_LEN,"Peer Receiver Thread initiation error.");
      logger(msg);
      return;
  }

  cout<<endl;
  peer_sender_team = create_team(2 * server_config.T_MAX,(char*)"Peer sender");


  return ;

}

void peer_receiver_control()
{
  char msg[MAX_LEN];  // logger string

  long int msock = passivesocket(server_config.SP,32);
  server_config.peer_listener_master_socket=msock;
  long int ssock;

  struct sockaddr_in client_addr; // the address of the client...
  socklen_t client_addr_len = sizeof(client_addr); // ... and its length

  team* peer_receiver_team;
  cout<<endl;
  peer_receiver_team = create_team(2 * server_config.T_MAX,(char*)"Peer receiver");

  // assemble client coordinates (communication socket + IP)
  client_t* clnt = new client_t;

  while (bealive)
  {
      // Accept connection:
      ssock = accept(msock, (struct sockaddr*)&client_addr, &client_addr_len);
      clnt -> sd = ssock;
      ip_to_dotted(client_addr.sin_addr.s_addr, clnt -> ip);
      if (ssock < 0)
      {
          terminate_team(peer_receiver_team);
          if (errno == EINTR) continue;
          snprintf(msg, MAX_LEN, "%s: Peer server accept : %s\n", __FILE__, strerror(errno));
          logger(msg);
          return;
      }
      else
      {
          if ( add_work_to_team(peer_receiver_team,(void (*) (void*))peer_receiver_operations,(void*)clnt) != 0 )
          {
              terminate_team(peer_receiver_team);
              return;
          }
      }
  }
  return;   // will never reach this anyway...

}


void peer_receiver_operations(client_t* clnt)
{
  char msg[2*MAX_LEN];  // logger string
  char request[MAX_LEN];  // logger string
  char response[2*MAX_LEN];  // logger string

  client_info client;
  client.id = clnt->sd;
  strcpy(client.ip,clnt->ip);

  long int ssock = client.id;

  client.uname_avl=false;
  client.user=new char[MAX_LEN];
  strcpy(client.user,"nobody");

  snprintf(msg, 2*MAX_LEN, "Peer %s:%ld thread allocation successfull.",client.ip,client.id);
  logger(msg);

  char undo_command[MAX_LEN];
  char undo_user[MAX_LEN];
  char undo_nmbr[MAX_LEN];
  char undo_message[MAX_LEN];

  while(readline(ssock,request,MAX_LEN)!=recv_nodata)
  {
      request[strlen(request)-1]='\0';

      snprintf(msg, 2*MAX_LEN, "Peer REQUEST : %s",request);
      logger(msg);

      char *command[MAX_LEN];
      char client_command[MAX_LEN];
      client_command[0]='\0';

      int t = str_tokenize(request, command, strlen(request));



      if(command[0]!=NULL)
      {
          strcpy(client_command,tolwr(command[0]));
      }
      else
      {
        break;
      }

      if(strcmp(client_command,"abort")==0)
      {
        if(undo_command!=NULL)
        {
          if(strcmp(undo_command,"write")==0)
          {
            delete_descriptor(undo_nmbr);
          }
          else if(strcmp(undo_command,"replace")==0)
          {
            replace_descriptor(undo_nmbr,undo_user,undo_message,true);
          }
        }
        shutdown(ssock, SHUT_RDWR);
        close(ssock);
        break;
      }
      else if(strcmp(client_command,"quit")==0)
      {
        shutdown(ssock, SHUT_RDWR);
        close(ssock);
        break;
      }
      else if(strcmp(client_command,"precommit")==0)
      {
        if(t>=2)
        {
          if(t>2)
          {
            char* message=new char[MAX_LEN];

            for(int i=1;i<t;i++)
            {
              sprintf(message,"%s %s",message,command[i]);
            }

            strcpy(client.user,message);
            delete[] message;

          }
          else
          {
            strcpy(client.user,command[1]);
          }

          snprintf(response, 2*MAX_LEN, "positive\n");
          send(ssock, response, strlen(response),0);

          snprintf(msg, 2*MAX_LEN, "Positive Reply for Precommit Peer Request");
          logger(msg);

        }
        else
        {
        snprintf(response, 2*MAX_LEN, "negative\n");
        send(ssock, response, strlen(response),0);

        snprintf(msg, 2*MAX_LEN, "Negative Reply for Precommit Peer Request, #less_tokens");
        logger(msg);

        shutdown(ssock, SHUT_RDWR);
        close(ssock);
        break;
        }
      }
      else if(strcmp(client_command,"commit")==0)
      {
        int tk = t-1;

        if(t>1)
        {
          if(strcmp(command[1],"write")==0)
          {
                if(tk>2)
                {
                    char* message=new char[MAX_LEN];

                    if(tk>3)
                    {
                      for(int i=3;i<t;i++)
                      {
                        sprintf(message,"%s %s",message,command[i]);
                      }
                    }
                    else
                    {
                      strcpy(message,command[3]);
                    }

                    char temp_line[BBLINE_LEN];
                    int read_return = read_descriptor(command[2], temp_line,1);

                      if(read_return==-1)
                      {

                      int write_return=write_descriptor(command[2],client.user,message,true);


                      if(write_return==0)
                      {
                        strcpy(undo_command,"write");
                        strcpy(undo_nmbr,command[2]);

                        snprintf(response, 2*MAX_LEN, "positive\n");
                        send(ssock, response, strlen(response),0);

                        snprintf(msg, 2*MAX_LEN, "Positive Reply for Peer Request");
                        logger(msg);

                        continue;
                      }
                      else
                      {
                        snprintf(response, 2*MAX_LEN, "negative\n");
                        send(ssock, response, strlen(response),0);

                        snprintf(msg, 2*MAX_LEN, "Negative Reply for Commit Write Peer Request, #line_already_exists");
                        logger(msg);
                      }

                    }
                    else
                    {
                      snprintf(response, 2*MAX_LEN, "negative_exists\n");
                      send(ssock, response, strlen(response),0);

                      snprintf(msg, 2*MAX_LEN, "Negative Reply for Commit Write Peer Request, #line_already_exists");
                      logger(msg);

                      break;
                    }
                  }
                  else
                  {
                    snprintf(response, 2*MAX_LEN, "negative\n");
                    send(ssock, response, strlen(response),0);

                    snprintf(msg, 2*MAX_LEN, "Negative Reply for Commit Write Peer Request, #line_improper_format_2");
                    logger(msg);

                    break;
                  }
          }
          else if(strcmp(command[1],"replace")==0)
          {
            if(t>2)
            {
              char repl_request[MAX_LEN];
              repl_request[0]='\0';
              for(int x=2;x<t;x++)
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
                  snprintf(response, 2*MAX_LEN, "negative\n");
                  send(ssock, response, strlen(response),0);

                  snprintf(msg, 2*MAX_LEN, "Negative Reply for Commit Replace Peer Request, #line_improper_format_1");
                  logger(msg);

                  break;

                }
              }


              char temp_line[BBLINE_LEN];
              int read_return = read_descriptor(repl_tokens[0], temp_line,1);

              if(read_return==-1)
              {
                  snprintf(response, 2*MAX_LEN, "negative\n");
                  send(ssock, response, strlen(response),0);

                  snprintf(msg, 2*MAX_LEN, "Negative Reply for Commit Replace Peer Request, #replace_line_not_found");
                  logger(msg);

                  break;
              }
              else
              {
                  strcpy(undo_command,"replace");
                  strcpy(undo_nmbr,repl_tokens[0]);
                  read_tokenized_descriptor(undo_nmbr,undo_user,undo_message);

                  replace_descriptor(repl_tokens[0],client.user,message,true);

                  snprintf(response, 2*MAX_LEN, "positive\n");
                  send(ssock, response, strlen(response),0);

                  snprintf(msg, 2*MAX_LEN, "Positive Reply for Peer REPACE Request");
                  logger(msg);

                  continue;
              }


            }
            else
            {
              snprintf(response, 2*MAX_LEN, "negative\n");
              send(ssock, response, strlen(response),0);

              snprintf(msg, 2*MAX_LEN, "Negative Reply for Commit Replace Peer Request, #line_improper_format_2");
              logger(msg);

              break;
            }
          }
          else
          {
            snprintf(response, 2*MAX_LEN, "negative\n");
            send(ssock, response, strlen(response),0);

            snprintf(msg, 2*MAX_LEN, "Negative Reply for Commit Replace Peer Request, #line_improper_format_3");
            logger(msg);

            break;

          }
        }
        else
        {
          snprintf(response, 2*MAX_LEN, "negative\n");
          send(ssock, response, strlen(response),0);

          snprintf(msg, 2*MAX_LEN, "Negative Reply for Commit Replace Peer Request, #line_improper_format_4");
          logger(msg);

          break;
        }
      }
      else
      {
        snprintf(response, 2*MAX_LEN, "negative\n");
        send(ssock, response, strlen(response),0);

        snprintf(msg, 2*MAX_LEN, "Negative Reply for Commit Peer Request, #invalid_command");
        logger(msg);

        break;
      }
  }
}
