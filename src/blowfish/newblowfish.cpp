/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
*/

// mc_blowfishnew.cpp
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "newblowfish.h"
#include "b64stuff.h"
#include "BlowfishCbc.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void ChooseIv(char *iv)
{
	// file the 8 byte IV with a nonce
	// ATTN: this is not the best nonce IV generator, we'd like to improve it later

	static unsigned int staticcounter=0;
	static int didinit=0;
	int i;
	time_t t1;
	int part1,part2;
	char *part1p,*part2p;

	// get current time
	time(&t1);
	
	// on first use, initialize random number generator to time
	if (didinit==0)
	    {
		srand((long) t1);
		didinit=1;
		}

	// increment counter
	++staticcounter;
	if (staticcounter>=65534L)
	    staticcounter=0;

	// ok now the IV will be 4 bytes random and 4 bytes time_t
	int intsize=sizeof(int);

	// now part1 and part2 of the 8 byte iv
	// part1 is a timestamp, number of seconds past since 1970
	part1=(int)t1;
	part1p=(char*)(&part1);
	// part2 is a random number, from prng initted with time, plus a counter
	part2=(int)(rand()+staticcounter);
	part2p=(char*)(&part2);

	// now fill IV
	for (i=0;i<4;++i)
		iv[i]=part1p[i%intsize];
	for (i=0;i<4;++i)
		iv[4+i]=part2p[i%intsize];
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// NEW CBCB METHODS
char *encrypt_string_new(char *key, char *str)
{
	char *p;
	char *s, *dest,*dest2;
	char iv[8];
	int i;
	int len;

	// Pad fake string with 8 bytes to make sure there's enough PLUS add 8 bytes for IV
	s = new char[strlen(str) + 9+8];
	if ((!key) || (!key[0]))
		return s;
	// allocate max space we will need for encrypted, base64d
	dest = new char[(strlen(str) + 9+8) * 2];

	// choose a (random or timestamped) IV block, 8 bytes wide and stick it into first 8 bytes of s
	ChooseIv(iv);

	// load the iv into start of s
	for (i=0;i<8;++i)
	    s[i]=iv[i];
	// add user string after
	strcpy(&s[8], str);
	len=8+strlen(str);

	// pad at end of source with 0s
	p = s+len;
	for (i = 0; i < 8; i++)
		*p++ = 0;

	// modify len to be mod%8
	if (len%8!=0)
	    len+=8-(len%8);

	// encrypt into dest (note len does not change)
	CBlowFish oBlowFish((unsigned char*)key, strlen(key));
	oBlowFish.ResetChain();
	oBlowFish.Encrypt((unsigned char*)s,(unsigned char*)dest, len, CBlowFish::CBC);

	// now base 64 it, allocating new dest2 in the process
	dest2=(char*)(spc_base64_encode((unsigned char*)dest,len,0));
	if (dest2==0)
	    {
		delete [] s;
	    return dest;
		}

	// now prefix a * to it while copying to dest and delete s
	strcpy(dest,"*");
	strcat(dest,dest2);
	delete [] dest2;
	delete [] s;

	// return the dest (user will free)
	return dest;
}


// Returned string must be freed when done with it!
char *decrypt_string_new(char *key, char *str)
{
	char *p, *s, *dest;
	char *dest2,*dest3;
	int i;
	int err;
	size_t len;
	char iv[9];

	// Pad encoded string with 0 bits in case it's bogus
	s = new char[strlen(str) + 12];
	strcpy(s, str);

	if ((!key) || (!key[0]))
		return s;
 	dest = new char[strlen(str) + 12 + 8];

	// pad with 0s
	p = s+strlen(str);
	for (i = 0; i < 12; i++)
		*p++ = 0;

	// now unbase64 it, allocating new dest2 in the process
	len=strlen(str);
	dest2=(char*)(spc_base64_decode((unsigned char*)s,&len,0,&err));
	if (dest2==0)
	    {
		delete [] dest;
		return s;
		}
	if (err)
	    {
		// FOR TESTING:
		// printf("ERROR IN BASE 64 decode!\n");
		delete [] dest;
		return s;
		}

	// copy unbase64'd to new temp dest3 then delete dest2
	// pad it to be mod%8 (this should not be necesary on decrypt but could be on a truncated transmission);
 	dest3 = new char[strlen(str) + 12 + 8];
	memcpy(dest3,dest2,len);
	if (len%8!=0)
		{
		int newbytes=8-(len%8);
		while (newbytes>0)
			{
			++len;
			--newbytes;
			dest3[len]='\0';
			}
		}
	delete [] dest2;

	// decrypt from dest3 (length len) to dest
	CBlowFish oBlowFish((unsigned char*)key, strlen(key));
	oBlowFish.ResetChain();
	oBlowFish.Decrypt((unsigned char*)dest3,(unsigned char*)dest, len, CBlowFish::CBC);
	// 0 terminate in case it is exactly mod 8 real characters
	dest[len]='\0';

	// now the first block (8bytes) is just the IV so we don't want to return that
	strncpy(iv,dest,8);
	iv[8]='\0';
	strcpy(dest,&dest[8]);

	// delete s and dest3
	delete [] s;
        delete [] dest3;
	
	// return dest
	return dest;
}
//---------------------------------------------------------------------------
