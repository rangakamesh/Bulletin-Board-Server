#include "shfd.h"
#include "peer_comms.h"
void sync_non_read(char* cmd,char* file_iden,char* file_content)
{

  // printf("\nAbsolute command is : %s",cmd);
  // printf("\nAbsolute filename is : %s\n",file_iden);
  // printf("Absolute content is : %s\n",file_content);

  int** id=new int*[no_of_peers];

  for(int ii=0;ii<no_of_peers;ii++)
  {
    pthread_mutex_lock(&peer_info[ii].peer_mutex);

      // peer_info[ii].cmd=new char[strlen(cmd)];
      strcpy(peer_info[ii].cmd,cmd);

      // peer_info[ii].file_iden=new char[strlen(file_iden)];
      strcpy(peer_info[ii].file_iden,file_iden);

      // peer_info[ii].file_iden=new char[strlen(file_iden)];
      strcpy(peer_info[ii].file_content,file_content);

      id[ii]=new int;
      *id[ii]=ii;


      // add_work_to_team(peer_ambassadors,(void (*) (void*))send_to_peer,reinterpret_cast<void* &>(id));
      add_work_to_team(peer_ambassadors,(void (*) (void*))send_to_peer,reinterpret_cast<void*>(id[ii]));

      if(1)
      {
          pthread_cond_wait(&peer_info[ii].peer_cond,&peer_info[ii].peer_mutex);//wait until the pointer variable is delivered
      }

    pthread_mutex_unlock(&peer_info[ii].peer_mutex);
  }

  for(int ii=0;ii<no_of_peers;ii++)
  {
    // if(1)
    // {
    //     pthread_cond_wait(&peer_info[ii].peer_cond,&peer_info[ii].peer_mutex);//wait until the pointer variable is delivered
    // }

    delete[] id[ii];
  }
  delete[] id;

}

void sync_read(char* cmd,char* file_iden,char* file_content)
{

  // printf("\nAbsolute command is : %s",cmd);
  // printf("\nAbsolute filename is : %s\n",file_iden);
  // printf("Absolute content is : %s\n",file_content);

  int** id=new int*[no_of_peers];

  for(int ii=0;ii<no_of_peers;ii++)
  {
    pthread_mutex_lock(&peer_info[ii].peer_mutex);

      // peer_info[ii].cmd=new char[strlen(cmd)];
      strcpy(peer_info[ii].cmd,cmd);

      // peer_info[ii].file_iden=new char[strlen(file_iden)];
      strcpy(peer_info[ii].file_iden,file_iden);

      // peer_info[ii].file_iden=new char[strlen(file_iden)];
      strcpy(peer_info[ii].file_content,file_content);

      id[ii]=new int;
      *id[ii]=ii;


      // add_work_to_team(peer_ambassadors,(void (*) (void*))send_to_peer,reinterpret_cast<void* &>(id));
      add_work_to_team(peer_ambassadors,(void (*) (void*))comms_to_peer,reinterpret_cast<void*>(id[ii]));

      if(1)
      {
          pthread_cond_wait(&peer_info[ii].peer_cond,&peer_info[ii].peer_mutex);//wait until the pointer variable is delivered
      }

    pthread_mutex_unlock(&peer_info[ii].peer_mutex);
  }

  for(int ii=0;ii<no_of_peers;ii++)
  {
    // if(1)
    // {
    //     pthread_cond_wait(&peer_info[ii].peer_cond,&peer_info[ii].peer_mutex);//wait until the pointer variable is delivered
    // }

    delete[] id[ii];
  }
  delete[] id;

}

