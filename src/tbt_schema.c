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

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t start;
    size_t end;
} Span;

static int push_field(TbtSchema *schema, TbtField field) {
    size_t new_count = schema->count+1;
    if (new_count == 0) {
        return TBT_ERR;
    }

    size_t bytes = 0;
    if (checked_array_size(new_count, sizeof(*schema->fields), &bytes) != TBT_OK) {
        return TBT_ERR;
    }

    TbtField *next = realloc(schema->fields, bytes);
    if (!next) {
        return TBT_ERR;
    }

    schema->fields = next;
    schema->fields[schema->count] = field;
    schema->count++;

    return TBT_OK;
}

int tbt_parse_schema_line(const char *line, TbtSchema *schema) {
    schema->fields = NULL;
    schema->count = 0;

    size_t len = strlen(line);
    Span *spans = NULL;
    size_t span_count = 0;

    size_t i = 0;
    while (i < len) {
        while (i < len && isspace((unsigned char)line[i])) {
            i++;
        }

        if (i >= len) {
            break;
        }

        size_t start = i;

        while (i < len && !isspace((unsigned char)line[i])) {
            i++;
        }

        size_t end = i;

        Span *next = realloc(spans, sizeof(Span)*(span_count+1));
        if (!next) {
            free(spans);
            return TBT_ERR;
        }

        spans = next;
        spans[span_count].start = start;
        spans[span_count].end = end;
        span_count++;
    }

    if (span_count == 0) {
        free(spans);
        fprintf(stderr, "error: empty schema header\n");
        return TBT_ERR;
    }

    for (size_t n = 0; n < span_count; n++) {
        size_t name_len = spans[n].end - spans[n].start;

        char *name = tbt_strdup_range(line, spans[n].start, name_len);
        if (!name) {
            free(spans);
            tbt_free_schema(schema);
            return TBT_ERR;
        }

        TbtField field;
        field.name = name;
        field.offset = spans[n].start;

        if (n + 1 < span_count) {
            field.width = spans[n+1].start - spans[n].start;
            field.unbounded = 0;
        } else {
            field.width = 0;
            field.unbounded = 1;
        }

        if (push_field(schema, field) != TBT_OK) {
            free(name);
            free(spans);
            tbt_free_schema(schema);
            return TBT_ERR;
        }
    }

    free(spans);
    return TBT_OK;
}

void tbt_free_schema(TbtSchema *schema) {
    if (!schema) {
        return;
    }

    for (size_t i = 0; i < schema->count; i++) {
        free(schema->fields[i].name);
    }

    free(schema->fields);
    schema->fields = NULL;
    schema->count = 0;
}

void tbt_print_schema(const TbtSchema *schema) {
    printf("%-16s %-8s %-12s\n", "FIELD", "OFFSET", "WIDTH");

    for (size_t i =0; i < schema->count; i++) {
        const TbtField *field = &schema->fields[i];

        if (field->unbounded) {
            printf("%-16s %-8zu %-12s\n", field->name, field->offset, "unbounded");
        } else {
            printf("%-16s %-8zu %-12zu\n", field->name, field->offset, field->width);
        }
    }
}

const TbtField *tbt_find_field(const TbtSchema *schema, const char *name) {
    for (size_t i = 0; i < schema->count; i++) {
        if (strcmp(schema->fields[i].name, name) == 0) {
            return &schema->fields[i];
        }
    }
    return NULL;
}
