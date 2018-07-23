/* ezxml.c
 *
 * Copyright 2004-2006 Aaron Voisine <aaron@voisine.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "scaffold.h"

#ifdef __linux
#include <unistd.h>
#endif /* __linux */

#ifndef __palmos__
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#endif /* __palmos__ */

#include <stdio.h>
#include <stdarg.h>
#ifndef EZXML_NOMMAP
#include <sys/mman.h>
#endif /* EZXML_NOMMAP */

#define EZXML_C
#include "ezxml.h"

#define EZXML_WS   "\t\r\n "  /* whitespace */
#define EZXML_ERRL 128      /* maximum error string length */

typedef struct ezxml_root *ezxml_root_t;
struct ezxml_root {      /* additional data for the root tag */
   struct ezxml xml;    /*!< A super-struct built on top of ezxml struct. */
   ezxml_t cur;         /*!< Current xml tree insertion point. */
   char *m;             /*!< Original xml string. */
   SCAFFOLD_SIZE_SIGNED len;   /*!< Length of memory for mmap, -1 for malloc. */
   char *u;             /*!< UTF-8 conversion of string if orig was UTF-16. */
   char *s;             /*!< Start of work area. */
   char *e;             /*!< End of work area. */
   char **ent;          /*!< General entities (ampersand sequences). */
   char ***attr;        /*!< Default attributes. */
   char ***pi;          /*!< Processing instructions. */
   short standalone;    /*!< Non-zero if <?xml standalone="yes"?>. */
   char err[EZXML_ERRL];/*!< Error string. */
};

/** \brief Empty, null terminated array of strings.
 */
char *EZXML_NIL[] = { NULL };

/** \brief Returns the first child tag with the given name or NULL if not
 *         found.
 */
ezxml_t ezxml_child( ezxml_t xml, const char *name ) {
   if( NULL != xml ) {
      xml = xml->child;
   } else {
      xml = NULL;
   }
   while( NULL != xml && 0 != strcmp( name, xml->name ) ) {
      xml = xml->sibling;
   }
   return xml;
}

/** \brief Returns the Nth tag with the same name in the same subsection or
 *         NULL if not found.
 */
ezxml_t ezxml_idx(ezxml_t xml, int idx) {
   for (; xml && idx; idx--) xml = xml->next;
   return xml;
}

/** \brief Returns the value of the requested tag attribute or NULL if not
 *         found.
 */
const char *ezxml_attr(ezxml_t xml, const char *attr) {
   int i = 0, j = 1;
   ezxml_root_t root = (ezxml_root_t)xml;

   if (! xml || ! xml->attr) return NULL;
   while (xml->attr[i] && strcmp(attr, xml->attr[i])) i += 2;
   if (xml->attr[i]) return xml->attr[i + 1]; /* found attribute */

   while (root->xml.parent) root = (ezxml_root_t)root->xml.parent; /* root tag */
   for (i = 0; root->attr[i] && strcmp(xml->name, root->attr[i][0]); i++);
   if (! root->attr[i]) return NULL; /* no matching default attributes */
   while (root->attr[i][j] && strcmp(attr, root->attr[i][j])) j += 3;
   return (root->attr[i][j]) ? root->attr[i][j + 1] : NULL; /* found default */
}

/** \brief Same as ezxml_get but takes an already initialized va_list.
 */
static ezxml_t ezxml_vget(ezxml_t xml, va_list ap) {
   char *name = va_arg(ap, char *);
   int idx = -1;

   if (name && *name) {
      idx = va_arg(ap, int);
      xml = ezxml_child(xml, name);
   }
   return (idx < 0) ? xml : ezxml_vget(ezxml_idx(xml, idx), ap);
}

/* Traverses the xml tree to retrieve a specific subtag. Takes a variable */
/* length list of tag names and indexes. The argument list must be terminated */
/* by either an index of -1 or an empty string tag name. Example: */
/* title = ezxml_get(library, "shelf", 0, "book", 2, "title", -1); */
/* This retrieves the title of the 3rd book on the 1st shelf of library. */
/* Returns NULL if not found. */
ezxml_t ezxml_get(ezxml_t xml, ...) {
   va_list ap;
   ezxml_t r;

   va_start(ap, xml);
   r = ezxml_vget(xml, ap);
   va_end(ap);
   return r;
}

/** \brief Returns a null terminated array of processing instructions for the
 *         given target.
 */
const char **ezxml_pi(ezxml_t xml, const char *target) {
   ezxml_root_t root = (ezxml_root_t)xml;
   int i = 0;

   if (! root) return (const char **)EZXML_NIL;
   while (root->xml.parent) root = (ezxml_root_t)root->xml.parent; /* root tag */
   while (root->pi[i] && strcmp(target, root->pi[i][0])) i++; /* find target */
   return (const char **)((root->pi[i]) ? root->pi[i] + 1 : EZXML_NIL);
}

/** \brief Set an error string and return root.
 */
static ezxml_t ezxml_err(ezxml_root_t root, char *s, const char *err, ...) {
   va_list ap;
   int line = 1;
   char *t, fmt[EZXML_ERRL];

#ifndef SNPRINTF_UNAVAILABLE
   for (t = root->s; t < s; t++) if (*t == '\n') line++;
   snprintf(fmt, EZXML_ERRL, "[error near line %d]: %s", line, err);

   va_start(ap, err);
   vsnprintf(root->err, EZXML_ERRL, fmt, ap);
   va_end(ap);
#endif /* SNPRINTF_UNAVAILABLE */

#if 0
bstring fmt = NULL,
      err = NULL;

   fmt = bfromcstralloc( EZXML_ERRL, "" );
   err = bfromcstralloc( EZXML_ERRL, "" );
   lgc_null( fmt );

   for (t = root->s; t < s; t++) if (*t == '\n') line++;
   /* snprintf(fmt, EZXML_ERRL, "[error near line %d]: %s", line, err); */
   bformat( fmt, "[error near line %d]: %s", line, err );

   /* va_start(ap, err); */
   /* vsnprintf(root->err, EZXML_ERRL, fmt, ap); */
   bvformata( &bstr_ret, err, bdata( fmt ), err );
   root->err
   /* va_end(ap); */
#endif

   return &root->xml;
}

