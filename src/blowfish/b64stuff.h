/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
*/

//---------------------------------------------------------------------------
//The base64 code used is from 'The Secure Programming Cookbook for C and C++', By John Viega, Matt Messier
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// To prevent multiple includes
#ifndef _b64stuffh
#define _b64stuffh
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
unsigned char *spc_base64_encode(unsigned char *input, size_t len, int wrap);
unsigned char *spc_base64_decode(unsigned char *buf, size_t *len, int strict, int *err);
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
