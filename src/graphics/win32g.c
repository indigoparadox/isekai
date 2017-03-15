
#define GRAPHICS_C
#include "../graphics.h"

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
   PAINTSTRUCT ps;
   HDC hdc;

   switch( message )
   {
      case WM_PAINT:
         hdc = BeginPaint( hWnd, &ps );

         EndPaint( hWnd, &ps );
         break;
      case WM_DESTROY:
         PostQuitMessage( 0 );
         break;
      default:
         return DefWindowProc( hWnd, message, wParam, lParam );
         break;
   }

   return 0;
}

void graphics_screen_init(
   GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h,
   SCAFFOLD_SIZE vw, SCAFFOLD_SIZE vh,  int32_t arg1, void* arg2
) {
   int result;
   HWND hWnd;
   INT nRetVal = 0;
   WNDCLASSEX winclass;
   HBRUSH hBrush;
   ZeroMemory( &winclass, sizeof( WNDCLASSEX ) );

   hBrush = CreateSolidBrush( RGB( 0, 0, 0 ) );

   winclass.cbSize = sizeof( WNDCLASSEX );
   winclass.hbrBackground = hBrush;
   winclass.hCursor = LoadCursor( NULL, IDC_ARROW );
   winclass.hInstance = arg2;
   winclass.lpfnWndProc = (WNDPROC)WndProc;
   winclass.lpszClassName = "IPWindowClass";
   winclass.style = CS_HREDRAW | CS_VREDRAW;

   result = RegisterClassEx( &winclass );
   if( !result ) {
      /* TODO: Display error. */
      GetLastError();
      MessageBox( NULL, "Error registering window class.", "Error", MB_ICONERROR | MB_OK );
      nRetVal = 1;
      goto cleanup;
   }

   hWnd = CreateWindowEx(
      NULL, "IPWindowClass", "ProIRCd",
      WS_OVERLAPPED | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU,
      0, 0, w, h, NULL, NULL, arg2, NULL
   );

   /* SetWindowDisplayAflfinity( hWnd, WDA_MONITOR ); */

   ShowWindow( hWnd, arg1 );

cleanup:
   return;
}

void graphics_surface_init( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h ) {

}

void graphics_surface_free( GRAPHICS* g ) {

}

void graphics_flip_screen( GRAPHICS* g ) {

}

void graphics_shutdown( GRAPHICS* g ) {

}

void graphics_set_font( GRAPHICS* g, bstring name ) {

}

void graphics_set_color( GRAPHICS* g, GRAPHICS_COLOR color ) {

}

void graphics_set_color_ex( GRAPHICS* gr, uint8_t r, uint8_t g, uint8_t b, uint8_t a ) {

}

void graphics_set_image_path( GRAPHICS* g, const bstring path ) {

}

void graphics_set_image_data(
   GRAPHICS* g, const BYTE* data, SCAFFOLD_SIZE length
) {
   BITMAPFILEHEADER bfh = *(BITMAPFILEHEADER*)data;
   BITMAPINFOHEADER bih = *(BITMAPINFOHEADER*)(data +
                                 sizeof( BITMAPFILEHEADER ));
   RGBQUAD             rgb = *(RGBQUAD*)(data +
                                 sizeof( BITMAPFILEHEADER ) +
                                 sizeof( BITMAPINFOHEADER ));
   BITMAPINFO bi;
   bi.bmiColors[0] = rgb;
   bi.bmiHeader = bih;
   BYTE* pPixels = (data + bfh.bfOffBits);
   BYTE* ppvBits;

   if( NULL != g->surface ) {
      DeleteObject( g->surface );
      g->surface = NULL;
   }

   g->surface =
      CreateDIBSection( NULL, &bi, DIB_RGB_COLORS, (void**) &ppvBits, NULL, 0 );
   SetDIBits( NULL, g->surface, 0, bih.biHeight, pPixels, &bi, DIB_RGB_COLORS );

   /* GetObject( g->surface, sizeof( BITMAP ), NULL ); */
}

BYTE* graphics_export_image_data( GRAPHICS* g, SCAFFOLD_SIZE* out_len ) {

}

void graphics_draw_text(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, GRAPHICS_TEXT_ALIGN align,
   const bstring text
) {

}

void graphics_draw_rect( GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h ) {

}

void graphics_measure_text( GRAPHICS* g, GRAPHICS_RECT* r, const bstring text ) {

}

void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx ) {

}

void graphics_scale( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h ) {


}

void graphics_blit(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE s_w, SCAFFOLD_SIZE s_h, const GRAPHICS* src
) {

}

void graphics_blit_partial(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE s_x, SCAFFOLD_SIZE s_y,
   SCAFFOLD_SIZE s_w, SCAFFOLD_SIZE s_h, const GRAPHICS* src
) {
   HDC hdcDest;
   HDC hdcSrc;
   BITMAP srcBitmap;
   HBITMAP srcHbm;

   hdcSrc = CreateCompatibleDC( hdcDest );
   srcHbm = SelectObject( hdcSrc, src->surface );

   GetObject( src->surface, sizeof( BITMAP ), &srcBitmap );

   BitBlt(
      hdcDest, 0, 0, srcBitmap.bmWidth, srcBitmap.bmHeight, hdcSrc, 0, 0,
      SRCCOPY
   );

   SelectObject( hdcDest, srcHbm );

   DeleteDC( hdcSrc );
   DeleteDC( hdcDest );
}

void graphics_sleep( uint16_t milliseconds ) {

}

void graphics_colors_to_surface(
   GRAPHICS* g, GRAPHICS_COLOR* colors, SCAFFOLD_SIZE colors_sz
) {

}

void graphics_wait_for_fps_timer() {

}