/* Recursively decodes entity and character references and normalizes new lines */
/* ent is a null terminated array of alternating entity names and values. set t */
/* to '&' for general entity decoding, '%' for parameter entity decoding, 'c' */
/* for cdata sections, ' ' for attribute normalization, or '*' for non-cdata */
/* attribute normalization. Returns s, or if the decoded string is longer than */
/* s, returns a malloced string that must be freed. */
static char *ezxml_decode(char *s, char **ent, char t) {
   char *e, *r = s, *m = s;
   long b, c, d, l;

   for (; *s; s++) { /* normalize line endings */
      while (*s == '\r') {
         *(s++) = '\n';
         if (*s == '\n') memmove(s, (s + 1), strlen(s));
      }
   }

   for (s = r; ; ) {
      while (*s && *s != '&' && (*s != '%' || t != '%') && !isspace(*s)) s++;

      if (! *s) break;
      else if (t != 'c' && ! strncmp(s, "&#", 2)) { /* character reference */
         if (s[2] == 'x') c = strtol(s + 3, &e, 16); /* base 16 */
         else c = strtol(s + 2, &e, 10); /* base 10 */
         if (! c || *e != ';') {
            s++;   /* not a character ref */
            continue;
         }

         if (c < 0x80) *(s++) = c; /* US-ASCII subset */
         else { /* multi-byte UTF-8 sequence */
            for (b = 0, d = c; d; d /= 2) b++; /* number of bits in c */
            b = (b - 2) / 5; /* number of bytes in payload */
            *(s++) = (0xFF << (7 - b)) | (c >> (6 * b)); /* head */
            while (b) *(s++) = 0x80 | ((c >> (6 * --b)) & 0x3F); /* payload */
         }

         memmove(s, strchr(s, ';') + 1, strlen(strchr(s, ';')));
      } else if ((*s == '&' && (t == '&' || t == ' ' || t == '*')) ||
               (*s == '%' && t == '%')) { /* entity reference */
         for (b = 0; ent[b] && strncmp(s + 1, ent[b], strlen(ent[b]));
               b += 2); /* find entity in entity list */

         if (ent[b++]) { /* found a match */
            if ((c = strlen(ent[b])) - 1 > (e = strchr(s, ';')) - s) {
               l = (d = (s - r)) + c + strlen(e); /* new length */
               /* FIXME: Soft realloc. */
               r = (r == m) ? strcpy(calloc(l, sizeof(char)), r) : mem_realloc(r, l, char);
               e = strchr((s = r + d), ';'); /* fix up pointers */
            }

            memmove(s + c, e + 1, strlen(e)); /* shift rest of string */
            strncpy(s, ent[b], c); /* copy in replacement text */
         } else s++; /* not a known entity */
      } else if ((t == ' ' || t == '*') && isspace(*s)) *(s++) = ' ';
      else s++; /* no decoding needed */
   }

   if (t == '*') { /* normalize spaces for non-cdata attributes */
      for (s = r; *s; s++) {
         if ((l = strspn(s, " "))) memmove(s, s + l, strlen(s + l) + 1);
         while (*s && *s != ' ') s++;
      }
      if (--s >= r && *s == ' ') *s = '\0'; /* trim any trailing space */
   }
   return r;
}

/* called when parser finds start of new tag */
static void ezxml_open_tag(ezxml_root_t root, char *name, char **attr) {
   ezxml_t xml = root->cur;

   if (xml->name) xml = ezxml_add_child(xml, name, strlen(xml->txt));
   else xml->name = name; /* first open tag */

   xml->attr = attr;
   root->cur = xml; /* update tag insertion point */
}

/* called when parser finds character content between open and closing tag */
static void ezxml_char_content(ezxml_root_t root, char *s, SCAFFOLD_SIZE len, char t) {
   ezxml_t xml = root->cur;
   char *m = s,
      * tmp = NULL;
   SCAFFOLD_SIZE l;

   /* sanity check */
   if( NULL == xml || NULL == xml->name || 0 == len ) {
      goto cleanup;
   }

   /* null terminate text (calling functions anticipate this) */
   s[len] = '\0';
   s = ezxml_decode( s, root->ent, t );
   len = strlen( s ) + 1;

   if( NULL == xml->txt ) {
      /* initial character content */
      xml->txt = s;
   } else {
      /* allocate our own memory and make a copy */
      if( 0 != (xml->flags & EZXML_TXTM) ) {
         /* allocate some space */
         l = strlen( xml->txt );
         tmp = mem_realloc( xml->txt, (l + len), char );
         lgc_null( tmp );
      } else {
         l = strlen( xml->txt );
         tmp = mem_alloc( (l + len), char );
         lgc_null( tmp );
         strcpy( tmp, xml->txt );
      }
      xml->txt = tmp;

      /* add new char content */
      strcpy( xml->txt + l, s );

      /* free s if it was malloced by ezxml_decode() */
      if( s != m ) {
         mem_free( s );
      }
   }

   if( xml->txt != m ) {
      ezxml_set_flag( xml, EZXML_TXTM );
   }

cleanup:
   return;
}

/* called when parser finds closing tag */
static ezxml_t ezxml_close_tag(ezxml_root_t root, char *name, char *s) {
   if (! root->cur || ! root->cur->name || strcmp(name, root->cur->name))
      return ezxml_err(root, s, "unexpected closing tag </%s>", name);

   root->cur = root->cur->parent;
   return NULL;
}

#ifdef EZXML_ENTOK
/* checks for circular entity references, returns non-zero if no circular */
/* references are found, zero otherwise */
static int ezxml_ent_ok(char *name, char *s, char **ent) {
   int i;

   for (; ; s++) {
      while (*s && *s != '&') s++; /* find next entity reference */
      if (! *s) return 1;
      if (! strncmp(s + 1, name, strlen(name))) return 0; /* circular ref. */
      for (i = 0; ent[i] && strncmp(ent[i], s + 1, strlen(ent[i])); i += 2);
      if (ent[i] && ! ezxml_ent_ok(name, ent[i + 1], ent)) return 0;
   }
}
#endif /* EZXML_ENTOK */

