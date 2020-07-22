Bhuvaneshwaran Ravi,Jayashree Srinivasan,Kameswaran Rangasamy,Serlin Tamilselvam
bravi19@ubishops.ca,jsrinivasan19@ubishops.ca,krangasamy19@ubishops.ca,stamilselvam19@ubishops.ca

Bulletin Board Server
---------------------
A classic multi-threaded, peer enabled BBS.

References :
------------
	[1] Course material and utility packages from Professor Stefan Bruda as part of CS564,
	    Computer Networks/Computer Networks and Distributed Algorithms (Winter 2020)
	[2] Concept reference - The Linux Programming Interface by Michael Kerrisk.
	[3] Course Work done as part of CS564.


Root Directory Content:
-----------------------

0) Module "bbserv_utils"    	:	The mother module which serves utility functions to basic features like config file loading, command line fetch,
  															information maintenance (peer info, current client etc..)
																-> contains a subroutine ip_to_dotted and data structure reference from [1]
																[bbserv_utils.h,bbserv_utils.cc]

1) bbfile.txt               	: The default bulletin board file where all the BBS messages are stored and managed.
2) bbserv.cc                	: The MAIN Module, where the program flow starts.
																-> conceptual reference from [2] and [3].

3) Module "descriptor"      	: All file operations are embedded into a single library. It also provides the file concurrent access system.
																[descriptor.h,descriptor.cc]

4) Module "linkedList"      	: A simple linked list to maintain the slave client sockets that are current being served to.  
																It is mainly used to ease of access to sockets in case of SIGHUP or SIGQUIT issue.
																[linkedList.h,linkedList.cc]

5) Module "peer_operations" 	: Subroutines which communicates to the peer servers and also keeps mother server informed.
 																This library only provides the subroutines which are used to receive request "from" the peers and respond to their requests.
																All the subroutines which are used to communicate "to" the peers are embedded in server_operations.
																However, this library help initiate and run a seperate group of threads which are used to communicate to the peers concurrently.
																[peer_operations.h,peer_operations.cc]
																
																1 mother thread will spawn and initiate all the peer controls and die.
																2 * T_MAX threads are spawn for the purpose of sending request to the peers("Peer receiver").
																2 * T_MAX threads are spawn for the purpose of serving to peer request("Peer senders").
																Hence at any time there will be 3 + T_MAX + 4 * T_MAX threads running on the process.
																The constant 3 threads controls the main loops.
																ex : T_MAX of 5, yields 28 threads.

6) Module "thrd_mgmt"  			  : Subroutines which makes life easier handling the thread preallocation and management.
														    The thread management process has been inspired from using the Agile Scrum work methodology.
														    There are teams. Teams contains employees and a work queue.
															  The employess constantly wait for a work queue and work on them whenever available.
															  Built as part of Course work [3] and edited to suit the current requirement.
																[thrd_mgmt.h,thrd_mgmt.cc]

7) Module "server_operations" : Subroutines that are threaded to interact with the client and that are threaded to sync client requests to the peers.
																Subsroutines are explained as part of protocol2pc.txt .
																[server_operations.h,server_operations.cc]

8) Module "tcp-utils"  			  :	Contains the utilities for network communication and more.
															  Module is a part of the course materials by Professor Stefan Bruda [1].

9) Module "tokenize"          : A string tokenizer module. Module is a part of the course materials by Dr. Stefan Bruda.
			   												This modules produces object files upon make.

10) bbserv.log,bbserv.pid..   : Several log files that will be created as part of program initiation.
																There will also be a cache files as part of SIGHUP.

User guide:
-----------

The following command line is accepted:

  bbserv [-d] [-f] [-c config_file] [-T T_MAX] [-b bbfile] [-p client_serve_port] [-s peer_serve_port] [peer0_IP:peer0_port] ...

the options can be given in any order, except the command line arguments which should always be given at the end.

The server can handle any type of client (telnet is preferred).

SIGQUIT can be issued to kill the server ensuring proper routine and data maintenance.

SIGHUP can be issue to restart the server ensuring proper routine and data maintenance.
SIGHUP also clears garbage that has been produced as part of "replace" command in the bbfile.

"#" or "any other character" in the beggining of a line in "bbserv.conf" will make the line commented.

