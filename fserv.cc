/*
 * Part of the solution for Assignment 3, by Stefan Bruda.
 *
 * This file contains the code for the file server.
 */

#include "shfd.h"
#include "peer_comms.h"

/*
 * The access control structure for the opened files (initialized in
 * the main function), and its size.
 */
rwexcl_t** flocks;
size_t flocks_size;

/*
 * Handles the FOPEN command.
 *
 * Allocates the access control structure, initializes the mutex and
 * condition for the file given as argument, and opens the file for
 * both reading and writing.  Returns the file descriptor or a
 * negative number in case of failure.
 */
int file_init (char* filename,char* abs_filename)
{


    char msg[MAX_LEN];  // logger string
    snprintf(msg, MAX_LEN, "~~%s~~: attempting to open ~~%s~~\n", __FILE__, filename);
    logger(msg);

    // at this file should be guaranteed not to be opened already
    trimTrailingSpaces(filename);
    int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if ( fd == -1 ) {
        snprintf(msg, MAX_LEN, "%s: open error: %s\n", __FILE__, strerror(errno));
        logger(msg);
        return -1;
    }

    // now that we have a file we allocate and initialize the access
    // control structure:
    rwexcl_t* lck = new rwexcl_t;
    pthread_mutex_init(&lck -> mutex, 0);
    pthread_cond_init(&lck -> can_write,0);
    lck -> reads = 0;
    lck -> fd = fd;
    lck -> owners = 1;
    lck -> name = new char[strlen(filename) + 1];
    strcpy(lck -> name, filename);

    lck -> abs_name = new char[strlen(abs_filename) + 1];
    trimTrailingSpaces(abs_filename);
    strcpy(lck -> abs_name, abs_filename);

    flocks[fd] = lck;
    snprintf(msg, MAX_LEN, "%s: %s opened on descriptor %d\n", __FILE__, filename, fd);
    logger(msg);
    return fd;
}

/*
 * Handles the FCLOSE command.
 *
 * Physically closes the file iff the file has only one owner.
 */
int file_exit (int fd)
{

    char msg[MAX_LEN];  // logger string
    snprintf(msg, MAX_LEN, "%s: attempting to close descriptor %d\n", __FILE__, fd);
    logger(msg);

    if (flocks[fd] == 0)
        return err_nofile;

    // close is a major event, we treat it as a write.
    // so we wait for everybody to finish working with the file
    pthread_mutex_lock(&flocks[fd] -> mutex);
    while (flocks[fd] -> reads != 0) {
        // release the mutex while waiting...
        pthread_cond_wait(&flocks[fd] -> can_write, &flocks[fd] -> mutex);
    }
    // now we have the mutex _and_ the condition, so we process the
    // close event

    flocks[fd] -> owners --;
    if ( flocks[fd] -> owners != 0) {
        snprintf(msg, MAX_LEN, "%s: descriptor %d owned by other clients\n", __FILE__, fd);
        logger(msg);
        // this might have released the file for access, so we
        // broadcast the condition to unlock whatever process happens
        // to wait for us.
        pthread_cond_broadcast(&flocks[fd] -> can_write);
        pthread_mutex_unlock(&flocks[fd] -> mutex);
        return 0;
    }

    // no other client is accessing this file, we destroy the
    // descriptor
    //
    // char dummy[1];
    // dummy=NULL;

    char cmd[10];
    char file_iden[50];
    char file_content[1025];
    file_content[1024] = '\0';

    strcpy(cmd,"FCLOSE");
    strcpy(file_iden,flocks[fd]->abs_name);
    sprintf(file_content," ");

    sync_non_read(cmd,file_iden,file_content); //Sync with peers


    int closed = close(flocks[fd] -> fd);
    delete[] flocks[fd] -> name;
    delete flocks[fd];
    flocks[fd] = 0;
    snprintf(msg, MAX_LEN, "%s: descriptor %d closed\n", __FILE__, fd);
    logger(msg);

    // Note: mutex destroyed, no need to unlock
    return closed;
}