/* called when the parser finds a processing instruction */
static void ezxml_proc_inst(ezxml_root_t root, char *s, SCAFFOLD_SIZE len) {
   int i = 0, j = 1;
   char *target = s;

   s[len] = '\0'; /* null terminate instruction */
   if (*(s += strcspn(s, EZXML_WS))) {
      *s = '\0'; /* null terminate target */
      s += strspn(s + 1, EZXML_WS) + 1; /* skip whitespace after target */
   }

   if (! strcmp(target, "xml")) { /* <?xml ... ?> */
      if ((s = strstr(s, "standalone")) && ! strncmp(s + strspn(s + 10,
            EZXML_WS "='\"") + 10, "yes", 3)) root->standalone = 1;
      return;
   }

   if (! root->pi[0]) *(root->pi = calloc(1,sizeof(char **))) = NULL; /*first pi */

   while (root->pi[i] && strcmp(target, root->pi[i][0])) i++; /* find target */
   if (! root->pi[i]) { /* new target */
      /* FIXME: Soft realloc. */
      root->pi = mem_realloc(root->pi, (i + 2), char**);
      root->pi[i] = calloc(1,sizeof(char *) * 3);
      root->pi[i][0] = target;
      root->pi[i][1] = (char *)(root->pi[i + 1] = NULL); /* terminate pi list */
#ifdef _WIN32
      root->pi[i][2] = _strdup( "" ); /* empty document position list */
#else
      root->pi[i][2] = strdup(""); /* empty document position list */
#endif /* _WIN32 */
   }

   while (root->pi[i][j]) j++; /* find end of instruction list for this target */
   /* FIXME: Soft realloc. */
   root->pi[i] = mem_realloc(root->pi[i], (j + 3), char*);
   root->pi[i][j + 2] = mem_realloc(root->pi[i][j + 1], j + 1, char);
   strcpy(root->pi[i][j + 2] + j - 1, (root->xml.name) ? ">" : "<");
   root->pi[i][j + 1] = NULL; /* null terminate pi list for this target */
   root->pi[i][j] = s; /* set instruction */
}

#ifdef EZXML_RISKY_DTD
/* called when the parser finds an internal doctype subset */
/* This function creates warnings for undefined behavior. We don't need it so *
 * just disable it for now.                                                   */
short ezxml_internal_dtd(ezxml_root_t root, char *s, SCAFFOLD_SIZE len) {
   char q, *c, *t, *n = NULL, *v, **ent, **pe;
   int i, j;

   pe = memcpy(calloc(1,sizeof(EZXML_NIL)), EZXML_NIL, sizeof(EZXML_NIL));

   for (s[len] = '\0'; s; ) {
      while (*s && *s != '<' && *s != '%') s++; /* find next declaration */

      if (! *s) break;
      else if (! strncmp(s, "<!ENTITY", 8)) { /* parse entity definitions */
         c = s += strspn(s + 8, EZXML_WS) + 8; /* skip white space separator */
         n = s + strspn(s, EZXML_WS "%"); /* find name */
         *(s = n + strcspn(n, EZXML_WS)) = ';'; /* append ; to name */

         v = s + strspn(s + 1, EZXML_WS) + 1; /* find value */
         if ((q = *(v++)) != '"' && q != '\'') { /* skip externals */
            s = strchr(s, '>');
            continue;
         }

         for (i = 0, ent = (*c == '%') ? pe : root->ent; ent[i]; i++);
         /* FIXME: Soft realloc. */
         ent = mem_realloc(ent, (i + 3), char*); /* space for next ent */
         if (*c == '%') pe = ent;
         else root->ent = ent;

         *(++s) = '\0'; /* null terminate name */
         if ((s = strchr(v, q))) *(s++) = '\0'; /* null terminate value */
         ent[i + 1] = ezxml_decode(v, pe, '%'); /* set value */
         ent[i + 2] = NULL; /* null terminate entity list */
         if (! ezxml_ent_ok(n, ent[i + 1], ent)) { /* circular reference */
            if (ent[i + 1] != v) mem_free(ent[i + 1]);
            ezxml_err(root, v, "circular entity declaration &%s", n);
            break;
         } else ent[i] = n; /* set entity name */
      } else if (! strncmp(s, "<!ATTLIST", 9)) { /* parse default attributes */
         t = s + strspn(s + 9, EZXML_WS) + 9; /* skip whitespace separator */
         if (! *t) {
            ezxml_err(root, t, "unclosed <!ATTLIST");
            break;
         }
         if (*(s = t + strcspn(t, EZXML_WS ">")) == '>') continue;
         else *s = '\0'; /* null terminate tag name */
         for (i = 0; root->attr[i] && strcmp(n, root->attr[i][0]); i++);

         while (*(n = ++s + strspn(s, EZXML_WS)) && *n != '>') {
            if (*(s = n + strcspn(n, EZXML_WS))) *s = '\0'; /* attr name */
            else {
               ezxml_err(root, t, "malformed <!ATTLIST");
               break;
            }

            s += strspn(s + 1, EZXML_WS) + 1; /* find next token */
            c = (strncmp(s, "CDATA", 5)) ? "*" : " "; /* is it cdata? */
            if (! strncmp(s, "NOTATION", 8))
               s += strspn(s + 8, EZXML_WS) + 8;
            s = (*s == '(') ? strchr(s, ')') : s + strcspn(s, EZXML_WS);
            if (! s) {
               ezxml_err(root, t, "malformed <!ATTLIST");
               break;
            }

            s += strspn(s, EZXML_WS ")"); /* skip white space separator */
            if (! strncmp(s, "#FIXED", 6))
               s += strspn(s + 6, EZXML_WS) + 6;
            if (*s == '#') { /* no default value */
               s += strcspn(s, EZXML_WS ">") - 1;
               if (*c == ' ') continue; /* cdata is default, nothing to do */
               v = NULL;
            } else if ((*s == '"' || *s == '\'')  && /* default value */
                     (s = strchr(v = s + 1, *s))) *s = '\0';
            else {
               ezxml_err(root, t, "malformed <!ATTLIST");
               break;
            }

            if (! root->attr[i]) { /* new tag name */
               root->attr = (! i) ? calloc(2,sizeof(char **))
               /* FIXME: Soft realloc. */
                         : mem_realloc(root->attr, (i + 2), char**);
               root->attr[i] = calloc(2,sizeof(char *));
               root->attr[i][0] = t; /* set tag name */
               root->attr[i][1] = (char *)(root->attr[i + 1] = NULL);
            }

            for (j = 1; root->attr[i][j]; j += 3); /* find end of list */
            /* FIXME: Soft realloc. */
            root->attr[i] = mem_realloc(root->attr[i], (j + 4), char*);

            root->attr[i][j + 3] = NULL; /* null terminate list */
            root->attr[i][j + 2] = c; /* is it cdata? */
            root->attr[i][j + 1] = (v) ? ezxml_decode(v, root->ent, *c)
                              : NULL;
            root->attr[i][j] = n; /* attribute name */
         }
      } else if (! strncmp(s, "<!--", 4)) s = strstr(s + 4, "-->"); /* comments */
      else if (! strncmp(s, "<?", 2)) { /* processing instructions */
         if ((s = strstr(c = s + 2, "?>")))
            ezxml_proc_inst(root, c, s++ - c);
      } else if (*s == '<') s = strchr(s, '>'); /* skip other declarations */
      else if (*(s++) == '%' && ! root->standalone) break;
   }

   mem_free(pe);
   return ! *root->err;
}
#endif /* EZXML_RISKY_DTD */

