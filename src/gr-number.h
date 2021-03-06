/* gr-number.h
 *
 * Copyright (C) 2016 Matthias Clasen <mclasen@redhat.com>
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

#pragma once

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
        gboolean fraction;
        int num, denom;
        double value;
} GrNumber;

GrNumber *gr_number_new_fraction (int num, int denom);
GrNumber *gr_number_new_float    (double value);

void      gr_number_set_fraction (GrNumber *number, int num, int denom);
void      gr_number_set_float    (GrNumber *number, double value);
void      gr_number_add          (GrNumber *a1,
                                  GrNumber *a2,
                                  GrNumber *b);
void      gr_number_multiply     (GrNumber *a1,
                                  GrNumber *a2,
                                  GrNumber *b);

gboolean  gr_number_parse  (GrNumber  *number,
                            char     **input,
                            GError   **error);
char     *gr_number_format (GrNumber *number);



G_END_DECLS
