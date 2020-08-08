#include "bbserv_utils.h"
#include "thrd_mgmt.h"
#include "server_operations.h"
#include "peer_operations.h"
#include "linkedList.h"


const char* logfile = "bbserv.log";
const char* pidfile = "bbserv.pid";

sconf server_config;
pthread_mutex_t logger_mutex;
peer_info_s* peer_info;
team* file_team;
bool bealive=true;

pthread_mutex_t h_mutex;
pthread_cond_t h_cond;

int hupReceived=0;
int quitReceived=0;

pthread_mutex_t access_clients;
lList* clients_list;

static void sighupHandler(int sig)
{
    hupReceived = 1;
    char msg[MAX_LEN];

    snprintf(msg, MAX_LEN, "Received SIGHUP . \n");
    logger(msg);

    pthread_mutex_lock(&h_mutex);
    pthread_cond_signal(&h_cond);
    pthread_mutex_unlock(&h_mutex);

}

static void sigQuitHandler(int sig)
{
    quitReceived = 1;
    char msg[MAX_LEN];

    snprintf(msg, MAX_LEN, "Received SIGQUIT . \n");
    logger(msg);

    pthread_mutex_lock(&h_mutex);
    pthread_cond_signal(&h_cond);
    pthread_mutex_unlock(&h_mutex);

}

void term_handler(int sig)
{
  char msg[MAX_LEN];  // logger string

  snprintf(msg, MAX_LEN, "Current Process : %d , Parent Process : %d.\n", getpid(),getppid());
  logger(msg);
}


void quit_handler(int sig)
{
  char msg[MAX_LEN];  // logger string

  snprintf(msg, MAX_LEN, "%s: KILLING SIG received. Shutting down server gracefuly.\n", __FILE__);
  logger(msg);

  bealive=false;

  shutdown(server_config.master_socket, SHUT_RDWR);
  close(server_config.master_socket);

  shutdown(server_config.peer_listener_master_socket, SHUT_RDWR);
  close(server_config.peer_listener_master_socket);

  shutdownList(clients_list);

  snprintf(msg, MAX_LEN, "Starting file team termination.\n");
  logger(msg);
  terminate_team(file_team); //Wait for clients to disconnect and purge thread
  snprintf(msg, MAX_LEN, "File team termination complete.\n");
  logger(msg);


  snprintf(msg, MAX_LEN, "Starting Peer receiver team termination.\n");
  logger(msg);
  terminate_team(peer_receiver_team); //Wait for clients to disconnect and purge thread
  snprintf(msg, MAX_LEN, "Peer receiver team termination complete.\n");
  logger(msg);

  snprintf(msg, MAX_LEN, "Starting Peer sender team termination.\n");
  logger(msg);
  terminate_team(peer_sender_team); //Wait for clients to disconnect and purge thread
  snprintf(msg, MAX_LEN, "Peer sender team termination complete.\n");
  logger(msg);

  snprintf(msg, MAX_LEN, "Starting descriptor housekeep.\n");
  logger(msg);
  housekeep_descriptor();
  snprintf(msg, MAX_LEN, "Descriptor housekeep complete.\n");
  logger(msg);

  snprintf(msg, MAX_LEN, "Starting Peer control team termination.\n");
  logger(msg);
  terminate_team(peer_control_team); //Wait for clients to disconnect and purge thread
  snprintf(msg, MAX_LEN, "Peer control team termination complete.\n");
  logger(msg);

  snprintf(msg, MAX_LEN, "Program Purge Complete.\n");
  logger(msg);
}

int pass_to_team(team* file_team, client_t* clnt)
{
      client_t* clnt1 = new client_t;
      *clnt1 = *clnt;

      if ( add_work_to_team(file_team,(void (*) (void*))server_operations,(void*)clnt1) != 0 )
      {
          terminate_team(file_team);
          return -1;
      }

      return 0;
}