/** \brief Converts a UTF-16 string to UTF-8. Returns a new string that must
 *         be freed or NULL if no conversion was needed.
 */
static char *ezxml_str2utf8( char **s, SCAFFOLD_SIZE *len ) {
   /* TODO: Make this a wrapper for one of the bstring functions. */
   char *u,
      * retval = NULL,
      * u_tmp;
   SCAFFOLD_SIZE l = 0,
      sl,
      max = *len;
   long c, d;
   int b,
      be = -1;

   if( '\xFE' == **s ) {
      be = 1;
   } else if( '\xFF' == **s ) {
      be = 0;
   } else {
      /* not UTF-16 */
      goto cleanup;
   }

   u = mem_alloc( max, char );
   lgc_null( u );

   for( sl = 2 ; sl < *len - 1 ; sl += 2 ) {
      if( 0 != be ) {
         /*UTF-16BE */
         c = (((*s)[sl] & 0xFF) << 8) | ((*s)[sl + 1] & 0xFF);
      } else {
         /*UTF-16LE */
         c = (((*s)[sl + 1] & 0xFF) << 8) | ((*s)[sl] & 0xFF);
      }

      if( 0xD800 <= c && 0xDFFF >= c && *len - 1 > (sl += 2) ) {
         /* high-half */
         if( 0 != be ) {
            d = (((*s)[sl] & 0xFF) << 8) | ((*s)[sl + 1] & 0xFF);
         } else {
            d = (((*s)[sl + 1] & 0xFF) << 8) | ((*s)[sl] & 0xFF);
         }
         c = (((c & 0x3FF) << 10) | (d & 0x3FF)) + 0x10000;
      }

      while( l + 6 > max ) {
         u_tmp = mem_realloc( u, (max += EZXML_BUFSIZE), char );
         lgc_null( u_tmp );
         u = u_tmp;
      }

      if( 0x80 > c ) {
         /* US-ASCII subset */
         u[l++] = c;
      } else {
         /* multi-byte UTF-8 sequence */
         for( b = 0, d = c ; d ; d /= 2 ) {
            /* bits in c */
            b++;
         }

         /* bytes in payload */
         b = (b - 2) / 5;

         /* head */
         u[l++] = (0xFF << (7 - b)) | (c >> (6 * b));

         while( b ) {
            /* payload */
            u[l++] = 0x80 | ((c >> (6 * --b)) & 0x3F);
         }
      }
   }

   u_tmp = mem_realloc( u, l, char );
   lgc_null( u_tmp );
   *len = l;
   *s = u_tmp;
   retval = u_tmp;

cleanup:
   return retval;
}

/* frees a tag attribute list */
static void ezxml_free_attr(char **attr) {
   int i = 0;
   char *m = NULL;

   if (! attr || attr == EZXML_NIL) return; /* nothing to free */
   while (attr[i]) i += 2; /* find end of attribute list */
   m = attr[i + 1]; /* list of which names and values are malloced */
   for (i = 0; m[i]; i++) {
      if (m[i] & EZXML_NAMEM) mem_free(attr[i * 2]);
      if (m[i] & EZXML_TXTM) mem_free(attr[(i * 2) + 1]);
   }
   mem_free(m);
   mem_free(attr);
}

/** \brief Parse the given xml string and return an ezxml structure.
 * \warning The given string *will* be modified by placing null terminators
 *          after tag names, among other things.
 * \param
 * \param
 * \return
 */