/*
 * Handles the FWRITE command.
 *
 * Ensures exclusive access to given file descriptor using the
 * associated rwexcl_t structure, and writes `stuff_length' bytes from
 * `stuff'.  Returns the number of bytes actually written,
 * `err_nofile' if the file descriptor is invalid, and -1 on error
 * conditions.
 */
int write_excl(int fd, const char* stuff, size_t stuff_length)
{


    int result;
    char msg[MAX_LEN];  // logger string

    snprintf(msg, MAX_LEN, "%s: Inside write function.\n", __FILE__);
    logger(msg);

    if (flocks[fd] == 0)
        return err_nofile;

    // The fact that only one thread writes at any given time is ensured
    // by the fact that the successful thread does not release the mutex
    // until the writing process is done.
    snprintf(msg, MAX_LEN, "%s:Attainning lock......\n", __FILE__);
    logger(msg);

    pthread_mutex_lock(&flocks[fd] -> mutex);

    snprintf(msg, MAX_LEN, "%s: Lock obtained.\n", __FILE__);
    logger(msg);
    // we wait for condition as long as somebody is doing things with
    // the file
    while (flocks[fd] -> reads != 0) {
        // release the mutex while waiting...
        snprintf(msg, MAX_LEN, "%s: Gonna wait until 0 reads.\n", __FILE__);
        logger(msg);
        pthread_cond_wait(&flocks[fd] -> can_write, &flocks[fd] -> mutex);
    }
    // now we have the mutex _and_ the condition, so we write

    if (debugs[DEBUG_DELAY]) {
        // ******************** TEST CODE ********************
        // bring the write process to a crawl to figure out whether we
        // implement the concurrent access correctly.
        snprintf(msg, MAX_LEN, "%s: debug write delay 5 seconds begins\n", __FILE__);
        logger(msg);
        sleep(5);
        snprintf(msg, MAX_LEN, "%s: debug write delay 5 seconds ends\n", __FILE__);
        logger(msg);
        // ******************** TEST CODE DONE ***************
    } /* DEBUG_DELAY */

    result = write(fd,stuff,stuff_length);
    if (result == -1) {
        snprintf(msg, MAX_LEN, "%s: write error: %s\n", __FILE__, strerror(errno));
        logger(msg);
    }

    // done writing.
    // this might have released the file for access, so we broadcast the
    // condition to unlock whatever process happens to wait for us.
    pthread_cond_broadcast(&flocks[fd] -> can_write);
    // we are done!
    pthread_mutex_unlock(&flocks[fd] -> mutex);
    return result;  // the count of bytes written
}

/*
 * Handles the FSEEK command.
 *
 * A seek affects them all, so we use the same mutual exclusion scheme
 * like the one for the write operation.  Other than this, the
 * function is just a wrapper for lseek.
 */
int seek_excl(int fd, off_t offset)
{


    int result;
    char msg[MAX_LEN];  // logger string

    if (flocks[fd] == 0)
        return err_nofile;

    if (debugs[DEBUG_FILE]) {
        snprintf(msg, MAX_LEN, "%s: seek to %d into descriptor %d\n", __FILE__, (int)offset, fd);
        logger(msg);
    }

    // The fact that only one thread writes at any given time is ensured
    // by the fact that the successful thread does not release the mutex
    // until the seek process is done.
    pthread_mutex_lock(&flocks[fd] -> mutex);
    // we wait for condition as long as somebody is doing things with
    // the file
    while (flocks[fd] -> reads != 0) {
        // release the mutex while waiting...
        pthread_cond_wait(&flocks[fd] -> can_write, &flocks[fd] -> mutex);
    }
    // now we have the mutex _and_ the condition, so we change the offset

    result = lseek(fd, offset, SEEK_CUR);
    if (result == -1) {
        snprintf(msg, MAX_LEN, "%s: lseek error: %s\n", __FILE__, strerror(errno));
        logger(msg);
    }

    // this might have released the file for access, so we broadcast the
    // condition to unlock whatever process happens to wait for us.
    pthread_cond_broadcast(&flocks[fd] -> can_write);
    // we are done!
    pthread_mutex_unlock(&flocks[fd] -> mutex);
    return result;  // the new offset
}

