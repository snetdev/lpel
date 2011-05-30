#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#include "workermsg.h"

typedef struct mailbox_t mailbox_t;

mailbox_t *MailboxCreate(void);
void MailboxDestroy( mailbox_t *mbox);
void MailboxSend( mailbox_t *mbox, workermsg_t *msg);
void MailboxRecv( mailbox_t *mbox, workermsg_t *msg);
int  MailboxHasIncoming( mailbox_t *mbox);


#endif /* _MAILBOX_H_ */
