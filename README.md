*Please make and run.*

"#" or any other character in the beggining of a line in "bbserv.conf" will make the line commented.

If "-f" is not issued, logs are written to "bbserv.log".

Command to run :
----------------
./bbserv -d -c bbserv.conf -T 5 -b bbfile.txt -p 9002 -s 10002 -f


Current Bugs:
-------------

-> The DAEMON flag must be enabled only if it is set to true both on config file and command line.
	* The funciton fetch_cmndLine and fetch_config must be fixed as required.

-> The "name" in USER command should not accept "\".
		(Fixed)

-> On SIGHUP, the server should not wait for all clients to get disconnected. It should inform the client threads to quit after current command or immediately if idle.
		(Fixed)

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
			-> SIGHUP restarts the server immediately, but uses the config from configuration file.
