#include "bbserv_utils.h"



void welcome_message()
{
  time_t tt = time(0);
  char* ts = ctime(&tt);
  cout<<"\n   BULLETTIN BOARD SERVER v1.0\n    "<<ts<<endl;
}

void logger(const char * msg)
{
    pthread_mutex_lock(&logger_mutex);

      time_t tt = time(0);
      char* ts = ctime(&tt);
      ts[strlen(ts) - 1] = '\0';
      cout<<ts<<" : "<<msg<<endl;
      fflush(stdout);

    pthread_mutex_unlock(&logger_mutex);
}

void ip_to_dotted(unsigned int ip, char* buffer)
{
    char* ipc = (char*)(&ip);
    sprintf(buffer, "%d.%d.%d.%d", ipc[0], ipc[1], ipc[2], ipc[3]);
}

struct line* new_line(char msg_nmbr[MAX_LEN], char poster[MAX_LEN], char message[MAX_LEN])
{
  struct line* line_a;
  line_a=(struct line*)malloc(sizeof(struct line));//memory allocated to create a teamspace
  line_a->msg_nmbr=new char[MAX_LEN];
  line_a->poster=new char[MAX_LEN];
  line_a->message=new char[MAX_LEN];

  strcpy(line_a->msg_nmbr,msg_nmbr);
  strcpy(line_a->poster,poster);
  strcpy(line_a->message,message);

  return line_a;
}

void fetch_cmndLine(int argc, char** argv)
{
  int copt;
  char msg[MAX_LEN];
  while ((copt = getopt(argc,argv,"c:b:T:p:s:fd")) != -1)
  {
        switch ((char)copt)
        {
            case 'f':
                server_config.DETACH_IND = 1;
                server_config.CDETACH = false;
                break;

            case 'd':
                server_config.DEBUG_DELAY_IND = 1;
                server_config.DEBUG_DELAY = true;
                break;

            case 'c':
                strcpy(server_config.CONF_FILE,optarg);
                server_config.CONF_FILE_IND=1;
                break;

            case 'b':
                strcpy(server_config.BBFILE,optarg);
                server_config.BBFILE_IND = 1;
                break;

            case 'T':
                server_config.T_MAX_IND = 1;
                server_config.T_MAX = atoi(optarg);
                break;

            case 'p':
                server_config.BP_IND = 1;
                server_config.BP = atoi(optarg);
                break;

            case 's':
                server_config.SP_IND = 1;
                server_config.SP = atoi(optarg);
                break;

            case 'x':
                server_config.SERV_REST = true;
                break;

            default:
                snprintf(msg, MAX_LEN, "-> This switch is unidentified : %d\n", copt);
                logger(msg);
                break;
        }
    }

    argc -= optind; argv += optind;
    int no_of_peers = argc;

    server_config.NO_OF_PEERS=no_of_peers;

    if(no_of_peers>=1)
    {
            server_config.PEER_COMMS_IND=1;
            peer_info=new peer_info_s[no_of_peers];

            for (int i = 0; i < no_of_peers; i++)
            {
                if(strstr(argv[i],":")!=NULL)
                {
                  char *sep=strtok(argv[i],":");
                  snprintf(peer_info[i].ip,MAX_LEN,"%s",sep);

                  sep = strtok (NULL, ":");
                  peer_info[i].port=atoi(sep);

                  peer_info[i].id=i;
                }
                else
                {
                  server_config.NO_OF_PEERS--;
                }

            }

    }
}

