/* This file is part of herzi's playground
 *
 * Copyright (C) 2010  Sven Herzberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include "path-element.h"

#include <string.h> /* memcmp() */
#include "shaped.h"

enum
{
  CLICKED,
  N_SIGNALS
};

static guint  signals[N_SIGNALS] = {0};

#define PRIV(i) ((ProgressPathElement*)(i))

static void implement_shaped (ProgressShapedIface* iface);
G_DEFINE_TYPE_WITH_CODE (ProgressPathElement, progress_path_element, PROGRESS_TYPE_SIMPLE_WIDGET_IMPL,
                         G_IMPLEMENT_INTERFACE (PROGRESS_TYPE_SHAPED, implement_shaped));

static void
progress_path_element_init (ProgressPathElement* self G_GNUC_UNUSED)
{
}

static void
finalize (GObject* object)
{
  ProgressPathElement* element = PROGRESS_PATH_ELEMENT (object);

  g_free (element->icon_name);
  if (element->icon)
    {
      g_object_unref (element->icon);
    }
  g_free (element->label);
  if (element->layout)
    {
      g_object_unref (element->layout);
    }
  if (PRIV (object)->path)
    {
      cairo_path_destroy (PRIV (object)->path);
    }

  G_OBJECT_CLASS (progress_path_element_parent_class)->finalize (object);
}

static gboolean
button_press_event (GtkWidget     * widget,
                    GdkEventButton* event)
{
  if (event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
      PRIV (widget)->button_press_time = event->time;
      return TRUE;
    }

  if (GTK_WIDGET_CLASS (progress_path_element_parent_class)->button_press_event)
    {
      return GTK_WIDGET_CLASS (progress_path_element_parent_class)->button_press_event (widget, event);
    }

  return FALSE;
}

static gboolean
button_release_event (GtkWidget     * widget,
                      GdkEventButton* event)
{
  if (event->button == 1)
    {
      if (event->time - PRIV (widget)->button_press_time < 250)
        {
          PRIV (widget)->button_press_time = 0;
          g_signal_emit (widget, signals[CLICKED], 0);
          return TRUE;
        }

      PRIV (widget)->button_press_time = 0;
    }

  if (GTK_WIDGET_CLASS (progress_path_element_parent_class)->button_release_event)
    {
      return GTK_WIDGET_CLASS (progress_path_element_parent_class)->button_release_event (widget, event);
    }

  return FALSE;
}

static void
ensure_path (GtkWidget* widget)
{
  cairo_t* cr;

  if (PRIV (widget)->path)
    {
      return;
    }

  cr = gdk_cairo_create (widget->window);

  cairo_save (cr);
  cairo_translate (cr, widget->allocation.x, widget->allocation.y);

  if (!PRIV (widget)->first)
    {
      cairo_new_path (cr);
      cairo_move_to (cr, 0.5, 23.5);
      cairo_line_to (cr, 12.5, 12.0);
      cairo_line_to (cr, 0.5, 0.5);
    }
  else
    {
      cairo_arc (cr, 4.5, 19.5, 4.0, 0.5 * G_PI, G_PI);
      cairo_arc (cr, 4.5, 4.5, 4.0, G_PI, 1.5 * G_PI);
    }
  if (!PRIV (widget)->last)
    {
      cairo_line_to (cr, widget->allocation.width - 12.5, 0.5);
      cairo_line_to (cr, widget->allocation.width - 0.5, 12.0);
      cairo_line_to (cr, widget->allocation.width - 12.5, 23.5);
    }
  else
    {
      cairo_arc (cr, widget->allocation.width - 4.5, 4.5, 4.0, 1.5 * G_PI, 2.0 * G_PI);
      cairo_arc (cr, widget->allocation.width - 4.5, 19.5, 4.0, 0.0, 0.5 * G_PI);
    }

  cairo_close_path (cr);
  cairo_restore (cr);

  PRIV (widget)->path = cairo_copy_path (cr);
  cairo_destroy (cr);
}

