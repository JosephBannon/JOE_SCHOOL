#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  /* int i = -7; */
  /* int i2 = 0; */
  /* setwritecount(0); */
  /* i = writecount(); */
  /* printf(1,"Count for setwritecount(0): %d \n", i); */
  /* setwritecount(40); */
  /* i = writecount(); */
  /* printf(1,"Count for setwritecount(40): %d \n", i); */
  /* setwritecount(0); */
  /* write(1,"p\n",3); */
  /* i = writecount(); */
  /* printf(1,"Count for one write: %d \n", i); */
  /* setwritecount(0); */
  /* write(1,"o\n",3); */
  /* write(1,"pcorn\n",7); */
  /* i2 = writecount(); */
  /* i = i+i2; */
  /* printf(1,"Count for three writes: %d \n", i); */
  settickets(10);
  settickets(20);
  settickets(30);
  exit();
}
