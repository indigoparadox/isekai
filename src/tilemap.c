
#include "tilemap.h"

#include "ezxml/ezxml.h"

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
    layer->tiles = NULL;
}

void tilemap_layer_cleanup( TILEMAP_LAYER* layer ) {
    if( NULL != layer->tiles ) {
        free( layer->tiles );
    }
}

void tilemap_position_cleanup( TILEMAP_POSITION* position ) {

}

static void tilemap_parse_properties( TILEMAP* t, ezxml_t xml_props ) {
    ezxml_t xml_prop_iter = NULL;

    scaffold_check_null( xml_props );

    xml_prop_iter = ezxml_child( xml_props, "property" );

    while( NULL != xml_prop_iter ) {
        //if( 0 == strcmp( ezxml_attr( xml_prop_iter, "name" ), "light_str" ) ) {
        //    map_out->light_str = atoi( ezxml_attr( xml_prop_iter, "value" ) );

        //if( 0 == strcmp( ezxml_attr( xml_prop_iter, "name" ), "music_path" ) ) {
        //    map_out->music_path = bfromcstr( ezxml_attr( xml_prop_iter, "value" ) );

        //if( 0 == strcmp( ezxml_attr( xml_prop_iter, "name" ), "time_moves" ) ) {
        //    if( 0 == strcmp( ezxml_attr( xml_prop_iter, "value" ), "true" ) ) {
        //       map_out->time_moves = TRUE;
        //    } else {
        //       map_out->time_moves = FALSE;
        //    }

        xml_prop_iter = xml_prop_iter->next;
    }

cleanup:
    return;
}

void tilemap_parse_tiledata( TILEMAP* t, ezxml_t xml_tiledata ) {
    scaffold_check_null( xml_tiledata );

cleanup:
    return;
}

void tilemap_parse_layer( TILEMAP* t, ezxml_t xml_layer ) {
    TILEMAP_LAYER* layer = NULL;
    ezxml_t xml_layer_data = NULL;
    bstring buffer = NULL;
    struct bstrList* tiles_list = NULL;
    int i;

    scaffold_check_null( xml_layer );

    tilemap_layer_new( layer );

    layer->width = atoi( ezxml_attr( xml_layer, "width" ) );
    layer->height = atoi( ezxml_attr( xml_layer, "height" ) );

    xml_layer_data = ezxml_child( xml_layer, "data" );
    scaffold_check_null( xml_layer_data );

    buffer = bfromcstr( ezxml_txt( xml_layer_data ) );
    scaffold_check_null( buffer );
    tiles_list = bsplit( buffer, ',' );
    scaffold_check_null( tiles_list );
    layer->tiles_alloc = tiles_list->qty;
    layer->tiles = calloc( layer->tiles_alloc, sizeof( uint16_t ) );
    scaffold_check_null( layer->tiles );

    for( i = 0 ; tiles_list->qty > i ; i++ ) {
        layer->tiles[i] = atoi( (bdata( tiles_list->entry[i] )) );
        layer->tiles_count++;
    }

    vector_add( &(t->layers), layer );

cleanup:
    if( SCAFFOLD_ERROR_NONE != scaffold_error ) {
        tilemap_layer_free( layer );
    }
    bdestroy( buffer );
    bstrListDestroy( tiles_list );
    return;
}

void tilemap_load_data( TILEMAP* t, const uint8_t* tmdata, int datasize ) {
    ezxml_t xml_map = NULL, xml_layer = NULL, xml_props = NULL,
        xml_tiledata = NULL;

    xml_map = ezxml_parse_str( tmdata, datasize );

    xml_tiledata = ezxml_child( xml_map, "tileset" );
    tilemap_parse_tiledata( t, xml_tiledata );

    xml_props = ezxml_child( xml_map, "properties" );
    tilemap_parse_properties( t, xml_props );

    /* Load the map's tile data. */
    //map_out->tileset = tilemap_create_tileset( tiledata_path );

    xml_layer = ezxml_child( xml_map, "layer" );
    while( NULL != xml_layer ) {
        tilemap_parse_layer( t, xml_layer );
        xml_layer = xml_layer->next;
    }


cleanup:
    ezxml_free( xml_map );
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

    tilemap_load_data( t, tmdata, datasize );

cleanup:
    if( NULL != tmdata ) {
        free( tmdata );
    }
    return;
}

void tilemap_iterate_screen_row(
    TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
    void (*callback)( TILEMAP* t, uint32_t x, uint32_t y )
) {

}
