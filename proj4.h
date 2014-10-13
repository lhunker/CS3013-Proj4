//Definitions
#define MAXCHUNK 8192
#define MAXTHREAD 16

//structs
struct stringinfo{	//A structure to hold the information about strings found
  int numstrings;
  int maxlen;
};

//Function definitions
struct stringinfo readFile (int chunk, int fd);
struct stringinfo readMap(int fd, int size, int threads);
void * worker (void * arg);
