/* gr-utils.c:
 *
 * Copyright (C) 2016 Matthias Clasen <mclasen@redhat.com>
 *
 * Licensed under the GNU General Public License Version 3
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib/gi18n.h>
#include <stdlib.h>

#include "gr-utils.h"

/* load image rotated by angle to fit in width x height while preserving
 * aspect ratio, filling seams with transparency
 */
GdkPixbuf *
load_pixbuf_fit_size (const char *path,
                      int         angle,
                      int         width,
                      int         height,
                      gboolean    pad)
{
        g_autoptr(GdkPixbuf) original = NULL;
        GdkPixbuf *pixbuf;
        int dest_x, dest_y, dest_width, dest_height;

        int load_width, load_height;

        if (angle == 90 || angle == 270) {
                load_width = height;
                load_height = width;
        }
        else {
                load_width = width;
                load_height = height;
        }

        pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
        gdk_pixbuf_fill (pixbuf, 0x00000000);

        original = gdk_pixbuf_new_from_file_at_size (path, load_width, load_height, NULL);
        if (!original) {
                g_warning ("Failed to load image %s", path);
                return pixbuf;
        }
        if (angle != 0) {
                g_autoptr(GdkPixbuf) pb = NULL;
                pb = gdk_pixbuf_rotate_simple (original, angle);
                g_set_object (&original, pb);
        }

        if (pad) {
                dest_width = gdk_pixbuf_get_width (original);
                dest_height = gdk_pixbuf_get_height (original);
                dest_x = (width - dest_width) / 2;
                dest_y = (height - dest_height) / 2;

                gdk_pixbuf_composite (original, pixbuf,
                                      dest_x, dest_y, dest_width, dest_height,
                                      dest_x, dest_y, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
        }
        else {
                g_set_object (&pixbuf, original);
        }

        return pixbuf;
}

/* load image rotated by angle to fill width x height while preserving
 * aspect ratio, cutting off overshoots
 */
GdkPixbuf *
load_pixbuf_fill_size (const char *path,
                       int         angle,
                       int         width,
                       int         height)
{
        g_autoptr(GdkPixbuf) original = NULL;
        int x, y;
        int load_width, load_height;

        if (angle == 90 || angle == 270) {
                load_width = height;
                load_height = width;
        }
        else {
                load_width = width;
                load_height = height;
        }

        original = gdk_pixbuf_new_from_file_at_scale (path, -1, load_height, TRUE, NULL);
        if (!original) {
                GdkPixbuf *pixbuf;
                g_warning ("Failed to load image %s", path);
                pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
                gdk_pixbuf_fill (pixbuf, 0x00000000);
                return pixbuf;
        }

        if (angle != 0) {
                g_autoptr(GdkPixbuf) pb = NULL;
                pb = gdk_pixbuf_rotate_simple (original, angle);
                g_set_object (&original, pb);
        }

        if (gdk_pixbuf_get_width (original) < width) {
                g_autoptr(GdkPixbuf) pb1 = NULL;
                pb1 = gdk_pixbuf_new_from_file_at_scale (path, load_width, -1, TRUE, NULL);
                g_set_object (&original, pb1);
                if (angle != 0) {
                        g_autoptr(GdkPixbuf) pb = NULL;
                        pb = gdk_pixbuf_rotate_simple (original, angle);
                        g_set_object (&original, pb);
                }
        }

        g_assert (gdk_pixbuf_get_width (original) >= width &&
                  gdk_pixbuf_get_height (original) >= height);

        x = (gdk_pixbuf_get_width (original) - width) / 2;
        y = (gdk_pixbuf_get_height (original) - height) / 2;

        if (x == 0 && y == 0)
                return g_object_ref (original);
        else
                return gdk_pixbuf_new_subpixbuf (original, x, y, width, height);
}

const char *
get_user_data_dir (void)
{
        static char *dir = NULL;

        if (!dir) {
                dir = g_build_filename (g_get_user_data_dir (), "recipes", NULL);
                g_mkdir_with_parents (dir, 0755);
        }

        return (const char *)dir;
}

const char *
get_pkg_data_dir (void)
{
        return PKGDATADIR;
}

void
container_remove_all (GtkContainer *container)
{
        GList *children, *l;

        children = gtk_container_get_children (container);
        for (l = children; l; l = l->next) {
                gtk_container_remove (container, GTK_WIDGET (l->data));
        }
        g_list_free (children);
}

static void
gr_utils_widget_css_parsing_error_cb (GtkCssProvider *provider,
                                      GtkCssSection *section,
                                      GError *error,
                                      gpointer user_data)
{
        g_warning ("CSS parse error %u:%u: %s",
                   gtk_css_section_get_start_line (section),
                   gtk_css_section_get_start_position (section),
                   error->message);
}

static void
gr_utils_widget_set_css_internal (GtkWidget *widget,
                                  const gchar *class_name,
                                  const gchar *css)
{
        GtkStyleContext *context;
        g_autoptr(GtkCssProvider) provider = NULL;

        g_debug ("using custom CSS %s", css);

        /* set the custom CSS class */
        context = gtk_widget_get_style_context (widget);
        gtk_style_context_add_class (context, class_name);

        /* set up custom provider and store on the widget */
        provider = gtk_css_provider_new ();
        g_signal_connect (provider, "parsing-error",
                          G_CALLBACK (gr_utils_widget_css_parsing_error_cb), NULL);
        gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                                   GTK_STYLE_PROVIDER (provider),
                                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        gtk_css_provider_load_from_data (provider, css, -1, NULL);
        g_object_set_data_full (G_OBJECT (widget),
                                "custom-css-provider",
                                g_object_ref (provider),
                                g_object_unref);
}

void
gr_utils_widget_set_css_simple (GtkWidget *widget,
                                const      gchar *css)
{
        g_autofree gchar *class_name = NULL;
        g_autoptr(GString) str = NULL;

        /* remove custom class if NULL */
        class_name = g_strdup_printf ("themed-widget_%p", widget);
        if (css == NULL) {
                GtkStyleContext *context = gtk_widget_get_style_context (widget);
                gtk_style_context_remove_class (context, class_name);
                return;
        }
        str = g_string_sized_new (1024);
        g_string_append_printf (str, ".%s {\n", class_name);
        g_string_append_printf (str, "%s\n", css);
        g_string_append (str, "}");

        gr_utils_widget_set_css_internal (widget, class_name, str->str);
}

char *
date_time_to_string (GDateTime *dt)
{
        return g_strdup_printf ("%d-%d-%d %d:%d:%d",
                                g_date_time_get_year (dt),
                                g_date_time_get_month (dt),
                                g_date_time_get_day_of_month (dt),
                                g_date_time_get_hour (dt),
                                g_date_time_get_minute (dt),
                                g_date_time_get_second (dt));
}

GDateTime *
date_time_from_string (const char *string)
{
        g_auto(GStrv) s = NULL;
        g_auto(GStrv) s1 = NULL;
        g_auto(GStrv) s2 = NULL;
        int year, month, day, hour, minute, second;
        char *endy, *endm, *endd, *endh, *endmi, *ends;

        s = g_strsplit (string, " ", -1);
        if (g_strv_length (s) != 2)
                return NULL;

        s1 = g_strsplit (s[0], "-", -1);
        if (g_strv_length (s1) != 3)
                return NULL;

        s2 = g_strsplit (s[1], ":", -1);
        if (g_strv_length (s1) != 3)
                return NULL;

        year = strtol (s1[0], &endy, 10);
        month = strtol (s1[1], &endm, 10);
        day = strtol (s1[2], &endd, 10);

        hour = strtol (s2[0], &endh, 10);
        minute = strtol (s2[1], &endmi, 10);
        second = strtol (s2[2], &ends, 10);

        if (!*endy && !*endm && !*endd &&
            !*endh && !*endmi && !*ends &&
            0 < month && month <= 12 &&
            0 < day && day <= 31 &&
            0 <= hour && hour < 24 &&
            0 <= minute && minute < 60 &&
            0 <= second && second < 60)
                    return g_date_time_new_utc (year, month, day,
                                                hour, minute, second);

        return NULL;
}

gboolean
skip_whitespace (char **input)
{
        char *in = *input;

        while (*input && g_ascii_isspace (**input)) {
                *input += 1;
        }

        return in != *input;
}

gboolean
space_or_nul (char p)
{
        return (p == '\0' || g_ascii_isspace (p));
}

/* this is for translating strings that have been extracted using recipe-extract */
char *
translate_multiline_string (const char *s)
{
        g_auto(GStrv) strv = NULL;
        int i;
        GString *out;

        out = g_string_new ("");

        strv = g_strsplit (s, "\n", -1);

        for (i = 0; strv[i]; i++) {
                if (i > 0)
                        g_string_append (out, "\n");
                if (strv[i][0] != 0)
                        g_string_append (out, _(strv[i]));
        }

        return g_string_free (out, FALSE);
}
