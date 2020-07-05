/*
 * In-place string tokenizer, by Stefan Bruda.
 */

#ifndef __SERVER_OPERATIONS_H
#define __SERVER_OPERATIONS_H

#include "bbserv_utils.h"

void* server_operations(client_t* clnt);
int sync_write(line*);
int sync_replace(line* line_a);
int send_precommit(peer_stat*);
int send_commit(peer_stat* peers);
int send_del(peer_stat* peers);
int send_quit(peer_stat* peers);


#endif /* __SERVER_OPERATIONS_H */
