/*
 * Part of the solution for Assignment 3, by Stefan Bruda.
 *
 * This files contains some common code for the two servers, the main
 * functions of the two listening threads, and the main function of
 * the program.
 */
#include <iostream>
#include "shfd.h"
#include "thrd_mgmt.h"
#include "peer_comms.h"


using namespace std;


/*
 * Log file
 */

const char* logfile = "shfd.log";
const char* pidfile = "shfd.pid";

/*
 * true iff the file server is alive (and kicking).
 */
bool falive;

/*
 * file server thread control variables
 */
team** file_team;
bool max_reached=false;
int exit_peace=0;

int fserv_cntrl=1;
int shserv_cntrl=1;
int fserv_msock;
int shserv_msock;

pthread_mutex_t logger_mutex;

extern char **environ;
int n_working=0;
int n_available=0;
pthread_mutex_t counter_mutex;


/*
 * What to debug (nothing by default):
 */
bool debugs[4] = {false, false, false,false};

int server_threads[4] = {0,0,0,1};

team* peer_ambassadors=NULL;
team* peer_receiver=NULL;
int no_of_peers;
peer_info_s* peer_info;
vote_read* vote_info;
int peer_port;

void logger(const char * msg)
{
    pthread_mutex_lock(&logger_mutex);
    time_t tt = time(0);
    char* ts = ctime(&tt);
    ts[strlen(ts) - 1] = '\0';
    printf("%s: %s", ts, msg);
    fflush(stdout);
    pthread_mutex_unlock(&logger_mutex);
}



/*
 * Simple conversion of IP addresses from unsigned int to dotted
 * notation.
 */
void ip_to_dotted(unsigned int ip, char* buffer)
{
    char* ipc = (char*)(&ip);
    sprintf(buffer, "%d.%d.%d.%d", ipc[0], ipc[1], ipc[2], ipc[3]);
}

int manage_file_server(int id)
{
  if(n_available==server_threads[T_INCR])
  {
    return 0;
  }
  else
  {
    int var = n_available - n_working;
    if((n_working+var)>server_threads[T_INCR])
    {
      int kill_count=0;
      for(int i=0;i<server_threads[T_MAX];i++)
      {
        if(kill_count==server_threads[T_INCR])
        {
          break;
        }
        else
        {
          if(is_working(file_team[i])==0&&is_available(file_team[i])>0)
          {
            file_team[i]->kill_all=true;
            kill_count++;
          }
        }
      }
    }
    else
    {
      return 0;
    }

  }
  return 0;
}

int pass_to_team(team* file_team, client_t* clnt)
{
      char msg[MAX_LEN];  // logger string

      if ( add_work_to_team(file_team,(void (*) (void*))file_client,(void*)clnt) != 0 )
      {
          terminate_team(file_team);

          snprintf(msg, MAX_LEN, "%s: file server cant accept clients: %s\n", __FILE__, strerror(errno));
          logger(msg);

          snprintf(msg, MAX_LEN, "%s: the file server died.\n", __FILE__);
          logger(msg);
          falive = false;
          return 0;
      }
      return 0;
}

int start_peer_receiver()
{
      char msg[MAX_LEN];  // logger string
      int* peerp = new int(peer_port);
      pthread_t tt;
      pthread_attr_t ta;
      pthread_attr_init(&ta);
      pthread_attr_setdetachstate(&ta,PTHREAD_CREATE_DETACHED);

      if ( pthread_create(&tt, &ta, (void* (*) (void*))peer_receiver_sub, (void*)peerp) != 0 )
      {
          snprintf(msg, MAX_LEN, "%s: shell server peer pthread_create: %s\n", __FILE__, strerror(errno));
          logger(msg);
          return 0;
      }

      return 0;
}

team* select_team()
{
  char msg[MAX_LEN];  // logger string

  if (debugs[DEBUG_THREAD])
  {
      snprintf(msg, MAX_LEN, "%s: Available threads : %d Working : %d Max Threads : %d\n", __FILE__, n_available,n_working,server_threads[T_MAX]);
      logger(msg);
  } /* DEBUG_COMM */

  for(int i=0;i<server_threads[T_MAX];i++)
  {
    if(file_team[i]==NULL)
    {
      continue;
    }
    if((is_available(file_team[i])==1))
    {
      if((is_working(file_team[i])==0))
      {
          if (debugs[DEBUG_THREAD])
          {
              snprintf(msg, MAX_LEN, "%s: New client will be handled by thread : %d\n", __FILE__, i);
              logger(msg);
          } /* DEBUG_COMM */

          return file_team[i];
      }
    }
  }

  return NULL;
}

