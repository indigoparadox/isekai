
#include <stdlib.h>
#include <string.h>

#define HEATSHRINK_DECODER_C
#include "hsdecode.h"

#if ! HEATSHRINK_DYNAMIC_ALLOC
#warning HEATSHRINK_DYNAMIC_ALLOC disabled. Chunker may not work.
#endif /* HEATSHRINK_DYNAMIC_ALLOC */

/* States for the polling state machine. */
typedef enum {
   HSDS_TAG_BIT,               /* tag bit */
   HSDS_YIELD_LITERAL,         /* ready to yield literal byte */
   HSDS_BACKREF_INDEX_MSB,     /* most significant byte of index */
   HSDS_BACKREF_INDEX_LSB,     /* least significant byte of index */
   HSDS_BACKREF_COUNT_MSB,     /* most significant byte of count */
   HSDS_BACKREF_COUNT_LSB,     /* least significant byte of count */
   HSDS_YIELD_BACKREF         /* ready to yield back-reference */
} HSD_state;

#ifdef DEBUG
static const char* state_names[] = {
   "tag_bit",
   "yield_literal",
   "backref_index_msb",
   "backref_index_lsb",
   "backref_count_msb",
   "backref_count_lsb",
   "yield_backref",
};
#endif

typedef struct {
   uint8_t* buf;               /* output buffer */
   SCAFFOLD_SIZE buf_size;            /* buffer size */
   SCAFFOLD_SIZE* output_size;        /* bytes pushed to buffer, so far */
} output_info;

#define NO_BITS ((uint16_t)-1)

/* Forward references. */
static uint16_t get_bits(heatshrink_decoder* hsd, uint8_t count);
static void push_byte(heatshrink_decoder* hsd, output_info* oi, uint8_t byte);

#if HEATSHRINK_DYNAMIC_ALLOC
heatshrink_decoder* heatshrink_decoder_alloc(
   uint16_t input_buffer_size, uint8_t window_sz2, uint8_t lookahead_sz2
) {
   SCAFFOLD_SIZE buffers_sz,
      sz;
   heatshrink_decoder* hsd = NULL;

   if ((window_sz2 < HEATSHRINK_MIN_WINDOW_BITS) ||
         (window_sz2 > HEATSHRINK_MAX_WINDOW_BITS) ||
         (input_buffer_size == 0) ||
         (lookahead_sz2 < HEATSHRINK_MIN_LOOKAHEAD_BITS) ||
         (lookahead_sz2 >= window_sz2)) {
      return NULL;
   }
   /* TODO: Fix this defeat of malloc safety. */
   buffers_sz = (1 << window_sz2) + input_buffer_size;
   sz = sizeof(heatshrink_decoder) + buffers_sz;
   hsd = (heatshrink_decoder*)mem_alloc(sz, BYTE);
   if (hsd == NULL) {
      return NULL;
   }
   hsd->input_buffer_size = input_buffer_size;
   hsd->window_sz2 = window_sz2;
   hsd->lookahead_sz2 = lookahead_sz2;
   heatshrink_decoder_reset(hsd);
#ifdef DEBUG_HEATSHRINK
   lg_debug( __FILE__, "-- allocated decoder with buffer size of %zu (%zu + %u + %u)\n",
       sz, sizeof(heatshrink_decoder), (1 << window_sz2), input_buffer_size);
#endif /* DEBUG_HEATSHRINK */
   return hsd;
}

void heatshrink_decoder_free(heatshrink_decoder* hsd) {
   size_t buffers_sz = (1 << hsd->window_sz2) + hsd->input_buffer_size;
   size_t sz = sizeof(heatshrink_decoder) + buffers_sz;
   mem_free(hsd); /* TODO: Adopt bookkeeping for mem_free(). */
   (void)sz;   /* may not be used by free */
}
#endif

void heatshrink_decoder_reset(heatshrink_decoder* hsd) {
   size_t buf_sz = 1 << HEATSHRINK_DECODER_WINDOW_BITS(hsd);
   size_t input_sz = HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(hsd);
   memset(hsd->buffers, 0, buf_sz + input_sz);
   hsd->state = HSDS_TAG_BIT;
   hsd->input_size = 0;
   hsd->input_index = 0;
   hsd->bit_index = 0x00;
   hsd->current_byte = 0x00;
   hsd->output_count = 0;
   hsd->output_index = 0;
   hsd->head_index = 0;
}