void send_to_peer (int* x)
{

    const int ALEN = 2048;
    char req[ALEN];
    int xx=*x;

    pthread_mutex_lock(&peer_info[xx].peer_mutex);
          pthread_cond_signal(&peer_info[xx].peer_cond);//inform main thread to resume
    pthread_mutex_unlock(&peer_info[xx].peer_mutex);

    char* port=new char;
    sprintf(port,"%d",peer_info[xx].port);
    // printf("\nSending to %s:%s",peer_info[xx].ip,port);
    int sd = connectbyport(peer_info[xx].ip,port);

    if (sd == err_host)
    {
        printf("\nSYNC FAIL with %s:%s on %s\n", peer_info[xx].ip,port,peer_info[xx].cmd);
        return;
    }

    if (sd < 0)
    {
        printf("\nLSYNC FAIL with %s:%s on %s\n", peer_info[xx].ip,port,peer_info[xx].cmd);
        return;
    }

    sprintf(req,"%s %s %s",peer_info[xx].cmd,peer_info[xx].file_iden,peer_info[xx].file_content);


    if(strlen(req) > 0 && req[strlen(req) - 1] == '\n')
        req[strlen(req) - 1] = '\0';
    send(sd,req,strlen(req),0);
    send(sd,"\n",1,0);

    shutdown(sd, SHUT_RDWR);
    close(sd);

    delete[] port;
}

void comms_to_peer (int* x)
{


    const int ALEN = 2048;
    char req[ALEN];
    char ans[ALEN];
    char buff[ALEN];
    int xx=*x;

    pthread_mutex_lock(&peer_info[xx].peer_mutex);

    char* port=new char;
    sprintf(port,"%d",peer_info[xx].port);
    // printf("\nSending to %s:%s",peer_info[xx].ip,port);
    int sd = connectbyport(peer_info[xx].ip,port);

    if (sd == err_host)
    {
        printf("\nSYNC FAIL with %s:%s on %s\n", peer_info[xx].ip,port,peer_info[xx].cmd);
        pthread_cond_signal(&peer_info[xx].peer_cond);//inform main thread to resume
        pthread_mutex_unlock(&peer_info[xx].peer_mutex);
        return;
    }

    if (sd < 0)
    {
        printf("\nSYNC FAIL with %s:%s on %s\n", peer_info[xx].ip,port,peer_info[xx].cmd);
        pthread_cond_signal(&peer_info[xx].peer_cond);//inform main thread to resume
        pthread_mutex_unlock(&peer_info[xx].peer_mutex);
        return;
    }

    sprintf(req,"%s %s %s",peer_info[xx].cmd,peer_info[xx].file_iden,peer_info[xx].file_content);


    if(strlen(req) > 0 && req[strlen(req) - 1] == '\n')
        req[strlen(req) - 1] = '\0';
    send(sd,req,strlen(req),0);
    send(sd,"\n",1,0);

    // // read and display the response (which is exactly one line long):
    // readline(sd,ans,ALEN-1);
    // printf("\nResponse from peer Start: \n");

    memset(&ans[0],0,strlen(ans));
    int n;
    int m=0;
    while ((n = recv_nonblock(sd,buff,ALEN-1,10)) != recv_nodata)
    {
        if (n == 0)
        {
            shutdown(sd, SHUT_RDWR);
            close(sd);
            // printf("Connection closed by %s\n", port);
            ans[m-2] = '\0';
            break;
        }
        buff[n] = '\0';
        // printf("\n%s",buff);
        // sprintf(ans,"%s%s", ans,buff);
        strcat(ans,buff);
        memset(&buff[0],0,strlen(buff));
        m+=n;
        // fflush(stdout);
    }

    // printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n%s\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", ans);
    // printf("\nResponse from peer End: \n");
    strcpy(peer_info[xx].peer_read_reply,ans);

    shutdown(sd, SHUT_RDWR);
    close(sd);

    delete[] port;
          pthread_cond_signal(&peer_info[xx].peer_cond);//inform main thread to resume
    pthread_mutex_unlock(&peer_info[xx].peer_mutex);

}



