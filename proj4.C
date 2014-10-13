/*
Lukas Hunker
proj4.C
Takes in a file and a method to read it, then counts te number of strings with length > 3 
*/

#include <iostream>
using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "proj4.h"
#include "mailboxs.h"

mailboxs * boxs;  //Mailboxs for interthread communication
char * filemap; //Where the file is mapped
int threadNum;  //The umber of threads to use

/*
 * worker
 * The worker process for reading multithreaded from a memory map
*/
void * worker (void * arg){
  int myId = *(int *) arg;
  delete (int *) arg;
  struct msg carry;
  carry.type = BLANK;
  struct msg range;

  //recive range
  boxs->RecvMsg(myId, &range);
  if (range.type == CARRY){
    carry = range;
    cout << "got carry\n";
    boxs->RecvMsg(myId, &range);
  }
  int start = range.value1;
  int end = range.value2;

  //recive carry
  if( myId != threadNum && carry.type == BLANK){
    boxs->RecvMsg(myId, &carry);
  }else if (myId == threadNum){
    carry.value1 = 0;
  }

  //Find first string
  int pos = start;
  if (myId != 1){
    int carrylen = 0;
    while ((isprint(filemap[pos]) || isspace(filemap[pos])) && 
          pos < end){
      carrylen++;
      pos++;
    }
    if (pos  == end){
      carrylen += carry.value1;
      carry.value1 = 0;
      cout << "Thread " << myId << "Carrying all\n";
    }
    struct msg * carryout = new struct msg;
    carryout->type = CARRY;
    carryout->iFrom = myId;
    carryout->value1 = carrylen;
    boxs->SendMsg ((myId-1), carryout);
  }


  //process chunk
  int curstr = 0, maxlen = 0, numstr = 0;
  for (int i = pos; i < end; i++){
    char cur = filemap[i];

    if (isprint(cur) || isspace(cur)){
      curstr++;
    }else if (curstr > 3){
      numstr++;
      if (curstr > maxlen)
        maxlen = curstr;
      curstr = 0;
    }else{
      curstr = 0;
    }
  }
  curstr += carry.value1;
  if (curstr > 3){
    numstr++;
    if (curstr > maxlen)
      maxlen = curstr;
  }
  //Send done message
  struct msg * done = new struct msg;
  done->type = ALLDONE;
  done->iFrom = myId;
  done->value1 = numstr;
  done->value2 = maxlen;
  boxs->SendMsg (0, done);
}

/*
 * readMap
 * Reads the file useing memory mapped IO
 * Params:
 *    fd - the file descriptor to read from
 *    size - the file size
 *    threads - the number of threads to read with
 * Returns:
 *    A structure countaining the number of strings and max string length
*/
struct stringinfo readMap(int fd, int size, int threads){
  struct stringinfo out;
  filemap = (char *) mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
  if (filemap == (char *) -1){
    cerr << "Could not map file\n";
    exit(1);
  }
  threadNum = threads;
  boxs = new mailboxs(threads +1);

  //Initilaize thread ids
  pthread_t workers [threads];

  //calculate range and spawn threads
  int perthread = size/threads;
  int rem = size % threads;
  int next = 0;
  for (int i = 0; i < threads; i++){
    int start = next;
    int end = start + perthread;
    if(rem > 0){
      end++;
      rem--;
    }
    
    int * id = new int;
    *id = i +1;
    if(pthread_create(&workers[i], NULL, worker, (void *)id) != 0){
      cerr << "error creating thread\n";
      exit(1);
    }
    struct msg * send = new struct msg;
    send->iFrom = 0;
    send->type = RANGE;
    send->value1 = start;
    send->value2 = end;
    boxs->SendMsg(i+1, send);

    next = end;
  }

  int numstr = 0, maxlen = 0;
  for (int i = 0; i < threads; i++){
    struct msg msgin;
    boxs->RecvMsg(0, &msgin);
    numstr += msgin.value1;
    if (msgin.value2 > maxlen)
      maxlen = msgin.value2;
  }

  //rejoin worker threads
  for (int i = 0; i < threads; i++){
    pthread_join(workers[i], NULL);
  }
  delete boxs;

  

  out.numstrings = numstr;
  out.maxlen = maxlen;
  munmap(filemap, size);
  return out;
}

/*
 * readFile
 * Reads the file in chunks using the read system call
 * Params:
 *    chunk - the size of chunk to read each time
 *    fd - the file descriptor to read from
 * Returns:
 *    A structure countaining the number of strings and max string length
*/
struct stringinfo readFile (int chunk, int fd){
  struct stringinfo out;
  char buf[chunk];
  int cnt, numstr = 0, maxlen = 0;
  int carry = 0;

  while((cnt = read(fd, buf, chunk)) > 0){
    int curstr = carry;
    for (int i = 0; i < cnt; i++){
      char cur = buf[i];

      if (isprint(cur) || isspace(cur)){
        curstr++;
      }else if (curstr > 3){
        numstr++;
        if (curstr > maxlen)
          maxlen = curstr;
        curstr = 0;
      }else{
        curstr = 0;
      }

    }
    carry = curstr;
  }

  if (carry > 3){
    numstr++;
    if (carry > maxlen){
      maxlen = carry;
    }
  }


  out.numstrings = numstr;
  out.maxlen = maxlen;

  return out;
}

int main (int argc, char * argv[]){
    if(argc < 2){
      cout << "Usage ./proj4 srcfile [size|mmap]\n";
      return 1;
    }
    int chunk = 1024;
    int threads = 1;
    if(argc > 2){
      if (strcmp (argv[2], "mmap") == 0){
        chunk = -1;
      }else if (argv[2][0] == 'p'){  //use multithreading
        chunk = -1;
        char * threadstr = argv[2];
        threadstr++;  //eliminate 'p'
        threads = atoi(threadstr);
      }else{
        chunk = atoi(argv[2]);
      }
    }
    if (threads <= 0){
      cerr << "There must be at least 1 thread\n";
      return 1;
    } else if (threads > MAXTHREAD){
      cerr << "Too many threads specified, setting threads to " << MAXTHREAD << endl;
      threads = MAXTHREAD;
    }

    //Open File
    int fd;
    if((fd = open(argv[1], O_RDONLY)) < 0){
      cerr << "Error Opening file\n";
      return 1;
    }

    //Get file info
    struct stat inputstat;
    if(fstat(fd, &inputstat) < 0){
      cerr << "Error getting file stats\n";
      return 1;
    }

    //Validate chunk input and call methods
    struct stringinfo out;
    if(chunk > MAXCHUNK){
      cerr << "Chunk size is too big, setting to max size\n";
      chunk = MAXCHUNK;
    }
    if(chunk == 0){
      cerr << "Invalid input for [size|mmap]\n";
      return 1;
    } else if (chunk == -1){
      out = readMap (fd, inputstat.st_size, threads);
    }else{
      out = readFile(chunk, fd);
    }

    cout << "File size " << inputstat.st_size << " bytes\n";
    cout << "Strings with length at least 4: " << out.numstrings << endl;
    cout << "Maximum string length: " << out.maxlen << endl;

    close(fd);

    return 0;
}

