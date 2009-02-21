/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Blowfish algorythms: Bruce Schneier and Jim Conger
  Bruce Schneier, 1996, Applied Cryptography, 2nd ed., John Wiley & Sons
  Blowfish Eggdrop algorythms:  Robey Pointer

  Copyright (C) 1994 Bruce Schneier <schneier@counterpane.com>
  Copyright (C) 1996 Jim Conger <jconger@cox.net>
  Copyright (C) Robey Pointer <robey@lag.net>
*/

//---------------------------------------------------------------------------
#include <string.h>
#include "oldblowfish1.h"
#include "oldblowfish2.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#define S(x,i) (SBoxes[i][x.w.byte##i])
#define bf_F(x) (((S(x,0) + S(x,1)) ^ S(x,2)) + S(x,3))
#define ROUND(a,b,n) (a.dword ^= bf_F(b) ^ PArray[n])
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
oldCBlowFish::oldCBlowFish ()
{
 	PArray = new DWORD [18] ;
 	SBoxes = new DWORD [4][256] ;
}

oldCBlowFish::~oldCBlowFish ()
{
	delete [] PArray ;
	delete [] SBoxes ;
}

// the low level (private) encryption function
void oldCBlowFish::Blowfish_encipher (DWORD *xl, DWORD *xr)
{
	union aword  Xl, Xr ;

	Xl.dword = *xl ;
	Xr.dword = *xr ;

	Xl.dword ^= PArray [0];
	ROUND (Xr, Xl, 1) ;  ROUND (Xl, Xr, 2) ;
	ROUND (Xr, Xl, 3) ;  ROUND (Xl, Xr, 4) ;
	ROUND (Xr, Xl, 5) ;  ROUND (Xl, Xr, 6) ;
	ROUND (Xr, Xl, 7) ;  ROUND (Xl, Xr, 8) ;
	ROUND (Xr, Xl, 9) ;  ROUND (Xl, Xr, 10) ;
	ROUND (Xr, Xl, 11) ; ROUND (Xl, Xr, 12) ;
	ROUND (Xr, Xl, 13) ; ROUND (Xl, Xr, 14) ;
	ROUND (Xr, Xl, 15) ; ROUND (Xl, Xr, 16) ;
	Xr.dword ^= PArray [17] ;

	*xr = Xl.dword ;
	*xl = Xr.dword ;
}

// the low level (private) decryption function
void oldCBlowFish::Blowfish_decipher (DWORD *xl, DWORD *xr)
{
   union aword  Xl ;
   union aword  Xr ;

   Xl.dword = *xl ;
   Xr.dword = *xr ;

   Xl.dword ^= PArray [17] ;
   ROUND (Xr, Xl, 16) ;  ROUND (Xl, Xr, 15) ;
   ROUND (Xr, Xl, 14) ;  ROUND (Xl, Xr, 13) ;
   ROUND (Xr, Xl, 12) ;  ROUND (Xl, Xr, 11) ;
   ROUND (Xr, Xl, 10) ;  ROUND (Xl, Xr, 9) ;
   ROUND (Xr, Xl, 8) ;   ROUND (Xl, Xr, 7) ;
   ROUND (Xr, Xl, 6) ;   ROUND (Xl, Xr, 5) ;
   ROUND (Xr, Xl, 4) ;   ROUND (Xl, Xr, 3) ;
   ROUND (Xr, Xl, 2) ;   ROUND (Xl, Xr, 1) ;
   Xr.dword ^= PArray[0];

   *xl = Xr.dword;
   *xr = Xl.dword;
}


// constructs the enctryption sieve
void oldCBlowFish::Initialize (BYTE key[], int keybytes)
{
	int  		i, j ;
	DWORD  		data, datal, datar ;
	union aword temp ;


	// ATTN: new fix for keys > 56, should make it more compatible with FISH
	// but fish uses 80 ???
	if (keybytes>MAXKEYBYTES_COMPATMODE)
		keybytes=MAXKEYBYTES_COMPATMODE;

	// first fill arrays from data tables
	for (i = 0 ; i < 18 ; i++)
		PArray [i] = bf_P [i] ;

	for (i = 0 ; i < 4 ; i++)
	{
	 	for (j = 0 ; j < 256 ; j++)
	 		SBoxes [i][j] = bf_S [i][j] ;
	}


	j = 0 ;
	for (i = 0 ; i < NPASS + 2 ; ++i)
	{
		temp.dword = 0 ;
		temp.w.byte0 = key[j];
		temp.w.byte1 = key[(j+1) % keybytes] ;
		temp.w.byte2 = key[(j+2) % keybytes] ;
		temp.w.byte3 = key[(j+3) % keybytes] ;
		data = temp.dword ;
		PArray [i] ^= data ;
		j = (j + 4) % keybytes ;
	}

	datal = 0 ;
	datar = 0 ;

	for (i = 0 ; i < NPASS + 2 ; i += 2)
	{
		Blowfish_encipher (&datal, &datar) ;
		PArray [i] = datal ;
		PArray [i + 1] = datar ;
	}

	for (i = 0 ; i < 4 ; ++i)
	{
		for (j = 0 ; j < 256 ; j += 2)
		{
		  Blowfish_encipher (&datal, &datar) ;
		  SBoxes [i][j] = datal ;
		  SBoxes [i][j + 1] = datar ;
		}
	}
}

