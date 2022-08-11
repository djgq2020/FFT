#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DEF_NFFT	10
#define PI 			3.14159265359

void getfft(int nfft, unsigned short *pSrc, unsigned short *pDst);

unsigned short src[1<<DEF_NFFT];
unsigned short dst[1<<DEF_NFFT];

int main(int argc, int argv[])
{
	for(int i=0;i<(1<<DEF_NFFT);i++)
	{
		src[i]  = ((sin(2*PI*i*16/(1<<DEF_NFFT)+PI/2)+sin(2*PI*i*64/(1<<DEF_NFFT))+1)*1024);
	}
	getfft(DEF_NFFT, src, dst);
	for(int i=0;i<(1<<DEF_NFFT);i++)
	{
		printf("freq %4d ~ amp %4d\n", i, dst[i]);
	}
	return 0;
}