/*
 * Handles the FREAD command.
 *
 * Waits based on the associated rwexcl_t structure until no write
 * operation happens on the file descriptor fd, and then reads
 * `stuff_length' bytes putting them into `stuff'.
 */
int read_excl(int fd, char* stuff, size_t stuff_length) {
    int result;
    char msg[MAX_LEN];  // logger string

    if (flocks[fd] == 0)
        return err_nofile;

    if (debugs[DEBUG_FILE]) {
        snprintf(msg, MAX_LEN, "%s: read %lu bytes from descriptor %d\n", __FILE__, stuff_length, fd);
        logger(msg);
    }

    pthread_mutex_lock(&flocks[fd] -> mutex);
    // We increment the number of concurrent reads, so that a write
    // request knows to wait after us.
    flocks[fd] -> reads ++;
    pthread_mutex_unlock(&flocks[fd] -> mutex);

    // now we have the condition set, so we read (we do not need the
    // mutex, concurrent reads are fine):

    if (debugs[DEBUG_DELAY]) {
        // ******************** TEST CODE ********************
        // bring the read process to a crawl to figure out whether we
        // implement the concurrent access correctly.
        snprintf(msg, MAX_LEN, "%s: debug read delay 20 seconds begins\n", __FILE__);
        logger(msg);
        sleep(20);
        snprintf(msg, MAX_LEN, "%s: debug read delay 20 seconds ends\n", __FILE__);
        logger(msg);
        // ******************** TEST CODE DONE ***************
    } /* DEBUG_DELAY */

    result = read(fd, stuff, stuff_length);

    // we are done with the file, we first signal the condition
    // variable and then we return.

    pthread_mutex_lock(&flocks[fd] -> mutex);
    flocks[fd] -> reads --;
    // this might have released the file for write access, so we
    // broadcast the condition to unlock whatever writing process
    // happens to wait after us.
    if (flocks[fd] -> reads == 0)
        pthread_cond_broadcast(&flocks[fd] -> can_write);
    pthread_mutex_unlock(&flocks[fd] -> mutex);

    return result;
}

/*
 * Client handler for the file server.  Receives the descriptor of the
 * communication socket.
 */