ezxml_t ezxml_parse_str(char *s, SCAFFOLD_SIZE len) {
   ezxml_root_t root = (ezxml_root_t)ezxml_new(NULL);
   char q, e, *d, **attr, **a = NULL; /* initialize a to avoid compile warning */
   int l, i, j;

   root->m = s;
   if (! len) return ezxml_err(root, NULL, "root tag missing");
   root->u = ezxml_str2utf8(&s, &len); /* convert utf-16 to utf-8 */
   root->e = (root->s = s) + len; /* record start and end of work area */

   e = s[len - 1]; /* save end char */
   s[len - 1] = '\0'; /* turn end char into null terminator */

   while (*s && *s != '<') s++; /* find first tag */
   if (! *s) return ezxml_err(root, s, "root tag missing");

   for (; ; ) {
      attr = (char **)EZXML_NIL;
      d = ++s;

      if (isalpha(*s) || *s == '_' || *s == ':' || *s < '\0') { /* new tag */
         if (! root->cur)
            return ezxml_err(root, d, "markup outside of root element");

         s += strcspn(s, EZXML_WS "/>");
         while (isspace(*s)) *(s++) = '\0'; /* null terminate tag name */

         if (*s && *s != '/' && *s != '>') /* find tag in default attr list */
            for (i = 0; (a = root->attr[i]) && strcmp(a[0], d); i++);

         for (l = 0; *s && *s != '/' && *s != '>'; l += 2) { /* new attrib */
            /* FIXME: Soft realloc. */
            attr = (l) ? mem_realloc(attr, (l + 4), char*)
                  : calloc(4,sizeof(char *)); /* allocate space */
            /* FIXME: Soft realloc. */
            attr[l + 3] = (l) ? mem_realloc(attr[l + 1], (l / 2) + 2, char)
                       : calloc(2,sizeof(char)); /* mem for list of maloced vals */
            strcpy(attr[l + 3] + (l / 2), " "); /* value is not malloced */
            attr[l + 2] = NULL; /* null terminate list */
            attr[l + 1] = ""; /* temporary attribute value */
            attr[l] = s; /* set attribute name */

            s += strcspn(s, EZXML_WS "=/>");
            if (*s == '=' || isspace(*s)) {
               *(s++) = '\0'; /* null terminate tag attribute name */
               q = *(s += strspn(s, EZXML_WS "="));
               if (q == '"' || q == '\'') { /* attribute value */
                  attr[l + 1] = ++s;
                  while (*s && *s != q) s++;
                  if (*s) *(s++) = '\0'; /* null terminate attribute val */
                  else {
                     ezxml_free_attr(attr);
                     return ezxml_err(root, d, "missing %c", q);
                  }

                  for (j = 1; a && a[j] && strcmp(a[j], attr[l]); j +=3);
                  attr[l + 1] = ezxml_decode(attr[l + 1], root->ent, (a
                                       && a[j]) ? *a[j + 2] : ' ');
                  if (attr[l + 1] < d || attr[l + 1] > s)
                     attr[l + 3][l / 2] = EZXML_TXTM; /* value malloced */
               }
            }
            while (isspace(*s)) s++;
         }

         if (*s == '/') { /* self closing tag */
            *(s++) = '\0';
            if ((*s && *s != '>') || (! *s && e != '>')) {
               if (l) ezxml_free_attr(attr);
               return ezxml_err(root, d, "missing >");
            }
            ezxml_open_tag(root, d, attr);
            ezxml_close_tag(root, d, s);
         } else if ((q = *s) == '>' || (! *s && e == '>')) { /* open tag */
            *s = '\0'; /* temporarily null terminate tag name */
            ezxml_open_tag(root, d, attr);
            *s = q;
         } else {
            if (l) ezxml_free_attr(attr);
            return ezxml_err(root, d, "missing >");
         }
      } else if (*s == '/') { /* close tag */
         s += strcspn(d = s + 1, EZXML_WS ">") + 1;
         if (! (q = *s) && e != '>') return ezxml_err(root, d, "missing >");
         *s = '\0'; /* temporarily null terminate tag name */
         if (ezxml_close_tag(root, d, s)) return &root->xml;
         if (isspace(*s = q)) s += strspn(s, EZXML_WS);
      } else if (! strncmp(s, "!--", 3)) { /* xml comment */
         if (! (s = strstr(s + 3, "--")) || (*(s += 2) != '>' && *s) ||
               (! *s && e != '>')) return ezxml_err(root, d, "unclosed <!--");
      } else if (! strncmp(s, "![CDATA[", 8)) { /* cdata */
         if ((s = strstr(s, "]]>")))
            ezxml_char_content(root, d + 8, (s += 2) - d - 10, 'c');
         else return ezxml_err(root, d, "unclosed <![CDATA[");
      } else if (! strncmp(s, "!DOCTYPE", 8)) { /* dtd */
         for (l = 0; *s && ((! l && *s != '>') || (l && (*s != ']' ||
                        *(s + strspn(s + 1, EZXML_WS) + 1) != '>')));
               l = (*s == '[') ? 1 : l) s += strcspn(s + 1, "[]>") + 1;
         if (! *s && e != '>')
            return ezxml_err(root, d, "unclosed <!DOCTYPE");
         d = (l) ? strchr(d, '[') + 1 : d;
#ifdef EZXML_RISKY_DTD
         if (l && ! ezxml_internal_dtd(root, d, s++ - d)) return &root->xml;
#endif /* EZXML_RISKY_DTD */
      } else if (*s == '?') { /* <?...?> processing instructions */
         do {
            s = strchr(s, '?');
         } while (s && *(++s) && *s != '>');
         if (! s || (! *s && e != '>'))
            return ezxml_err(root, d, "unclosed <?");
         else ezxml_proc_inst(root, d + 1, s - d - 2);
      } else return ezxml_err(root, d, "unexpected <");

      if (! s || ! *s) break;
      *s = '\0';
      d = ++s;
      if (*s && *s != '<') { /* tag character content */
         while (*s && *s != '<') s++;
         if (*s) ezxml_char_content(root, d, s - d, '&');
         else break;
      } else if (! *s) break;
   }

   if (! root->cur) return &root->xml;
   else if (! root->cur->name) return ezxml_err(root, d, "root tag missing");
   else return ezxml_err(root, d, "unclosed tag <%s>", root->cur->name);
}

#ifdef USE_FILE

/* Wrapper for ezxml_parse_str() that accepts a file stream. Reads the entire */
/* stream into memory and then parses it. For xml files, use ezxml_parse_file() */
/* or ezxml_parse_fd() */
ezxml_t ezxml_parse_fp(FILE *fp) {
   ezxml_root_t root;
   SCAFFOLD_SIZE l, len = 0;
   char *s;

   if (! (s = calloc(EZXML_BUFSIZE,sizeof(char)))) return NULL;
   do {
      len += (l = fread((s + len), 1, EZXML_BUFSIZE, fp));
      /* FIXME: Soft realloc. */
      if (l == EZXML_BUFSIZE) s = mem_realloc(s, len + EZXML_BUFSIZE, char);
   } while (s && l == EZXML_BUFSIZE);

   if (! s) return NULL;
   root = (ezxml_root_t)ezxml_parse_str(s, len);
   root->len = -1; /* so we know to free s in ezxml_free() */
   return &root->xml;
}

