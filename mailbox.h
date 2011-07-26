#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#include "lpel_name.h"
#include "workermsg.h"

typedef struct mailbox_t mailbox_t;

mailbox_t *LPEL_EXPORT(MailboxCreate)(void);
void LPEL_EXPORT(MailboxDestroy)(mailbox_t *mbox);
void LPEL_EXPORT(MailboxSend)(mailbox_t *mbox, workermsg_t *msg);
void LPEL_EXPORT(MailboxRecv)(mailbox_t *mbox, workermsg_t *msg);
int  LPEL_EXPORT(MailboxHasIncoming)(mailbox_t *mbox);


#endif /* _MAILBOX_H_ */
