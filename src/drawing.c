
#include <math.h>
#include "drawing.h"
#include "main.h"

void draw_line (GromitData *data,
		GdkDevice *dev,
		gint x1, gint y1,
		gint x2, gint y2)
{
  GdkRectangle rect;
  GromitDeviceData *devdata = g_hash_table_lookup(data->devdatatable, dev);

  rect.x = MIN (x1,x2) - data->maxwidth / 2;
  rect.y = MIN (y1,y2) - data->maxwidth / 2;
  rect.width = ABS (x1-x2) + data->maxwidth;
  rect.height = ABS (y1-y2) + data->maxwidth;

  if(data->debug)
    g_printerr("DEBUG: draw line from %d %d to %d %d\n", x1, y1, x2, y2);

  if (devdata->cur_context->paint_ctx)
    {
      cairo_set_line_width(devdata->cur_context->paint_ctx, data->maxwidth);
      cairo_set_line_cap(devdata->cur_context->paint_ctx, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join(devdata->cur_context->paint_ctx, CAIRO_LINE_JOIN_ROUND);
 
      cairo_move_to(devdata->cur_context->paint_ctx, x1, y1);
      cairo_line_to(devdata->cur_context->paint_ctx, x2, y2);
      cairo_stroke(devdata->cur_context->paint_ctx);

      data->modified = 1;

      gdk_window_invalidate_rect(gtk_widget_get_window(data->win), &rect, 0);
    }

  data->painted = 1;
}


void draw_arrow (GromitData *data, 
		 GdkDevice *dev,
		 gint x1, gint y1,
		 gfloat width,
		 gfloat direction)
{
  GdkRectangle rect;
  GdkPoint arrowhead [4];

  /* get the data for this device */
  GromitDeviceData *devdata = g_hash_table_lookup(data->devdatatable, dev);

  width = width / 2;

  /* I doubt that calculating the boundary box more exact is very useful */
  rect.x = x1 - 4 * width - 1;
  rect.y = y1 - 4 * width - 1;
  rect.width = 8 * width + 2;
  rect.height = 8 * width + 2;

  arrowhead [0].x = x1 + 4 * width * cos (direction);
  arrowhead [0].y = y1 + 4 * width * sin (direction);

  arrowhead [1].x = x1 - 3 * width * cos (direction)
                       + 3 * width * sin (direction);
  arrowhead [1].y = y1 - 3 * width * cos (direction)
                       - 3 * width * sin (direction);

  arrowhead [2].x = x1 - 2 * width * cos (direction);
  arrowhead [2].y = y1 - 2 * width * sin (direction);

  arrowhead [3].x = x1 - 3 * width * cos (direction)
                       - 3 * width * sin (direction);
  arrowhead [3].y = y1 + 3 * width * cos (direction)
                       - 3 * width * sin (direction);

  if (devdata->cur_context->paint_ctx)
    {
      cairo_set_line_width(devdata->cur_context->paint_ctx, 1);
      cairo_set_line_cap(devdata->cur_context->paint_ctx, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join(devdata->cur_context->paint_ctx, CAIRO_LINE_JOIN_ROUND);
 
      cairo_move_to(devdata->cur_context->paint_ctx, arrowhead[0].x, arrowhead[0].y);
      cairo_line_to(devdata->cur_context->paint_ctx, arrowhead[1].x, arrowhead[1].y);
      cairo_line_to(devdata->cur_context->paint_ctx, arrowhead[2].x, arrowhead[2].y);
      cairo_line_to(devdata->cur_context->paint_ctx, arrowhead[3].x, arrowhead[3].y);
      cairo_fill(devdata->cur_context->paint_ctx);

      gdk_cairo_set_source_rgba(devdata->cur_context->paint_ctx, data->black);

      cairo_move_to(devdata->cur_context->paint_ctx, arrowhead[0].x, arrowhead[0].y);
      cairo_line_to(devdata->cur_context->paint_ctx, arrowhead[1].x, arrowhead[1].y);
      cairo_line_to(devdata->cur_context->paint_ctx, arrowhead[2].x, arrowhead[2].y);
      cairo_line_to(devdata->cur_context->paint_ctx, arrowhead[3].x, arrowhead[3].y);
      cairo_line_to(devdata->cur_context->paint_ctx, arrowhead[0].x, arrowhead[0].y);
      cairo_stroke(devdata->cur_context->paint_ctx);

      gdk_cairo_set_source_rgba(devdata->cur_context->paint_ctx, devdata->cur_context->paint_color);
    
      data->modified = 1;

      gdk_window_invalidate_rect(gtk_widget_get_window(data->win), &rect, 0); 
    }

  data->painted = 1;
}

void draw_frame (GromitData *data,
    GdkDevice *dev,
    gint x, gint y,
    gint xlength, gint ylength,
    gint radius, gint strokewidth,
    GdkRGBA *fill_color)
{
  GdkRectangle rect;
  GromitDeviceData *devdata = g_hash_table_lookup(data->devdatatable, dev);

  x = x - xlength / 2 + 1;
  y = y - ylength / 2 + 1;

  rect.x = x - strokewidth;
  rect.y = y - strokewidth;
  rect.width = xlength + strokewidth * 2;
  rect.height = ylength + strokewidth * 2;

  if (radius > MIN (xlength, ylength) / 2)
    radius = MIN (xlength, ylength) / 2;

  if(data->debug)
    g_printerr("DEBUG: draw frame with center %d, %d, width %d, height %d, corner radius %d and fill color %s\n", x, y, xlength, ylength, radius, gdk_rgba_to_string(fill_color));

  if (devdata->cur_context->paint_ctx)
    {
      double degrees = M_PI / 180.0;
      cairo_new_sub_path(devdata->cur_context->paint_ctx);
      cairo_arc(devdata->cur_context->paint_ctx, x + xlength - radius, y + radius, radius, -90 * degrees, 0 * degrees);
      cairo_arc(devdata->cur_context->paint_ctx, x + xlength - radius, y + ylength - radius, radius, 0 * degrees, 90 * degrees);
      cairo_arc(devdata->cur_context->paint_ctx, x + radius, y + ylength - radius, radius, 90 * degrees, 180 * degrees);
      cairo_arc(devdata->cur_context->paint_ctx, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
      cairo_close_path(devdata->cur_context->paint_ctx);
      if (fill_color)
        {
          gdk_cairo_set_source_rgba(devdata->cur_context->paint_ctx, devdata->cur_context->fill_color);
          cairo_fill_preserve(devdata->cur_context->paint_ctx);
          gdk_cairo_set_source_rgba(devdata->cur_context->paint_ctx, devdata->cur_context->paint_color);
        }
      cairo_stroke(devdata->cur_context->paint_ctx);

      data->modified = 1;

      gdk_window_invalidate_rect(gtk_widget_get_window(data->win), &rect, 0);
    }

  data->painted = 1;
}

/*
 * whether a text of the given extents, centered in a frame like draw_frame()
 * draws it, stays inside the frame's border with a pixel to spare
 */
static gboolean text_fits_frame (cairo_text_extents_t *te,
    gint xlength, gint ylength,
    gint radius, gint strokewidth)
{
  gdouble inset = strokewidth / 2.0 + 1;
  gdouble half_x = xlength / 2.0 - inset;
  gdouble half_y = ylength / 2.0 - inset;
  gdouble corner = MIN (radius, MIN (xlength, ylength) / 2) - inset;
  gdouble px = te->width / 2;
  gdouble py = te->height / 2;

  if (px > half_x || py > half_y)
    return FALSE;
  if (corner <= 0)
    return TRUE;
  /* the text's corner must not reach into the rounded corner of the frame */
  gdouble dx = px - (half_x - corner);
  gdouble dy = py - (half_y - corner);
  return dx <= 0 || dy <= 0 || dx * dx + dy * dy <= corner * corner;
}

void draw_counter (GromitData *data,
    GdkDevice *dev,
    gint x, gint y,
    gint xlength, gint ylength,
    gint radius, gint strokewidth,
    GdkRGBA *fill_color,
    GdkRGBA *font_color,
    gchar *font_face,
    guint font_size,
    gint count)
{
  draw_frame(data, dev, x, y, xlength, ylength, radius, strokewidth, fill_color);

  GdkRectangle rect;
  GromitDeviceData *devdata = g_hash_table_lookup(data->devdatatable, dev);

  rect.x = x - xlength / 2 - strokewidth;
  rect.y = y - ylength / 2 - strokewidth;
  rect.width = xlength + strokewidth * 2;
  rect.height = ylength + strokewidth * 2;

  if (devdata->cur_context->paint_ctx)
    {
      char countstr[12];
      g_snprintf(countstr, sizeof(countstr), "%d", count);

      if (count > 9 || count < -9)
        font_size = font_size * 0.9;

      cairo_text_extents_t te;
      gdk_cairo_set_source_rgba(devdata->cur_context->paint_ctx, devdata->cur_context->font_color);
      cairo_select_font_face (devdata->cur_context->paint_ctx, font_face, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_font_size (devdata->cur_context->paint_ctx, font_size);
      cairo_text_extents (devdata->cur_context->paint_ctx, countstr, &te);

      /* shrink numbers too wide for the frame, a frameless counter has no limit */
      while (xlength > 0 && ylength > 0 && font_size > 1 &&
             !text_fits_frame (&te, xlength, ylength, radius, strokewidth))
        {
          font_size = font_size * 0.9;
          cairo_set_font_size (devdata->cur_context->paint_ctx, font_size);
          cairo_text_extents (devdata->cur_context->paint_ctx, countstr, &te);
        }

      if(data->debug)
        g_printerr("DEBUG: draw counter with center %d, %d, width %d, height %d, corner radius %d, fill color %s, font_color %s, font_face %s, font_size %u and count %d\n", x, y, xlength, ylength, radius, gdk_rgba_to_string(fill_color), gdk_rgba_to_string(font_color), font_face, font_size, count);

      cairo_move_to (devdata->cur_context->paint_ctx, x + 1 - te.width / 2 - te.x_bearing, y + 1 - te.height / 2 - te.y_bearing);
      cairo_show_text (devdata->cur_context->paint_ctx, countstr);

      /* a frameless counter's text can be larger than its frame */
      GdkRectangle text_rect = { x - te.width / 2 - 1, y - te.height / 2 - 1, te.width + 4, te.height + 4 };
      gdk_rectangle_union (&rect, &text_rect, &rect);

      data->modified = 1;

      gdk_window_invalidate_rect(gtk_widget_get_window(data->win), &rect, 0);
    }

  data->painted = 1;
}

void draw_stamp (GromitData *data,
    GdkDevice *dev,
    gint x, gint y,
    gint xlength, gint ylength,
    gint radius, gint strokewidth,
    GdkRGBA *fill_color,
    GdkRGBA *font_color,
    gchar *font_face,
    guint font_size,
    gchar *stamp)
{
  draw_frame(data, dev, x, y, xlength, ylength, radius, strokewidth, fill_color);

  GdkRectangle rect;
  GromitDeviceData *devdata = g_hash_table_lookup(data->devdatatable, dev);

  if (devdata->cur_context->paint_ctx)
    {
      if(data->debug)
        g_printerr("DEBUG: draw stamp with center %d, %d, width %d, height %d, corner radius %d, fill color %s, font color %s, font_face %s, font_size %u and stamp %s\n", x, y, xlength, ylength, radius, gdk_rgba_to_string(fill_color), gdk_rgba_to_string(font_color), font_face, font_size, stamp);

      cairo_text_extents_t te;
      gdk_cairo_set_source_rgba(devdata->cur_context->paint_ctx, devdata->cur_context->font_color);
      cairo_select_font_face (devdata->cur_context->paint_ctx, font_face, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_font_size (devdata->cur_context->paint_ctx, font_size);
      cairo_text_extents (devdata->cur_context->paint_ctx, stamp, &te);

      xlength = MAX (xlength, te.width);
      ylength = MAX (ylength, te.height);
      rect.x = x - xlength / 2 - strokewidth;
      rect.y = y - ylength / 2 - strokewidth;
      rect.width = xlength + strokewidth * 2;
      rect.height = ylength + strokewidth * 2;

      cairo_move_to (devdata->cur_context->paint_ctx, x + 1 - te.width / 2 - te.x_bearing, y - te.height / 2 - te.y_bearing);
      cairo_show_text (devdata->cur_context->paint_ctx, stamp);

      data->modified = 1;

      gdk_window_invalidate_rect(gtk_widget_get_window(data->win), &rect, 0);
    }

  data->painted = 1;
}

/*
 * fills the current path with the tool's fill color, then strokes it
 */
static void fill_and_stroke (GromitPaintContext *context)
{
  if (context->fill_color)
    {
      gdk_cairo_set_source_rgba(context->paint_ctx, context->fill_color);
      cairo_fill_preserve(context->paint_ctx);
      gdk_cairo_set_source_rgba(context->paint_ctx, context->paint_color);
    }
  cairo_stroke(context->paint_ctx);
}

void draw_circle (GromitData *data,
                 GdkDevice *dev,
                 gint x, gint y,
                 gfloat radius)
{
  GdkRectangle rect;
  GromitDeviceData *devdata = g_hash_table_lookup(data->devdatatable, dev);

  /* Invalidation rectangle */
  rect.x = x - radius - data->maxwidth / 2;
  rect.y = y - radius - data->maxwidth / 2;
  rect.width = 2 * radius + data->maxwidth;
  rect.height = 2 * radius + data->maxwidth;

  if (devdata->cur_context->paint_ctx)
    {
      cairo_set_line_width(devdata->cur_context->paint_ctx, data->maxwidth);
      cairo_set_line_cap(devdata->cur_context->paint_ctx, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join(devdata->cur_context->paint_ctx, CAIRO_LINE_JOIN_ROUND);

      cairo_arc(devdata->cur_context->paint_ctx, x, y, radius, 0, 2 * M_PI);
      fill_and_stroke(devdata->cur_context);

      data->modified = 1;

      gdk_window_invalidate_rect(gtk_widget_get_window(data->win), &rect, 0);
    }

  data->painted = 1;
}

void draw_rect (GromitData *data,
                GdkDevice *dev,
                gint x1, gint y1,
                gint x2, gint y2)
{
  GdkRectangle rect;
  GromitDeviceData *devdata = g_hash_table_lookup(data->devdatatable, dev);

  /* Invalidation rectangle */
  rect.x = MIN (x1,x2) - data->maxwidth / 2;
  rect.y = MIN (y1,y2) - data->maxwidth / 2;
  rect.width = ABS (x1-x2) + data->maxwidth;
  rect.height = ABS (y1-y2) + data->maxwidth;

  if(data->debug)
    g_printerr("DEBUG: draw rect from %d %d to %d %d\n", x1, y1, x2, y2);

  if (devdata->cur_context->paint_ctx)
    {
      cairo_set_line_width(devdata->cur_context->paint_ctx, data->maxwidth);
      cairo_set_line_cap(devdata->cur_context->paint_ctx, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join(devdata->cur_context->paint_ctx, CAIRO_LINE_JOIN_ROUND);

      cairo_rectangle(devdata->cur_context->paint_ctx, MIN (x1,x2), MIN (y1,y2), ABS (x1-x2), ABS (y1-y2));
      fill_and_stroke(devdata->cur_context);

      data->modified = 1;

      gdk_window_invalidate_rect(gtk_widget_get_window(data->win), &rect, 0);
    }

  data->painted = 1;
}

void draw_length_label (GromitData *data,
                        GdkDevice *dev,
                        gint x1, gint y1,
                        gint x2, gint y2)
{
  gdouble dx = x2 - x1;
  gdouble dy = y2 - y1;
  gdouble length = sqrt(dx * dx + dy * dy);

  if (length == 0) return;

  char label[64];
  snprintf(label, sizeof(label), "%.0f px", length);

  gdouble mx = (x1 + x2) / 2;
  gdouble my = (y1 + y2) / 2;

  draw_string_label(data, dev, mx, my, label);
}

void draw_string_label (GromitData *data, GdkDevice *dev, gint x, gint y, char *label)
{
  GromitDeviceData *devdata = g_hash_table_lookup(data->devdatatable, dev);
  cairo_t *cr = devdata->cur_context->paint_ctx;

  cairo_save(cr);

  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, devdata->cur_context->textsize);
  cairo_text_extents_t extents;
  cairo_text_extents(cr, label, &extents);

  gdouble padding = 4.0;
  gdouble tx = x - extents.width / 2.0;
  gdouble ty = y - extents.height / 2.0;

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
  cairo_rectangle(cr,
                  tx + extents.x_bearing - padding,
                  ty + extents.y_bearing - padding,
                  extents.width + 2 * padding,
                  extents.height + 2 * padding);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1, 1, 1, 1.0);
  cairo_move_to(cr, tx, ty);
  cairo_show_text(cr, label);

  cairo_restore(cr);
  gdk_cairo_set_source_rgba(cr, devdata->cur_context->paint_color);

  /* Invalidation rectangle */
  GdkRectangle rect;
  rect.x = (int)(tx + extents.x_bearing - padding - 1);
  rect.y = (int)(ty + extents.y_bearing - padding - 1);
  rect.width = (int)(extents.width + 2 * padding + 3);
  rect.height = (int)(extents.height + 2 * padding + 3);

  data->modified = 1;

  gdk_window_invalidate_rect(gtk_widget_get_window(data->win), &rect, 0);
}