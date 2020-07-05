/*
 * In-place string tokenizer, by Stefan Bruda.
 */

#ifndef __PEER_OPERATIONS_H
#define __PEER_OPERATIONS_H

#include "bbserv_utils.h"
#include "thrd_mgmt.h"

extern team* peer_control_team;
extern team* peer_receiver_team;
extern team* peer_sender_team;

void peer_establisher();
//Establishes the peer receiver and peer sender threads.
// 1 Peer receiver Parent +  (2 * server_config.T_MAX) peer receivers + (2 * server_config.T_MAX) peer senders

void peer_receiver_control(); // Parent thread for peer receiver operations.
void peer_receiver_operations(client_t* clnt); // Receiver operation.




#endif /* __PEER_OPERATIONS_H */