/* A wrapper for ezxml_parse_str() that accepts a file descriptor. First */
/* attempts to mem map the file. Failing that, reads the file into memory. */
/* Returns NULL on failure. */
ezxml_t ezxml_parse_fd(int fd) {
   ezxml_root_t root;
   struct stat st;
   SCAFFOLD_SIZE l;
   void *m;

   if (fd < 0) return NULL;
   fstat(fd, &st);

#ifndef EZXML_NOMMAP
   l = (st.st_size + sysconf(_SC_PAGESIZE) - 1) & ~(sysconf(_SC_PAGESIZE) -1);
   if ((m = mmap(NULL, l, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) !=
         MAP_FAILED) {
      madvise(m, l, MADV_SEQUENTIAL); /* optimize for sequential access */
      root = (ezxml_root_t)ezxml_parse_str(m, st.st_size);
      madvise(m, root->len = l, MADV_NORMAL); /* put it back to normal */
   } else { /* mmap failed, read file into memory */
#endif /* EZXML_NOMMAP */
      l = read(fd, m = calloc(st.st_size,sizeof(char)), st.st_size);
      root = (ezxml_root_t)ezxml_parse_str(m, l);
      root->len = -1; /* so we know to free s in ezxml_free() */
#ifndef EZXML_NOMMAP
   }
#endif /* EZXML_NOMMAP */
   return &root->xml;
}

/* a wrapper for ezxml_parse_fd that accepts a file name */
ezxml_t ezxml_parse_file(const char *file) {
   int fd = open(file, O_RDONLY, 0);
   ezxml_t xml = ezxml_parse_fd(fd);

   if (fd >= 0) close(fd);
   return xml;
}

#endif /* USE_FILE */

static void ezxml_ampencode_b(bstring txt_in, bstring txt_out, short a) {
   int bstr_ret;
   int i;

   for( i = 0 ; blength( txt_in ) > i ; i++ ) {

      switch (bchar(txt_in, i)) {
      case '\0':
         goto cleanup;
      case '&':
         bstr_ret = bcatcstr( txt_out, "&amp;" );
         lgc_nonzero( bstr_ret );
         break;
      case '<':
         bstr_ret = bcatcstr( txt_out, "&lt;" );
         lgc_nonzero( bstr_ret );
         break;
      case '>':
         bstr_ret = bcatcstr( txt_out, "&gt;" );
         lgc_nonzero( bstr_ret );
         break;
      case '"':
         if( a ) {
            bstr_ret = bcatcstr( txt_out, "&quot;" );
            lgc_nonzero( bstr_ret );
         } else {
            bstr_ret = bconchar( txt_out, '"' );
            lgc_nonzero( bstr_ret );
         }
         break;
      case '\n':
#ifdef EZXML_NEWLINES
         if( a ) {
            bstr_ret = bcatcstr( txt_out, "&#xA;" );
            lgc_nonzero( bstr_ret );
         } else {
            bstr_ret = bconchar( txt_out, '\n' );
            lgc_nonzero( bstr_ret );
         }
#endif /* EZXML_NEWLINES */
         break;
      case '\t':
#ifdef EZXML_NEWLINES
         if( a ) {
            bstr_ret = bcatcstr( txt_out, "&#x9;" );
            lgc_nonzero( bstr_ret );
         } else {
            bstr_ret = bconchar( txt_out, '\t' );
            lgc_nonzero( bstr_ret );
         }
#endif /* EZXML_NEWLINES */
         break;
      case '\r':
#ifdef EZXML_NEWLINES
         bstr_ret = bcatcstr( txt_out, "&#xD;" );
         lgc_nonzero( bstr_ret );
#endif /* EZXML_NEWLINES */
         break;
      default:
         bstr_ret = bconchar( txt_out, bchar( txt_in, i ) );
         lgc_nonzero( bstr_ret );
      }
   }
cleanup:
   return;
}

/* Recursively converts each tag to xml appending it to *s. Reallocates *s if */
/* its length excedes max. start is the location of the previous tag in the */
/* parent tag's character content. Returns *s. */
static void ezxml_toxml_r(ezxml_t xml, bstring xml_out, SCAFFOLD_SIZE start, char ***attr) {
   int i, j;
   SCAFFOLD_SIZE off = 0;
   int bstr_ret;
   bstring txt_b = NULL;

   if( NULL != xml->parent ) {
      txt_b = bfromcstr( xml->parent->txt );
   } else {
      txt_b = bfromcstr( "" );
   }

   /* parent character content up to this tag */
   ezxml_ampencode_b( txt_b, xml_out, 0 );

   bstr_ret = bformata( xml_out, "<%s", xml->name );
   lgc_nonzero( bstr_ret );
   for (i = 0; xml->attr[i]; i += 2) { /* tag attributes */
      if (ezxml_attr(xml, xml->attr[i]) != xml->attr[i + 1]) continue;
      bstr_ret = bformata( xml_out, " %s=\"", xml->attr[i] );
      lgc_nonzero( bstr_ret );
      bstr_ret = bassigncstr( txt_b, xml->attr[i + 1] );
      lgc_nonzero( bstr_ret );
      ezxml_ampencode_b( txt_b, xml_out, 1);
      bstr_ret = bconchar( xml_out, '"' );
      lgc_nonzero( bstr_ret );
   }

   for (i = 0; attr[i] && strcmp(attr[i][0], xml->name); i++);
   for (j = 1; attr[i] && attr[i][j]; j += 3) { /* default attributes */
      if (! attr[i][j + 1] || ezxml_attr(xml, attr[i][j]) != attr[i][j + 1])
         continue; /* skip duplicates and non-values */
      bstr_ret = bformata( xml_out, " %s=\"", attr[i][j] );
      lgc_nonzero( bstr_ret );
      bstr_ret = bassigncstr( txt_b, attr[i][j + 1] );
      lgc_nonzero( bstr_ret );
      ezxml_ampencode_b( txt_b, xml_out, 1 );
      bstr_ret = bconchar( xml_out, '"' );
      lgc_nonzero( bstr_ret );
   }
   bstr_ret = bconchar( xml_out, '>' );
   lgc_nonzero( bstr_ret );

   if( NULL != xml->child ) {
      ezxml_toxml_r( xml->child, xml_out, 0, attr );
   } else {
      bstr_ret = bassigncstr( txt_b, xml->txt );
      lgc_nonzero( bstr_ret );
      ezxml_ampencode_b( txt_b, xml_out, 0 );  /*data */
   }

   bstr_ret = bformata( xml_out, "</%s>", xml->name );
   lgc_nonzero( bstr_ret );

   while( NULL != xml->parent && xml->parent->txt[off] && off < xml->off ) {
      off++; /* make sure off is within bounds */
   }

   if( xml->ordered ) {
      ezxml_toxml_r( xml->ordered, xml_out, off, attr );
   } else if( NULL != xml->parent ) {
      bstr_ret = bassigncstr( txt_b, xml->parent->txt + off );
      lgc_nonzero( bstr_ret );
      ezxml_ampencode_b( txt_b, xml_out, 0);
   }

cleanup:
   bdestroy( txt_b );
   return;
}

/* Converts an ezxml structure back to xml. Returns a string of xml data that */
/* must be freed. */
bstring ezxml_toxml(ezxml_t xml) {
   /*ezxml_t p = (xml) ? xml->parent : NULL, o = (xml) ? xml->ordered : NULL; */
   ezxml_t parent = (xml) ? xml->parent : NULL,
      ordered = (xml) ? xml->ordered : NULL;
   ezxml_root_t root = (ezxml_root_t)xml;
   int bstr_ret;
   /*SCAFFOLD_SIZE len = 0, max = EZXML_BUFSIZE; */
   /*
   char *s = strcpy(calloc(max, sizeof(char)), ""),
      *t,
      *n;
   */
   bstring xml_out = NULL;
   char* tag_attribs = NULL,
      * tag = NULL;
   int i, j, k;

   xml_out = bfromcstralloc( EZXML_BUFSIZE, "" );
   lgc_null( xml_out );

   /*if (! xml || ! xml->name) return realloc(s, len + 1); */
   if (! xml || ! xml->name) goto cleanup;
   while (root->xml.parent) root = (ezxml_root_t)root->xml.parent; /* root tag */

   for( i = 0 ; !parent && root->pi[i]; i++ ) { /* pre-root processing instructions */
      for(k = 2; root->pi[i][k - 1]; k++);
      for( j = 1 ; (tag_attribs = root->pi[i][j]) ; j++ ) {
         if( root->pi[i][k][j - 1] == '>' ) {
            continue; /* not pre-root */
         }
         tag = root->pi[i][0];
         /*
         while( len + strlen(t = root->pi[i][0]) + strlen(n) + 7 > max ) {
            s = realloc(s, max += EZXML_BUFSIZE);
         }
         len += sprintf( s + len, "<?%s%s%s?>\n", t, *n ? " " : "", n );
         */
         bstr_ret = bformata(
            xml_out,
#ifdef EZXML_NEWLINES
            "<?%s%s%s?>\n",
#else
            "<?%s%s%s?>",
#endif /* EZXML_NEWLINES */
            tag, *tag_attribs ? " " : "", tag_attribs
         );
         lgc_nonzero( bstr_ret );
      }
   }

   xml->parent = xml->ordered = NULL;
   /*s = ezxml_toxml_r(xml, &s, &len, &max, 0, root->attr); */
   ezxml_toxml_r( xml, xml_out, 0, root->attr );
   xml->parent = parent;
   xml->ordered = ordered;

   for (i = 0; ! parent && root->pi[i]; i++) { /* post-root processing instructions */
      for (k = 2; root->pi[i][k - 1]; k++);
      for (j = 1; (tag_attribs = root->pi[i][j]); j++) {
         if (root->pi[i][k][j - 1] == '<') continue; /* not post-root */
         /*
         while (len + strlen(t = root->pi[i][0]) + strlen(n) + 7 > max) {
            s = realloc(s, max += EZXML_BUFSIZE);
         }
         len += sprintf(s + len, "\n<?%s%s%s?>", t, *n ? " " : "", n);
         */
         bstr_ret = bformata(
            xml_out,
#ifdef EZXML_NEWLINES
            "\n<?%s%s%s?>",
#else
            "<?%s%s%s?>",
#endif /* EZXML_NEWLINES */
            tag, *tag_attribs ? " " : "", tag_attribs
         );
         lgc_nonzero( bstr_ret );
      }
   }
   /*return realloc(s, len + 1); */
cleanup:
   return xml_out;
}

/** \brief Free the memory allocated for the ezxml structure. */
void ezxml_free(ezxml_t xml) {
   ezxml_root_t root = (ezxml_root_t)xml;
   int i, j;
   char **a, *s;

   if (! xml) return;
   ezxml_free(xml->child);
   ezxml_free(xml->ordered);

   if (! xml->parent) { /* free root tag allocations */
      for (i = 10; root->ent[i]; i += 2) /* 0 - 9 are default entites (<>&"') */
         if ((s = root->ent[i + 1]) < root->s || s > root->e) mem_free(s);
      mem_free(root->ent); /* free list of general entities */

      for (i = 0; (a = root->attr[i]); i++) {
         for (j = 1; a[j++]; j += 2) /* free malloced attribute values */
            if (a[j] && (a[j] < root->s || a[j] > root->e)) mem_free(a[j]);
         mem_free(a);
      }
      if (root->attr[0]) mem_free(root->attr); /* free default attribute list */

      for (i = 0; root->pi[i]; i++) {
         for (j = 1; root->pi[i][j]; j++);
         mem_free(root->pi[i][j + 1]);
         mem_free(root->pi[i]);
      }
      if (root->pi[0]) mem_free(root->pi); /* free processing instructions */

      if (root->len == -1) mem_free(root->m); /* malloced xml data */
#ifndef EZXML_NOMMAP
      else if (root->len) munmap(root->m, root->len); /* mem mapped xml data */
#endif /* EZXML_NOMMAP */
      if (root->u) mem_free(root->u); /* utf8 conversion */
   }

   ezxml_free_attr(xml->attr); /* tag attributes */
   if ((xml->flags & EZXML_TXTM)) mem_free(xml->txt); /* character content */
   if ((xml->flags & EZXML_NAMEM)) mem_free(xml->name); /* tag name */
   mem_free(xml);
}

/** \brief Return parser error message or empty string if none. */
const char *ezxml_error(ezxml_t xml) {
   while (xml && xml->parent) xml = xml->parent; /* find root tag */
   return (xml) ? ((ezxml_root_t)xml)->err : "";
}

/** \brief Returns a new empty ezxml structure with the given root tag name. */
ezxml_t ezxml_new(const char *name) {
   static char *ent[] = { "lt;", "&#60;", "gt;", "&#62;", "quot;", "&#34;",
                     "apos;", "&#39;", "amp;", "&#38;", NULL
                   };
   ezxml_root_t root = (ezxml_root_t)calloc(1, sizeof(struct ezxml_root));
   root->xml.name = (char *)name;
   root->cur = &root->xml;
   strcpy(root->err, root->xml.txt = "");
   root->ent = memcpy(calloc(1, sizeof(ent)), ent, sizeof(ent));
   root->attr = root->pi = (char ***)(root->xml.attr = EZXML_NIL);
   return &root->xml;
}

/** \brief Inserts an existing tag into an ezxml structure. */
ezxml_t ezxml_insert(ezxml_t xml, ezxml_t dest, SCAFFOLD_SIZE off) {
   ezxml_t cur, prev, head;

   xml->next = xml->sibling = xml->ordered = NULL;
   xml->off = off;
   xml->parent = dest;

   if ((head = dest->child)) { /* already have sub tags */
      if (head->off <= off) { /* not first subtag */
         for (cur = head; cur->ordered && cur->ordered->off <= off;
               cur = cur->ordered);
         xml->ordered = cur->ordered;
         cur->ordered = xml;
      } else { /* first subtag */
         xml->ordered = head;
         dest->child = xml;
      }

      for (cur = head, prev = NULL; cur && strcmp(cur->name, xml->name);
            prev = cur, cur = cur->sibling); /* find tag type */
      if (cur && cur->off <= off) { /* not first of type */
         while (cur->next && cur->next->off <= off) cur = cur->next;
         xml->next = cur->next;
         cur->next = xml;
      } else { /* first tag of this type */
         if (prev && cur) prev->sibling = cur->sibling; /* remove old first */
         xml->next = cur; /* old first tag is now next */
         for (cur = head, prev = NULL; cur && cur->off <= off;
               prev = cur, cur = cur->sibling); /* new sibling insert point */
         xml->sibling = cur;
         if (prev) prev->sibling = xml;
      }
   } else dest->child = xml; /* only sub tag */

   return xml;
}

/* Adds a child tag. off is the offset of the child tag relative to the start */
/* of the parent tag's character content. Returns the child tag. */
ezxml_t ezxml_add_child(ezxml_t xml, const char *name, SCAFFOLD_SIZE off) {
   ezxml_t child;

   if (! xml) return NULL;
   child = (ezxml_t)calloc(1,sizeof(struct ezxml));
   child->name = (char *)name;
   child->attr = EZXML_NIL;
   child->txt = "";

   return ezxml_insert(child, xml, off);
}

/* sets the character content for the given tag and returns the tag */
ezxml_t ezxml_set_txt(ezxml_t xml, const char *txt) {
   if (! xml) return NULL;
   if (xml->flags & EZXML_TXTM) mem_free(xml->txt); /* existing txt was malloced */
   xml->flags &= ~EZXML_TXTM;
   xml->txt = (char *)txt;
   return xml;
}

void ezxml_set_txt_b(ezxml_t xml, bstring txt) {
   if (! xml) {
      goto cleanup;
   }
   if (xml->flags & EZXML_TXTM) {
      mem_free(xml->txt); /* existing txt was malloced */
   }
   xml->flags &= ~EZXML_TXTM;
   xml->txt = bstr2cstr( txt, '\n' );
cleanup:
   return;
}

#ifdef EZXML_SET_ATTR

/* Sets the given tag attribute or adds a new attribute if not found. A value */
/* of NULL will remove the specified attribute. Returns the tag given. */
ezxml_t ezxml_set_attr(ezxml_t xml, const char *name, const char *value) {
   int l = 0, c;

   if (! xml) return NULL;
   while (xml->attr[l] && strcmp(xml->attr[l], name)) l += 2;
   if (! xml->attr[l]) { /* not found, add as new attribute */
      if (! value) return xml; /* nothing to do */
      if (xml->attr == EZXML_NIL) { /* first attribute */
         xml->attr = calloc(4, sizeof(char *));
#ifdef _WIN32
         xml->attr[1] = _strdup(""); /* empty list of malloced names/vals */
#else
         xml->attr[1] = strdup( "" ); /* empty list of malloced names/vals */
#endif /* _WIN32 */
      /* FIXME: Soft realloc. */
      } else xml->attr = mem_realloc(xml->attr, (l + 4), char*);

      xml->attr[l] = (char *)name; /* set attribute name */
      xml->attr[l + 2] = NULL; /* null terminate attribute list */
      /* FIXME: Soft realloc. */
      xml->attr[l + 3] = mem_realloc(
            xml->attr[l + 1], (c = strlen(xml->attr[l + 1])) + 2, char);
      strcpy(xml->attr[l + 3] + c, " "); /* set name/value as not malloced */
      if( xml->flags & EZXML_DUP ) {
         xml->attr[l + 3][c] = EZXML_NAMEM;
      }
   } else if (xml->flags & EZXML_DUP) free((char *)name); /* name was strduped */

   for (c = l; xml->attr[c]; c += 2); /* find end of attribute list */
   if (xml->attr[c + 1][l / 2] & EZXML_TXTM) mem_free(xml->attr[l + 1]); /*old val */
   if (xml->flags & EZXML_DUP) xml->attr[c + 1][l / 2] |= EZXML_TXTM;
   else xml->attr[c + 1][l / 2] &= ~EZXML_TXTM;

   if (value) xml->attr[l + 1] = (char *)value; /* set attribute value */
   else { /* remove attribute */
      if (xml->attr[c + 1][l / 2] & EZXML_NAMEM) mem_free(xml->attr[l]);
      memmove(xml->attr + l, xml->attr + l + 2, (c - l + 2) * sizeof(char*));
      /* FIXME: Soft realloc. */
      xml->attr = mem_realloc(xml->attr, (c + 2), char*);
      memmove(xml->attr[c + 1] + (l / 2), xml->attr[c + 1] + (l / 2) + 1,
            (c / 2) - (l / 2)); /* fix list of which name/vals are malloced */
   }
   xml->flags &= ~EZXML_DUP; /* clear strdup() flag */
   return xml;
}

#endif /* EZXML_SET_ATTR */

/* sets a flag for the given tag and returns the tag */
ezxml_t ezxml_set_flag(ezxml_t xml, short flag) {
   if (xml) xml->flags |= flag;
   return xml;
}

/* removes a tag along with its subtags without freeing its memory */
ezxml_t ezxml_cut(ezxml_t xml) {
   ezxml_t cur;

   if (! xml) return NULL; /* nothing to do */
   if (xml->next) xml->next->sibling = xml->sibling; /* patch sibling list */

   if (xml->parent) { /* not root tag */
      cur = xml->parent->child; /* find head of subtag list */
      if (cur == xml) xml->parent->child = xml->ordered; /* first subtag */
      else { /* not first subtag */
         while (cur->ordered != xml) cur = cur->ordered;
         cur->ordered = cur->ordered->ordered; /* patch ordered list */

         cur = xml->parent->child; /* go back to head of subtag list */
         if (strcmp(cur->name, xml->name)) { /* not in first sibling list */
            while (strcmp(cur->sibling->name, xml->name))
               cur = cur->sibling;
            if (cur->sibling == xml) { /* first of a sibling list */
               cur->sibling = (xml->next) ? xml->next
                           : cur->sibling->sibling;
            } else cur = cur->sibling; /* not first of a sibling list */
         }

         while (cur->next && cur->next != xml) cur = cur->next;
         if (cur->next) cur->next = cur->next->next; /* patch next list */
      }
   }
   xml->ordered = xml->sibling = xml->next = NULL;
   return xml;
}

#ifdef EZXML_TEST /* test harness */
int main(int argc, char **argv) {
   ezxml_t xml;
   char *s;
   int i;

   if (argc != 2) return fprintf(stderr, "usage: %s xmlfile\n", argv[0]);

   xml = ezxml_parse_file(argv[1]);
   printf("%s\n", (s = ezxml_toxml(xml)));
   mem_free(s);
   i = fprintf(stderr, "%s", ezxml_error(xml));
   ezxml_free(xml);
   return (i) ? 1 : 0;
}
#endif /* EZXML_TEST */