void extend_teams()
{
  char msg[MAX_LEN];  // logger string

  if(n_available<server_threads[T_MAX])
  {
    int limit=n_available+server_threads[T_INCR];
    for(int i=n_available;i<limit;i++)
    {
      // printf("\nInitiating new team %d",i);
      file_team[i] = create_team(1);
    }
  }
  else
  {
    max_reached=true;
    if (debugs[DEBUG_THREAD])
    {
        snprintf(msg, MAX_LEN, "%s: Cannot preallocate more threads. Max reached.\n", __FILE__);
        logger(msg);
    } /* DEBUG_COMM */
  }

}


int next_arg(const char* line, char delim)
{
    int arg_index = 0;
    char msg[MAX_LEN];  // logger string

    // look for delimiter (or for the end of line, whichever happens first):
    while ( line[arg_index] != '\0' && line[arg_index] != delim)
        arg_index++;
    // if at the end of line, return -1 (no argument):
    if (line[arg_index] == '\0') {
        if (debugs[DEBUG_COMM])
        {
            snprintf(msg, MAX_LEN, "%s: next_arg(%s, %c): no argument\n", __FILE__, line ,delim);
            logger(msg);
        } /* DEBUG_COMM */
        return -1;
    }
    // we have the index of the delimiter, we need the index of the next
    // character:
    arg_index++;
    // empty argument = no argument...
    if (line[arg_index] == '\0') {
        if (debugs[DEBUG_COMM]) {
            snprintf(msg, MAX_LEN, "%s: next_arg(%s, %c): no argument\n", __FILE__, line ,delim);
            logger(msg);
        } /* DEBUG_COMM */
        return -1;
    }
    if (debugs[DEBUG_COMM]) {
        snprintf(msg, MAX_LEN, "%s: next_arg(%s, %c): split at %d\n", __FILE__, line ,delim, arg_index);
        logger(msg);
    } /* DEBUG_COMM */
    return arg_index;
}

void quit_handler(int sig)
{
  char msg[MAX_LEN];  // logger string

  if (debugs[DEBUG_THREAD])
  {
      snprintf(msg, MAX_LEN, "%s: SIGQUIT received. Shutting down server gracefuly.\n", __FILE__);
      logger(msg);
  } /* DEBUG_COMM */

  threads_keepalive=0;

  for(int i=0;i<server_threads[T_MAX];i++)
  {
    terminate_team(file_team[i]);
  }

  fserv_cntrl=0;
  shserv_cntrl=0;

  close(shserv_msock);
  close(fserv_msock);

  falive=false;
}

void hang_handler(int sig)
{
  char msg[MAX_LEN];  // logger string

  if (debugs[DEBUG_THREAD])
  {
      snprintf(msg, MAX_LEN, "%s: SIGHUP received. Restarting server gracefuly.\n", __FILE__);
      logger(msg);
  } /* DEBUG_COMM */

  threads_keepalive=0;

  for(int i=0;i<server_threads[T_MAX];i++)
  {
    terminate_team(file_team[i]);
  }

  for(int i=n_available;i<server_threads[T_INCR];i++)
  {
    file_team[i] = create_team(1);
  }

}

