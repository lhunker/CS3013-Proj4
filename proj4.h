//Definitions
#define MAXCHUNK 8192

//structs
struct stringinfo{
  int numstrings;
  int maxlen;
};

//Function definitions
struct stringinfo readFile (int chunk, int fd);
