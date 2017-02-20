
#include "tilemap.h"

#include "json/json.h"

#include <memory.h>
#include <string.h>

void tilemap_init( TILEMAP* t ) {
#ifdef INIT_ZEROES
    memset( t, '\0', sizeof( TILEMAP ) );
#endif /* INIT_ZEROES */

    vector_init( &(t->layers) );
    vector_init( &(t->positions) );
}

void tilemap_cleanup( TILEMAP* t ) {
    int i;
    for( i = 0 ; vector_count( &(t->layers) ) > i ; i++ ) {
        tilemap_layer_free( vector_get( &(t->layers), i ) );
    }
    vector_free( &(t->layers) );
    for( i = 0 ; vector_count( &(t->positions) ) > i ; i++ ) {
        tilemap_position_free( vector_get( &(t->positions), i ) );
    }
    vector_free( &(t->positions) );
}

void tilemap_layer_init( TILEMAP_LAYER* layer ) {
    vector_init( &(layer->tiles) );
}

void tilemap_layer_cleanup( TILEMAP_LAYER* layer ) {

}

void tilemap_position_cleanup( TILEMAP_POSITION* position ) {

}

typedef struct {
    struct tagbstring path;
    json_type type;
    void (*callback)( TILEMAP* t, json_value* j, bstring path );
} tilemap_loader_entry;

static void tilemap_parse_layer( TILEMAP* t, json_value* j, bstring path ) {
    TILEMAP_LAYER* layer = NULL;
    int layer_index;
    struct bstrList* path_list = NULL;
    char* base_cstr = NULL;

    /* Figure out the layer index. */
    path_list = bsplit( path, '/' );
    scaffold_check_null( path_list );
    base_cstr = bdata( path_list->entry[path_list->qty - 1] );
    scaffold_check_null( base_cstr );
    layer_index = atoi( base_cstr );

    /* Create the layer if it's missing. */
    layer = vector_get( &(t->layers), layer_index );
    if( NULL == layer ) {
        /* We're starting a new layer. */
        /* TODO: Allow for out-of-order layers? */
        tilemap_layer_new( layer );
        /* TODO: Fail on init failure. */
        vector_add( &(t->layers), layer );
    }

cleanup:
    bstrListDestroy( path_list );
    return;
}

static void tilemap_parse_tile( TILEMAP* t, json_value* j, bstring path ) {
    TILEMAP_LAYER* layer = NULL;
    TILEMAP_TILE* tile = NULL;

    /* This must be a tile. */
    layer = vector_get( &(t->layers), (vector_count( &(t->layers) ) - 1) );
    scaffold_check_null( layer );

    tile = calloc( 1, sizeof( TILEMAP_TILE ) );
    scaffold_check_null( tile );

    tile->tile = j->u.integer;

cleanup:
    return;
}

static const tilemap_loader_entry tilemap_loader_table[] = {
    {bsStatic( "/layers" ), json_array, tilemap_parse_layer},
    {bsStatic( "/layers/data" ), json_integer, tilemap_parse_tile},
    {bsStatic( "" ), json_none, NULL}
};

static void tilemap_parse_walk(
    TILEMAP* t, json_value* j, bstring path, int depth
) {
    bstring buffer = NULL;
    int i;
    const tilemap_loader_entry* loader_entry = tilemap_loader_table;
    BOOL destroy_path = FALSE;

    if( NULL == path ) {
        path = bfromcstralloc( 30, "" );
        destroy_path = TRUE;
        scaffold_check_null( path );
    }

#ifdef DEBUG_JSON
    for( i = 0 ; depth > i ; i++ ) {
        fprintf( stderr, "." );
    }

    if( NULL != name ) {
        fprintf( stderr, "%s: ", bdata( name ) );
    }
#endif /* DEBUG_JSON */

    while( NULL != loader_entry->callback ) {
        if(
            0 == bstrncmp(
                &(loader_entry->path), path, blength( &(loader_entry->path) )
            ) && json_array == j->type
        ) {
            loader_entry->callback( t, j, path );
        }
        loader_entry++;
    }

    switch( j->type ) {
    case json_object:
        for( i = 0 ; j->u.object.length > i ; i++ ) {
            buffer = bstrcpy( path );
            scaffold_check_null( buffer );
            bconchar( buffer, '/' );
            binsertblk( buffer, blength( buffer ), j->u.object.values[i].name, j->u.object.values[i].name_length, '\0' );
            scaffold_print_debug( "Descending into path: %s\n", bdata( buffer ) );
            tilemap_parse_walk( t, j->u.object.values[i].value, buffer, depth + 1 );
            bdestroy( buffer );
        }
        break;

    case json_array:
        for( i = 0 ; j->u.array.length > i ; i++ ) {
            buffer = bformat( "%s/%d", bdata( path ), i );
            scaffold_check_null( buffer );
            scaffold_print_debug( "Descending into path: %s\n", bdata( buffer ) );
            tilemap_parse_walk( t, j->u.array.values[i], buffer, depth + 1 );
            bdestroy( buffer );
        }
        break;

#ifdef DEBUG_JSON
    case json_integer:
        fprintf( stderr, "%d\n", j->u.integer );
        break;

    case json_string:
        fprintf( stderr,"%s\n", j->u.string.ptr );
        break;

    default:
        fprintf( stderr,"\n" );
        break;
#endif /* DEBUG_JSON */
    }

cleanup:
    if( TRUE == destroy_path ) {
        bdestroy( path );
    }
    return;
}

void tilemap_load( TILEMAP* t, const uint8_t* tmdata, int datasize ) {
    json_value* tmapjson = NULL;

    tmapjson = json_parse( (json_char*)tmdata, datasize );
    scaffold_check_null( tmapjson );

    tilemap_parse_walk( t, tmapjson, NULL, 0 );

cleanup:
    json_value_free( tmapjson );
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
    scaffold_check_null( tmdata );
    fseek( tmfile, 0, SEEK_SET );

    /* Read and close the map. */
    fread( tmdata, sizeof( uint8_t ), datasize, tmfile );
    fclose( tmfile );
    tmfile = NULL;

    tilemap_load( t, tmdata, datasize );

cleanup:
    free( tmdata );
    return;
}

void tilemap_iterate_screen_row(
    TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
    void (*callback)( TILEMAP* t, uint32_t x, uint32_t y, TILEMAP_TILE e )
) {

}
