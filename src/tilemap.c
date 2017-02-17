
#include "tilemap.h"

#include <memory.h>
#include <string.h>

void tilemap_init( TILEMAP* t ) {
    memset( t, '\0', sizeof( TILEMAP ) );
}

typedef enum {
    TILEMAP_TAG_UNKNOWN,
    TILEMAP_TAG_NONE,
    TILEMAP_TAG_LAYERS,
} TILEMAP_TAG;

typedef enum {
    TILEMAP_P_FLAG_NONE = 0x0,
    TILEMAP_P_FLAG_STRING = 0x1,
    TILEMAP_P_FLAG_LIST = 0x2,
    TILEMAP_P_FLAG_EXECUTE = 0x4,
    TILEMAP_P_FLAG_ATTRIBUTE = 0x8,
} TILEMAP_P_FLAG;

#define TILEMAP_PARSER_BUFFER_ALLOC 30
#define TILEMAP_PARSER_MAX_DEPTH_ALLOC 30

typedef struct {
    struct tagbstring tag;
    TILEMAP_TAG value;
} TILEMAP_P_ENTRY;

const TILEMAP_P_ENTRY tilemap_tag_lookup[] = {
    //{ bsStatic( "" ), TILEMAP_TAG_ROOT },
    { bsStatic( "layers" ), TILEMAP_TAG_LAYERS },
    { bsStatic( "data" ), TILEMAP_TAG_LAYERS },
    { { 0 }, TILEMAP_TAG_UNKNOWN },
};

static TILEMAP_TAG tilemap_parse_tag( TILEMAP* t, const bstring tag ) {
    TILEMAP_TAG tag_out = TILEMAP_TAG_UNKNOWN;
    TILEMAP_P_ENTRY* p_entry = (TILEMAP_P_ENTRY*)tilemap_tag_lookup;

    while( TILEMAP_TAG_UNKNOWN != p_entry->value ) {
        if( 0 == bstrncmp( &(p_entry->tag), tag, blength( &(p_entry->tag) ) ) ) {
            tag_out = p_entry->value;
            goto cleanup;
        }
        p_entry++;
    }

cleanup:
    return tag_out;
}