void fetch_config()
{

  size_t tmax_ind = 0, bp_ind = 0,sp_ind = 0,pc_ind = 0,detach_ind = 0,debug_ind = 0,bb_ind = 0,peers_ind=0;
  char msg[MAX_LEN];

  char init_command[513];
  init_command[512] = '\0';

  char command[513];
  command[512] = '\0';

  char* com_tok[513];
  size_t num_tok;

  int confd = open(server_config.CONF_FILE, O_RDONLY);
  if (confd < 0)
  {
    snprintf(msg,MAX_LEN,"Configuration file %s not found",server_config.CONF_FILE);
    logger(msg);
  }

  while (tmax_ind == 0||bp_ind == 0||sp_ind == 0||pc_ind == 0||detach_ind == 0||debug_ind == 0||bb_ind == 0||peers_ind==0)
  {
    int n = readline(confd, command, 512);

    if (n == recv_nodata)
    {
      break;
    }

    if (n < 0)
    {
      sprintf(command, "config error");
      perror(command);
      break;
    }

    num_tok = str_tokenize(command, com_tok, strlen(command));

    if (strcmp(com_tok[0], "THMAX") == 0 && atol(com_tok[1]) > 0)
    {
                tmax_ind=1;
                if(server_config.T_MAX_IND==0)
                {
                server_config.T_MAX=atol(com_tok[1]);
                server_config.T_MAX_IND=2;
                }
                else
                {
                  continue;
                }
    }
    else if (strcmp(com_tok[0], "BBPORT") == 0 && atol(com_tok[1]) > 0)
    {
                bp_ind=1;
                if(server_config.BP_IND==0)
                {server_config.BP=atol(com_tok[1]);server_config.BP_IND=2;}
                else
                {continue;}
    }
    else if (strcmp(com_tok[0], "SYNCPORT") == 0 && atol(com_tok[1]) > 0)
    {
                sp_ind=1;
                if(server_config.SP_IND==0)
                {server_config.SP=atol(com_tok[1]);server_config.SP_IND=2;}
                else
                {continue;}
    }
    else if (strcmp(com_tok[0], "BBFILE") == 0)
    {
                if(num_tok>1)
                {
                  bb_ind=1;
                  if(server_config.BBFILE_IND==0)
                  {strcpy(server_config.BBFILE,com_tok[1]);server_config.BBFILE_IND=2;}
                  else
                  {continue;}
                }
                else
                {
                  bb_ind=1;
                }

    }
    else if (strcmp(com_tok[0], "PEERS") == 0)
    {
                if(num_tok>1)
                {
                  pc_ind=1;
                  if(server_config.PEER_COMMS_IND==0)
                  {
                    int no_of_peers=num_tok-1;
                    if(no_of_peers>=1)
                    {
                            peer_info=new peer_info_s[no_of_peers];

                            for (int i = 1; i <= no_of_peers; i++)
                            {
                                if(strstr(com_tok[i],":")!=NULL)
                                {
                                  server_config.NO_OF_PEERS++;

                                  char *sep=strtok(com_tok[i],":");
                                  snprintf(peer_info[i-1].ip,MAX_LEN,"%s",sep);

                                  sep = strtok (NULL, ":");
                                  peer_info[i-1].port=atoi(sep);
                                  int xx = i-1;
                                  peer_info[i-1].id=xx;
                                }

                            }

                    }

                    server_config.PEER_COMMS_IND=2;
                  }
                  else
                  {continue;}
                }
                else
                {
                  peers_ind=1;
                }

    }
    else if (strcmp(com_tok[0], "DAEMON") == 0)
    {
                detach_ind=1;
                if(server_config.DETACH_IND==0)
                {server_config.FDETACH=atol(com_tok[1]);server_config.DETACH_IND=2;}
                else
                {continue;}
    }
    else if (strcmp(com_tok[0], "DEBUG") == 0)
    {
                debug_ind=1;
                if(server_config.DEBUG_DELAY_IND==0)
                {server_config.DEBUG_DELAY=atol(com_tok[1]);server_config.DEBUG_DELAY_IND=2;}
                else
                {continue;}
    }
  }
  close(confd);

}

void force_fetch_config()
{

  size_t tmax_ind = 0, bp_ind = 0,sp_ind = 0,pc_ind = 0,detach_ind = 0,debug_ind = 0,bb_ind = 0,peers_ind=0;
  char msg[MAX_LEN];

  char init_command[513];
  init_command[512] = '\0';

  char command[513];
  command[512] = '\0';

  char* com_tok[513];
  size_t num_tok;

  int confd = open(server_config.CONF_FILE, O_RDONLY);
  if (confd < 0)
  {
    snprintf(msg,MAX_LEN,"Configuration file %s not found",server_config.CONF_FILE);
    logger(msg);
  }

  while (tmax_ind == 0||bp_ind == 0||sp_ind == 0||pc_ind == 0||detach_ind == 0||debug_ind == 0||bb_ind == 0||peers_ind==0)
  {
    int n = readline(confd, command, 512);

    if (n == recv_nodata)
    {
      break;
    }

    if (n < 0)
    {
      sprintf(command, "config error");
      perror(command);
      break;
    }

    num_tok = str_tokenize(command, com_tok, strlen(command));

    if (strcmp(com_tok[0], "THMAX") == 0 && atol(com_tok[1]) > 0)
    {

                server_config.T_MAX=atol(com_tok[1]);
                server_config.T_MAX_IND=2;

    }
    else if (strcmp(com_tok[0], "BBPORT") == 0 && atol(com_tok[1]) > 0)
    {
        server_config.BP=atol(com_tok[1]);server_config.BP_IND=2;
    }
    else if (strcmp(com_tok[0], "SYNCPORT") == 0 && atol(com_tok[1]) > 0)
    {
                server_config.SP=atol(com_tok[1]);server_config.SP_IND=2;
    }
    else if (strcmp(com_tok[0], "BBFILE") == 0)
    {
                if(num_tok>1)
                {
                    strcpy(server_config.BBFILE,com_tok[1]);server_config.BBFILE_IND=2;
                }

    }
    else if (strcmp(com_tok[0], "PEERS") == 0)
    {
                if(num_tok>1)
                {
                    int no_of_peers=num_tok-1;

                    delete[] peer_info;
                    server_config.NO_OF_PEERS=0;

                    if(no_of_peers>=1)
                    {
                            peer_info=new peer_info_s[no_of_peers];

                            for (int i = 1; i <= no_of_peers; i++)
                            {
                                if(strstr(com_tok[i],":")!=NULL)
                                {
                                  server_config.NO_OF_PEERS++;

                                  char *sep=strtok(com_tok[i],":");
                                  snprintf(peer_info[i-1].ip,MAX_LEN,"%s",sep);

                                  sep = strtok (NULL, ":");
                                  peer_info[i-1].port=atoi(sep);
                                  int xx = i-1;
                                  peer_info[i-1].id=xx;
                                }

                            }

                    }

                    server_config.PEER_COMMS_IND=2;

                }

    }
    else if (strcmp(com_tok[0], "DAEMON") == 0)
    {
                server_config.DETACH=atol(com_tok[1]);server_config.DETACH_IND=2;
    }
    else if (strcmp(com_tok[0], "DEBUG") == 0)
    {
                server_config.DEBUG_DELAY=atol(com_tok[1]);server_config.DEBUG_DELAY_IND=2;
    }
  }
  close(confd);

}

