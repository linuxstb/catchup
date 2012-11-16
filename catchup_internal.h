#ifndef _CATCHUP_INTERNAL_H
#define _CATCHUP_INTERNAL_H

#include <stdint.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "catchup.h"
#include <curl/curl.h>
  
struct MemoryStruct {
  char *memory;
  size_t size;
};

/* catchup_utils.c */

xmlChar* myXpathGetValue(xmlXPathContextPtr xpathCtx, const xmlChar* str, const xmlChar* attrname);
int xml_fetch(char* url, struct MemoryStruct* chunk);
int do_post(char* url, struct curl_slist *slist, struct MemoryStruct* postthis, struct MemoryStruct* chunk);

/* catchup_iplayer.c */
int catchup_get_info_iplayer(struct catchup_t* cu);

/* catchup_itv.c */
int catchup_get_info_itv(struct catchup_t* cu);

/* catchup_4oD.c */
int catchup_get_info_4oD(struct catchup_t* cu);

/* catchup_demand5.c */
int catchup_get_info_demand5(struct catchup_t* cu);

/*
blowfish.h:  Header file for blowfish.c

Copyright (C) 1997 by Paul Kocher

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


See blowfish.c for more information about this file.
*/

typedef struct {
  uint32_t P[16 + 2];
  uint32_t S[4][256];
} BLOWFISH_CTX;

void Blowfish_Init(BLOWFISH_CTX *ctx, unsigned char *key, int keyLen);
void Blowfish_Encrypt(BLOWFISH_CTX *ctx, uint32_t *xl, uint32_t *xr);
void Blowfish_Decrypt(BLOWFISH_CTX *ctx, unsigned char* x);

/*
cdecode.h - c header for a base64 decoding algorithm

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

typedef enum
{
  step_a, step_b, step_c, step_d
} base64_decodestep;

typedef struct
{
  base64_decodestep step;
  signed char plainchar;
} base64_decodestate;

void base64_init_decodestate(base64_decodestate* state_in);
int base64_decode_value(signed char value_in);
int base64_decode_block(const signed char* code_in, const int length_in, signed char* plaintext_out, base64_decodestate* state_in);


#endif