/* Copy SIZE bytes into the decoder's input buffer, if it will fit. */
HSD_sink_res heatshrink_decoder_sink(
   heatshrink_decoder* hsd, BYTE* in_buf, SCAFFOLD_SIZE size, SCAFFOLD_SIZE* input_size
) {
   SCAFFOLD_SIZE rem;

   if ((hsd == NULL) || (in_buf == NULL) || (input_size == NULL)) {
      return HSDR_SINK_ERROR_NULL;
   }

   rem = HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(hsd) - hsd->input_size;
   if (rem == 0) {
      *input_size = 0;
      return HSDR_SINK_FULL;
   }

   size = rem < size ? rem : size;
#ifdef DEBUG_HEATSHRINK
   lg_debug( __FILE__, "-- sinking %zd bytes\n", size);
#endif /* DEBUG_HEATSHRINK */
   /* copy into input buffer (at head of buffers) */
   memcpy(&hsd->buffers[hsd->input_size], in_buf, size);
   hsd->input_size += size;
   *input_size = size;
   return HSDR_SINK_OK;
}


/*****************
 * Decompression *
 *****************/

#define BACKREF_COUNT_BITS(HSD) (HEATSHRINK_DECODER_LOOKAHEAD_BITS(HSD))
#define BACKREF_INDEX_BITS(HSD) (HEATSHRINK_DECODER_WINDOW_BITS(HSD))

/* States */
static HSD_state st_tag_bit(heatshrink_decoder* hsd);
static HSD_state st_yield_literal(heatshrink_decoder* hsd,
                                  output_info* oi);
static HSD_state st_backref_index_msb(heatshrink_decoder* hsd);
static HSD_state st_backref_index_lsb(heatshrink_decoder* hsd);
static HSD_state st_backref_count_msb(heatshrink_decoder* hsd);
static HSD_state st_backref_count_lsb(heatshrink_decoder* hsd);
static HSD_state st_yield_backref(heatshrink_decoder* hsd,
                                  output_info* oi);

HSD_poll_res heatshrink_decoder_poll(
   heatshrink_decoder* hsd, BYTE* out_buf, SCAFFOLD_SIZE out_buf_size,
   SCAFFOLD_SIZE* output_size
) {
   output_info oi;
   uint8_t in_state;

   if ((hsd == NULL) || (out_buf == NULL) || (output_size == NULL)) {
      return HSDR_POLL_ERROR_NULL;
   }
   *output_size = 0;

   oi.buf = out_buf;
   oi.buf_size = out_buf_size;
   oi.output_size = output_size;

   while (1) {
#ifdef DEBUG_HEATSHRINK
      lg_debug( __FILE__, "-- poll, state is %d (%s), input_size %d\n",
          hsd->state, state_names[hsd->state], hsd->input_size);
#endif /* DEBUG_HEATSHRINK */
      in_state = hsd->state;
      switch (in_state) {
      case HSDS_TAG_BIT:
         hsd->state = st_tag_bit(hsd);
         break;
      case HSDS_YIELD_LITERAL:
         hsd->state = st_yield_literal(hsd, &oi);
         break;
      case HSDS_BACKREF_INDEX_MSB:
         hsd->state = st_backref_index_msb(hsd);
         break;
      case HSDS_BACKREF_INDEX_LSB:
         hsd->state = st_backref_index_lsb(hsd);
         break;
      case HSDS_BACKREF_COUNT_MSB:
         hsd->state = st_backref_count_msb(hsd);
         break;
      case HSDS_BACKREF_COUNT_LSB:
         hsd->state = st_backref_count_lsb(hsd);
         break;
      case HSDS_YIELD_BACKREF:
         hsd->state = st_yield_backref(hsd, &oi);
         break;
      default:
         return HSDR_POLL_ERROR_UNKNOWN;
      }

      /* If the current state cannot advance, check if input or output
       * buffer are exhausted. */
      if (hsd->state == in_state) {
         if (*output_size == out_buf_size) {
            return HSDR_POLL_MORE;
         }
         return HSDR_POLL_EMPTY;
      }
   }
}

static HSD_state st_tag_bit(heatshrink_decoder* hsd) {
   uint32_t bits = get_bits(hsd, 1);  /* get tag bit */
   if (bits == NO_BITS) {
      return HSDS_TAG_BIT;
   } else if (bits) {
      return HSDS_YIELD_LITERAL;
   } else if (HEATSHRINK_DECODER_WINDOW_BITS(hsd) > 8) {
      return HSDS_BACKREF_INDEX_MSB;
   } else {
      hsd->output_index = 0;
      return HSDS_BACKREF_INDEX_LSB;
   }
}