// get output length, which must be even MOD 8
DWORD oldCBlowFish::GetOutputLength (DWORD lInputLong)
{
	DWORD 	lVal ;

	lVal = lInputLong % 8 ;	// find out if uneven number of bytes at the end
	if (lVal != 0)
		return lInputLong + 8 - lVal ;
	else
		return lInputLong ;
}


// Encode pIntput into pOutput.  Input length in lSize.  Returned value
// is length of output which will be even MOD 8 bytes.  Input buffer and
// output buffer can be the same, but be sure buffer length is even MOD 8.
DWORD oldCBlowFish::Encode (BYTE * pInput, BYTE * pOutput, DWORD lSize)
{
	DWORD 	lCount, lOutSize, lGoodBytes ;
	BYTE	*pi, *po ;
	int		i, j ;
	int		SameDest = (pInput == pOutput ? 1 : 0) ;

	lOutSize = GetOutputLength (lSize) ;
	for (lCount = 0 ; lCount < lOutSize ; lCount += 8)
	{
		if (SameDest)	// if encoded data is being written into input buffer
		{
		 	if (lCount < lSize - 7)	// if not dealing with uneven bytes at end
		 	{
		 	 	Blowfish_encipher ((DWORD *) pInput,
		 	 		(DWORD *) (pInput + 4)) ;
		 	}
		 	else		// pad end of data with null bytes to complete encryption
		 	{
				po = pInput + lSize ;	// point at byte past the end of actual data
				j = (int) (lOutSize - lSize) ;	// number of bytes to set to null
				for (i = 0 ; i < j ; i++)
					*po++ = 0 ;
		 	 	Blowfish_encipher ((DWORD *) pInput,
		 	 		(DWORD *) (pInput + 4)) ;
		 	}
		 	pInput += 8 ;
		}
		else 			// output buffer not equal to input buffer, so must copy
		{               // input to output buffer prior to encrypting
		 	if (lCount < lSize - 7)	// if not dealing with uneven bytes at end
		 	{
		 		pi = pInput ;
		 		po = pOutput ;
		 		for (i = 0 ; i < 8 ; i++)  // copy bytes to output
		 			*po++ = *pi++ ;
		 	 	Blowfish_encipher ((DWORD *) pOutput,	// now encrypt them
		 	 		(DWORD *) (pOutput + 4)) ;
		 	}
		 	else		// pad end of data with null bytes to complete encryption
		 	{
		 		lGoodBytes = lSize - lCount ;	// number of remaining data bytes
		 		po = pOutput ;
		 		for (i = 0 ; i < (int) lGoodBytes ; i++)
		 			*po++ = *pInput++ ;
		 		for (j = i ; j < 8 ; j++)
		 			*po++ = 0 ;
		 	 	Blowfish_encipher ((DWORD *) pOutput,
		 	 		(DWORD *) (pOutput + 4)) ;
		 	}
		 	pInput += 8 ;
		 	pOutput += 8 ;
		}
	}
	return lOutSize ;
 }