void* bb_server (int msock)
{
    signal(SIGQUIT, sigQuitHandler);
    signal(SIGTERM, term_handler);
    signal(SIGCHLD , SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGSTOP, SIG_IGN);
    signal(SIGKILL, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    // signal(SIGINT, SIG_IGN);//Foreground test ignore
    signal(SIGABRT, SIG_IGN);


    char msg[MAX_LEN];  // logger string

    int ssock;                      // slave sockets
    struct sockaddr_in client_addr; // the address of the client...
    socklen_t client_addr_len = sizeof(client_addr); // ... and its length
    cout<<endl;
    file_team = create_team(server_config.T_MAX,(char*)"Server worker");

    // Setting up the peer threads creation:
    pthread_t tt;
    pthread_attr_t ta;
    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta,PTHREAD_CREATE_DETACHED);

    if(pthread_create(&tt,NULL,(void* (*)(void*))peer_establisher,NULL)!=0)
    {
      cout<<"Peer thread error."<<endl;
    }
    pthread_detach(tt);

    // assemble client coordinates (communication socket + IP)
    client_t* clnt = new client_t;

    while (bealive)
    {
        // Accept connection:
        ssock = accept(msock, (struct sockaddr*)&client_addr, &client_addr_len);
        clnt -> sd = ssock;

        snprintf(msg, MAX_LEN, "New client accepted : %d",ssock);
        logger(msg);

        ip_to_dotted(client_addr.sin_addr.s_addr, clnt -> ip);
        if (ssock < 0)
        {
            if (errno == EINTR) continue;
            snprintf(msg, MAX_LEN, "%s: Bullettin Board server accept : %s\n", __FILE__, strerror(errno));
            logger(msg);
            return 0;
        }
        else
        {
            pass_to_team(file_team,clnt);
        }
    }
    return 0;   // will never reach this anyway...
}

