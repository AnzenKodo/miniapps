/********************************************************************************
 * `opml_feed_link`: It extracts link from opml file and prints it in output file.
 * Usage:
 *      opml_feed_link <opml_file> <output_file>
 * Compile:
 *      GCC: cc opml_feed_link.c -o opml_feed_link
 *      Clang: cl opml_feed_link.c -o opml_feed_link
 *      tcc: tcc opml_feed_link.c -o opml_feed_link
 *      MSVC: cl opml_feed_link.c -O:opml_feed_link
 * Run:
 *      ./opml_feed_link <opml-filename>
 * ------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright 2024 AnzenKodo
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 450

int main(int argc, char *argv[]) {
    FILE *output_file = fopen(argv[2], "r+");
    if (output_file == NULL) {
        perror("Error: couldn't opening output file.");
        return 1;
    }

    FILE *temp_file = tmpfile();
    if (temp_file == NULL) {
        perror("Error: couldn't opening temp file.");
        return 1;
    }

    char line[MAX_LINE_LENGTH];

    while (fgets(line, MAX_LINE_LENGTH, output_file)) {
        fprintf(temp_file, line);

        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, "## Podcast") == 0) {
            fprintf(temp_file, "\n");
            break;
        }
    }

    FILE *opml_file = fopen(argv[1], "r");
    if (opml_file == NULL) {
        perror("Error: couldn't opening opml file.");
        return 1;
    }

    printf("Written following line in %s:\n", argv[2]);
    while (fgets(line, MAX_LINE_LENGTH, opml_file) != NULL) {
        for (size_t i = 0; i < strlen(line); i++) {
            if (line[i] == 'x' && line[i+7] == '"' && line[i+8] == 'h') {
                putc('-', temp_file); putc(' ', temp_file);
                printf("- ");

                for (size_t j = i+8; line[j] != '"'; j++) {
                    putc(line[j], temp_file);
                    printf("%c", line[j]);
                }

                putc('\n', temp_file);
                printf("\n");
            }
        }
    }

    rewind(temp_file);
    rewind(output_file);
    while (fgets(line, MAX_LINE_LENGTH, temp_file)) {
        fprintf(output_file, line);
    }

    fclose(opml_file);
    fclose(output_file);
    fclose(temp_file);
    return 0;
}
