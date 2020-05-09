AUTHORS:
--------
	Name     -> Kameswaran Rangasamy 
	Stud. id -> 002262216  
	Email.id -> krangasamy19@ubishops.ca

	Name     -> Sarthak Chhabra 
	Stud. id -> 002260431  
	Email.id -> schhabra19@ubishops.ca

	Name     -> Lokender Sarna 
	Stud. id -> 002270933 
	Email.id -> lsarna19@ubishops.ca

	Name     -> Yukuan Hao
	Stud. id -> 002253251
	Email.id -> yhao18@ubishops.ca


Root Directory Content:
-----------------------

(The assignment was built with Professor Stefan Bruda's solution to CN-Assignment 3 as base code. )

0) shfd.h 	       : Header file to everything.
1) misc.cc             : Contains the main skeleton of the assignment. The file server and the shell server are hooked to this code via shfd.h .
2) fserv.cc            : Source code for the file server subroutines. 
3) shshell.cc          : Source code for the shell server subroutines.
4) shfd.log	       : Log file to which stdout and stderr are redirected incase if the server is spirited as daemon. 

5) Module "thrd_mgmt"  : Subroutines which makes life easier handling the thread preallocation and management.
			 The thread management process has been inspired from using the Agile Scrum work methodology.
			 There are teams. Teams contains employees and a work queue. The employess constantly wait for a work queue and work on them 
			 whenver available. 
			 We have considered, for this project, that each team will have only one memeber. However the server can be altered to be more 				 concurrent. 

6) Module "peer_comms" : Subroutines which communicates to the peer servers and also keeps mother server informed. (Further explained below)

7) Module "tcp-utils"  : Contains the utilities for network communication and more. Module is a part of the course materials by Dr. Stefan Bruda.
			 This modules produces object files upon make.

8) Module "tokenize"   : A string tokenizer module. Module is a part of the course materials by Dr. Stefan Bruda.
			 This modules produces object files upon make.

User guide:
-----------

The following command line is accepted:

  shfd [-d] [-D] [-v all|file|comm] [-f port] [-s port] [-t t_incr value] [-T t_max value] [-s peer_comms_port] [peer0_IP:peer0_port] ... 

where

  -f  overrides the default file server port
  -s  overrides the default shell server port
  -d  does not detach (debugging mode)
  -D  delays read and write operations (see report for rationale)
  -v  comm: prints out (to stdout) messages related to communication
            events
      file: prints out (to stdout) messages related to file access
      all:  `-v comm' and `-v file' together plus some new additonal thread management information.

  -t  t_incr value : indicates the number of threads that will be pre allocated once the server is initiated 
	and the further allocation incrementations when the situation arises to spawn new thread, 
	also the values indicates the limit that the no. of threads die together when idle together for a minute 

  -T  t_max value  : indicates the number of threads that can be allocated maximum for client interaction.

  -s  peer_comms_port : indicates the port through which the mother server communicates with all its peers.

  -> command line argument -> [peer0_IP:peer0_port] ...
	These argument specify the ip and port of single or multiple peers.


the options can be given in any order except the command line arguments which should always be given at the end.

The server can handle any type of client.

The Peer communication protocol:
--------------------------------

The communication between peer servers will be managed by a seperate group of threads called the "Ambassador threads".
There are two types of ambassador threads. The Peer Listener and the Peer Informers.

The peer listener is a single thread that listens to port specified to the "-p" switch. It almost mimics the file_server functionalities.
Except that it does not reply to the peers to any other than the "fread" command. The peer listener upon receipt of read commands reads data from the specified file and informs the peer which has requested it for the information. The peer listener is a sequential process. One peer at a time can accept connections only from one peer.

The peer informers are a group of threads. The group is set to consists of 3*t_incr threads. Since the server is capable of serving multiple clients at the same only one peer informer thread does not provide a meaningfull support to the peer informing service and hence concurrent.
The peer informers are invoked immediately after the file server is started, so is the peer listener along with the preallocation of the first batch of t_incr threads. The peer informers just pass the information and conveyed to them to the peer they had been said to inform the information.

The peer informers group of threads are invoked during the start of the file server, they can work in two mode. 
The inform mode and wait_for_reply mode. Where as the name suggests they wait for the reply in the later case . The wait_for_reply mode can be used only by the code that serves the "fread" command.


Features restrictions to the shell server:
-------------------------------------------

The shell server unlike the previous versions can now server only one client at a time and can only server clients that from "localhost".


