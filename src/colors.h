
#ifndef COLORS_H
#define COLORS_H

#define COLOR_TABLE( name ) \
   typedef enum name ## _COLOR { \
      name ## _COLOR_TRANSPARENT =  0, \
      name ## _COLOR_DARK_BLUE   =  1, \
      name ## _COLOR_DARK_GREEN  =  2, \
      name ## _COLOR_DARK_CYAN   =  3, \
      name ## _COLOR_DARK_RED    =  4, \
      name ## _COLOR_PURPLE      =  5, \
      name ## _COLOR_BROWN       =  6, \
      name ## _COLOR_GRAY        =  7, \
      name ## _COLOR_CHARCOAL    =  8, \
      name ## _COLOR_BLUE        =  9, \
      name ## _COLOR_GREEN       = 10, \
      name ## _COLOR_CYAN        = 11, \
      name ## _COLOR_RED         = 12, \
      name ## _COLOR_MAGENTA     = 13, \
      name ## _COLOR_YELLOW      = 14, \
      name ## _COLOR_WHITE       = 15 \
   } name ## _COLOR;

#endif /* COLORS_H */
