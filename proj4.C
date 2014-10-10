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
#include "proj4.h"

struct stringinfo readMap(int fd, int size, int threads){
  struct stringinfo out;
  char* file = (char *) mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
  if (file == (char *) -1){
    cerr << "Could not map file\n";
    exit(1);
  }

  int curstr = 0, maxlen = 0, numstr = 0;
  for (int i = 0; i < size; i++){
    char cur = file[i];

    if (isprint(cur) || isspace(cur)){
      curstr++;
    }else if (curstr > 3){
      numstr++;
      if (curstr > maxlen)
        maxlen = curstr;
      curstr = 0;
    }

  }
  if (curstr > 0){
    numstr++;
    if (curstr > maxlen)
      maxlen = curstr;
  }

  out.numstrings = numstr;
  out.maxlen = maxlen;
  munmap(file, size);
  return out;
}

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
    if(argc > 2){
      if (strcmp (argv[2], "mmap") == 0){
        chunk = -1;
      }else{
        chunk = atoi(argv[2]);
      }
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
      out = readMap (fd, inputstat.st_size, 1);
    }else{
      out = readFile(chunk, fd);
    }

    cout << "File size " << inputstat.st_size << " bytes\n";
    cout << "Strings with length at least 4: " << out.numstrings << endl;
    cout << "Maximum string length: " << out.maxlen << endl;

    close(fd);

    return 0;
}
