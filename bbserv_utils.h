#ifndef __BBSERV_UTILS_H
#define __BBSERV_UTILS_H


#include <iostream>
#include <ctime>


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>


#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "tcp-utils.h"
#include "tokenize.h"
#include "descriptor.h"
#include "linkedList.h"


using namespace std;

//Global holder of the servers configuration
typedef struct sconfdescriptor
{
  int T_MAX;
  int T_MAX_IND;

  int BP;
  int BP_IND;

  int SP;
  int SP_IND;

  int NO_OF_PEERS;
  int PEER_COMMS_IND;

  bool DETACH;
  int DETACH_IND;

  bool DEBUG_DELAY;
  int DEBUG_DELAY_IND;

  bool SERV_REST;

  char* BBFILE;
  int BBFILE_IND;

  char CONF_FILE[50];
  int CONF_FILE_IND;

  size_t master_socket;
  size_t peer_listener_master_socket;

}sconf;

//STRUCT client_t : used to pass information about a client to the client handler function from server handler
struct client_t {
    int sd;    // the communication socket
    char ip[20];   // the (dotted) IP address
};

//STRUCT client_info : used to pass information about a client to the client handler function from server handler
struct client_info {
  long int id;
  char ip[20];
  bool uname_avl;
  char *user;
};

//refer the variable created below for more info
struct peer_info_s
{
    int id;
    int port;    // the communication socket
    char ip[20];   // the (dotted) IP address
};

//STRUCT line : Used to ease peer communication data transfers
typedef struct line
{
  char* msg_nmbr;
  char* poster;
  char* message;
}line;

//STRUCT peer_stat_address : Used for peer communication purpose for individual handling
struct peer_stat_address
{
  long int port;
  char ip[20];
  bool marked;
  size_t peer_port;
  bool dont_reissue;
};

//STRUCT peer_stat : Used for peer communication purpose
struct peer_stat
{
  int pos_count;
  int neg_count;
  line* line_a;
  peer_stat_address* peer_addr;
  pthread_mutex_t access_peer_stat;
  pthread_cond_t peer_stat_cond;
  bool writer;
  bool replacer;
};

extern lList* clients_list; //A linked list maintaining the slave sockets that are opened for communication with the clients

extern sconf server_config; //Global holder of server's config

extern pthread_mutex_t logger_mutex; // The logger function is thread unsafe, hence it uses a lock.

extern peer_info_s* peer_info;//Global holder of information about the server's peers.

extern bool bealive;//if set to false , server will die on the next ~accept~ interupt or port interupt.


const size_t MAX_LEN = 1024;
const size_t BBLINE_LEN = 3*1024+3;


void welcome_message();
//Prints a welcome message... !any server initiation setup can be added to this

void logger(const char * msg);
//A simple logger function . REFERENCE : Code designed and inspired from Professor Stefan Bruda as part of CS564.

void fetch_cmndLine(int argc, char** argv);
//As per the name fetches the command line arguments and feeds it to the server_config struct.

void fetch_config();
//As per the name fetches the configuration file settings and feeds it to the server_config struct.
//It also handles precidence with the command line arguments. If a setting is already fetched as part of command line it is ignored.
//except if any exclusions.

void force_fetch_config();
//Does the same as fetch_config, except force push the setting to server_config config.
//Used as part of server restart on SIGHUP

void print_config();
//A logger function which summarizes server_config as part of server initiation.

struct line* new_line(char msg_nmbr[MAX_LEN], char poster[MAX_LEN], char message[MAX_LEN]);
//Returns a new line to bbfile in ~line~ struct format. Used as part of peer communication to ease data transfer between functions.

void ip_to_dotted(unsigned int ip, char* buffer);
//Edits the ip address to dotted decimal format.REFERENCE : Code designed and inspired from Professor Stefan Bruda as part of CS564.

#endif /* __BBSERV_UTILS_H */