void* peer_receiver_sub (int* msock)
{

    int msocket = passivesocket(*msock,32);
    if (msocket < 0) {
        perror("peer receiver passivesocket");
        return NULL;
    }
    printf("Peer receiver up and listening on port %d\n", *msock);

    char req[MAX_LEN];  // current request
    char msg[MAX_LEN];  // logger string
    int n;
    bool* opened_fds = new bool[flocks_size];
    for (size_t i = 0; i < flocks_size; i++)
        opened_fds[i] = false;

    req[MAX_LEN-1] = '\0';

    int ssock;                      // slave sockets
    struct sockaddr_in client_addr; // the address of the client...
    socklen_t client_addr_len = sizeof(client_addr); // ... and its length


    while (1)
    {
        // Accept connection:
        ssock = accept(msocket, (struct sockaddr*)&client_addr, &client_addr_len);

        char* ans = new char[MAX_LEN]; // the response sent back to the client
        // we allocate space for the answer dinamically because the
        // result of a FREAD command is virtually unbounded in size
        ans[MAX_LEN-1] = '\0';

        if (ssock < 0) {
            if (errno == EINTR) continue;
            snprintf(msg, MAX_LEN, "%s: peer port accept: %s\n", __FILE__, strerror(errno));
            logger(msg);
            return 0;
        }
        else if(ssock > 0)
        {
          if ((n = readline(ssock,req,MAX_LEN-1)) != recv_nodata)
          {

                // fprintf(stderr,"Peer msg: %s.\n",req);
                char* req_cpy=new char;
                strcpy(req_cpy,req);

                char *sep=strtok(req_cpy," ");
                // printf("\tCommand : %s -> %s",sep,req_cpy);

                sep=strtok(NULL," ");
                char filenamex[MAX_LEN];
                strcpy(filenamex,sep);
                // printf("\tFile : %s -> %s",filenamex,req_cpy);

                sep=strtok(NULL," ");
                // printf("\tRem : %s -> %s\n",sep,req_cpy);

                if ( n > 1 && req[n-1] == '\r' )
                    req[n-1] = '\0';
                if (debugs[DEBUG_COMM]) {
                    snprintf(msg, MAX_LEN, "%s: --> %s\n", __FILE__, req);
                    logger(msg);
                } /* DEBUG_COMM */

                // ### COMMAND HANDLER ###

                // ### FOPEN ###
                if (strncasecmp(req,"FOPEN",strlen("FOPEN")) == 0 )
                {
                    // sync_non_read(req); //Sync with peers

                    int idx = next_arg(req,' ');
                    if (idx == -1 ) {
                        snprintf(ans, MAX_LEN, "SYNC FAIL");
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

                            snprintf(abs_filename, MAX_LEN, "%s", &req[idx]);
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
                        snprintf(ans,MAX_LEN,"SYNC FAIL");
                    else
                    {
                        int idx1 = next_arg(&req[idx],' ');
                        if (idx1 == -1) // no identifier
                            snprintf(ans,MAX_LEN,"SYNC FAIL");
                        else
                        {
                            idx1 = idx + idx1;
                            req[idx1 - 1] = '\0';
                            if (debugs[DEBUG_COMM])
                            {
                                snprintf(msg, MAX_LEN, "%s: (before decoding) will read %s bytes from %s \n",
                                         __FILE__, &req[idx1], &req[idx]);
                                logger(msg);
                            }

                            int fd = -1;
                            for (size_t i = 0; i < flocks_size; i++)
                            {
                                // if((flocks[i] != 0))
                                    // printf("\nInfo %d : -%s- -%s- %d",fd,filenamex,flocks[i] -> abs_name,strcmp(filenamex, flocks[i] -> abs_name));

                                if ((flocks[i] != 0)  && (strcmp(filenamex, flocks[i] -> abs_name) == 0))
                                {
                                    fd = i;
                                    // printf("\nFound %d : %s",fd,flocks[i] -> abs_name);
                                    break;
                                }
                            }

                            idx = fd;  // get the identifier and length
                            idx1 = atoi(&req[idx1]);
                            if (debugs[DEBUG_COMM])
                            {
                                snprintf(msg, MAX_LEN, "%s: (after decoding) will read %d bytes from %d \n",
                                         __FILE__, idx1, idx);
                                logger(msg);
                            }
                            if (idx <= 0 || idx1 <= 0)
                                snprintf(ans, MAX_LEN,
                                         "SYNC FAIL");
                            else
                            {
                                // now we can finally read the thing!
                                // read buffer
                                char* read_buff = new char[idx1+1];
                                int result = read_excl(idx, read_buff, idx1);
                                // ASSUMPTION: we never read null bytes from the file.
                                if (result == err_nofile) {
                                    snprintf(ans, MAX_LEN, "SYNC FAIL");
                                }
                                else if (result < 0) {
                                    snprintf(ans, MAX_LEN, "SYNC FAIL");
                                }
                                else {
                                    read_buff[result] = '\0';
                                    // we may need to allocate a larger buffer
                                    // besides the message, we give 40 characters to OK + number of bytes read.
                                    delete[] ans;
                                    ans = new char[40 + result];
                                    snprintf(ans, MAX_LEN, "%s", read_buff);
                                }
                                delete [] read_buff;
                            }
                        }
                    }
                    send(ssock,ans,strlen(ans),0);
                    send(ssock,"\r\n",2,0);        // telnet expects \r\n
                } // end FREAD

                // ### FWRITE ###
                else if (strncasecmp(req,"FWRITE",strlen("FWRITE")) == 0 )
                {

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

                            int fd = -1;
                            for (size_t i = 0; i < flocks_size; i++)
                            {
                                // if((flocks[i] != 0))
                                    // printf("\nInfo %d : -%s- -%s- %d",fd,filenamex,flocks[i] -> abs_name,strcmp(filenamex, flocks[i] -> abs_name));

                                if ((flocks[i] != 0)  && (strcmp(filenamex, flocks[i] -> abs_name) == 0))
                                {
                                    fd = i;
                                    // printf("\nFound %d : %s",fd,flocks[i] -> abs_name);
                                    break;
                                }
                            }

                            idx = atoi(&req[idx]);  // get the identifier and data
                            idx=fd;
                            if (idx <= 0)
                                snprintf(ans,MAX_LEN,
                                         "FAIL %d identifier must be positive", EBADMSG);
                            else { // now we can finally write!
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
                            // idx = atoi(&req[idx]);  // get the identifier and offset
                            idx1 = atoi(&req[idx1]);

                            int fd = -1;
                            for (size_t i = 0; i < flocks_size; i++)
                            {
                                // if((flocks[i] != 0))
                                    // printf("\nInfo %d : -%s- -%s- %d",fd,filenamex,flocks[i] -> abs_name,strcmp(filenamex, flocks[i] -> abs_name));

                                if ((flocks[i] != 0)  && (strcmp(filenamex, flocks[i] -> abs_name) == 0))
                                {
                                    fd = i;
                                    // printf("\nFound %d : %s",fd,flocks[i] -> abs_name);
                                    break;
                                }
                            }
                            idx=fd;

                            if (idx <= 0)
                                snprintf(ans,MAX_LEN,
                                         "FAIL %d identifier must be positive", EBADMSG);
                            else { // now we can finally seek!
                                int result = seek_excl(idx, idx1);
                                if (result == err_nofile)
                                    snprintf(ans, MAX_LEN, "FAIL %d bad file descriptor %d", EBADF, idx);
                                else if (result < 0) {
                                    snprintf(ans, MAX_LEN, "FAIL %d %s", errno, strerror(errno));
                                }
                                else {
                                    snprintf(ans, MAX_LEN, "OK 0 offset is now %d", result);
                                }
                            }
                        }
                    }
                } // end FSEEK

                // ### FCLOSE ###
                else if (strncasecmp(req,"FCLOSE",strlen("FCLOSE")) == 0 )
                {
                    int idx = next_arg(req,' ');
                    if (idx == -1) // no identifier
                        snprintf(ans,MAX_LEN,"FAIL %d FCLOSE requires a file identifier", EBADMSG);
                    else {
                        // idx = atoi(&req[idx]);  // get the identifier and offset

                        int fd = -1;
                        for (size_t i = 0; i < flocks_size; i++)
                        {
                            // if((flocks[i] != 0))
                                // printf("\nInfo %d : -%s- -%s- %d",fd,filenamex,flocks[i] -> abs_name,strcmp(filenamex, flocks[i] -> abs_name));

                            if ((flocks[i] != 0)  && (strcmp(filenamex, flocks[i] -> abs_name) == 0))
                            {
                                fd = i;
                                // printf("\nFound %d : %s",fd,flocks[i] -> abs_name);
                                break;
                            }
                        }
                        idx=fd;

                        if (idx <= 0)
                            snprintf(ans,MAX_LEN,
                                     "FAIL %d identifier must be positive", EBADMSG);
                        else { // now we can finally close!

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


                fprintf(stderr,"%s",ans);
                fprintf(stderr,"%c",'\n');
                delete[] ans;

          }
        }

        shutdown(ssock, SHUT_RDWR);
        close(ssock);

      }


    return 0;   // will never reach this anyway...
}