void* file_server (int msock)
{
    signal(SIGQUIT, quit_handler);
    signal(SIGHUP, hang_handler);
    char msg[MAX_LEN];  // logger string
    int ssock;                      // slave sockets
    struct sockaddr_in client_addr; // the address of the client...
    socklen_t client_addr_len = sizeof(client_addr); // ... and its length


    for(int i=0;i<no_of_peers;i++)
    {
      printf("Peer %d : IP is %s, Port is %d.\n",i,peer_info[i].ip,peer_info[i].port);
    }

    // Setting up the thread creation:

    for(int i=n_available;i<server_threads[T_INCR];i++)
    {
      file_team[i] = create_team(1);
    }

    /* now that we are ready to handle client lets prep the ambassadors */
    /* we create 3*T_INCR numbers of ambassador to handle external requests concurrently*/
    peer_ambassadors = create_team(3*server_threads[T_INCR]);
    start_peer_receiver();

    while (fserv_cntrl)
    {
        // Accept connection:
        ssock = accept(msock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (ssock < 0)
        {
            if (errno == EINTR) continue;
            snprintf(msg, MAX_LEN, "%s: file server accept: %s\n", __FILE__, strerror(errno));
            logger(msg);
            snprintf(msg, MAX_LEN, "%s: the file server died.\n", __FILE__);
            logger(msg);
            falive = false;
            return 0;
        }

        // assemble client coordinates (communication socket + IP)
        client_t* clnt = new client_t;
        clnt -> sd = ssock;
        ip_to_dotted(client_addr.sin_addr.s_addr, clnt -> ip);

        team* temp_team = select_team();

        // pass the client to the team
        if(temp_team!=NULL)
        {
                pass_to_team(temp_team,clnt);
                // go back and block on accept.
                // printf("Available : %d, Working : %d",n_available,n_working);
                if((n_working+1)==n_available)
                {
                  extend_teams();
                }
        }
        else
        {
                // if(!max_reached)
                // {
                //       team* temp_team2 = select_team();
                //       if(temp_team2!=NULL)
                //       {
                //               pass_to_team(temp_team2,clnt);
                //       }
                // }
                // else
                // {
                      max_reached=false;

                      snprintf(msg, MAX_LEN, "T MAX REACHED ON THREADS\n");
                      logger(msg);

                      snprintf(msg, MAX_LEN, "Sorry ! Server is busy at the moment.\n");
                      send(ssock,msg,strlen(msg),0);

                      shutdown(ssock, SHUT_RDWR);
                      close(ssock);
                // }

        }
        // go back and block on accept.
    }

    quit_handler(0);
    return 0;   // will never reach this anyway...
}

void* shell_server (int msock)
{
    int ssock;                      // slave sockets
    struct sockaddr_in client_addr; // the address of the client...
    socklen_t client_addr_len = sizeof(client_addr); // ... and its length

    // Setting up the thread creation:
    pthread_t tt;
    pthread_attr_t ta;
    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta,PTHREAD_CREATE_DETACHED);

    char msg[MAX_LEN];  // logger string

    // assemble client coordinates (communication socket + IP)
    client_t* clnt = new client_t;

    while (shserv_cntrl)
    {
        // Accept connection:
        ssock = accept(msock, (struct sockaddr*)&client_addr, &client_addr_len);
        clnt -> sd = ssock;
        ip_to_dotted(client_addr.sin_addr.s_addr, clnt -> ip);
        if (ssock < 0)
        {
            if (errno == EINTR) continue;
            snprintf(msg, MAX_LEN, "%s: shell server accept: %s\n", __FILE__, strerror(errno));
            logger(msg);
            return 0;
        }
        else if(ssock > 0)
        {
            if(clnt->nconnections>0 || strcmp(clnt->ip,"127.0.0.1")!=0)
            {
                snprintf(msg, MAX_LEN, "Shell Server accepts one client at a time. And only from local. !!\n");
                logger(msg);

                send(ssock, msg, strlen(msg),0);
                shutdown(ssock, SHUT_RDWR);
                close(ssock);
            }
            else
            {
                 // create a new thread for the incoming client:
                if ( pthread_create(&tt, &ta, (void* (*) (void*))shell_client, (void*)clnt) != 0 )
                {
                    snprintf(msg, MAX_LEN, "%s: shell server pthread_create: %s\n", __FILE__, strerror(errno));
                    logger(msg);
                    return 0;
                }
                // go back and block on accept.
            }
        }
        clnt -> nconnections = 1;
    }
    return 0;   // will never reach this anyway...
}


/*
 * Initializes the access control structures, fires up a thread that
 * handles the file server, and then does the standard job of the main
 * function in a multithreaded shell server.
 */
