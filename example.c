#include "bestline.h"

#ifndef __COSMOPOLITAN__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#if 0
// should be ~50kb statically linked
// will save history to ~/.foo_history
// cc -fno-jump-tables -Os -o foo foo.c bestline.c
int main() {
    char *line;
    while ((line = bestlineWithHistory("IN> ", "foo"))) {
        fputs("OUT> ", stdout);
        fputs(line, stdout);
        fputs("\n", stdout);
        free(line);
    }
}
#endif

void completion(const char *buf, int pos, bestlineCompletions *lc) {
    (void) pos;
    if (buf[0] == 'h') {
        bestlineAddCompletion(lc, "hello");
        bestlineAddCompletion(lc, "hello there");
    }
}

char *hints(const char *buf, const char **ansi1, const char **ansi2) {
    if (!strcmp(buf, "hello")) {
        *ansi1 = "\033[35m"; /* magenta foreground */
        *ansi2 = "\033[39m"; /* reset foreground */
        return " World";
    }
    return NULL;
}

int main(int argc, char **argv) {
    char *line;

    /* Set the completion callback. This will be called every time the
     * user uses the <tab> key. */
    bestlineSetCompletionCallback(completion);
    bestlineSetHintsCallback(hints);

    /* Load history from file. The history file is just a plain text file
     * where entries are separated by newlines. */
    bestlineHistoryLoad("history.txt"); /* Load the history at startup */

    /* Now this is the main loop of the typical bestline-based application.
     * The call to bestline() will block as long as the user types something
     * and presses enter.
     *
     * The typed string is returned as a malloc() allocated string by
     * bestline, so the user needs to free() it. */

    while ((line = bestline("hello> ")) != NULL) {
        /* Do something with the string. */
        if (line[0] != '\0' && line[0] != '/') {
            fputs("echo: '", stdout);
            fputs(line, stdout);
            fputs("'\n", stdout);
            bestlineHistoryAdd(line); /* Add to the history. */
            bestlineHistorySave("history.txt"); /* Save the history on disk. */
        } else if (!strncmp(line, "/mask", 5)) {
            bestlineMaskModeEnable();
        } else if (!strncmp(line, "/unmask", 7)) {
            bestlineMaskModeDisable();
        } else if (!strncmp(line, "/balance", 8)) {
            bestlineBalanceMode(1);
        } else if (!strncmp(line, "/unbalance", 10)) {
            bestlineBalanceMode(0);
        } else if (line[0] == '/') {
            fputs("Unrecognized command: '", stdout);
            fputs(line, stdout);
            fputs("'\n", stdout);
        }
        free(line);
    }
    return 0;
}