If "-f" is not issued, logs are written to "bbserv.log".


Test Suite :
------------

					1. On successful client-server connection establishment, the client receives "0.0 greeting" from the server.

			USER Command:
			-------------

					2. "USER name" makes the server assumes the username as "name". "name" should not contain "/".
					3. If the USER command is not issued then the user is "nobody".
					4. The server must reply "1.0 HELLO name text".

			READ Command:
			-------------

					3. "READ msg_nmbr" is the format and nothing else is accepted.
					4. If exists server replies with "1.0 HELLO name text"
					5. If message does not exists server replies "2.1 UNKNOWN message-number text".
					6. If internal error, i.e bbfile does not exists then server replies "2.2 ERROR READ text".

					READ wit -d [DEBUG enabled]:
					----------------------------

					7. Upon issue of "READ" ,
						-> A Log , denoting the start of the read wait of 3 seconds is issued.
						-> Program halts for 3 seconds.
						-> Program executes READ.
						-> A Log, denoting the end of the read wait of 6 seconds is issue.


			WRITE Command:
			--------------

					Single Peered:
					--------------

					8.  "WRITE message" is the format and nothing else is accepted.
					9.  "message" can contain spaced content and special characters.
					10.  In case file not found or system error then "3.2 ERROR WRITE text" is issued.

					Multi-Peered :
					-------------

					11.  In case file not found on local or on peer then "3.2 ERROR WRITE text" is issued.
					12.  If peer issues error to write then "3.2 ERROR WRITE text" is issued.
					13.  In case peer not found then "3.2 ERROR WRITE text" is issued.
					14.  On successfull write then content is written to local and peers.


					Single Peered with "-d" [DEBUG enabled]:
					----------------------------------------

					15. Upon issue of "WRITE" ,
						-> A Log , denoting the start of the write wait of 6 seconds is issued.
						-> Program halts for 6 seconds.
						-> Program executes WRITE.
						-> A Log, denoting the end of the write wait of 6 seconds is issue.

					Multi Peered with "-d" [DEBUG enabled]:
					----------------------------------------

					16. Upon issue of "WRITE",
						-> The server should print all messages communicated to and from the peers.
						-> If all success , then [14] will proceed.
						-> If peer failure encountered, "3.2 ERROR WRITE text" is issued.

			REPLACE Command:
			----------------

					Single Peered:
					--------------

					17.  "REPLACE message-number/message" is the format and nothing else is accepted.
					18.  "message" can contain spaced content and special characters.
					19.  In case file not found or system error then "3.2 ERROR REPLACE text" is issued.

					Multi-Peered :
					--------------

					20.  In case file not found on local or on peer then "3.2 ERROR REPLACE text" is issued.
					21.  If peer issues error to write then "3.2 ERROR REPLACE text" is issued.
					22.  In case peer not found then "3.2 ERROR REPLACE text" is issued.
					23.  On successfull write then content is written to local and peers.


					Single Peered with "-d" [DEBUG enabled]:
					----------------------------------------

					24. Upon issue of "REPLACE" ,
						-> A Log , denoting the start of the write wait of 6 seconds is issued.
						-> Program halts for 6 seconds.
						-> Program executes REPLACE.
						-> A Log, denoting the end of the write wait of 6 seconds is issue.

			QUIT Command:
			-------------

					25. Upon the issue of QUIT, server replies with "4.0 BYE some-text" and closes the connection.
					26. Any type of connection interupt will have "4.0 BYE some-text" issued.


			SYSTEM Feature test :
			---------------------

			-> A max of only T_MAX client only can be connected to the server, incase of overflow the clients are put on wait queue.

			-> If bulletin file does not exists upon server initiation, it is created.
			-> If bulletin file exists upon server initiation it is used as is.
			-> New message numbers should not exists already.[generate_number function]

			-> If multiple reads and a write is issued at the same time, where any one of the read is issued before the write, then.
			   The server allows all the "reads" to be executed and then executes the write.
			-> If write is issued before reads, any number reads should wait before until write is completed.

			-> SIGQUIT kills the server.
			-> SIGHUP restarts the server immediately, but uses the config from configuration file and clears garbage from bbfile.