static gboolean
expose_event (GtkWidget     * widget,
              GdkEventExpose* event  G_GNUC_UNUSED)
{
  cairo_pattern_t* pattern;
  cairo_t        * cr = gdk_cairo_create (widget->window);

  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  ensure_path (widget);

  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.5);

  cairo_append_path (cr, PRIV (widget)->path);

  cairo_translate (cr, widget->allocation.x, widget->allocation.y);

  pattern = cairo_pattern_create_linear (0.0, 0.0, widget->allocation.width, widget->allocation.height);
  switch (gtk_widget_get_state (widget))
    {
    case GTK_STATE_PRELIGHT:
      cairo_pattern_add_color_stop_rgba (pattern, 0.0, 0.0, 0.0, 0.0, 0.15);
      cairo_pattern_add_color_stop_rgba (pattern, 1.0, 1.0, 1.0, 1.0, 0.15);
      break;
    case GTK_STATE_ACTIVE:
      cairo_pattern_add_color_stop_rgba (pattern, 0.0, 0.0, 0.0, 0.0, 0.5);
      cairo_pattern_add_color_stop_rgba (pattern, 1.0, 0.5, 0.5, 0.5, 0.5);
      break;
    default:
      cairo_pattern_add_color_stop_rgba (pattern, 0.0, 0.0, 0.0, 0.0, 0.25);
      cairo_pattern_add_color_stop_rgba (pattern, 1.0, 1.0, 1.0, 1.0, 0.25);
      break;
    }

  cairo_save (cr);
  cairo_set_source (cr, pattern);
  cairo_fill_preserve (cr);
  cairo_pattern_destroy (pattern);
  cairo_restore (cr);

  cairo_path_t* path = cairo_copy_path (cr);
  cairo_new_path (cr);
  cairo_save (cr);

  if (!PRIV (widget)->first)
    {
      cairo_translate (cr, 12.0, 0.0);
    }
  cairo_translate (cr, 4.0, 0.0);
  if (PRIV (widget)->icon)
    {
      cairo_save (cr);
      cairo_translate (cr, 0.0, widget->allocation.height / 2 - gdk_pixbuf_get_height (PRIV (widget)->icon) / 2);
      gdk_cairo_set_source_pixbuf (cr, PRIV (widget)->icon, 0.0, 0.0);
      cairo_rectangle (cr, 0.0, 0.0,
                       gdk_pixbuf_get_width (PRIV (widget)->icon),
                       gdk_pixbuf_get_height (PRIV (widget)->icon));
      cairo_fill (cr);
      cairo_restore (cr);
      cairo_translate (cr, gdk_pixbuf_get_width (PRIV (widget)->icon) + 4.0, 0.0);
    }
  if (PRIV (widget)->layout)
    {
      PangoRectangle  logical;

      pango_layout_get_extents (PRIV (widget)->layout, NULL, &logical);

      cairo_translate (cr, 0.0, widget->allocation.height / 2 - PANGO_PIXELS (logical.height) / 2);
      if (gtk_widget_get_state (widget) == GTK_STATE_PRELIGHT)
        {
          cairo_set_source_rgb (cr, 0.15, 0.15, 0.25);
        }
      else
        {
          cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
        }
      pango_cairo_show_layout (cr, PRIV (widget)->layout);
    }
  cairo_restore (cr);
  cairo_new_path (cr);
  cairo_append_path (cr, path);
  cairo_path_destroy (path);

  cairo_stroke (cr);
  cairo_destroy (cr);
  return FALSE;
}

static gboolean
enter_notify_event (GtkWidget       * widget,
                    GdkEventCrossing* event  G_GNUC_UNUSED)
{
  switch (gtk_widget_get_state (widget))
    {
    case GTK_STATE_NORMAL:
      gtk_widget_set_state (widget, GTK_STATE_PRELIGHT);
      break;
    case GTK_STATE_ACTIVE:
    case GTK_STATE_PRELIGHT:
    case GTK_STATE_INSENSITIVE:
    case GTK_STATE_SELECTED:
      break;
    }

  return FALSE;
}

static gboolean
leave_notify_event (GtkWidget       * widget,
                    GdkEventCrossing* event  G_GNUC_UNUSED)
{
  if (gtk_widget_get_state (widget) == GTK_STATE_PRELIGHT)
    {
      gtk_widget_set_state (widget, GTK_STATE_NORMAL);
    }

  return FALSE;
}

static void
style_set (GtkWidget* widget,
           GtkStyle * old_style)
{
  if (PRIV (widget)->icon)
    {
      g_object_unref (PRIV (widget)->icon);
      PRIV (widget)->icon = gtk_widget_render_icon (widget, PRIV (widget)->icon_name, GTK_ICON_SIZE_MENU, NULL);
    }
  if (PRIV (widget)->layout)
    {
      g_object_unref (PRIV (widget)->layout);
      PRIV (widget)->layout = gtk_widget_create_pango_layout (widget, PRIV (widget)->label);
    }

  GTK_WIDGET_CLASS (progress_path_element_parent_class)->style_set (widget, old_style);

  if (PRIV (widget)->path)
    {
      cairo_path_destroy (PRIV (widget)->path);
      PRIV (widget)->path = NULL;
    }
}

static void
size_allocate (GtkWidget    * widget,
               GtkAllocation* allocation)
{
  gboolean needs_update = memcmp (allocation, &widget->allocation, sizeof (GtkAllocation));

  GTK_WIDGET_CLASS (progress_path_element_parent_class)->size_allocate (widget, allocation);

  if (needs_update && PRIV (widget)->path)
    {
      cairo_path_destroy (PRIV (widget)->path);
      PRIV (widget)->path = NULL;
    }
}