void tilemap_parse( TILEMAP* t, const uint8_t* data ) {
    uint8_t* i = (uint8_t*)data;
    int j;
    TILEMAP_TAG breadcrumbs[TILEMAP_PARSER_MAX_DEPTH_ALLOC] = { 0 };
    uint16_t parser_flags = 0;
    bstring string_buffer = NULL;
    int8_t tag_depth = 0;
    bstring attribute = NULL;
    struct bstrList* list_buffer = NULL;
    int bstr_result = 0;

    string_buffer = bfromcstralloc( TILEMAP_PARSER_BUFFER_ALLOC, "" );
    scaffold_check_null( string_buffer );
    attribute = bfromcstralloc( TILEMAP_PARSER_BUFFER_ALLOC, "" );
    scaffold_check_null( attribute );

    while( '\0' != *i ) {

        switch( *i ) {
            case '{':
                parser_flags &= ~TILEMAP_P_FLAG_ATTRIBUTE;
                if( 0 == blength( string_buffer ) ) {
                    breadcrumbs[tag_depth] = TILEMAP_TAG_NONE;
                } else {
                    breadcrumbs[tag_depth] = tilemap_parse_tag( t, string_buffer );
                    if( TILEMAP_TAG_UNKNOWN != breadcrumbs[tag_depth] ) {
                        scaffold_print_debug(
                            "Good tag: %s, Depth: %d\n",
                            bdata( string_buffer ), tag_depth
                        );
                    } else {
                        scaffold_print_debug(
                            "Unknown tag: %s, Depth: %d\n",
                            bdata( string_buffer ), tag_depth
                        );
                    }
                    btrunc( string_buffer, 0 );
                }
                /* Descend into the last read tag. */
                tag_depth++;
                break;

            case '}':
                /* Go back up one level, but not above the root! */
                tag_depth--;
                scaffold_check_negative( tag_depth );
                break;

            case '"':
                if( TILEMAP_P_FLAG_STRING & parser_flags ) {
                    /* Closing a string. */
                    parser_flags &= ~TILEMAP_P_FLAG_STRING;
                } else {
                    /* Starting a string. */
                    parser_flags |= TILEMAP_P_FLAG_STRING;
                    btrunc( string_buffer, 0 );
                }
                break;

            case '[':
                /* This is not a single attribute, but a list! */
                parser_flags &= ~TILEMAP_P_FLAG_ATTRIBUTE;
                parser_flags |= TILEMAP_P_FLAG_LIST;
                /* TODO: Implement lists. */
                break;

            case ']':
                parser_flags |= TILEMAP_P_FLAG_EXECUTE;
                break;

            case ',':
                if( !(parser_flags & TILEMAP_P_FLAG_LIST) ) {
                    /* Done reading a normal attribute. */
                    parser_flags |= TILEMAP_P_FLAG_EXECUTE;
                } else {
                    /* In the middle of a list. */
                    bconchar( string_buffer, *i );
                }
                break;

            case ':':
                parser_flags |= TILEMAP_P_FLAG_ATTRIBUTE;
                bassignmidstr(
                    attribute, string_buffer, 0, blength( string_buffer )
                );
                btrunc( string_buffer, 0 );
                break;

            default:
                /* If we're not on a control character then just consume! */
                if( scaffold_char_is_printable( *i ) ) {
                    bconchar( string_buffer, *i );
                }
                break;
        }

        if( (TILEMAP_P_FLAG_EXECUTE & parser_flags) ) {
            parser_flags &= ~TILEMAP_P_FLAG_EXECUTE;
            /* Do something with the last object harvested. */

            if( (TILEMAP_P_FLAG_LIST & parser_flags) ) {
                /* This is the final item on this list. */
                parser_flags &= ~TILEMAP_P_FLAG_LIST;

                list_buffer = bsplit( string_buffer, ',' );
                scaffold_check_null( list_buffer );
                btrunc( string_buffer, 0 );

                /* TODO: Determine by the attribute what the list goes to. */
                if( 0 < list_buffer->qty ) {
                    scaffold_print_debug(
                        "List, %d items. First: %s, Depth: %d\n",
                        list_buffer->qty, bdata( list_buffer->entry[0] ), tag_depth
                    );
                } else {
                    scaffold_print_debug(
                        "Empty list, Depth: %d\n",
                        bdata( string_buffer ), tag_depth
                    );
                }

                bstrListDestroy( list_buffer );
                list_buffer = NULL;

            } else if( (TILEMAP_P_FLAG_ATTRIBUTE & parser_flags) ) {
                scaffold_print_debug(
                    "Attribute: %s, Value: %s, Depth: %d\n",
                    bdata( attribute ), bdata( string_buffer ), tag_depth
                );
                parser_flags &= ~TILEMAP_P_FLAG_ATTRIBUTE;
            }
        }

        i++;
    }

cleanup:
    bdestroy( string_buffer );
    bdestroy( attribute );
    bstrListDestroy( list_buffer );
    return;
}

void tilemap_load_file( TILEMAP* t, bstring filename ) {
    FILE* tmfile = NULL;
    uint8_t* tmdata = NULL;
    uint32_t datasize = 0;

    tmfile = fopen( bdata( filename ), "rb" );
    scaffold_check_null( tmfile );

    /* Allocate enough space to hold the map. */
    fseek( tmfile, 0, SEEK_END );
    datasize = ftell( tmfile );
    tmdata = calloc( datasize, sizeof( uint8_t ) + 1 ); /* +1 for term. */
    fseek( tmfile, 0, SEEK_SET );

    /* Read and close the map. */
    fread( tmdata, sizeof( uint8_t ), datasize, tmfile );
    fclose( tmfile );
    tmfile = NULL;

    tilemap_parse( t, tmdata );

cleanup:
    return;
}

void tilemap_iterate_screen_row(
    TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
    void (*callback)( TILEMAP* t, uint32_t x, uint32_t y, TILEMAP_TILE e )
) {

}
