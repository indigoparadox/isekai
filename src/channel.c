
#include "channel.h"

void channel_init( CHANNEL* l ) {
    vector_init( &(l->clients) );
    l->name = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
    scaffold_check_null( l->name );

cleanup:
    return;
}

BOOL channel_client_present( CHANNEL* l, CLIENT* c ) {
    int i;
    CLIENT* c_test = NULL;
    BOOL found = FALSE;

    channel_lock( l );
    for( i = 0 ; vector_count( &(l->clients) ) > i ; i++ ) {
        c_test = (CLIENT*)vector_get( &(l->clients), i );
        if( NULL != c_test && 0 == bstrcmp( c_test->nick, c->nick ) ) {
            found = TRUE;
            break;
        }
    }
    channel_unlock( l );

    return found;
}

void channel_add_client( CHANNEL* l, CLIENT* c ) {
    if( channel_client_present( l, c ) ) {
        goto cleanup;
    }

    channel_lock( l );
    vector_add( &(l->clients), c );
    channel_unlock( l );

cleanup:
    return;
}

void channel_remove_client( CHANNEL* l, CLIENT* c ) {
    CLIENT* c_test = NULL;
    int i;

    channel_lock( l );
    for( i = 0 ; vector_count( &(l->clients) ) > i ; i++ ) {
        c_test = (CLIENT*)vector_get( &(l->clients), i );
        if( NULL != c_test && 0 == bstrcmp( c_test->nick, c->nick ) ) {
            vector_delete( &(l->clients), i );
            goto cleanup;
        }
    }

cleanup:
    channel_unlock( l );
    return;
}

struct bstrList* channel_list_clients( CHANNEL* l ) {
    struct bstrList* list = NULL;
    int bstr_check;
    int client_count;
    int i;
    CLIENT* c;

    channel_lock( l );

    client_count = vector_count( &(l->clients) );
    list = bstrListCreate();
    scaffold_check_null( list );
    bstr_check = bstrListAlloc( list, client_count );
    scaffold_check_nonzero( bstr_check );

    for( i = 0 ; client_count > i ; i++ ) {
        c = (CLIENT*)vector_get( &(l->clients), i );
        if( NULL != c ) {
            list->entry[list->qty] = c->nick;
            list->qty++;
        }
    }

cleanup:
    channel_unlock( l );
    return list;
}

void channel_send( CHANNEL* l, bstring message ) {

}

void channel_lock( CHANNEL* l ) {

}

void channel_unlock( CHANNEL* l ) {

}
