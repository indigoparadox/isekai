
#include "json.h"

#if 0
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
#endif

typedef enum {
    JSON_STATE_NONE,
    JSON_STATE_STRING,
} JSON_STATE;

int json_parse_string( bstring string_buffer, const uint8_t* data ) {
    uint8_t* i = (uint8_t*)data;
    int bstr_result = 0;
    int consumed = 0,
        jumpahead = 0;

    /* TODO: Error if first character is not " */

    /* Skip the first ". */

    while( '\0' != *i && '"' != *i ) {
        consumed++;
        i++;
        bconchar( string_buffer, *i );
    }

cleanup:
    return;
}

int json_parse_list( JSON* j, const uint8_t* data ) {
    int consumed = 0;
    uint8_t* i = (uint8_t*)data;
    JSON* child = NULL;

    while( '\0' != *i && ']' != *i ) {
        switch( *i ) {
            case '{':
                break;

            case ',':
                break;

            default:
                break;
        }

        vector_add( &(j->children), child );
    }

cleanup:
    return consumed;
}

/* We purposely don't take the data as a bstring in order to save memory. */
int json_parse_object( JSON* j, const uint8_t* data ) {
    uint8_t* i = (uint8_t*)data;
    //TILEMAP_TAG breadcrumbs[TILEMAP_PARSER_MAX_DEPTH_ALLOC] = { 0 };
    uint16_t parser_flags = 0;
    bstring string_buffer = NULL;
    int8_t tag_depth = 0;
    //bstring attribute = NULL;
    struct bstrList* list_buffer = NULL;
    int bstr_result = 0;
    JSON* child = NULL;
    int consumed = 0,
        jumpahead = 0;

    /* TODO: Error if first character is not {? */

    vector_init( &(j->children) );

    string_buffer = bfromcstralloc( JSON_BUFFER_ALLOC, "" );
    scaffold_check_null( string_buffer );
    //attribute = bfromcstralloc( JSON_BUFFER_ALLOC, "" );
    //scaffold_check_null( attribute );

    while( '\0' != *i ) {
        consumed++;
        i++;
        switch( *i ) {
            case '[':
            case '{':
                child = calloc( 1, sizeof( JSON ) );
                child->depth = j->depth + 1;
                //child->name = bstrcpy( string_buffer );
                //btrunc( string_buffer, 0 );

                /* Recurse on the child and move forward by consumed chars. */
                if( '[' == * i ) {
                    jumpahead = json_parse_list( child, i );
                } else {
                    jumpahead = json_parse_object( child, i );
                }
                vector_add( &(j->children), child );

#if 0
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
#endif
                break;

            case ']':
            case '}':
                /* Go back up one level, but not above the root! */
                goto cleanup;

            case '"':
                if( NULL != j->name ) {
                    /* We've got a name, so this must be a child. */
                }
                btrunc( string_buffer, 0 );
                jumpahead = json_parse_string( child, i );
                break;

            case ':':
                bassignmidstr( j->name, string_buffer, 0, blength( string_buffer ) );
#if 0
                parser_flags |= TILEMAP_P_FLAG_ATTRIBUTE;
                bassignmidstr(
                    attribute, string_buffer, 0, blength( string_buffer )
                );
                btrunc( string_buffer, 0 );
#endif
                break;

            default:
                /* If we're not on a control character then just consume! */
                /*if( scaffold_char_is_printable( *i ) ) {
                    bconchar( string_buffer, *i );
                }*/
                break;
        }

        i += jumpahead;
        consumed += jumpahead;

#if 0
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
#endif
    }

cleanup:
    bdestroy( string_buffer );
    return consumed;
}

void json_walk( JSON* j ) {
    int i;
    int child_count;

    if( NULL != j->name && NULL != j->value ) {
        scaffold_print_debug( "JSON object: %s, Value: %s\n", bdata( j->name ), bdata( j->value ) );
    }

    child_count = vector_count( &(j->children ) );

    if( 0 < child_count ) {
        for( i = 0 ; child_count > i ; i++ ) {
            json_walk( vector_get( &(j->children), i ) );
        }
    }
}
