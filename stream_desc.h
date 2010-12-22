#ifndef _STREAM_DESC_H_
#define _STREAM_DESC_H_


struct task;
struct stream;


/** stream descriptor */
typedef struct stream_desc stream_desc_t;    



/**
 * A stream descriptor
 *
 * A producer/consumer must open a stream before using it, and by opening
 * a stream, a stream descriptor is created and returned. 
 */
struct stream_desc {
  struct task   *task;        /** the task which opened the stream */
  struct stream *stream;      /** pointer to the stream */
  char mode;                  /** either 'r' or 'w' */
  char state;                 /** one of IOCR, for monitoring */
  unsigned long counter;      /** counts the number of items transmitted
                                  over the stream descriptor */
  int event_flags;            /** which events happened on that stream */
  struct stream_desc *next;   /** for organizing in lists */
  struct stream_desc *dirty;  /** for maintaining a list of 'dirty' items */
};



/**
 * The state of a stream descriptor
 */
#define STDESC_INUSE    'I'
#define STDESC_OPENED   'O'
#define STDESC_CLOSED   'C'
#define STDESC_REPLACED 'R'

/**
 * The event_flags of a stream descriptor
 */
#define STDESC_MOVED    (1<<0)
#define STDESC_WOKEUP   (1<<1)
#define STDESC_WAITON   (1<<2)

/**
 * This special value indicates the end of the dirty list chain.
 * NULL cannot be used as NULL indicates that the SD is not dirty.
 */
#define STDESC_DIRTY_END   ((stream_desc_t *)-1)



#endif /* _STREAM_DESC_H_ */