void* file_client (client_t* clnt)
{

    int sd = clnt -> sd;
    char* ip = clnt -> ip;

    char req[MAX_LEN];  // current request
    char msg[MAX_LEN];  // logger string

    char cmd[10];
    char file_iden[50];
    char file_content[1025];
    file_content[1024] = '\0';

    int n;
    bool* opened_fds = new bool[flocks_size];
    for (size_t i = 0; i < flocks_size; i++)
        opened_fds[i] = false;

    // make sure req is null-terminated...
    req[MAX_LEN-1] = '\0';

    snprintf(msg, MAX_LEN, "%s: new client from %s assigned socket descriptor %d\n",
             __FILE__, ip, sd);
    logger(msg);
    snprintf(msg, MAX_LEN,
             "Welcome to shfd v.1 [%s]. FOPEN FSEEK FREAD FWRITE FCLOSE QUIT spoken here.\r\n",
             ip);
    send(sd, msg, strlen(msg),0);

    // Loop while the client has something to say...
    while ((n = readline(sd,req,MAX_LEN-1)) != recv_nodata)
    {

        char* ans = new char[MAX_LEN]; // the response sent back to the client
        // we allocate space for the answer dinamically because the
        // result of a FREAD command is virtually unbounded in size
        ans[MAX_LEN-1] = '\0';

        // If we talk to telnet, we get \r\n at the end of the line
        // instead of just \n, so we take care of the possible \r:
        if ( n > 1 && req[n-1] == '\r' )
            req[n-1] = '\0';

        if (debugs[DEBUG_COMM])
        {
            snprintf(msg, MAX_LEN, "%s: --> %s\n", __FILE__, req);
            logger(msg);
        } /* DEBUG_COMM */

        if ( strncasecmp(req,"QUIT",strlen("QUIT")) == 0 )
        {  // we are done!
            snprintf(msg, MAX_LEN, "%s: QUIT received from client %d (%s), closing\n", __FILE__, sd, ip);
            logger(msg);
            if (debugs[DEBUG_COMM]) {
                snprintf(msg, MAX_LEN, "%s: <-- OK 0 nice talking to you\n", __FILE__);
                logger(msg);
            } /* DEBUG_COMM */
            send(sd,"OK 0 nice talking to you\r\n", strlen("OK 0 nice talking to you\r\n"),0);
            shutdown(sd, SHUT_RDWR);
            close(sd);
            delete[] ans;
            snprintf(msg, MAX_LEN, "%s: Attempting to close the files opened by this client\n", __FILE__);
            logger(msg);
            for (size_t i = 0; i < flocks_size; i++)
                if (opened_fds[i])
                    file_exit(i);
            delete[] opened_fds;
            delete clnt;
            return 0;
        }
        // ### COMMAND HANDLER ###

        // ### FOPEN ###
        else if (strncasecmp(req,"FOPEN",strlen("FOPEN")) == 0 )
        {


            int idx = next_arg(req,' ');
            if (idx == -1 ) {
                snprintf(ans,MAX_LEN,"FAIL %d FOPEN requires a file name", EBADMSG);
            }
            else { // we attempt to open the file
                char filename[MAX_LEN];
                char abs_filename[MAX_LEN];
                // do we have a relative path?
                // awkward test, do we have anything better?
                if (req[idx] == '/')
                { // absolute
                    snprintf(filename, MAX_LEN, "%s", &req[idx]);
                }
                else
                { // relative
                    char cwd[MAX_LEN];
                    getcwd(cwd, MAX_LEN);
                    snprintf(filename, MAX_LEN, "%s/%s", cwd, &req[idx]);

                    snprintf(abs_filename, MAX_LEN,"%s",&req[idx]);
                    abs_filename[-1]='\0';
                }

                // already opened?
                int fd = -1;
                for (size_t i = 0; i < flocks_size; i++) {
                    if (flocks[i] != 0 && strcmp(filename, flocks[i] -> name) == 0) {
                        fd = i;
                        pthread_mutex_lock(&flocks[fd] -> mutex);
                        if (! opened_fds[fd])  // file already opened by the same client?
                            flocks[fd] -> owners ++;
                        pthread_mutex_unlock(&flocks[fd] -> mutex);
                        opened_fds[fd] = true;
                        break;
                    }
                }
                if (fd >= 0)
                {   // already opened
                    snprintf(ans, MAX_LEN,
                             "ERR %d file already opened, please use the supplied identifier", fd);
                }
                else { // we open the file anew

                    strcpy(cmd,"FOPEN");
                    strcpy(file_iden,abs_filename);
                    strcpy(file_content,"");

                    sync_non_read(cmd,file_iden,file_content); //Sync with peers

                    fd = file_init(filename,abs_filename);

                    if (fd < 0)
                        snprintf(ans, MAX_LEN, "FAIL %d %s", errno, strerror(errno));
                    else
                    {
                        snprintf(ans, MAX_LEN, "OK %d file opened, please use supplied identifier", fd);
                        opened_fds[fd] = true;
                    }
                }

            }
        } // end FOPEN

        // ### FREAD ###
        else if (strncasecmp(req,"FREAD",strlen("FREAD")) == 0 )
        {
            int idx = next_arg(req,' ');
            if (idx == -1) // no identifier
                snprintf(ans,MAX_LEN,"FAIL %d FREAD requires a file identifier", EBADMSG);
            else {
                int idx1 = next_arg(&req[idx],' ');
                if (idx1 == -1) // no identifier
                    snprintf(ans,MAX_LEN,"FAIL %d FREAD requires a number of bytes to read", EBADMSG);
                else {
                    idx1 = idx + idx1;
                    req[idx1 - 1] = '\0';
                    if (debugs[DEBUG_COMM]) {
                        snprintf(msg, MAX_LEN, "%s: (before decoding) will read %s bytes from %s \n",
                                 __FILE__, &req[idx1], &req[idx]);
                        logger(msg);
                    }
                    idx = atoi(&req[idx]);  // get the identifier and length
                    idx1 = atoi(&req[idx1]);
                    if (debugs[DEBUG_COMM]) {
                        snprintf(msg, MAX_LEN, "%s: (after decoding) will read %d bytes from %d \n",
                                 __FILE__, idx1, idx);
                        logger(msg);
                    }
                    if (idx <= 0 || idx1 <= 0)
                        snprintf(ans, MAX_LEN,
                                 "FAIL %d identifier and length must both be positive numbers", EBADMSG);
                    else { // now we can finally read the thing!
                        // read buffer

                        char* read_buff = new char[idx1+1];

                        vote_info=new vote_read[no_of_peers+1];

                        int result = read_excl(idx, read_buff, idx1);
                        // ASSUMPTION: we never read null bytes from the file.
                        if (result == err_nofile) {
                            snprintf(ans, MAX_LEN, "FAIL %d bad file descriptor %d", EBADF, idx);
                        }
                        else if (result < 0) {
                            snprintf(ans, MAX_LEN, "FAIL %d %s", errno, strerror(errno));
                        }
                        else {

                            strcpy(cmd,"FREAD");
                            strcpy(file_iden,flocks[idx]->abs_name);
                            sprintf(file_content,"%d",idx1);
                            sync_read(cmd,file_iden,file_content); //Sync with peers

                            read_buff[result] = '\0';
                            // we may need to allocate a larger buffer
                            // besides the message, we give 40 characters to OK + number of bytes read.
                            delete[] ans;
                            ans = new char[40 + result];
                            int positive = 0;
                            int negative = 0;
                            for(int j=0;j<no_of_peers;j++)
                            {
                              // printf("\nReply from Peer %d is %ld:\n",j,strlen(peer_info[j].peer_read_reply));
                              // printf("~~~%s~~~",peer_info[j].peer_read_reply);
                              vote_info[j].peer_read_reply=new char[idx1+1];
                              strcpy(vote_info[j].peer_read_reply,peer_info[j].peer_read_reply);

                              if(strcmp(peer_info[j].peer_read_reply,read_buff)==0)
                              {
                                positive++;
                              }
                              else if(strcmp(peer_info[j].peer_read_reply,"SYNC FAIL")==0 || strcmp(peer_info[j].peer_read_reply,"")==0)
                              {
                                negative++;
                              }
                            }

                            vote_info[no_of_peers].peer_read_reply=new char[idx1+1];
                            strcpy(vote_info[no_of_peers].peer_read_reply,read_buff);

                            if(positive>negative || positive==negative)
                            {

                                  for(int j=0;j<=no_of_peers;j++)
                                  {
                                      vote_info[j].vote_count=0;
                                  }

                                  for(int i=0;i<=no_of_peers;i++)
                                  {
                                      for(int j=0;j<=no_of_peers;j++)
                                      {
                                        if(strcmp(vote_info[i].peer_read_reply,vote_info[j].peer_read_reply)==0)
                                        {
                                          vote_info[i].vote_count++;
                                        }

                                      }
                                  }

                                  int neutral=0;

                                  for(int i=0;i<=no_of_peers;i++)
                                  {
                                      for(int j=0;j<=no_of_peers;j++)
                                      {
                                        if (vote_info[i].vote_count==vote_info[j].vote_count)
                                        {
                                          neutral++;
                                        }

                                      }
                                  }
                                  // printf("Neutral rate: %d, Num: %d", neutral,(no_of_peers+1)*(no_of_peers+1));
                                  if(neutral==(no_of_peers+1)*(no_of_peers+1))
                                  {
                                    for(int j=0;j<=no_of_peers;j++)
                                    {
                                        memset(&peer_info[j].peer_read_reply[0], 0, sizeof(peer_info[j].peer_read_reply));
                                    }

                                    snprintf(ans, MAX_LEN, "FAIL %d RAID SYNC error.", -1);

                                  }
                                  else
                                  {

                                          int winner=0;

                                          for(int i=0;i<=no_of_peers;i++)
                                          {
                                            if (vote_info[i].vote_count>vote_info[winner].vote_count)
                                            {
                                              winner = i;
                                            }

                                          }

                                          char winner_rep[1024];
                                          strcpy(winner_rep,vote_info[winner].peer_read_reply);
                                          winner_rep[1023]='\0';


                                          for(int j=0;j<=no_of_peers;j++)
                                          {
                                              // printf("\n Peer data: %s  , Peer voted : %d\n",vote_info[j].peer_read_reply,vote_info[j].vote_count);
                                              vote_info[j].vote_count=0;
                                              memset(&peer_info[j].peer_read_reply[0], 0, sizeof(peer_info[j].peer_read_reply));
                                              // memset(&vote_info[j].peer_read_reply[0], 0, sizeof(vote_info[j].peer_read_reply));
                                              delete [] vote_info[j].peer_read_reply;
                                          }

                                          // printf("\nWinner: %d , Winner rep : %s , Winner vote : %d\n",winner,winner_rep,vote_info[winner].vote_count);

                                    // printf("\nReply from Self is %d:\n",result);
                                    // printf("%s\n",read_buff);

                                    // printf("\nPositive : %d , Negative : %d\n",positive,negative);

                                        snprintf(ans, MAX_LEN, "OK %d %s", result, winner_rep);
                                }
                              }
                            else
                            {
                              for(int j=0;j<=no_of_peers;j++)
                              {
                                  memset(&peer_info[j].peer_read_reply[0], 0, sizeof(peer_info[j].peer_read_reply));
                              }

                              snprintf(ans, MAX_LEN, "FAIL %d RAID SYNC error.", -1);
                            }
                        }
                        delete [] read_buff;
                    }
                }
            }
        } // end FREAD

        // ### FWRITE ###
        else if (strncasecmp(req,"FWRITE",strlen("FWRITE")) == 0 ) {
            int idx = next_arg(req,' ');
            if (idx == -1) // no argument!
                snprintf(ans,MAX_LEN,"ERROR %d FWRITE required a file identifier", EBADMSG);
            else {
                int idx1 = next_arg(&req[idx],' ');
                if (idx1 == -1) // no data to write
                    snprintf(ans,MAX_LEN,"FAIL %d FWRITE requires data to be written", EBADMSG);
                else {
                    idx1 = idx1 + idx;
                    req[idx1 - 1] = '\0';
                    idx = atoi(&req[idx]);  // get the identifier and data
                    if (idx <= 0)
                        snprintf(ans,MAX_LEN,
                                 "FAIL %d identifier must be positive", EBADMSG);
                    else { // now we can finally write!
                        // sync_non_read(req);
                        if (debugs[DEBUG_FILE]) {
                            snprintf(msg, MAX_LEN, "%s: will write %s\n", __FILE__, &req[idx1]);
                            logger(msg);
                        }
                        int result = write_excl(idx, &req[idx1], strlen(&req[idx1]));
                        if (result == err_nofile)
                            snprintf(ans, MAX_LEN, "FAIL %d bad file descriptor %d", EBADF, idx);
                        else if (result < 0) {
                            snprintf(ans, MAX_LEN, "FAIL %d %s", errno, strerror(errno));
                        }
                        else {

                            char* stuff = &req[idx1];
                            // printf("returned");

                            strcpy(cmd,"FWRITE");
                            strcpy(file_iden,flocks[idx]->abs_name);
                            strcpy(file_content,stuff);
                            sync_non_read(cmd,file_iden,file_content); //Sync with peers

                            snprintf(ans, MAX_LEN, "OK 0 wrote %d bytes", result);
                        }
                    }
                }
            }
        } // end WRITE

        // ### FSEEK ###
        else if (strncasecmp(req,"FSEEK",strlen("FSEEK")) == 0 ) {
            int idx = next_arg(req,' ');
            if (idx == -1) // no identifier
                snprintf(ans,MAX_LEN,"FAIL %d FSEEK requires a file identifier", EBADMSG);
            else {
                int idx1 = next_arg(&req[idx],' ');
                if (idx1 == -1) // no identifier
                    snprintf(ans,MAX_LEN,"FAIL %d FSEEK requires an offset", EBADMSG);
                else {
                    idx1 = idx1 + idx;
                    req[idx1 - 1] = '\0';
                    idx = atoi(&req[idx]);  // get the identifier and offset
                    idx1 = atoi(&req[idx1]);
                    if (idx <= 0)
                        snprintf(ans,MAX_LEN,
                                 "FAIL %d identifier must be positive", EBADMSG);
                    else { // now we can finally seek!
                        // sync_non_read(req);
                        int result = seek_excl(idx, idx1);
                        if (result == err_nofile)
                            snprintf(ans, MAX_LEN, "FAIL %d bad file descriptor %d", EBADF, idx);
                        else if (result < 0) {
                            snprintf(ans, MAX_LEN, "FAIL %d %s", errno, strerror(errno));
                        }
                        else {

                            strcpy(cmd,"FSEEK");
                            strcpy(file_iden,flocks[idx]->abs_name);
                            sprintf(file_content,"%d",idx1);
                            sync_non_read(cmd,file_iden,file_content); //Sync with peers


                            snprintf(ans, MAX_LEN, "OK 0 offset is now %d", result);
                        }
                    }
                }
            }
        } // end FSEEK

        // ### FCLOSE ###
        else if (strncasecmp(req,"FCLOSE",strlen("FCLOSE")) == 0 ) {
            int idx = next_arg(req,' ');
            if (idx == -1) // no identifier
                snprintf(ans,MAX_LEN,"FAIL %d FCLOSE requires a file identifier", EBADMSG);
            else {
                idx = atoi(&req[idx]);  // get the identifier and offset
                if (idx <= 0)
                    snprintf(ans,MAX_LEN,
                             "FAIL %d identifier must be positive", EBADMSG);
                else { // now we can finally close!
                    // sync_non_read(req);
                    int result = file_exit(idx);
                    opened_fds[idx] = false;
                    if (result == err_nofile)
                        snprintf(ans, MAX_LEN, "FAIL %d bad file descriptor %d", EBADF, idx);
                    else if (result < 0) {
                        snprintf(ans, MAX_LEN, "FAIL %d %s", errno, strerror(errno));
                    }
                    else {

                        snprintf(ans, MAX_LEN, "OK 0 file closed");

                    }
                }
            }
        } // end FCLOSE

        // ### UNKNOWN COMMAND ###
        else {
            int idx = next_arg(req,' ');
            if ( idx == 0 )
                idx = next_arg(req,' ');
            if (idx != -1)
                req[idx-1] = '\0';
            snprintf(ans,MAX_LEN,"FAIL %d %s not understood", EBADMSG, req);
        }

        // ### END OF COMMAND HANDLER ###

        // Effectively send the answer back to the client:
        if (debugs[DEBUG_COMM]) {
            snprintf(msg, MAX_LEN, "%s: <-- %s\n", __FILE__, ans);
            logger(msg);
        } /* DEBUG_COMM */
        send(sd,ans,strlen(ans),0);
        send(sd,"\r\n",2,0);        // telnet expects \r\n
        delete[] ans;
    } // end of main loop.

    // read 0 bytes = EOF:
    snprintf(msg, MAX_LEN, "%s: client on socket descriptor %d (%s) went away, closing\n",
             __FILE__, sd, ip);
    logger(msg);
    shutdown(sd, SHUT_RDWR);
    close(sd);
    for (size_t i = 0; i < flocks_size; i++)
        if (opened_fds[i])
            file_exit(i);
    delete[] opened_fds;
    delete clnt;
    return 0;
}