int start_server()
{

  const int qlen = 32;                  // queue length for incoming connections
  char msg[MAX_LEN];

  snprintf(msg, MAX_LEN, "Initiating the server start.");
  logger(msg);
  bealive=1;

  clients_list=createList();

  // The pid file does not make sense as a lock file since our
  // server never goes down willlingly.  So we do not lock the file,
  // we just store the pid therein.  In other words, we hint to the
  // existence of a pid file but we are not really using it.
  int pfd = open(pidfile, O_RDWR| O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (pfd < 0)
  {
      perror("pid file");
      printf("Will not write the PID.\n");
  }

  snprintf(msg, MAX_LEN, "%d\n", getpid());
  write(pfd, msg, strlen(msg));
  close(pfd);

  // close(server_config.BP);  //Pull and purge the port incase if already occupied.

  long int bpsock = passivesocket(server_config.BP,qlen);

  if (bpsock < 0)
  {
      printf("Bullettin board server passivesocket");
      return 1;
  }


  //load the file
  int init_ret = initiate_descriptor(server_config.BBFILE);
  if(init_ret!=0)
  {
    printf("Bullettin board file not found and unable to create one!");
    return 1;
  }

  server_config.master_socket=bpsock;

  bb_server(bpsock);

  snprintf(msg,MAX_LEN,"Server Dies.");
  logger(msg);

  return 0;

}

int main (int argc, char** argv, char** envp)
{
  const int qlen = 32;                  // queue length for incoming connections
  char msg[MAX_LEN];
  pthread_mutex_init(&logger_mutex, 0);

  server_config.T_MAX=20;
  server_config.BP=9000;
  server_config.SP=10000;
  server_config.BBFILE=new char[MAX_LEN];
  server_config.DETACH = true;
  server_config.DEBUG_DELAY=false;
  strcpy(server_config.CONF_FILE,"bbserv.conf");

  server_config.T_MAX_IND=0;
  server_config.BP_IND=0;
  server_config.SP_IND=0;
  server_config.DETACH_IND = 0;
  server_config.DEBUG_DELAY_IND=0;
  server_config.PEER_COMMS_IND=0;
  server_config.BBFILE_IND=0;
  server_config.CONF_FILE_IND=0;

  server_config.FDETACH=true;
  server_config.CDETACH=true;

  welcome_message();          //Server info notice
  fetch_cmndLine(argc,argv);  //Fetch all command line inputs
  fetch_config();             //Fetch info from the Configuration File

  if(server_config.CDETACH==true && server_config.FDETACH==true)
  {
    server_config.DETACH=true;
  }
  else
  {
    server_config.DETACH=false;
  }

  print_config();             //Print Configurations

  // ... and we detach! // ~if {}~ REFERENCE : Code designed and inspired from Professor Stefan Bruda as part of CS564.
  if (server_config.DETACH)
  {
      // umask:
      umask(0177);

      // ignore SIGHUP, SIGINT, SIGQUIT, SIGTERM, SIGALRM, SIGSTOP:
      // (we do not need to do anything about SIGTSTP, SIGTTIN, SIGTTOU)
      // SIGHUP setup is done further down!.
      //SIGQUIT handled on the application function

      // signal(SIGQUIT, quit_handler);
      signal(SIGTERM, term_handler);
      signal(SIGCHLD , SIG_IGN);
      signal(SIGALRM, SIG_IGN);
      signal(SIGSTOP, SIG_IGN);
      signal(SIGKILL, SIG_IGN);
      signal(SIGPIPE, SIG_IGN);
      signal(SIGINT, SIG_IGN);
      signal(SIGABRT, SIG_IGN);

      // private group:
      setpgid(getpid(),0);

      // close everything (except the master socket) and then reopen what we need:
      for (int i = getdtablesize() - 1; i >= 0 ; i--)
              close(i);

      // stdin:
      int fd = open("/dev/null", O_RDONLY);

      // stdout:
      fd = open(logfile, O_WRONLY|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR);

      // stderr:
      dup(fd);

      // we detach:
      fd = open("/dev/tty",O_RDWR);
      ioctl(fd,TIOCNOTTY,0);
      close(fd);

      // become daemon:
      int pid = fork();
      if (pid < 0)
      {
          perror("fork");
          return 1;
      }

      if (pid > 0) return 0;  // parent dies peacefully


      // and now we are a daemon. Print info at the log file too..
      welcome_message();        //Server info notice
      print_config();           //Print Configurations

  }

  //handle the the SIGHUP
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sa.sa_handler = sighupHandler;
  if (sigaction(SIGHUP, &sa, NULL) == -1)
      perror("sigaction");

  while(1)
  {
    if(hupReceived==1)
    {
      server_config.T_MAX=20;
      server_config.BP=9000;
      server_config.SP=10000;
      server_config.BBFILE=new char[MAX_LEN];
      server_config.DETACH = true;
      server_config.DEBUG_DELAY=false;

      server_config.T_MAX_IND=0;
      server_config.BP_IND=0;
      server_config.SP_IND=0;
      server_config.DETACH_IND = 0;
      server_config.DEBUG_DELAY_IND=0;
      server_config.PEER_COMMS_IND=0;
      server_config.BBFILE_IND=0;
      server_config.CONF_FILE_IND=0;

      force_fetch_config();
      print_config();             //Print Configurations
    }

    pthread_t tt;
    pthread_attr_t ta;
    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta,PTHREAD_CREATE_DETACHED);

    //spawn a new thread. This thread will become the parent accept thread of the server.
    if(pthread_create(&tt,NULL,(void* (*)(void*))start_server,NULL)!=0)
    {
      cout<<"Peer thread error."<<endl;
    }
    pthread_detach(tt);

    //main thread waits until a signal interupt is done
    pthread_mutex_lock(&h_mutex);
      pthread_cond_wait(&h_cond,&h_mutex);
    pthread_mutex_unlock(&h_mutex);

    //kills the children thread
    quit_handler(0);

    //if it is SIGQUIT the server is Derezzed
    if(quitReceived)
    {
      break;
    }

    // if it is SIGHUP the server is re-spawn
  }

  snprintf(msg,MAX_LEN,"Server Derezzed.");
  logger(msg);

  return 0;
}
