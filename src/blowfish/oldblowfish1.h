/*
  blowfish1.h     interface file for mc_blowfish.cpp
  _THE BLOWFISH ENCRYPTION ALGORITHM_
  by Bruce Schneier, 1996, Applied Cryptography, 2nd ed., John Wiley & Sons

  Public Domain.

  Copyright (C) 1994 Bruce Schneier <schneier@counterpane.com>
  Copyright (C) 1996 Jim Conger <jconger@cox.net>
*/

//---------------------------------------------------------------------------
// To prevent multiple includes
#ifndef _oldblowfish1h
#define _oldblowfish1h
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Forward declarations
char *encrypt_string_oldecb(char *key, char *str);
char *decrypt_string_oldecb(char *key, char *str);
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
#define MAXKEYBYTES 	56		// 448 bits max
#define NPASS           16		// SBox passes

#define DWORD  		unsigned long
#define WORD  		unsigned short
#define BYTE  		unsigned char
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// for fish compatibility we use 80 instead of 56
#define MAXKEYBYTES_COMPATMODE 80
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
//#include "oldblowfish2.h"
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// choose a byte order for your hardware

// NEW 7/10/03 trying to be smarter about endian
// first try to be smart about it, and user can define LITTLE_ENDIAN or BIG_ENDIAN from makefile
#ifdef LITTLE_ENDIAN
 #define ORDER_DCBA // choosing Intel in this case
#else
 #ifdef BIG_ENDIAN
  #define ORDER_ABCD
 #endif
#endif

// default to intel if not overridden
#ifndef ORDER_DCBA
 #ifndef ORDER_ABCD
  #define ORDER_DCBA
 #endif
#endif

#ifdef ORDER_DCBA  	// DCBA - little endian - intel
// ATTN: THIS IS THE MAJOR CAUSE OF INCOMPATIBILITIES WITH OTHER BLOWFISH IMPLEMENTATIONS
//  AS THIS LITTLE ENDIAN MODE IS USED ON THE PC BY THIS BLOWFISH, AND *NOT* IN OTHER BLOWFISHES
//  THE FIX I THINK IS SIMPLE, THE bytex VARS ARE USED IN ONLY 1 PLACE IN MC_BLOWFISH.CPP AND YOU
//  SHOULD BE ABLE TO MODIFY YOUR BLOWFISH CODE TO USE (4-BYTE#) instead of BYTE# IN ONE LINE OF CODE
//  TO FIX THE INCOMPATIBILITY.
// THERE IS ALSO A BASE64 INCOMPATIBILITY THAT YOU NEED TO FIX TO USE OTHER BLOWFISH ALGORITHMS.
//  WE ARE WORKING TO MAKE A COLLECTION OF COMPATIBLE ROUTINES IN DIF. LANGUAGES (java, php, perl).
// PLEASE BARE WITH US - WE CHOSE TO USE THESE METHODS TO INSURE BACKWARD COMPATIBILITY WITH EXISTING SCRIPTS
	union aword {
	  DWORD dword;
	  BYTE byte [4];
	  struct {
	    unsigned int byte3:8;
	    unsigned int byte2:8;
	    unsigned int byte1:8;
	    unsigned int byte0:8;
	  } w;
	};
#endif

#ifdef ORDER_ABCD  	// ABCD - big endian - motorola
	union aword {
	  DWORD dword;
	  BYTE byte [4];
	  struct {
	    unsigned int byte0:8;
	    unsigned int byte1:8;
	    unsigned int byte2:8;
	    unsigned int byte3:8;
	  } w;
	};
#endif

#ifdef ORDER_BADC  	// BADC - vax
	union aword {
	  DWORD dword;
	  BYTE byte [4];
	  struct {
	    unsigned int byte1:8;
	    unsigned int byte0:8;
	    unsigned int byte3:8;
	    unsigned int byte2:8;
	  } w;
};
#endif
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
class oldCBlowFish
{
private:
	DWORD 		* PArray ;
	DWORD		(* SBoxes)[256];
public:
	void 		Blowfish_encipher (DWORD *xl, DWORD *xr) ;
	void 		Blowfish_decipher (DWORD *xl, DWORD *xr) ;
public:
	oldCBlowFish () ;
	~oldCBlowFish () ;
	void 		Initialize (BYTE key[], int keybytes) ;
	DWORD		GetOutputLength (DWORD lInputLong) ;
	DWORD		Encode (BYTE * pInput, BYTE * pOutput, DWORD lSize) ;
	void		Decode (BYTE * pInput, BYTE * pOutput, DWORD lSize) ;
} ;
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