int main (int argc, char** argv, char** envp)
{

    int shport = 9001;                    // ports to listen to
    int fport = 9002;

    long int shsock, fsock;
                 // master sockets
    const int qlen = 32;                  // queue length for incoming connections
    char* progname = basename(argv[0]);   // informational use only.

    char msg[MAX_LEN];  // logger string

    pthread_mutex_init(&logger_mutex, 0);

    // parse command line
    extern char *optarg;
    extern int optind;
    int copt;
    bool detach = true;  // Detach by default
    while ((copt = getopt (argc,argv,"s:f:v:t:T:p:dD")) != -1) {
        switch ((char)copt) {
        case 'd':
            detach = false;
            break;
        case 'D':
            debugs[DEBUG_DELAY] = 1;
            printf("will delay file\n");
            break;
        case 'v':
            if (strcmp(optarg,"all") == 0)
                debugs[DEBUG_COMM] = debugs[DEBUG_FILE] = debugs[DEBUG_THREAD] = 1;
            else if (strcmp(optarg,"comm") == 0)
                debugs[DEBUG_COMM] = 1;
            else if (strcmp(optarg,"file") == 0)
                debugs[DEBUG_FILE] = 1;
            break;
        case 's':
            shport = atoi(optarg);
            break;
        case 'f':
            fport = atoi(optarg);
            break;
        case 't':
            server_threads[T_INCR]=atoi(optarg);
            break;
        case 'T':
            server_threads[T_MAX]=atoi(optarg);
            break;
        case 'p':
            peer_port=atoi(optarg);
            break;
        default:
            printf("This switch is unidentified : %s",optarg);
            break;
        }
    }

    argc -= optind - 1; argv += optind - 1;

    peer_info=new peer_info_s[argc-1];
    no_of_peers=argc-1;

    if (no_of_peers <= 0 && peer_port>0)
    {
        printf("Usage: %s  [-d] [-D] [-v all|file|comm] [-s port] [-f port] [-t t_incr] [-T t_max] [-p sync_port] [peers].\n", progname);
        printf("Specific, please provide peers since peer communiator was activated using -p.\n");
        printf("Peer name format -> ip:port\n");
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        char *sep=strtok(argv[i],":");
        sprintf(peer_info[i-1].ip,"%s",sep);

        sep = strtok (NULL, ":");
        peer_info[i-1].port=atoi(sep);
        int xx = i-1;
        peer_info[i-1].id=xx;
    }

    if (shport <= 0 || fport <= 0 || peer_port<=0 || server_threads[T_INCR]<=0 || server_threads[T_MAX]<=0)
    {
        printf("Usage: %s  [-d] [-D] [-v all|file|comm] [-s port] [-f port] [-t t_incr] [-T t_max] [-p sync_port] [peers].\n", progname);
        return 1;
    }

    if(server_threads[T_MAX]>0)
    {
      file_team=new team*[server_threads[T_MAX]];
    }
    else
    {
      printf("T_MAX was not specified using -T switch, T_MAX set to 100.\n");
      file_team=new team*[100];
    }

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

    // Initialize the file locking structure:
    flocks_size = getdtablesize();
    flocks = new rwexcl_t*[flocks_size];
    for (size_t i = 0; i < flocks_size; i++)
        flocks[i] = 0;

    // Open the master sockets (this is the startup code, since we
    // might not have permissions to open this socket for some reason
    // or another, case in which the startup fails):

    close(shport);
    shsock = passivesocket(shport,qlen);
    if (shsock < 0)
    {
        perror("shell server passivesocket");
        return 1;
    }
    printf("Shell server up and listening on port %d\n", shport);

    close(fport);
    fsock = passivesocket(fport,qlen);
    if (fsock < 0)
    {
        perror("file server passivesocket");
        return 1;
    }
    printf("File server up and listening on port %d\n", fport);

    shserv_msock=shsock;
    fserv_msock=fsock;

    // ... and we detach!
    if (detach)
    {
        // umask:
        umask(0177);

        // ignore SIGHUP, SIGINT, SIGQUIT, SIGTERM, SIGALRM, SIGSTOP:
        // (we do not need to do anything about SIGTSTP, SIGTTIN, SIGTTOU)
        signal(SIGHUP,  SIG_IGN);
        signal(SIGINT,  SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);
        signal(SIGALRM, SIG_IGN);
        signal(SIGSTOP, SIG_IGN);

        // private group:
        setpgid(getpid(),0);

        // close everything (except the master socket) and then reopen what we need:
        for (int i = getdtablesize() - 1; i >= 0 ; i--)
            if (i != shsock && i != fsock)
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

        // and now we are a real server.
    }

    // Setting up the thread creation:
    pthread_t tt;
    pthread_attr_t ta;
    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta,PTHREAD_CREATE_DETACHED);

    // Launch the thread that becomes a file server:
    if ( pthread_create(&tt, &ta, (void* (*) (void*))file_server, (void*)fsock) != 0 )
    {
        snprintf(msg, MAX_LEN, "%s: pthread_create: %s\n", __FILE__, strerror(errno));
        logger(msg);
        return 1;
    }
    falive = true;

    // Continue and become the shell server:
    shell_server(shsock);

    // If we get this far the shell server has died
    snprintf(msg, MAX_LEN, "%s: the shell server died.\n", __FILE__);
    logger(msg);
    // keep this thread alive for the file server
    while (falive)
    {
        sleep(30);
    }

    snprintf(msg, MAX_LEN, "%s: all the servers died, exiting.\n", __FILE__);
    logger(msg);

    return 1;
}