static HSD_state st_yield_literal( heatshrink_decoder* hsd, output_info* oi ) {
   uint16_t byte;
   uint8_t* buf = NULL;
   uint16_t mask;
   uint8_t c;

   /* Emit a repeated section from the window buffer, and add it (again)
    * to the window buffer. (Note that the repetition can include
    * itself.)*/
   if (*oi->output_size < oi->buf_size) {
      byte = get_bits(hsd, 8);
      if (byte == NO_BITS) {
         return HSDS_YIELD_LITERAL;   /* out of input */
      }
      buf = &hsd->buffers[HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(hsd)];
      mask = (1 << HEATSHRINK_DECODER_WINDOW_BITS(hsd))  - 1;
      c = byte & 0xFF;
#ifdef DEBUG_HEATSHRINK
      lg_debug( __FILE__, "-- emitting literal byte 0x%02x ('%c')\n", c, isprint(c) ? c : '.');
#endif /* DEBUG_HEATSHRINK */
      buf[hsd->head_index++ & mask] = c;
      push_byte(hsd, oi, c);
      return HSDS_TAG_BIT;
   } else {
      return HSDS_YIELD_LITERAL;
   }
}

static HSD_state st_backref_index_msb(heatshrink_decoder* hsd) {
   uint8_t bit_ct = BACKREF_INDEX_BITS(hsd);
   uint16_t bits;

   assert(bit_ct > 8);
   bits = get_bits(hsd, bit_ct - 8);
#ifdef DEBUG_HEATSHRINK
   lg_debug( __FILE__, "-- backref index (msb), got 0x%04x (+1)\n", bits);
#endif /* DEBUG_HEATSHRINK */
   if (bits == NO_BITS) {
      return HSDS_BACKREF_INDEX_MSB;
   }
   hsd->output_index = bits << 8;
   return HSDS_BACKREF_INDEX_LSB;
}

static HSD_state st_backref_index_lsb(heatshrink_decoder* hsd) {
   uint8_t bit_ct = BACKREF_INDEX_BITS(hsd);
   uint16_t bits = get_bits(hsd, bit_ct < 8 ? bit_ct : 8);
   uint8_t br_bit_ct;

#ifdef DEBUG_HEATSHRINK
   lg_debug( __FILE__, "-- backref index (lsb), got 0x%04x (+1)\n", bits);
#endif /* DEBUG_HEATSHRINK */
   if (bits == NO_BITS) {
      return HSDS_BACKREF_INDEX_LSB;
   }
   hsd->output_index |= bits;
   hsd->output_index++;
   br_bit_ct = BACKREF_COUNT_BITS(hsd);
   hsd->output_count = 0;
   return (br_bit_ct > 8) ? HSDS_BACKREF_COUNT_MSB : HSDS_BACKREF_COUNT_LSB;
}

static HSD_state st_backref_count_msb(heatshrink_decoder* hsd) {
   uint8_t br_bit_ct = BACKREF_COUNT_BITS(hsd);
   uint16_t bits;

   assert(br_bit_ct > 8);
   bits = get_bits(hsd, br_bit_ct - 8);
#ifdef DEBUG_HEATSHRINK
   lg_debug( __FILE__, "-- backref count (msb), got 0x%04x (+1)\n", bits);
#endif /* DEBUG_HEATSHRINK */
   if (bits == NO_BITS) {
      return HSDS_BACKREF_COUNT_MSB;
   }
   hsd->output_count = bits << 8;
   return HSDS_BACKREF_COUNT_LSB;
}

static HSD_state st_backref_count_lsb(heatshrink_decoder* hsd) {
   uint8_t br_bit_ct = BACKREF_COUNT_BITS(hsd);
   uint16_t bits = get_bits(hsd, br_bit_ct < 8 ? br_bit_ct : 8);
#ifdef DEBUG_HEATSHRINK
   lg_debug( __FILE__, "-- backref count (lsb), got 0x%04x (+1)\n", bits);
#endif /* DEBUG_HEATSHRINK */
   if (bits == NO_BITS) {
      return HSDS_BACKREF_COUNT_LSB;
   }
   hsd->output_count |= bits;
   hsd->output_count++;
   return HSDS_YIELD_BACKREF;
}

static HSD_state st_yield_backref( heatshrink_decoder* hsd, output_info* oi) {
   uint8_t* buf;
   uint16_t mask;
   uint16_t neg_offset;
   uint8_t c;

   SCAFFOLD_SIZE count = oi->buf_size - *oi->output_size;
   if (count > 0) {
      SCAFFOLD_SIZE i = 0;
      if (hsd->output_count < count) count = hsd->output_count;
      buf = &hsd->buffers[HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(hsd)];
      mask = (1 << HEATSHRINK_DECODER_WINDOW_BITS(hsd)) - 1;
      neg_offset = hsd->output_index;
#ifdef DEBUG_HEATSHRINK
      lg_debug( __FILE__, "-- emitting %zu bytes from -%u bytes back\n", count, neg_offset);
#endif /* DEBUG_HEATSHRINK */
      assert(neg_offset <= mask + 1);
      assert(count <= (SCAFFOLD_SIZE)(1 << BACKREF_COUNT_BITS(hsd)));

      for (i=0; i<count; i++) {
         c = buf[(hsd->head_index - neg_offset) & mask];
         push_byte(hsd, oi, c);
         buf[hsd->head_index & mask] = c;
         hsd->head_index++;
#ifdef DEBUG_HEATSHRINK
         lg_debug( __FILE__, "  -- ++ 0x%02x\n", c);
#endif /* DEBUG_HEATSHRINK */
      }
      hsd->output_count -= count;
      if (hsd->output_count == 0) {
         return HSDS_TAG_BIT;
      }
   }
   return HSDS_YIELD_BACKREF;
}

