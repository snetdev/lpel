#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#include "lpel_name.h"
#include "workermsg.h"

typedef struct mailbox_t mailbox_t;

mailbox_t *LPEL_FUNC(MailboxCreate)(void);
void LPEL_FUNC(MailboxDestroy)(mailbox_t *mbox);
void LPEL_FUNC(MailboxSend)(mailbox_t *mbox, workermsg_t *msg);
void LPEL_FUNC(MailboxRecv)(mailbox_t *mbox, workermsg_t *msg);
int  LPEL_FUNC(MailboxHasIncoming)(mailbox_t *mbox);


#endif /* _MAILBOX_H_ */
