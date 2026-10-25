#ifndef DRAWING_H
#define DRAWING_H

/*
  Functions that manipulate the surfaces pixel-wise.
  Does not include functions that treat a surface like a buffer, like undo/redo etc.
*/

#include "main.h"

typedef struct
{
  gint x;
  gint y;
  gint width;
} GromitStrokeCoordinate;


void draw_line (GromitData *data, GdkDevice *dev, gint x1, gint y1, gint x2, gint y2);
void draw_arrow (GromitData *data, GdkDevice *dev, gint x1, gint y1, gfloat width, gfloat direction);
void draw_frame (GromitData *data, GdkDevice *dev, gint x, gint y, gint xlength, gint ylength, gint radius, gint strokewidth, GdkRGBA *fill_color);
void draw_counter (GromitData *data, GdkDevice *dev, gint x, gint y, gint xlength, gint ylength, gint radius, gint strokewidth, GdkRGBA *fill_color, GdkRGBA *font_color, gchar *font_face, guint font_size, gint count);
void draw_stamp (GromitData *data, GdkDevice *dev, gint x, gint y, gint xlength, gint ylength, gint radius, gint strokewidth, GdkRGBA *fill_color, GdkRGBA *font_color, gchar *font_face, guint font_size, gchar *stamp);
void draw_circle (GromitData *data, GdkDevice *dev, gint x, gint y, gfloat radius);
void draw_rect (GromitData *data, GdkDevice *dev, gint x1, gint y1, gint x2, gint y2);
void draw_length_label (GromitData *data, GdkDevice *dev, gint x1, gint y1, gint x2, gint y2);
void draw_string_label (GromitData *data, GdkDevice *dev, gint x, gint y, char *string);

#endif
