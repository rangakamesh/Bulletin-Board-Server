#ifndef __DESCRIPTOR_H
#define __DESCRIPTOR_H
#include "bbserv_utils.h"

typedef struct num_gen
{
  pthread_mutex_t access_num_gen;
  int count;
  long int* temp_list;
}num_gen;

extern num_gen gen_on_queue;
//Whenver a new number is created it is added to this buffer.
//So that when a cuncurrent number request is done duplication is handled.
//What's a number request ? when a new line is to be written, a new message number is generated.

typedef struct file_queue
{
    pthread_mutex_t access_file_queue;
    pthread_cond_t can_write;
    unsigned int reads;
}file_queue;
extern file_queue file_queue_a; // file access locks used to handle concurrent file access.


void add_trailing_spaces(char *dest, int size, int num_of_spaces);//does what it is named :)

int initiate_descriptor(char* filename);
//checks if the bbfile exists. If yes, returns 0;
//If not creates one. If creation success returns 0, else -1
//The function create_descriptor is used as part of this process.

int write_descriptor(char* msg_number,char* user,char* message,bool peer_work);
//Writes a new line to the file.
//If the parameter peer_work is true , the file delay operations are force diabled.

int read_descriptor(char*,char*,int);
//Reads a requested line from the file.
//First argument is the message number to be read. Second to return the read line.
//If the third is 1 file read delay operations are force disabled.

int read_tokenized_descriptor(char* msg_number, char* msg_user,char* msg_message);
//Does the same as read_descriptor, except returns the output tokenized.
//Helpful for peer communication

int seek_msg_descriptor(char* msg_number,char*);
//seeks for the position in the file where the requested message begins and sets the pointer at that position.
//also returns the length of the line on the second argument

int delete_descriptor(char* msg_number);
//replaces the message mentioned with the '/b' character

int replace_descriptor(char* msg_number,char* user,char* message,bool peer_comms);
//It simply does a delete_descriptor on the mentioned message and does a write_descriptor the same with new content.

long int generate_number();
//Goes through the file. Finds the largest message number. Increments it by 1. Adds it to the buffer and returns it.
//Used for new line write.

long int release_number(long int num);
//If in case a generated number is not to be used it it erased from the buffer.

int housekeep_descriptor();
//Removes all the garbage generated as part of delete_descriptor or existing and ensures data cleanliness.


#endif /* __DESCRIPTOR_H */
