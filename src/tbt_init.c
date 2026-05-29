/*
 * tablatal - A command-line utility for creating and editing tablatal files.
 * Copyright (C) 2026 M. Peterson
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with 
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "tbt.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void free_specs(FieldSpec *specs, size_t count) {
    for (size_t i = 0; i < count; i++) {
        free(specs[i].name);
    }

    free(specs);
}

static int parse_size(const char *src, size_t *out) {
    if (!src || *src == '\0') {
        return TBT_ERR;
    }

    errno = 0;
    char *end = NULL;
    unsigned long value = strtoul(src, &end, 10);

    if (errno != 0 || *end != '\0' || value == 0) {
        return TBT_ERR;
    }

    *out = (size_t)value;
    return TBT_OK;
}

int parse_field_spec(const char *arg, int is_final, FieldSpec *out) {
    const char *colon = strchr(arg, ':');

    if (!colon) {
        if (!is_final) {
            fprintf(stderr, "error: non-final field requires width: %s\n", arg);
            return TBT_ERR;
        }

        out->name = strdup(arg);
        out->width = 0;
        out->unbounded = 1;

        if (!out->name) {
            return TBT_ERR;
        }

        return TBT_OK;
    }

    size_t name_len = (size_t)(colon-arg);
    if (name_len == 0) {
        fprintf(stderr, "error: empty field name in spec: %s\n", arg);
        return TBT_ERR;
    }

    char *name = tbt_strdup_range(arg, 0, name_len);
    if (!name) {
        return TBT_ERR;
    }

    size_t width = 0;
    if (parse_size(colon+1, &width) != TBT_OK) {
        fprintf(stderr, "error: invalid width in spec: %s\n", arg);
        free(name);
        return TBT_ERR;
    }

    if (width <= name_len) {
        fprintf(stderr,
            "error: width for field %s must be greater than field name length\n"
            "field ength: %zu\n"
            "width: %zu\n",
            name,
            name_len,
            width
        );
        free(name);
        return TBT_ERR;
    }
    out->name = name;
    out->width = width;
    out->unbounded = is_final ? 1 : 0;

    return TBT_OK;
}

int write_header(FILE *fp, const FieldSpec *specs, size_t count) {
    for (size_t i = 0; i < count; i++) {
        const FieldSpec *spec = &specs[i];

        if (spec->unbounded) {
            fputs(spec->name, fp);
            continue;
        }

        size_t name_len = strlen(spec->name);
        fputs(spec->name, fp);

        for (size_t pad = name_len; pad < spec->width; pad++) {
            fputc(' ', fp);
        }
    }

    fputc('\n', fp);

    if (ferror(fp)) {
        return TBT_ERR;
    }

    return TBT_OK;
}