/* Get the next COUNT bits from the input buffer, saving incremental progress.
 * Returns NO_BITS on end of input, or if more than 15 bits are requested. */
static uint16_t get_bits(heatshrink_decoder* hsd, uint8_t count) {
   uint16_t accumulator = 0;
   int i = 0;
   if (count > 15) {
      return NO_BITS;
   }
#ifdef DEBUG_HEATSHRINK
   lg_debug( __FILE__, "-- popping %u bit(s)\n", count);
#endif /* DEBUG_HEATSHRINK */

   /* If we aren't able to get COUNT bits, suspend immediately, because we
    * don't track how many bits of COUNT we've accumulated before suspend. */
   if (hsd->input_size == 0) {
      if (hsd->bit_index < (1 << (count - 1))) {
         return NO_BITS;
      }
   }

   for (i = 0; i < count; i++) {
      if (hsd->bit_index == 0x00) {
         if (hsd->input_size == 0) {
#ifdef DEBUG_HEATSHRINK
            lg_debug( __FILE__, "  -- out of bits, suspending w/ accumulator of %u (0x%02x)\n",
                accumulator, accumulator);
#endif /* DEBUG_HEATSHRINK */
            return NO_BITS;
         }
         hsd->current_byte = hsd->buffers[hsd->input_index++];
#ifdef DEBUG_HEATSHRINK
         lg_debug( __FILE__, "  -- pulled byte 0x%02x\n", hsd->current_byte);
#endif /* DEBUG_HEATSHRINK */
         if (hsd->input_index == hsd->input_size) {
            hsd->input_index = 0; /* input is exhausted */
            hsd->input_size = 0;
         }
         hsd->bit_index = 0x80;
      }
      accumulator <<= 1;
      if (hsd->current_byte & hsd->bit_index) {
         accumulator |= 0x01;
         if (0) {
#ifdef DEBUG_HEATSHRINK
            lg_debug( __FILE__, "  -- got 1, accumulator 0x%04x, bit_index 0x%02x\n",
                accumulator, hsd->bit_index);
#endif /* DEBUG_HEATSHRINK */
         }
      } else {
         if (0) {
#ifdef DEBUG_HEATSHRINK
            lg_debug( __FILE__, "  -- got 0, accumulator 0x%04x, bit_index 0x%02x\n",
                accumulator, hsd->bit_index);
#endif /* DEBUG_HEATSHRINK */
         }
      }
      hsd->bit_index >>= 1;
   }

   if (count > 1) {
#ifdef DEBUG_HEATSHRINK
      lg_debug( __FILE__, "  -- accumulated %08x\n", accumulator);
#endif /* DEBUG_HEATSHRINK */
   }
   return accumulator;
}

HSD_finish_res heatshrink_decoder_finish(heatshrink_decoder* hsd) {
   if (hsd == NULL) {
      return HSDR_FINISH_ERROR_NULL;
   }
   switch (hsd->state) {
   case HSDS_TAG_BIT:
      return hsd->input_size == 0 ? HSDR_FINISH_DONE : HSDR_FINISH_MORE;

   /* If we want to finish with no input, but are in these states, it's
    * because the 0-bit padding to the last byte looks like a backref
    * marker bit followed by all 0s for index and count bits. */
   case HSDS_BACKREF_INDEX_LSB:
   case HSDS_BACKREF_INDEX_MSB:
   case HSDS_BACKREF_COUNT_LSB:
   case HSDS_BACKREF_COUNT_MSB:
      return hsd->input_size == 0 ? HSDR_FINISH_DONE : HSDR_FINISH_MORE;

   /* If the output stream is padded with 0xFFs (possibly due to being in
    * flash memory), also explicitly check the input size rather than
    * uselessly returning MORE but yielding 0 bytes when polling. */
   case HSDS_YIELD_LITERAL:
      return hsd->input_size == 0 ? HSDR_FINISH_DONE : HSDR_FINISH_MORE;

   default:
      return HSDR_FINISH_MORE;
   }
}

static void push_byte(heatshrink_decoder* hsd, output_info* oi, uint8_t byte) {
#ifdef DEBUG_HEATSHRINK
   lg_debug( __FILE__, " -- pushing byte: 0x%02x ('%c')\n", byte, isprint(byte) ? byte : '.');
#endif /* DEBUG_HEATSHRINK */
   oi->buf[(*oi->output_size)++] = byte;
   (void)hsd;
}
