#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include <string.h>

#include "../hashtab.h"


hashtab_t *ht;
  


static void header(const char *s);



static void testPut(void)
{
  int i;

  header("Adding to hashtab");

  /* create the hashtab, capacity=2^4 */
  ht = HashtabCreate( 4);

  /* add up to max */
  for (i=0; i<12; i++) {
    HashtabPut( ht, 3+i*16, (void*) 0x100+i );
  }
  HashtabPrintDebug( ht, stdout);

  /* add one more */
  HashtabPut( ht, 3+i*16, (void*) 0x100+i );
  HashtabPrintDebug( ht, stdout);

  /* get the keys again */
  for (i=0; i<13; i++) {
    int key = 3+i*16;
    void *res = HashtabGet( ht, key);
    printf("get key %2d: value %p\n", key, res);
  }
  HashtabDestroy(ht);
}


int main(void)
{
  /* tests */
  testPut();

  return 0;
}


/*** HELPER *****************************************************************/

static void header(const char *s)
{
  int i,len = strlen(s);
  fprintf(stderr,"=== %s ", s);
  for (i=0; i<(79-len-5); i++) fprintf(stderr,"=");
  fprintf(stderr,"\n");
}

