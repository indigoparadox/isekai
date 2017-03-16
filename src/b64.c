/*********************************************************************\

MODULE NAME:    b64.c

AUTHOR:         Bob Trower 2001/08/04

PROJECT:        Crypt Data Packaging

COPYRIGHT:      Copyright (c) Trantor Standard Systems Inc., 2001

NOTES:          This source code may be used as you wish, subject to
                the MIT license.  See the LICENCE section below.

                Canonical source should be at:
                    http://base64.sourceforge.net

DESCRIPTION:
                This little utility implements the Base64
                Content-Transfer-Encoding standard described in
                RFC1113 (http://www.faqs.org/rfcs/rfc1113.html).

                This is the coding scheme used by MIME to allow
                binary data to be transferred by SMTP mail.

                Groups of 3 bytes from a binary stream are coded as
                groups of 4 bytes in a text stream.

                The input stream is 'padded' with zeros to create
                an input that is an even multiple of 3.

                A special character ('=') is used to denote padding so
                that the stream can be decoded back to its exact size.

                Encoded output is formatted in lines which should
                be a maximum of 72 characters to conform to the
                specification.  This program defaults to 72 characters,
                but will allow more or less through the use of a
                switch.  The program enforces a minimum line size
                of 4 characters.

                Example encoding:

                The stream 'ABCD' is 32 bits long.  It is mapped as
                follows:

                ABCD

                 A (65)     B (66)     C (67)     D (68)   (None) (None)
                01000001   01000010   01000011   01000100

                16 (Q)  20 (U)  9 (J)   3 (D)    17 (R) 0 (A)  NA (=) NA (=)
                010000  010100  001001  000011   010001 000000 000000 000000


                QUJDRA==

                Decoding is the process in reverse.  A 'decode' lookup
                table has been created to avoid string scans.

DESIGN GOALS:  Specifically:
      Code is a stand-alone utility to perform base64
      encoding/decoding. It should be genuinely useful
      when the need arises and it meets a need that is
      likely to occur for some users.
      Code acts as sample code to show the author's
      design and coding style.

      Generally:
      This program is designed to survive:
      Everything you need is in a single source file.
      It compiles cleanly using a vanilla ANSI C compiler.
      It does its job correctly with a minimum of fuss.
      The code is not overly clever, not overly simplistic
      and not overly verbose.
      Access is 'cut and paste' from a web page.
      Terms of use are reasonable.

VALIDATION:     Non-trivial code is never without errors.  This
                file likely has some problems, since it has only
                been tested by the author.  It is expected with most
                source code that there is a period of 'burn-in' when
                problems are identified and corrected.  That being
                said, it is possible to have 'reasonably correct'
                code by following a regime of unit test that covers
                the most likely cases and regression testing prior
                to release.  This has been done with this code and
                it has a good probability of performing as expected.

                Unit Test Cases:

                case 0:empty file:
                    CASE0.DAT  ->  ->
                    (Zero length target file created
                    on both encode and decode.)

                case 1:One input character:
                    CASE1.DAT A -> QQ== -> A

                case 2:Two input characters:
                    CASE2.DAT AB -> QUI= -> AB

                case 3:Three input characters:
                    CASE3.DAT ABC -> QUJD -> ABC

                case 4:Four input characters:
                    case4.dat ABCD -> QUJDRA== -> ABCD

                case 5:All chars from 0 to ff, linesize set to 50:

                    AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIj
                    JCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZH
                    SElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWpr
                    bG1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6P
                    kJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKz
                    tLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX
                    2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7
                    /P3+/w==

                case 6:Mime Block from e-mail:
                    (Data same as test case 5)

                case 7: Large files:
                    Tested 28 MB file in/out.

                case 8: Random Binary Integrity:
                    This binary program (b64.exe) was encoded to base64,
                    back to binary and then executed.

                case 9 Stress:
                    All files in a working directory encoded/decoded
                    and compared with file comparison utility to
                    ensure that multiple runs do not cause problems
                    such as exhausting file handles, tmp storage, etc.

                -------------

                Syntax, operation and failure:
                    All options/switches tested.  Performs as
                    expected.

                case 10:
                    No Args -- Shows Usage Screen
                    Return Code 1 (Invalid Syntax)
                case 11:
                    One Arg (invalid) -- Shows Usage Screen
                    Return Code 1 (Invalid Syntax)
                case 12:
                    One Arg Help (-?) -- Shows detailed Usage Screen.
                    Return Code 0 (Success -- help request is valid).
                case 13:
                    One Arg Help (-h) -- Shows detailed Usage Screen.
                    Return Code 0 (Success -- help request is valid).
                case 14:
                    One Arg (valid) -- Uses stdin/stdout (filter)
                    Return Code 0 (Sucess)
                case 15:
                    Two Args (invalid file) -- shows system error.
                    Return Code 2 (File Error)
                case 16:
                    Encode non-existent file -- shows system error.
                    Return Code 2 (File Error)
                case 17:
                    Out of disk space -- shows system error.
                    Return Code 3 (File I/O Error)
                case 18:
                    Too many args -- shows system error.
                    Return Code 1 (Invalid Syntax)

                -------------

                Compile/Regression test:
                    gcc compiled binary under Cygwin
                    Microsoft Visual Studio under Windows 2000
                    Microsoft Version 6.0 C under Windows 2000

DEPENDENCIES:   None

LICENCE:        Copyright (c) 2001 Bob Trower, Trantor Standard Systems Inc.

                Permission is hereby granted, free of charge, to any person
                obtaining a copy of this software and associated
                documentation files (the "Software"), to deal in the
                Software without restriction, including without limitation
                the rights to use, copy, modify, merge, publish, distribute,
                sublicense, and/or sell copies of the Software, and to
                permit persons to whom the Software is furnished to do so,
                subject to the following conditions:

                The above copyright notice and this permission notice shall
                be included in all copies or substantial portions of the
                Software.

                THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
                KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
                WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
                PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
                OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
                OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
                OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
                SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

VERSION HISTORY:
                Bob Trower 2001/08/04 -- Create Version 0.00.00B
                Bob Trower 2001/08/17 -- Correct documentation, messages.
                                      -- Correct help for linesize syntax.
                                      -- Force error on too many arguments.
                Bob Trower 2001/08/19 -- Add sourceforge.net reference to
                                         help screen prior to release.
                Bob Trower 2004/10/22 -- Cosmetics for package/release.
                Bob Trower 2008/02/28 -- More Cosmetics for package/release.
                Bob Trower 2011/02/14 -- Cast altered to fix warning in VS6.
                Bob Trower 2015/10/29 -- Change *bug* from 0 to EOF in putc
                                         invocation. BIG shout out to people
                                         reviewing on sourceforge,
                                         particularly rachidc, jorgeventura
                                         and zeroxia.
                                      -- Change date format to conform with
                                         latest convention.

\******************************************************************* */