static void
size_request (GtkWidget     * widget,
              GtkRequisition* requisition)
{
  int req_width = 4;

  if (!PRIV (widget)->first)
    {
      req_width += 12;
    }
  if (PRIV (widget)->icon)
    {
      requisition->height = MAX (requisition->height, gdk_pixbuf_get_height (PRIV (widget)->icon) + 2 * 4);
      req_width += gdk_pixbuf_get_width (PRIV (widget)->icon);
    }
  if (PRIV (widget)->icon && PRIV (widget)->layout)
    {
      req_width += 4;
    }
  if (PRIV (widget)->layout)
    {
      PangoRectangle  rectangle;
      pango_layout_get_extents (PRIV (widget)->layout, NULL, &rectangle);
      req_width += PANGO_PIXELS_CEIL (rectangle.width);
      requisition->height = MAX (requisition->height, PANGO_PIXELS_CEIL (rectangle.height) + 2 * 4);
    }
  req_width += 4;
  if (!PRIV (widget)->last)
    {
      req_width += 12;
    }

  requisition->width = MAX (requisition->width, req_width);

  GTK_WIDGET_CLASS (progress_path_element_parent_class)->size_request (widget, requisition);
}

static void
progress_path_element_class_init (ProgressPathElementClass* self_class)
{
  GObjectClass  * object_class = G_OBJECT_CLASS (self_class);
  GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (self_class);

  object_class->finalize           = finalize;

  widget_class->button_press_event   = button_press_event;
  widget_class->button_release_event = button_release_event;
  widget_class->expose_event       = expose_event;
  widget_class->enter_notify_event = enter_notify_event;
  widget_class->leave_notify_event = leave_notify_event;
  widget_class->size_allocate      = size_allocate;
  widget_class->size_request       = size_request;
  widget_class->style_set          = style_set;

  signals[CLICKED] = g_signal_new ("clicked", G_OBJECT_CLASS_TYPE (self_class),
                                   G_SIGNAL_ACTION, 0,
                                   NULL, NULL,
                                   g_cclosure_marshal_VOID__VOID,
                                   G_TYPE_NONE, 0);
}

static gboolean
test_hit (ProgressShaped* shaped,
          GdkEventMotion* event)
{
  gboolean  result = FALSE;
  cairo_t* cr = gdk_cairo_create (GTK_WIDGET (shaped)->window);

  ensure_path (GTK_WIDGET (shaped));
  cairo_append_path (cr, PRIV (shaped)->path);
  cairo_set_line_width (cr, 1.0);

  result = cairo_in_fill (cr, event->x, event->y) || cairo_in_stroke (cr, event->x, event->y);

  cairo_destroy (cr);

  return result;
}

static void
implement_shaped (ProgressShapedIface* iface)
{
  iface->test_hit = test_hit;
}

GtkWidget*
progress_path_element_new (gchar const* icon,
                           gchar const* label)
{
  GtkWidget* result = g_object_new (PROGRESS_TYPE_PATH_ELEMENT,
                                    NULL);
  ProgressPathElement* element = PRIV (result);

  element->icon_name = g_strdup (icon);
  element->icon      = NULL;
  if (element->icon_name)
    {
      element->icon = gtk_widget_render_icon (result, element->icon_name, GTK_ICON_SIZE_MENU, NULL);
    }

  element->label  = g_strdup (label);
  element->layout = NULL;
  if (element->label)
    {
      element->layout = gtk_widget_create_pango_layout (result, element->label);
    }

  return result;
}

void
progress_path_element_select (ProgressPathElement* self)
{
  g_return_if_fail (PROGRESS_IS_PATH_ELEMENT (self));

  gtk_widget_set_state (GTK_WIDGET (self), GTK_STATE_ACTIVE);
}

void
progress_path_element_set_first (ProgressPathElement* self,
                                 gboolean             first)
{
  g_return_if_fail (PROGRESS_IS_PATH_ELEMENT (self));
  g_return_if_fail (first == TRUE || first == FALSE);

  if (first == PRIV (self)->first)
    {
      return;
    }

  PRIV (self)->first = first;

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
progress_path_element_set_last (ProgressPathElement* self,
                                gboolean             last)
{
  g_return_if_fail (PROGRESS_IS_PATH_ELEMENT (self));
  g_return_if_fail (last == TRUE || last == FALSE);

  if (last == PRIV (self)->last)
    {
      return;
    }

  PRIV (self)->last = last;

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
progress_path_element_unselect (ProgressPathElement* self)
{
  g_return_if_fail (PROGRESS_IS_PATH_ELEMENT (self));

  gtk_widget_set_state (GTK_WIDGET (self), GTK_STATE_NORMAL);
}

/* vim:set et sw=2 cino=t0,f0,(0,{s,>2s,n-1s,^-1s,e2s: */
