#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#include <pthread.h>

#include "bool.h"
#include "workermsg.h"

typedef struct mailbox_node mailbox_node_t;

struct mailbox_node {
  struct mailbox_node *next;
  workermsg_t body;
};


typedef struct {
  pthread_mutex_t  lock_free;
  pthread_mutex_t  lock_inbox;
  pthread_cond_t   notempty;
  mailbox_node_t  *list_free;
  mailbox_node_t  *list_inbox;
} mailbox_t;


void MailboxInit( mailbox_t *mbox);
void MailboxCleanup( mailbox_t *mbox);
void MailboxSend( mailbox_t *mbox, workermsg_t *msg);
void MailboxRecv( mailbox_t *mbox, workermsg_t *msg);
bool MailboxHasIncoming( mailbox_t *mbox);


#endif /* _MAILBOX_H_ */