#define B64_C
#include "b64.h"

/*
** Translation Table as described in RFC1113
*/
static const char cb64[]=
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
** Translation Table to decode (created by author)
*/
static const char cd64[]=
   "|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

static const unsigned char d[] = {
    66,66,66,66,66,66,66,66,66,66,64,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,62,66,66,66,63,52,53,
    54,55,56,57,58,59,60,61,66,66,66,65,66,66,66, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,66,66,66,66,66,66,26,27,28,
    29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66
};

#define b64_beof( i, l ) (i < l)

/*
** encode
**
** base64 encode a stream adding padding and line breaks as per spec.
*/
void b64_encode( void* indata, SCAFFOLD_SIZE indata_len, bstring outstring, SCAFFOLD_SIZE_SIGNED linesz ) {
   const uint8_t* indata_bytes = (const uint8_t *)indata;
   SCAFFOLD_SIZE i;
   uint32_t concat32bits = 0;
   uint8_t split6bits[4] = { 0 };
   SCAFFOLD_SIZE padding = indata_len % 3;
   int bstr_ret;

   scaffold_assert( NULL != outstring );

   /* increment over the length of the string, three characters at a time */
   for( i = 0; indata_len > i ; i += 3 ) {
      /* Three bytes become one 24-bit number */
      /* Parents prevent a situation where compiler can do the shifting       *
       * before conversion to uint32_t, resulting in 0.                       */
      concat32bits = ((uint32_t)indata_bytes[i]) << 16;

      if( (i + 1) < indata_len ) {
         concat32bits += ((uint32_t)indata_bytes[i + 1]) << 8;
      }

      if( (i + 2) < indata_len ) {
         concat32bits += indata_bytes[i + 2];
      }

      /* 24-bit number gets separated into four 6-bit numbers. */
      split6bits[0] = (uint8_t)(concat32bits >> 18) & 63;
      split6bits[1] = (uint8_t)(concat32bits >> 12) & 63;
      split6bits[2] = (uint8_t)(concat32bits >> 6) & 63;
      split6bits[3] = (uint8_t)(concat32bits & 63);

      /* Spread one byte over two characters. */
      bstr_ret = bconchar( outstring, cb64[split6bits[0]] );
      scaffold_check_nonzero( bstr_ret );
      bstr_ret = bconchar( outstring, cb64[split6bits[1]] );
      scaffold_check_nonzero( bstr_ret );

      /* Add additional byte to third character. */
      if( (i + 1) < indata_len ) {
         bstr_ret = bconchar( outstring, cb64[split6bits[2]] );
         scaffold_check_nonzero( bstr_ret );
      }

      /* Add third byte to fourth character. */
      if( (i + 2) < indata_len ) {
         bstr_ret = bconchar( outstring, cb64[split6bits[3]] );
         scaffold_check_nonzero( bstr_ret );
      }
   }

   /* Padding. */
   if( 0 < padding ) {
      for( ; 3 > padding ; padding++ ) {
         bstr_ret = bconchar( outstring, '=' );
         scaffold_check_nonzero( bstr_ret );
      }
   }
cleanup:
   return;
}

