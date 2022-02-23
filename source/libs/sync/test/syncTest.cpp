#include <stdio.h>
#include "syncIO.h"
#include "syncInt.h"
#include "syncRaftStore.h"

void *pingFunc(void *param) {
  SSyncIO *io = (SSyncIO *)param;
  while (1) {
    sDebug("io->ping");
    io->ping(io);
    sleep(1);
  }
  return NULL;
}

int main() {

  testJson();
  testJson2();

  tsAsyncLog = 0;
  taosInitLog((char *)"syncTest.log", 100000, 10);

  sDebug("sync test");
  syncStartEnv();

  SSyncIO *syncIO = syncIOCreate();
  assert(syncIO != NULL);

  syncIO->start(syncIO);

  sleep(2);

  pthread_t tid;
  pthread_create(&tid, NULL, pingFunc, syncIO);

  while (1) {
    sleep(1);
  }
  return 0;
}