// Decode pIntput into pOutput.  Input length in lSize.  Input buffer and
// output buffer can be the same, but be sure buffer length is even MOD 8.
void oldCBlowFish::Decode (BYTE * pInput, BYTE * pOutput, DWORD lSize)
{
	DWORD 	lCount ;
	BYTE	*pi, *po ;
	int		i ;
	int		SameDest = (pInput == pOutput ? 1 : 0) ;

	for (lCount = 0 ; lCount < lSize ; lCount += 8)
	{
		if (SameDest)	// if encoded data is being written into input buffer
		{
	 	 	Blowfish_decipher ((DWORD *) pInput,
	 	 		(DWORD *) (pInput + 4)) ;
		 	pInput += 8 ;
		}
		else 			// output buffer not equal to input buffer
		{               // so copy input to output before decoding
	 		pi = pInput ;
	 		po = pOutput ;
	 		for (i = 0 ; i < 8 ; i++)
	 			*po++ = *pi++ ;
	 	 	Blowfish_decipher ((DWORD *) pOutput,
	 	 		(DWORD *) (pOutput + 4)) ;
		 	pInput += 8 ;
		 	pOutput += 8 ;
		}
	}
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// The following code is adapted from the eggdrop bot source:
// by Robey Pointer
// DR - as i understand this, what we have here is a function where you pass
//  it a string(passphrase), and it convolutes it into an encrypted version
//  (witout using a separate key).  and you use this one-way encrypted
//  string later, when you ask person for their passphrase.  when they give
//  you there passphrase, you call this again, and compare the two outputs.
//  if the outputs are the same, then the inputs presumably were, and so
//  passphrase is judged to match.  In this way you can store one-way encrypted
//  passphrases in plain files.
//

#define SALT1  0xdeadd061
#define SALT2  0x23f6b095

// Convert 64-bit encrypted passphrase to text for userfile
char base64[] = "./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

int base64dec(char c)
{
	// updated 1/20/03 for speed improvement
	static char base64unmap[255];
	static bool didinit=false;
	int i;

	if (!didinit)
		{
		// initialize base64unmap
		for (i=0;i<255;++i)
			base64unmap[i]=0;
		for (i=0;i<64;++i)
		        base64unmap[(int)base64[i]]=i;
		didinit=true;
		}

	return base64unmap[(int)c];
}

void blowfish_encrypt_pass(char *text, char *str)
{
	DWORD left,right;
	int n;
	char *p;
	oldCBlowFish caca;
	caca.Initialize((unsigned char*)text,strlen(text));
	left  = SALT1;
	right = SALT2;
	caca.Blowfish_encipher(&left, &right);
	p = str;
	*p++ = '+';			// + means encrypted pass
	n = 32;
	while (n > 0) {
		*p++ = base64[right & 0x3f];
		right = (right >> 6);
		n -= 6;
	}
	n = 32;
	while (n > 0) {
		*p++ = base64[left & 0x3f];
		left = (left >> 6);
		n -= 6;
	}
	*p = 0;
}
//---------------------------------------------------------------------------












//---------------------------------------------------------------------------
// ORIGINAL COMPATIBLE (ECB) METHODS

char *encrypt_string_oldecb(char *key, char *str)
{
	DWORD left, right;
	unsigned char *p;
	char *s, *dest, *d;
	int i;

	// Pad fake string with 8 bytes to make sure there's enough
	s = new char[strlen(str) + 9];
	strcpy(s, str);
	if ((!key) || (!key[0]))
		return s;
	p = (unsigned char*)s;
	dest = new char[(strlen(str) + 9) * 2];
	while (*p)
		p++;
	for (i = 0; i < 8; i++)
		*p++ = 0;

	oldCBlowFish caca;
	caca.Initialize((unsigned char*)key,strlen(key));
	p = (unsigned char*)s;
	d = dest;
	while (*p) {
		left = ((*p++) << 24);
		left += ((*p++) << 16);
		left += ((*p++) << 8);
		left += (*p++);
		right = ((*p++) << 24);
		right += ((*p++) << 16);
		right += ((*p++) << 8);
		right += (*p++);
		caca.Blowfish_encipher(&left, &right);
		for (i = 0; i < 6; i++) {
			*d++ = base64[right & 0x3f];
			right = (right >> 6);
		}
		for (i = 0; i < 6; i++) {
			*d++ = base64[left & 0x3f];
			left = (left >> 6);
		}
	}
	*d = 0;
	delete [] s;
	return dest;
}


// Returned string must be freed when done with it!
char *decrypt_string_oldecb(char *key, char *str)
{
	DWORD left, right;
	char *p, *s, *dest, *d;
	int i;

	// Pad encoded string with 0 bits in case it's bogus
	s = new char[strlen(str) + 12];
	strcpy(s, str);
	if ((!key) || (!key[0]))
		return s;
	p = s;
	dest = new char[strlen(str) + 12];
	while (*p)
		p++;
	for (i = 0; i < 12; i++)
		*p++ = 0;
	oldCBlowFish caca;
	caca.Initialize((unsigned char*)key,strlen(key));
	p = s;
	d = dest;
	while (*p) {
		right = 0L;
		left = 0L;
		for (i = 0; i < 6; i++)
			right |= (base64dec(*p++)) << (i * 6);
		for (i = 0; i < 6; i++)
			left |= (base64dec(*p++)) << (i * 6);
		caca.Blowfish_decipher(&left, &right);
		for (i = 0; i < 4; i++)
			*d++ = (char) ((left & (0xff << ((3 - i) * 8))) >> ((3 - i) * 8));
		for (i = 0; i < 4; i++)
			*d++ = (char) ((right & (0xff << ((3 - i) * 8))) >> ((3 - i) * 8));
	}
	*d = 0;
	delete [] s;
	return dest;
}
//---------------------------------------------------------------------------