void print_config()
{
  char type[50];
  strcpy(type,"System Default");

  cout<<endl<<"****************************************************"<<endl;
  fflush(stdout);


  if(server_config.T_MAX_IND==1)
  {
    strcpy(type,"Command Line Override");
  }
  else if(server_config.T_MAX_IND==2)
  {
    strcpy(type,"Config File Setting  ");
  }
  else
  {
    strcpy(type,"System Default       ");
  }

  cout<<"-> "<<type<<"-> Maximum Threads : "<<server_config.T_MAX<<" "<<"\n";
  fflush(stdout);


  if(server_config.BP_IND==1)
  {
    strcpy(type,"Command Line Override");
  }
  else if(server_config.BP_IND==2)
  {
    strcpy(type,"Config File Setting  ");
  }
  else
  {
    strcpy(type,"System Default       ");
  }

  cout<<"-> "<<type<<"-> Client listening port : "<<server_config.BP<<" "<<"\n";

  if(server_config.BBFILE_IND==1)
  {
    strcpy(type,"Command Line Override");
  }
  else if(server_config.BBFILE_IND==2)
  {
    strcpy(type,"Config File Setting  ");
  }
  else
  {
    strcpy(type,"System Default       ");
  }

  cout<<"-> "<<type<<"-> Bullettin Board File : "<<server_config.BBFILE<<" "<<"\n";


  if(server_config.SP_IND==1)
  {
    strcpy(type,"Command Line Override");
  }
  else if(server_config.SP_IND==2)
  {
    strcpy(type,"Config File Setting  ");
  }
  else
  {
    strcpy(type,"System Default       ");
  }
  cout<<"-> "<<type<<"-> Peer listening port : "<<server_config.SP<<" "<<"\n";


  if(server_config.CDETACH==1&&server_config.FDETACH==1)
  {
    strcpy(type,"Command Line/Conf. File agree on Detach");
  }

  if(server_config.CDETACH==1&&server_config.FDETACH==0)
  {
    strcpy(type,"Config File Disagree on Detach");
  }

  if(server_config.CDETACH==0&&server_config.FDETACH==1)
  {
    strcpy(type,"Command Line Disagree on Detach");
  }

  if(server_config.CDETACH==0&&server_config.FDETACH==0)
  {
    strcpy(type,"Command Line/Conf. File Disagree on Detach");
  }


  if(server_config.DETACH==0)
  {
    cout<<"-> "<<type<<"-> Server will run on Foreground."<<"\n";
  }
  else
  {
    cout<<"-> "<<type<<"-> Server will run on Background."<<"\n";
  }

  if(server_config.DEBUG_DELAY_IND==1)
  {
    strcpy(type,"Command Line Override");
  }
  else if(server_config.DEBUG_DELAY_IND==2)
  {
    strcpy(type,"Config File Setting  ");
  }
  else
  {
    strcpy(type,"System Default       ");
  }
  if(server_config.DEBUG_DELAY==0)
  {
    cout<<"-> "<<type<<"-> Server will not delay file operations."<<"\n";
  }
  else
  {
    cout<<"-> "<<type<<"-> Server will delay file operations."<<"\n";
  }

  if(server_config.CONF_FILE_IND==1)
  {
    strcpy(type,"Command Line Override");
  }
  else
  {
    strcpy(type,"System Default       ");
  }
  cout<<"-> "<<type<<"-> Configuration file : "<<server_config.CONF_FILE<<"\n";

  if(server_config.PEER_COMMS_IND==1)
  {
    strcpy(type,"Command Line Override");
  }
  else if(server_config.PEER_COMMS_IND==2)
  {
    strcpy(type,"Config File Setting  ");
  }
  else
  {
    strcpy(type,"System Default       ");
  }

  cout<<"-> "<<type<<"-> Peer servers : "<<endl;

  for(int x=0;x<server_config.NO_OF_PEERS;x++)
  {
    cout<<"   Peer "<<peer_info[x].id<<" "<<peer_info[x].ip<<" "<<peer_info[x].port<<endl;

  }

  cout<<"-> Atleast "<< ((5*server_config.T_MAX)+3)<<" threads will be created based on configuration."<<endl;

  cout<<"****************************************************"<<"\n";
}