#define WHITESPACE 64
#define EQUALS     65
#define INVALID    66

#if 0

/*
** decode
**
** decode a base64 encoded stream discarding padding, line breaks and noise
*/
void* b64_decode( SCAFFOLD_SIZE* outdata_len, bstring instring ) {
   char iter = 0;
   uint32_t buf = 0;
   SCAFFOLD_SIZE indata_index = 0;
   SCAFFOLD_SIZE outdata_index = 0;
   uint8_t* outdata = NULL;

   scaffold_error = SCAFFOLD_ERROR_NONE;

   *outdata_len = blength( instring );

   outdata = calloc( *outdata_len, sizeof( uint8_t ) );
   scaffold_check_null( outdata );

   while( blength( instring ) > indata_index ) {
      unsigned char c = d[bdata( instring )[indata_index++]];

      switch( c ) {
         case WHITESPACE: continue;   /* skip whitespace */

         case INVALID:
            scaffold_print_error( "Invalid input at position %d. Aborting.\n", indata_index );
            scaffold_error = SCAFFOLD_ERROR_MISC;
            goto cleanup;   /* invalid input, return error */

         case EQUALS:                 /* pad character, end of data */
            indata_index = blength( instring );
            continue;

         default:
            buf = buf << 6 | c;
            iter++; /* increment the number of iteration */
            /* If the buffer is full, split it into bytes */
            if( 4 == iter) {
               if( (outdata_index += 3) > *outdata_len ) {
                  /* return 1; / buffer overflow */
                  *outdata_len *= 2;
                  outdata = scaffold_realloc( outdata, *outdata_len, BYTE );
                  scaffold_check_null( outdata );
               }
               outdata[outdata_index++] = (buf >> 16) & 255;
               outdata[outdata_index++] = (buf >> 8) & 255;
               outdata[outdata_index++] = buf & 255;
               buf = 0;
               iter = 0;
            }
      }
   }

   if( 3 == iter ) {
      if( (outdata_index += 2) > *outdata_len ) {
         /* return 1; / buffer overflow */
         *outdata_len *= 2;
         outdata = scaffold_realloc( outdata, *outdata_len, BYTE );
         scaffold_check_null( outdata );
      }
      outdata[outdata_index++] = (buf >> 10) & 255;
      outdata[outdata_index++] = (buf >> 2) & 255;
   } else if( 2 == iter ) {
      if( ++outdata_index > *outdata_len ) {
         /* return 1; / buffer overflow */
         *outdata_len *= 2;
         outdata = scaffold_realloc( outdata, *outdata_len, BYTE );
         scaffold_check_null( outdata );

      }
      outdata[outdata_index++] = (buf >> 4) & 255;
   }

   /* Reality check for size. */
   *outdata_len = outdata_index + 1;
   outdata = scaffold_realloc( outdata, *outdata_len, BYTE );
   scaffold_check_null( outdata );
cleanup:
   return outdata;
}

#endif

int b64_decode( bstring indata, unsigned char *out, SCAFFOLD_SIZE *outLen ) {
    /* char *end = in + inLen; */
    char iter = 0;
    uint32_t buf = 0;
    SCAFFOLD_SIZE len = 0;
    SCAFFOLD_SIZE indata_index = 0;

    /* while (in < end) { */
    /* while( indata_index < inLen ) { */
    while( indata_index < blength( indata ) ) {
        /* unsigned char c = d[*in++]; */
        /* unsigned char c = d[in[indata_index++]]; */
        unsigned char c = d[(int)(bdata( indata )[indata_index++])];

        switch (c) {
        case WHITESPACE: continue;   /* skip whitespace */
        case INVALID:    return 1;   /* invalid input, return error */
        case EQUALS:                 /* pad character, end of data */
            /* in = end; */
            indata_index = blength( indata );
            continue;
        default:
            buf = buf << 6 | c;
            iter++; /* increment the number of iteration */
            /* If the buffer is full, split it into bytes */
            if (iter == 4) {
                if ((len += 3) > *outLen) return 1; /* buffer overflow */
                *(out++) = (buf >> 16) & 255;
                *(out++) = (buf >> 8) & 255;
                *(out++) = buf & 255;
                buf = 0; iter = 0;

            }
        }
    }

    if (iter == 3) {
        if ((len += 2) > *outLen) return 1; /* buffer overflow */
        *(out++) = (buf >> 10) & 255;
        *(out++) = (buf >> 2) & 255;
    }
    else if (iter == 2) {
        if (++len > *outLen) return 1; /* buffer overflow */
        *(out++) = (buf >> 4) & 255;
    }

    *outLen = len; /* modify to reflect the actual output size */
    return 0;
}
