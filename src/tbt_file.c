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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int tbt_read_first_line(const char *path, char **out) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror(path);
        return TBT_ERR;
    }

    char *line = NULL;
    size_t cap = 0;
    /* getline is POSIX linux only */
    ssize_t len = getline(&line, &cap, fp);

    fclose(fp);

    if (len < 0) {
        free(line);
        fprintf(stderr, "error: failed to read header from: %s\n", path);
        return TBT_ERR;
    }

    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
        line[len-1] = '\0';
        len--;
    }

    *out = line;
    return TBT_OK;
}

int tbt_print_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror(path);
        return TBT_ERR;
    }

    char buf[4096];

    while (fgets(buf, sizeof(buf), fp)) {
        fputs(buf, stdout);
    }

    if (ferror(fp)) {
        perror(path);
        fclose(fp);
        return TBT_ERR;
    }

    fclose(fp);
    return TBT_OK;
}

int tbt_print_file_numbered(const char *path) {
    TbtLines lines;

    if (tbt_read_all_lines(path, &lines) != TBT_OK) {
        return TBT_ERR;
    }

    if (lines.count == 0) {
        fprintf(stderr, "error: empty table: %s\n", path);
        tbt_free_lines(&lines);
        return TBT_ERR;
    }

    printf("%-4s %s\n", "ROW", lines.items[0]);

    for (size_t i = 1; i < lines.count; i++) {
        printf("%-4zu %s\n", i, lines.items[i]);
    }

    tbt_free_lines(&lines);
    return TBT_OK;
}

int tbt_append_line(const char *path, const char *line){
    FILE *fp = fopen(path, "a");
    if (!fp) {
        perror(path);
        return TBT_ERR;
    }

    if (fprintf(fp, "%s\n", line) < 0) {
        perror(path);
        fclose(fp);
        return TBT_ERR;
    }

    if (fclose(fp) != 0) {
        perror(path);
        return TBT_ERR;
    }

    return TBT_OK;
}

static int push_line(TbtLines *lines, char *line) {
    char **next = realloc(lines->items, sizeof(char *) * (lines->count + 1));
    if (!next) {
        return TBT_ERR;
    }

    lines->items = next;
    lines->items[lines->count] = line;
    lines->count++;

    return TBT_OK;
}

int tbt_read_all_lines(const char *path, TbtLines *lines) {
    lines->items = NULL;
    lines->count = 0;

    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror(path);
        return TBT_ERR;
    }

    char *line = NULL;
    size_t cap = 0;
    ssize_t len = 0;

    while ((len = getline(&line, &cap, fp)) >= 0) {
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[len-1] = '\0';
            len--;
        }

        char *copy = strdup(line);
        if (!copy) {
            free(line);
            fclose(fp);
            tbt_free_lines(lines);
            return TBT_ERR;
        }

        if (push_line(lines, copy) != TBT_OK) {
            free(copy);
            free(line);
            fclose(fp);
            tbt_free_lines(lines);
            return TBT_ERR;    
        }
    }

    free(line);

    if (ferror(fp)) {
        perror(path);
        fclose(fp);
        tbt_free_lines(lines);
        return TBT_ERR;
    }

    fclose(fp);
    return TBT_OK;
}

void tbt_free_lines(TbtLines *lines) {
    if (!lines) {
        return;
    }

    for (size_t i = 0; i < lines->count; i++) {
        free(lines->items[i]);
    }

    free(lines->items);
    lines->items = NULL;
    lines->count = 0;
}

char *tbt_strdup_range(const char *src, size_t start, size_t len) {
    char *out = malloc(len+1);
    if (!out) {
        return NULL;
    }

    memcpy(out, src+start, len);
    out[len] = '\0';
    return out;
}

char *tbt_trimmed_dup(const char *src, size_t len) {
    size_t start = 0;
    size_t end = len;

    while (start < end && (src[start] == ' ' || src[start] == '\t')) {
        start++;
    }

    while (end > start && (src[end-1] == ' ' || src[end-1] == '\t')) {
        end--;
    }

    return tbt_strdup_range(src, start, end-start);
}

static char *make_tmp_path(const char *path) {
    const char *suffix = ".tmp";

    size_t path_len = strlen(path);
    size_t suffix_len = strlen(suffix);

    char *tmp = malloc(path_len + suffix_len + 1);
    if (!tmp) {
        return NULL;
    }

    memcpy(tmp, path, path_len);
    memcpy(tmp + path_len, suffix, suffix_len);
    tmp[path_len + suffix_len] = '\0';

    return tmp;
}

int tbt_write_all_lines_atomic(const char *path, const TbtLines *lines) {
    char *tmp_path = make_tmp_path(path);
    if (!tmp_path) {
        return TBT_ERR;
    }

    FILE *fp = fopen(tmp_path, "w");
    if (!fp) {
        perror(tmp_path);
        free(tmp_path);
        return TBT_ERR;
    }

    for (size_t i = 0; i < lines->count; i++) {
        if (fprintf(fp, "%s\n", lines->items[i]) < 0) {
            perror(tmp_path);
            fclose(fp);
            remove(tmp_path);
            free(tmp_path);
            return TBT_ERR;
        }
    }

    if (fflush(fp) != 0) {
        perror(tmp_path);
        fclose(fp);
        remove(tmp_path);
        free(tmp_path);
        return TBT_ERR;
    }

    int fd = fileno(fp);
    if (fd >= 0 && fsync(fd) != 0) {
        perror(tmp_path);
        fclose(fp);
        remove(tmp_path);
        free(tmp_path);
        return TBT_ERR;
    }

    if (fclose(fp) != 0) {
        perror(tmp_path);
        remove(tmp_path);
        free(tmp_path);
        return TBT_ERR;
    }

    if (rename(tmp_path, path) != 0) {
        perror(path);
        remove(tmp_path);
        free(tmp_path);
        return TBT_ERR;
    }

    free(tmp_path);
    return TBT_OK;
}