Feature augmentation to the file server :
-----------------------------------------

	-> The file server unlike the previous version does not spawn a new thread whenver a new client has to be server. 
	   Instead the server will preallocate a new batch of t_incr whenever it does not have any more idle threads ready to server client, unless 
	   until the preallocation does not exceed t_max.

	-> Feature inclusion to commands except FREAD:
	   -------------------------------------------
	   The file server now whenever it receives a request of type that is not FREAD will send the commands to its peers.
	   The server replies the client with the output of the request concurrently.

	-> Feature inclusion to command FREAD:
	   ----------------------------------
	   The file server now, whenever it receives a request of type FREAD, will send the commands to its peers and wait for replies from them.
	   If a peer doesnt reply within a stipulated amount of time it considers the peer communication as a failures. After that the server does a 		   voting on the replies and its own output to the command and issue the winner to the client.

Signal handling
---------------

The server ignores most signals. Except mainly for two signals SIGQUIT and SIGHUP.

	SIGQUIT:
	--------

	The server terminates gracefully when it receives a SIGQUIT. It issues a terminate request to all the threads that are alive. 
	The threads, if they are idle quits immediately. If they are serving a client they wait until the client leaves and dies. 
	The server waits until all its threads are dead and terminates itself.

	SIGHUP:
	-------

	In case of SIGHUP receipt the server issues termination to all threads as it does when it receives a SIGQUIT. Once all the threads are dead, 		the server it now put in a state similar to where the server is switched off but does not terminate. The server then immediately does all the 		preallocation routines and is available for client interaction.


TEST SUITE
----------

-> Shell server tests:
   ------------------

	1. Shell server should accepts only one client and only from localost.
		-> Expectation met. The shell server accepts only one client and only from localhost.

	2. All existing features should work without any new bugs.
		-> Expectation met. Shell server existing features behaves the same.

-> File server tests:
   ------------------

	1. FOPEN on file server.
		-> Upon receipt of an fopen the server passes the command to its peers. The mother server (the one which serves the client) opens the 			   file as expected and replies the client with the file identifier. The peers are also opening the file perfectly and keeps the     			   reply to themselves.

	2. FWRITE on file server.
		-> Upon receipt of an fwrite the mother server writes the content to the file in case of no impediments. 
  		   If in case of any improper input values on FWRITE the mother server informs the client of the error. 
		   However after a successfull write the mother server alters the command and passes it to the peers. 
		   By alter it replaces with file identifier with the file name.
	           The peer server identifies the file by its name, finds the corresponding file identifier and writes it to file correctly.U
	
	3. FSEEK on file server.
	 	-> FSEEK moves the point to said locations from the current one. The action reflects successfull on both the mother server and peers.

	4. FCLOSE on file server.
		-> FCLOSE closes the file on both the mother and peer servers.

	5. FREAD on file server.
		-> file server on FREAD reads specified bytes of information from the mentioned file in the mother server and all the peer servers.
		   The FREAD output from all the servers are collected into a single pool. 
		   A voting is done on which is the most common reply and a winner is selected amonngst them. 
		   The output which has the most vote is replied back to the client as the output of the command request.

-> Thread management tests:
   ------------------------

	1. Pre-allocate t_incr threads when the server is initiated.
		-> behaviour respected. Server allocated t_incr threads on startup.

	2. Pre allocate t_incr threads when the server identifies that there are no preallocated threads for the next requested.
		-> behaviour respected. Server allocated t_incr threads when there are no preallocated threads for the next requested.

	3. Stop pre-allocation when no_of_threads active reaches t_max and proceed closure on the new client.
		-> behaviour respected. Server did not allocate more than t_max threads and gave closure to the client.

	4. t_incr threads, if idle for 1 minute , quits. and the next t_incr quits 1 min after the first batch of quits.
		-> behaviour respected. Server kills threads at the rate of t_incr for every 1 min.

	5. t_incr threads are always available when there are 0 client interaction with the server.
		-> behaviour respected. Server made sure that atleast one t_incr batch of threads are always preallocated and available for client.

-> Signal Handling tests:
   ----------------------

	1. On receipt of SIGHUP
		-> the server issued termination to all threads. Did not accept any new connection. Once all clients left server restarted and was 			   open for client service. Expected behaviour. Test case success.
	2. On receipt of SIGQUIT
		-> the server issued termination to all threads. Did not accept any new connection. Once alll clients left the server terminated.

--> Base code testing
    -----------------

	All test case from the previous assignments were done to ensure that the new functionalities did not affect 
	the functionalities built as part of previous versions.

--> UNIT TESTING
    ------------
	
	Apart from the final testing of the end product each new functionality was unit tested during the development.


		 

	





