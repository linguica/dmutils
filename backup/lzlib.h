#ifndef __LZLIB_H__
#define __LZLIB_H__

unsigned char *encode(unsigned char *input, int inputlen, int *size);
void decode(unsigned char *input, unsigned char *output);
int decodedsize(unsigned char *input);
int derror(char *msg);

#endif
