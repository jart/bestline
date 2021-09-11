/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:4;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=4 sts=4 sw=4 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│                                                                              │
│ Bestline ── Library for interactive pseudoteletypewriter command             │
│             sessions using ANSI Standard X3.64 control sequences             │
│                                                                              │
│ OVERVIEW                                                                     │
│                                                                              │
│   Bestline is a fork of linenoise (a popular readline alternative)           │
│   that fixes its bugs and adds the missing features while reducing           │
│   binary footprint (surprisingly) by removing bloated dependencies           │
│   which means you can finally have a permissively-licensed command           │
│   prompt w/ a 30kb footprint that's nearly as good as gnu readline           │
│                                                                              │
│ EXAMPLE                                                                      │
│                                                                              │
│   main() {                                                                   │
│       char *line;                                                            │
│       while ((line = ezbestline("IN> ", "foo"))) {                           │
│           fputs("OUT> ", stdout);                                            │
│           fputs(line, stdout);                                               │
│           fputs("\n", stdout);                                               │
│           free(line);                                                        │
│       }                                                                      │
│   }                                                                          │
│                                                                              │
│ CHANGES                                                                      │
│                                                                              │
│   - Remove bell                                                              │
│   - Add kill ring                                                            │
│   - Add UTF-8 editing                                                        │
│   - Add CTRL-R search                                                        │
│   - React to terminal resizing                                               │
│   - Don't generate .data section                                             │
│   - Support terminal flow control                                            │
│   - Make history loading 10x faster                                          │
│   - Make multiline mode the only mode                                        │
│   - Accommodate O_NONBLOCK file descriptors                                  │
│   - Restore raw mode on process foregrounding                                │
│   - Make source code compatible with C++ compilers                           │
│   - Fix corruption issues by using generalized parsing                       │
│   - Implement nearly all GNU readline editing shortcuts                      │
│   - Remove heavyweight dependencies like printf/sprintf                      │
│   - Remove ISIG→^C→EAGAIN hack and catch signals properly                    │
│   - Support running on Windows in MinTTY or CMD.EXE on Win10+                │
│   - Support diacratics, русский, Ελληνικά, 中国人, 한국인, 日本              │
│                                                                              │
│ SHORTCUTS                                                                    │
│                                                                              │
│   CTRL-E         END                                                         │
│   CTRL-A         START                                                       │
│   CTRL-B         BACK                                                        │
│   CTRL-F         FORWARD                                                     │
│   CTRL-L         CLEAR                                                       │
│   CTRL-H         BACKSPACE                                                   │
│   CTRL-D         DELETE                                                      │
│   CTRL-D         EOF (IF EMPTY)                                              │
│   CTRL-N         NEXT HISTORY                                                │
│   CTRL-P         PREVIOUS HISTORY                                            │
│   CTRL-R         SEARCH HISTORY                                              │
│   CTRL-G         CANCEL SEARCH                                               │
│   ALT-<          BEGINNING OF HISTORY                                        │
│   ALT->          END OF HISTORY                                              │
│   ALT-F          FORWARD WORD                                                │
│   ALT-B          BACKWARD WORD                                               │
│   CTRL-K         KILL LINE FORWARDS                                          │
│   CTRL-U         KILL LINE BACKWARDS                                         │
│   ALT-H          KILL WORD BACKWARDS                                         │
│   CTRL-W         KILL WORD BACKWARDS                                         │
│   CTRL-ALT-H     KILL WORD BACKWARDS                                         │
│   ALT-D          KILL WORD FORWARDS                                          │
│   CTRL-Y         YANK                                                        │
│   ALT-Y          ROTATE KILL RING AND YANK AGAIN                             │
│   CTRL-T         TRANSPOSE                                                   │
│   ALT-T          TRANSPOSE WORD                                              │
│   ALT-U          UPPERCASE WORD                                              │
│   ALT-L          LOWERCASE WORD                                              │
│   ALT-C          CAPITALIZE WORD                                             │
│   CTRL-SPACE     SET MARK                                                    │
│   CTRL-X CTRL-X  GOTO MARK                                                   │
│   CTRL-S         PAUSE OUTPUT                                                │
│   CTRL-Q         RESUME OUTPUT                                               │
│   CTRL-Z         SUSPEND PROCESS                                             │
│                                                                              │
╞══════════════════════════════════════════════════════════════════════════════╡
│                                                                              │
│ Copyright 2018-2021 Justine Tunney <jtunney@gmail.com>                       │
│ Copyright 2010-2016 Salvatore Sanfilippo <antirez@gmail.com>                 │
│ Copyright 2010-2013 Pieter Noordhuis <pcnoordhuis@gmail.com>                 │
│                                                                              │
│ All rights reserved.                                                         │
│                                                                              │
│ Redistribution and use in source and binary forms, with or without           │
│ modification, are permitted provided that the following conditions are       │
│ met:                                                                         │
│                                                                              │
│  *  Redistributions of source code must retain the above copyright           │
│     notice, this list of conditions and the following disclaimer.            │
│                                                                              │
│  *  Redistributions in binary form must reproduce the above copyright        │
│     notice, this list of conditions and the following disclaimer in the      │
│     documentation and/or other materials provided with the distribution.     │
│                                                                              │
│ THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS          │
│ "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT            │
│ LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR        │
│ A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT         │
│ HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,       │
│ SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT             │
│ LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,        │
│ DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY        │
│ THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          │
│ (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE        │
│ OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.         │
│                                                                              │
╚─────────────────────────────────────────────────────────────────────────────*/
#include "bestline.h"

#ifndef __COSMOPOLITAN__
#define _POSIX_C_SOURCE
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <setjmp.h>
#include <poll.h>
#include <assert.h>
#include <signal.h>
#include <fcntl.h>
#include <limits.h>
#endif

__asm__(".ident\t\"\\n\\n\
Bestline (BSD-2)\\n\
Copyright 2018-2020 Justine Tunney <jtunney@gmail.com>\\n\
Copyright 2010-2016 Salvatore Sanfilippo <antirez@gmail.com>\\n\
Copyright 2010-2013 Pieter Noordhuis <pcnoordhuis@gmail.com>\"");

#define BESTLINE_MAX_RING    8
#define BESTLINE_MAX_HISTORY 1024
#define BESTLINE_MAX_LINE    4096

#define BESTLINE_HISTORY_FIRST +BESTLINE_MAX_HISTORY
#define BESTLINE_HISTORY_PREV  +1
#define BESTLINE_HISTORY_NEXT  -1
#define BESTLINE_HISTORY_LAST  -BESTLINE_MAX_HISTORY

#define Ctrl(C)    ((C) ^ 0100)
#define Min(X, Y)  ((Y) > (X) ? (X) : (Y))
#define Max(X, Y)  ((Y) < (X) ? (X) : (Y))
#define Case(X, Y) case X: Y; break
#define Read32le(X)                  \
  ((unsigned)(255 & (X)[0]) << 000 | \
   (unsigned)(255 & (X)[1]) << 010 | \
   (unsigned)(255 & (X)[2]) << 020 | \
   (unsigned)(255 & (X)[3]) << 030)

struct abuf {
    char *b;
    int len;
};

struct rune {
    unsigned c;
    unsigned n;
};

struct bestlineRing {
    unsigned i;
    char *p[BESTLINE_MAX_RING];
};

/* The bestlineState structure represents the state during line editing.
 * We pass this state to functions implementing specific editing
 * functionalities. */
struct bestlineState {
    int ifd;            /* Terminal stdin file descriptor */
    int ofd;            /* Terminal stdout file descriptor */
    struct winsize ws;  /* Rows and columns in terminal */
    char *buf;          /* Edited line buffer */
    const char *prompt; /* Prompt to display */
    int hindex;         /* history index */
    unsigned buflen;    /* Edited line buffer size */
    unsigned pos;       /* Current cursor position */
    unsigned oldpos;    /* Previous refresh cursor position */
    unsigned len;       /* Current edited line length */
    unsigned maxrows;   /* Maximum num of rows used so far */
    unsigned mark;      /* Saved cursor position */
    unsigned yi, yj;    /* Boundaries of last yank */
    char seq[2][16];    /* Keystroke history for yanking code */
};

static const char *const kUnsupported[] = {"dumb","cons25","emacs"};

static jmp_buf jraw;
static char rawmode;
static char gotcont;
static char gotwinch;
static char maskmode;
static char iscapital;
static int historylen;
static struct bestlineRing ring;
static struct sigaction orig_int;
static struct sigaction orig_quit;
static struct sigaction orig_cont;
static struct sigaction orig_winch;
static struct termios orig_termios;
static char *history[BESTLINE_MAX_HISTORY];
static bestlineHintsCallback *hintsCallback;
static bestlineFreeHintsCallback *freeHintsCallback;
static bestlineCompletionCallback *completionCallback;

static void bestlineAtExit(void);
static void bestlineRefreshLine(struct bestlineState *);

static int IsControl(int c) {
    return ((0x00 <= c && c <= 0x1F) ||
            (0x7F <= c && c <= 0x9F));
}

static int GetMonospaceCharacterWidth(int c) {
    return !IsControl(c)
            + (c >= 0x1100 &&
               (c <= 0x115f || c == 0x2329 || c == 0x232a ||
                (c >= 0x2e80 && c <= 0xa4cf && c != 0x303f) ||
                (c >= 0xac00 && c <= 0xd7a3) ||
                (c >= 0xf900 && c <= 0xfaff) ||
                (c >= 0xfe10 && c <= 0xfe19) ||
                (c >= 0xfe30 && c <= 0xfe6f) ||
                (c >= 0xff00 && c <= 0xff60) ||
                (c >= 0xffe0 && c <= 0xffe6) ||
                (c >= 0x20000 && c <= 0x2fffd) ||
                (c >= 0x30000 && c <= 0x3fffd)));
}

/**
 * Returns nonzero if 𝑐 isn't alphanumeric.
 *
 * Line reading interfaces generally define this operation as UNICODE
 * characters that aren't in the letter category (Lu, Ll, Lt, Lm, Lo)
 * and aren't in the number categorie (Nd, Nl, No). We also add a few
 * other things like blocks and emoji (So).
 */
static int IsSeparator(int c) {
    int m, l, r;
    if (c < 0200) {
        return !(('0' <= c && c <= '9') ||
                 ('A' <= c && c <= 'Z') ||
                 ('a' <= c && c <= 'z'));
    }
    if (c <= 0xffff) {
        static const unsigned short kGlyphs[][2] = {
            {0x00aa, 0x00aa}, /*     1x English */
            {0x00b2, 0x00b3}, /*     2x English Arabic */
            {0x00b5, 0x00b5}, /*     1x Greek */
            {0x00b9, 0x00ba}, /*     2x English Arabic */
            {0x00bc, 0x00be}, /*     3x Vulgar English Arabic */
            {0x00c0, 0x00d6}, /*    23x Watin */
            {0x00d8, 0x00f6}, /*    31x Watin */
            {0x0100, 0x02c1}, /*   450x Watin-AB,IPA,Spacemod */
            {0x02c6, 0x02d1}, /*    12x Spacemod */
            {0x02e0, 0x02e4}, /*     5x Spacemod */
            {0x02ec, 0x02ec}, /*     1x Spacemod */
            {0x02ee, 0x02ee}, /*     1x Spacemod */
            {0x0370, 0x0374}, /*     5x Greek */
            {0x0376, 0x0377}, /*     2x Greek */
            {0x037a, 0x037d}, /*     4x Greek */
            {0x037f, 0x037f}, /*     1x Greek */
            {0x0386, 0x0386}, /*     1x Greek */
            {0x0388, 0x038a}, /*     3x Greek */
            {0x038c, 0x038c}, /*     1x Greek */
            {0x038e, 0x03a1}, /*    20x Greek */
            {0x03a3, 0x03f5}, /*    83x Greek */
            {0x03f7, 0x0481}, /*   139x Greek */
            {0x048a, 0x052f}, /*   166x Cyrillic */
            {0x0531, 0x0556}, /*    38x Armenian */
            {0x0560, 0x0588}, /*    41x Armenian */
            {0x05d0, 0x05ea}, /*    27x Hebrew */
            {0x0620, 0x064a}, /*    43x Arabic */
            {0x0660, 0x0669}, /*    10x Arabic */
            {0x0671, 0x06d3}, /*    99x Arabic */
            {0x06ee, 0x06fc}, /*    15x Arabic */
            {0x0712, 0x072f}, /*    30x Syriac */
            {0x074d, 0x07a5}, /*    89x Syriac,Arabic2,Thaana */
            {0x07c0, 0x07ea}, /*    43x NKo */
            {0x0800, 0x0815}, /*    22x Samaritan */
            {0x0840, 0x0858}, /*    25x Mandaic */
            {0x0904, 0x0939}, /*    54x Devanagari */
            {0x0993, 0x09a8}, /*    22x Bengali */
            {0x09e6, 0x09f1}, /*    12x Bengali */
            {0x0a13, 0x0a28}, /*    22x Gurmukhi */
            {0x0a66, 0x0a6f}, /*    10x Gurmukhi */
            {0x0a93, 0x0aa8}, /*    22x Gujarati */
            {0x0b13, 0x0b28}, /*    22x Oriya */
            {0x0c92, 0x0ca8}, /*    23x Kannada */
            {0x0caa, 0x0cb3}, /*    10x Kannada */
            {0x0ce6, 0x0cef}, /*    10x Kannada */
            {0x0d12, 0x0d3a}, /*    41x Malayalam */
            {0x0d85, 0x0d96}, /*    18x Sinhala */
            {0x0d9a, 0x0db1}, /*    24x Sinhala */
            {0x0de6, 0x0def}, /*    10x Sinhala */
            {0x0e01, 0x0e30}, /*    48x Thai */
            {0x0e8c, 0x0ea3}, /*    24x Lao */
            {0x0f20, 0x0f33}, /*    20x Tibetan */
            {0x0f49, 0x0f6c}, /*    36x Tibetan */
            {0x109e, 0x10c5}, /*    40x Myanmar,Georgian */
            {0x10d0, 0x10fa}, /*    43x Georgian */
            {0x10fc, 0x1248}, /*   333x Georgian,Hangul,Ethiopic */
            {0x13a0, 0x13f5}, /*    86x Cherokee */
            {0x1401, 0x166d}, /*   621x Aboriginal */
            {0x16a0, 0x16ea}, /*    75x Runic */
            {0x1700, 0x170c}, /*    13x Tagalog */
            {0x1780, 0x17b3}, /*    52x Khmer */
            {0x1820, 0x1878}, /*    89x Mongolian */
            {0x1a00, 0x1a16}, /*    23x Buginese */
            {0x1a20, 0x1a54}, /*    53x Tai Tham */
            {0x1a80, 0x1a89}, /*    10x Tai Tham */
            {0x1a90, 0x1a99}, /*    10x Tai Tham */
            {0x1b05, 0x1b33}, /*    47x Balinese */
            {0x1b50, 0x1b59}, /*    10x Balinese */
            {0x1b83, 0x1ba0}, /*    30x Sundanese */
            {0x1bae, 0x1be5}, /*    56x Sundanese */
            {0x1c90, 0x1cba}, /*    43x Georgian2 */
            {0x1cbd, 0x1cbf}, /*     3x Georgian2 */
            {0x1e00, 0x1f15}, /*   278x Watin-C,Greek2 */
            {0x2070, 0x2071}, /*     2x Supersub */
            {0x2074, 0x2079}, /*     6x Supersub */
            {0x207f, 0x2089}, /*    11x Supersub */
            {0x2090, 0x209c}, /*    13x Supersub */
            {0x2100, 0x2117}, /*    24x Letterlike */
            {0x2119, 0x213f}, /*    39x Letterlike */
            {0x2145, 0x214a}, /*     6x Letterlike */
            {0x214c, 0x218b}, /*    64x Letterlike,Numbery */
            {0x21af, 0x21cd}, /*    31x Arrows */
            {0x21d5, 0x21f3}, /*    31x Arrows */
            {0x230c, 0x231f}, /*    20x Technical */
            {0x232b, 0x237b}, /*    81x Technical */
            {0x237d, 0x239a}, /*    30x Technical */
            {0x23b4, 0x23db}, /*    40x Technical */
            {0x23e2, 0x2426}, /*    69x Technical,ControlPictures */
            {0x2460, 0x25b6}, /*   343x Enclosed,Boxes,Blocks,Shapes */
            {0x25c2, 0x25f7}, /*    54x Shapes */
            {0x2600, 0x266e}, /*   111x Symbols */
            {0x2670, 0x2767}, /*   248x Symbols,Dingbats */
            {0x2776, 0x27bf}, /*    74x Dingbats */
            {0x2800, 0x28ff}, /*   256x Braille */
            {0x2c00, 0x2c2e}, /*    47x Glagolitic */
            {0x2c30, 0x2c5e}, /*    47x Glagolitic */
            {0x2c60, 0x2ce4}, /*   133x Watin-D */
            {0x2d00, 0x2d25}, /*    38x Georgian2 */
            {0x2d30, 0x2d67}, /*    56x Tifinagh */
            {0x2d80, 0x2d96}, /*    23x Ethiopic2 */
            {0x2e2f, 0x2e2f}, /*     1x Punctuation2 */
            {0x3005, 0x3007}, /*     3x CJK Symbols & Punctuation */
            {0x3021, 0x3029}, /*     9x CJK Symbols & Punctuation */
            {0x3031, 0x3035}, /*     5x CJK Symbols & Punctuation */
            {0x3038, 0x303c}, /*     5x CJK Symbols & Punctuation */
            {0x3041, 0x3096}, /*    86x Hiragana */
            {0x30a1, 0x30fa}, /*    90x Katakana */
            {0x3105, 0x312f}, /*    43x Bopomofo */
            {0x3131, 0x318e}, /*    94x Hangul Compatibility Jamo */
            {0x31a0, 0x31ba}, /*    27x Bopomofo Extended */
            {0x31f0, 0x31ff}, /*    16x Katakana Phonetic Extensions */
            {0x3220, 0x3229}, /*    10x Enclosed CJK Letters & Months */
            {0x3248, 0x324f}, /*     8x Enclosed CJK Letters & Months */
            {0x3251, 0x325f}, /*    15x Enclosed CJK Letters & Months */
            {0x3280, 0x3289}, /*    10x Enclosed CJK Letters & Months */
            {0x32b1, 0x32bf}, /*    15x Enclosed CJK Letters & Months */
            {0x3400, 0x4db5}, /*  6582x CJK Unified Ideographs Extension A */
            {0x4dc0, 0x9fef}, /* 21040x Yijing Hexagram, CJK Unified Ideographs */
            {0xa000, 0xa48c}, /*  1165x Yi Syllables */
            {0xa4d0, 0xa4fd}, /*    46x Lisu */
            {0xa500, 0xa60c}, /*   269x Vai */
            {0xa610, 0xa62b}, /*    28x Vai */
            {0xa6a0, 0xa6ef}, /*    80x Bamum */
            {0xa80c, 0xa822}, /*    23x Syloti Nagri */
            {0xa840, 0xa873}, /*    52x Phags-pa */
            {0xa882, 0xa8b3}, /*    50x Saurashtra */
            {0xa8d0, 0xa8d9}, /*    10x Saurashtra */
            {0xa900, 0xa925}, /*    38x Kayah Li */
            {0xa930, 0xa946}, /*    23x Rejang */
            {0xa960, 0xa97c}, /*    29x Hangul Jamo Extended-A */
            {0xa984, 0xa9b2}, /*    47x Javanese */
            {0xa9cf, 0xa9d9}, /*    11x Javanese */
            {0xaa00, 0xaa28}, /*    41x Cham */
            {0xaa50, 0xaa59}, /*    10x Cham */
            {0xabf0, 0xabf9}, /*    10x Meetei Mayek */
            {0xac00, 0xd7a3}, /* 11172x Hangul Syllables */
            {0xf900, 0xfa6d}, /*   366x CJK Compatibility Ideographs */
            {0xfa70, 0xfad9}, /*   106x CJK Compatibility Ideographs */
            {0xfb1f, 0xfb28}, /*    10x Alphabetic Presentation Forms */
            {0xfb2a, 0xfb36}, /*    13x Alphabetic Presentation Forms */
            {0xfb46, 0xfbb1}, /*   108x Alphabetic Presentation Forms */
            {0xfbd3, 0xfd3d}, /*   363x Arabic Presentation Forms-A */
            {0xfe76, 0xfefc}, /*   135x Arabic Presentation Forms-B */
            {0xff10, 0xff19}, /*    10x Dubs */
            {0xff21, 0xff3a}, /*    26x Dubs */
            {0xff41, 0xff5a}, /*    26x Dubs */
            {0xff66, 0xffbe}, /*    89x Dubs */
            {0xffc2, 0xffc7}, /*     6x Dubs */
            {0xffca, 0xffcf}, /*     6x Dubs */
            {0xffd2, 0xffd7}, /*     6x Dubs */
            {0xffda, 0xffdc}, /*     3x Dubs */
        };
        l = 0;
        r = sizeof(kGlyphs) / sizeof(kGlyphs[0]);
        while (l < r) {
            m = (l + r) >> 1;
            if (kGlyphs[m][1] < c) {
                l = m + 1;
            } else {
                r = m;
            }
        }
        return !(kGlyphs[l][0] <= c && c <= kGlyphs[l][1]);
    } else {
        static const unsigned kAstralGlyphs[][2] = {
            {0x10107, 0x10133}, /*    45x Aegean */
            {0x10140, 0x10178}, /*    57x Ancient Greek Numbers */
            {0x1018a, 0x1018b}, /*     2x Ancient Greek Numbers */
            {0x10280, 0x1029c}, /*    29x Lycian */
            {0x102a0, 0x102d0}, /*    49x Carian */
            {0x102e1, 0x102fb}, /*    27x Coptic Epact Numbers */
            {0x10300, 0x10323}, /*    36x Old Italic */
            {0x1032d, 0x1034a}, /*    30x Old Italic, Gothic */
            {0x10350, 0x10375}, /*    38x Old Permic */
            {0x10380, 0x1039d}, /*    30x Ugaritic */
            {0x103a0, 0x103c3}, /*    36x Old Persian */
            {0x103c8, 0x103cf}, /*     8x Old Persian */
            {0x103d1, 0x103d5}, /*     5x Old Persian */
            {0x10400, 0x1049d}, /*    158x Deseret, Shavian, Osmanya */
            {0x104b0, 0x104d3}, /*    36x Osage */
            {0x104d8, 0x104fb}, /*    36x Osage */
            {0x10500, 0x10527}, /*    40x Elbasan */
            {0x10530, 0x10563}, /*    52x Caucasian Albanian */
            {0x10600, 0x10736}, /*   311x Linear A */
            {0x10800, 0x10805}, /*     6x Cypriot Syllabary */
            {0x1080a, 0x10835}, /*    44x Cypriot Syllabary */
            {0x10837, 0x10838}, /*     2x Cypriot Syllabary */
            {0x1083f, 0x1089e}, /*    86x Cypriot,ImperialAramaic,Palmyrene,Nabataean */
            {0x108e0, 0x108f2}, /*    19x Hatran */
            {0x108f4, 0x108f5}, /*     2x Hatran */
            {0x108fb, 0x1091b}, /*    33x Hatran */
            {0x10920, 0x10939}, /*    26x Lydian */
            {0x10980, 0x109b7}, /*    56x Meroitic Hieroglyphs */
            {0x109bc, 0x109cf}, /*    20x Meroitic Cursive */
            {0x109d2, 0x10a00}, /*    47x Meroitic Cursive */
            {0x10a10, 0x10a13}, /*     4x Kharoshthi */
            {0x10a15, 0x10a17}, /*     3x Kharoshthi */
            {0x10a19, 0x10a35}, /*    29x Kharoshthi */
            {0x10a40, 0x10a48}, /*     9x Kharoshthi */
            {0x10a60, 0x10a7e}, /*    31x Old South Arabian */
            {0x10a80, 0x10a9f}, /*    32x Old North Arabian */
            {0x10ac0, 0x10ac7}, /*     8x Manichaean */
            {0x10ac9, 0x10ae4}, /*    28x Manichaean */
            {0x10aeb, 0x10aef}, /*     5x Manichaean */
            {0x10b00, 0x10b35}, /*    54x Avestan */
            {0x10b40, 0x10b55}, /*    22x Inscriptional Parthian */
            {0x10b58, 0x10b72}, /*    27x Inscriptional Parthian and Pahlavi */
            {0x10b78, 0x10b91}, /*    26x Inscriptional Pahlavi, Psalter Pahlavi */
            {0x10c00, 0x10c48}, /*    73x Old Turkic */
            {0x10c80, 0x10cb2}, /*    51x Old Hungarian */
            {0x10cc0, 0x10cf2}, /*    51x Old Hungarian */
            {0x10cfa, 0x10d23}, /*    42x Old Hungarian, Hanifi Rohingya */
            {0x10d30, 0x10d39}, /*    10x Hanifi Rohingya */
            {0x10e60, 0x10e7e}, /*    31x Rumi Numeral Symbols */
            {0x10f00, 0x10f27}, /*    40x Old Sogdian */
            {0x10f30, 0x10f45}, /*    22x Sogdian */
            {0x10f51, 0x10f54}, /*     4x Sogdian */
            {0x10fe0, 0x10ff6}, /*    23x Elymaic */
            {0x11003, 0x11037}, /*    53x Brahmi */
            {0x11052, 0x1106f}, /*    30x Brahmi */
            {0x11083, 0x110af}, /*    45x Kaithi */
            {0x110d0, 0x110e8}, /*    25x Sora Sompeng */
            {0x110f0, 0x110f9}, /*    10x Sora Sompeng */
            {0x11103, 0x11126}, /*    36x Chakma */
            {0x11136, 0x1113f}, /*    10x Chakma */
            {0x11144, 0x11144}, /*     1x Chakma */
            {0x11150, 0x11172}, /*    35x Mahajani */
            {0x11176, 0x11176}, /*     1x Mahajani */
            {0x11183, 0x111b2}, /*    48x Sharada */
            {0x111c1, 0x111c4}, /*     4x Sharada */
            {0x111d0, 0x111da}, /*    11x Sharada */
            {0x111dc, 0x111dc}, /*     1x Sharada */
            {0x111e1, 0x111f4}, /*    20x Sinhala Archaic Numbers */
            {0x11200, 0x11211}, /*    18x Khojki */
            {0x11213, 0x1122b}, /*    25x Khojki */
            {0x11280, 0x11286}, /*     7x Multani */
            {0x11288, 0x11288}, /*     1x Multani */
            {0x1128a, 0x1128d}, /*     4x Multani */
            {0x1128f, 0x1129d}, /*    15x Multani */
            {0x1129f, 0x112a8}, /*    10x Multani */
            {0x112b0, 0x112de}, /*    47x Khudawadi */
            {0x112f0, 0x112f9}, /*    10x Khudawadi */
            {0x11305, 0x1130c}, /*     8x Grantha */
            {0x1130f, 0x11310}, /*     2x Grantha */
            {0x11313, 0x11328}, /*    22x Grantha */
            {0x1132a, 0x11330}, /*     7x Grantha */
            {0x11332, 0x11333}, /*     2x Grantha */
            {0x11335, 0x11339}, /*     5x Grantha */
            {0x1133d, 0x1133d}, /*     1x Grantha */
            {0x11350, 0x11350}, /*     1x Grantha */
            {0x1135d, 0x11361}, /*     5x Grantha */
            {0x11400, 0x11434}, /*    53x Newa */
            {0x11447, 0x1144a}, /*     4x Newa */
            {0x11450, 0x11459}, /*    10x Newa */
            {0x1145f, 0x1145f}, /*     1x Newa */
            {0x11480, 0x114af}, /*    48x Tirhuta */
            {0x114c4, 0x114c5}, /*     2x Tirhuta */
            {0x114c7, 0x114c7}, /*     1x Tirhuta */
            {0x114d0, 0x114d9}, /*    10x Tirhuta */
            {0x11580, 0x115ae}, /*    47x Siddham */
            {0x115d8, 0x115db}, /*     4x Siddham */
            {0x11600, 0x1162f}, /*    48x Modi */
            {0x11644, 0x11644}, /*     1x Modi */
            {0x11650, 0x11659}, /*    10x Modi */
            {0x11680, 0x116aa}, /*    43x Takri */
            {0x116b8, 0x116b8}, /*     1x Takri */
            {0x116c0, 0x116c9}, /*    10x Takri */
            {0x11700, 0x1171a}, /*    27x Ahom */
            {0x11730, 0x1173b}, /*    12x Ahom */
            {0x11800, 0x1182b}, /*    44x Dogra */
            {0x118a0, 0x118f2}, /*    83x Warang Citi */
            {0x118ff, 0x118ff}, /*     1x Warang Citi */
            {0x119a0, 0x119a7}, /*     8x Nandinagari */
            {0x119aa, 0x119d0}, /*    39x Nandinagari */
            {0x119e1, 0x119e1}, /*     1x Nandinagari */
            {0x119e3, 0x119e3}, /*     1x Nandinagari */
            {0x11a00, 0x11a00}, /*     1x Zanabazar Square */
            {0x11a0b, 0x11a32}, /*    40x Zanabazar Square */
            {0x11a3a, 0x11a3a}, /*     1x Zanabazar Square */
            {0x11a50, 0x11a50}, /*     1x Soyombo */
            {0x11a5c, 0x11a89}, /*    46x Soyombo */
            {0x11a9d, 0x11a9d}, /*     1x Soyombo */
            {0x11ac0, 0x11af8}, /*    57x Pau Cin Hau */
            {0x11c00, 0x11c08}, /*     9x Bhaiksuki */
            {0x11c0a, 0x11c2e}, /*    37x Bhaiksuki */
            {0x11c40, 0x11c40}, /*     1x Bhaiksuki */
            {0x11c50, 0x11c6c}, /*    29x Bhaiksuki */
            {0x11c72, 0x11c8f}, /*    30x Marchen */
            {0x11d00, 0x11d06}, /*     7x Masaram Gondi */
            {0x11d08, 0x11d09}, /*     2x Masaram Gondi */
            {0x11d0b, 0x11d30}, /*    38x Masaram Gondi */
            {0x11d46, 0x11d46}, /*     1x Masaram Gondi */
            {0x11d50, 0x11d59}, /*    10x Masaram Gondi */
            {0x11d60, 0x11d65}, /*     6x Gunjala Gondi */
            {0x11d67, 0x11d68}, /*     2x Gunjala Gondi */
            {0x11d6a, 0x11d89}, /*    32x Gunjala Gondi */
            {0x11d98, 0x11d98}, /*     1x Gunjala Gondi */
            {0x11da0, 0x11da9}, /*    10x Gunjala Gondi */
            {0x11ee0, 0x11ef2}, /*    19x Makasar */
            {0x11fc0, 0x11fd4}, /*    21x Tamil Supplement */
            {0x12000, 0x12399}, /*   922x Cuneiform */
            {0x12400, 0x1246e}, /*   111x Cuneiform Numbers & Punctuation */
            {0x12480, 0x12543}, /*   196x Early Dynastic Cuneiform */
            {0x13000, 0x1342e}, /*  1071x Egyptian Hieroglyphs */
            {0x14400, 0x14646}, /*   583x Anatolian Hieroglyphs */
            {0x16800, 0x16a38}, /*   569x Bamum Supplement */
            {0x16a40, 0x16a5e}, /*    31x Mro */
            {0x16a60, 0x16a69}, /*    10x Mro */
            {0x16ad0, 0x16aed}, /*    30x Bassa Vah */
            {0x16b00, 0x16b2f}, /*    48x Pahawh Hmong */
            {0x16b40, 0x16b43}, /*     4x Pahawh Hmong */
            {0x16b50, 0x16b59}, /*    10x Pahawh Hmong */
            {0x16b5b, 0x16b61}, /*     7x Pahawh Hmong */
            {0x16b63, 0x16b77}, /*    21x Pahawh Hmong */
            {0x16b7d, 0x16b8f}, /*    19x Pahawh Hmong */
            {0x16e40, 0x16e96}, /*    87x Medefaidrin */
            {0x16f00, 0x16f4a}, /*    75x Miao */
            {0x16f50, 0x16f50}, /*     1x Miao */
            {0x16f93, 0x16f9f}, /*    13x Miao */
            {0x16fe0, 0x16fe1}, /*     2x Ideographic Symbols & Punctuation */
            {0x16fe3, 0x16fe3}, /*     1x Ideographic Symbols & Punctuation */
            {0x17000, 0x187f7}, /*  6136x Tangut */
            {0x18800, 0x18af2}, /*   755x Tangut Components */
            {0x1b000, 0x1b11e}, /*   287x Kana Supplement */
            {0x1b150, 0x1b152}, /*     3x Small Kana Extension */
            {0x1b164, 0x1b167}, /*     4x Small Kana Extension */
            {0x1b170, 0x1b2fb}, /*   396x Nushu */
            {0x1bc00, 0x1bc6a}, /*   107x Duployan */
            {0x1bc70, 0x1bc7c}, /*    13x Duployan */
            {0x1bc80, 0x1bc88}, /*     9x Duployan */
            {0x1bc90, 0x1bc99}, /*    10x Duployan */
            {0x1d2e0, 0x1d2f3}, /*    20x Mayan Numerals */
            {0x1d360, 0x1d378}, /*    25x Counting Rod Numerals */
            {0x1d400, 0x1d454}, /*    85x 𝐀..𝑔 Math */
            {0x1d456, 0x1d49c}, /*    71x 𝑖..𝒜 Math */
            {0x1d49e, 0x1d49f}, /*     2x 𝒞..𝒟 Math */
            {0x1d4a2, 0x1d4a2}, /*     1x 𝒢..𝒢 Math */
            {0x1d4a5, 0x1d4a6}, /*     2x 𝒥..𝒦 Math */
            {0x1d4a9, 0x1d4ac}, /*     4x 𝒩..𝒬 Math */
            {0x1d4ae, 0x1d4b9}, /*    12x 𝒮..𝒹 Math */
            {0x1d4bb, 0x1d4bb}, /*     1x 𝒻..𝒻 Math */
            {0x1d4bd, 0x1d4c3}, /*     7x 𝒽..𝓃 Math */
            {0x1d4c5, 0x1d505}, /*    65x 𝓅..𝔅 Math */
            {0x1d507, 0x1d50a}, /*     4x 𝔇..𝔊 Math */
            {0x1d50d, 0x1d514}, /*     8x 𝔍..𝔔 Math */
            {0x1d516, 0x1d51c}, /*     7x 𝔖..𝔜 Math */
            {0x1d51e, 0x1d539}, /*    28x 𝔞..𝔹 Math */
            {0x1d53b, 0x1d53e}, /*     4x 𝔻..𝔾 Math */
            {0x1d540, 0x1d544}, /*     5x 𝕀..𝕄 Math */
            {0x1d546, 0x1d546}, /*     1x 𝕆..𝕆 Math */
            {0x1d54a, 0x1d550}, /*     7x 𝕊..𝕐 Math */
            {0x1d552, 0x1d6a5}, /*   340x 𝕒..𝚥 Math */
            {0x1d6a8, 0x1d6c0}, /*    25x 𝚨..𝛀 Math */
            {0x1d6c2, 0x1d6da}, /*    25x 𝛂..𝛚 Math */
            {0x1d6dc, 0x1d6fa}, /*    31x 𝛜..𝛺 Math */
            {0x1d6fc, 0x1d714}, /*    25x 𝛼..𝜔 Math */
            {0x1d716, 0x1d734}, /*    31x 𝜖..𝜴 Math */
            {0x1d736, 0x1d74e}, /*    25x 𝜶..𝝎 Math */
            {0x1d750, 0x1d76e}, /*    31x 𝝐..𝝮 Math */
            {0x1d770, 0x1d788}, /*    25x 𝝰..𝞈 Math */
            {0x1d78a, 0x1d7a8}, /*    31x 𝞊..𝞨 Math */
            {0x1d7aa, 0x1d7c2}, /*    25x 𝞪..𝟂 Math */
            {0x1d7c4, 0x1d7cb}, /*     8x 𝟄..𝟋 Math */
            {0x1d7ce, 0x1d9ff}, /*   562x Math, Sutton SignWriting */
            {0x1f100, 0x1f10c}, /*    13x Enclosed Alphanumeric Supplement */
            {0x20000, 0x2a6d6}, /* 42711x CJK Unified Ideographs Extension B */
            {0x2a700, 0x2b734}, /*  4149x CJK Unified Ideographs Extension C */
            {0x2b740, 0x2b81d}, /*   222x CJK Unified Ideographs Extension D */
            {0x2b820, 0x2cea1}, /*  5762x CJK Unified Ideographs Extension E */
            {0x2ceb0, 0x2ebe0}, /*  7473x CJK Unified Ideographs Extension F */
            {0x2f800, 0x2fa1d}, /*   542x CJK Compatibility Ideographs Supplement */
        };
        l = 0;
        r = sizeof(kAstralGlyphs) / sizeof(kAstralGlyphs[0]);
        while (l < r) {
            m = (l + r) >> 1;
            if (kAstralGlyphs[m][1] < c) {
                l = m + 1;
            } else {
                r = m;
            }
        }
        return !(kAstralGlyphs[l][0] <= c && c <= kAstralGlyphs[l][1]);
    }
}

static int Lowercase(int c) {
    int m, l, r;
    if (c < 0200) {
        if ('A' <= c && c <= 'Z') {
            return c + 32;
        } else {
            return c;
        }
    } else if (c <= 0xffff) {
        if ((0x0100 <= c && c <= 0x0176) || /* 60x Ā..ā → ā..ŵ Watin-A */
            (0x01de <= c && c <= 0x01ee) || /*  9x Ǟ..Ǯ → ǟ..ǯ Watin-B */
            (0x01f8 <= c && c <= 0x021e) || /* 20x Ǹ..Ȟ → ǹ..ȟ Watin-B */
            (0x0222 <= c && c <= 0x0232) || /*  9x Ȣ..Ȳ → ȣ..ȳ Watin-B */
            (0x1e00 <= c && c <= 0x1eff)) { /*256x Ḁ..Ỿ → ḁ..ỿ Watin-C */
            if (c == 0x0130) return c - 199;
            if (c == 0x1e9e) return c;
            return c + (~c & 1);
        } else if (0x01cf <= c && c <= 0x01db) {
            return c + (c & 1); /* 7x Ǐ..Ǜ → ǐ..ǜ Watin-B */
        } else if (0x13a0 <= c && c <= 0x13ef) {
            return c + 38864; /* 80x Ꭰ ..Ꮿ  → ꭰ ..ꮿ  Cherokee */
        } else {
            static const struct {
                unsigned short a;
                unsigned short b;
                short d;
            } kLower[] = {
                {0x00c0, 0x00d6,   +32}, /* 23x À ..Ö  → à ..ö  Watin */
                {0x00d8, 0x00de,   +32}, /*  7x Ø ..Þ  → ø ..þ  Watin */
                {0x0178, 0x0178,  -121}, /*  1x Ÿ ..Ÿ  → ÿ ..ÿ  Watin-A */
                {0x0179, 0x0179,    +1}, /*  1x Ź ..Ź  → ź ..ź  Watin-A */
                {0x017b, 0x017b,    +1}, /*  1x Ż ..Ż  → ż ..ż  Watin-A */
                {0x017d, 0x017d,    +1}, /*  1x Ž ..Ž  → ž ..ž  Watin-A */
                {0x0181, 0x0181,  +210}, /*  1x Ɓ ..Ɓ  → ɓ ..ɓ  Watin-B */
                {0x0182, 0x0182,    +1}, /*  1x Ƃ ..Ƃ  → ƃ ..ƃ  Watin-B */
                {0x0184, 0x0184,    +1}, /*  1x Ƅ ..Ƅ  → ƅ ..ƅ  Watin-B */
                {0x0186, 0x0186,  +206}, /*  1x Ɔ ..Ɔ  → ɔ ..ɔ  Watin-B */
                {0x0187, 0x0187,    +1}, /*  1x Ƈ ..Ƈ  → ƈ ..ƈ  Watin-B */
                {0x0189, 0x018a,  +205}, /*  2x Ɖ ..Ɗ  → ɖ ..ɗ  Watin-B */
                {0x018b, 0x018b,    +1}, /*  1x Ƌ ..Ƌ  → ƌ ..ƌ  Watin-B */
                {0x018e, 0x018e,   +79}, /*  1x Ǝ ..Ǝ  → ǝ ..ǝ  Watin-B */
                {0x018f, 0x018f,  +202}, /*  1x Ə ..Ə  → ə ..ə  Watin-B */
                {0x0190, 0x0190,  +203}, /*  1x Ɛ ..Ɛ  → ɛ ..ɛ  Watin-B */
                {0x0191, 0x0191,    +1}, /*  1x Ƒ ..Ƒ  → ƒ ..ƒ  Watin-B */
                {0x0193, 0x0193,  +205}, /*  1x Ɠ ..Ɠ  → ɠ ..ɠ  Watin-B */
                {0x0194, 0x0194,  +207}, /*  1x Ɣ ..Ɣ  → ɣ ..ɣ  Watin-B */
                {0x0196, 0x0196,  +211}, /*  1x Ɩ ..Ɩ  → ɩ ..ɩ  Watin-B */
                {0x0197, 0x0197,  +209}, /*  1x Ɨ ..Ɨ  → ɨ ..ɨ  Watin-B */
                {0x0198, 0x0198,    +1}, /*  1x Ƙ ..Ƙ  → ƙ ..ƙ  Watin-B */
                {0x019c, 0x019c,  +211}, /*  1x Ɯ ..Ɯ  → ɯ ..ɯ  Watin-B */
                {0x019d, 0x019d,  +213}, /*  1x Ɲ ..Ɲ  → ɲ ..ɲ  Watin-B */
                {0x019f, 0x019f,  +214}, /*  1x Ɵ ..Ɵ  → ɵ ..ɵ  Watin-B */
                {0x01a0, 0x01a0,    +1}, /*  1x Ơ ..Ơ  → ơ ..ơ  Watin-B */
                {0x01a2, 0x01a2,    +1}, /*  1x Ƣ ..Ƣ  → ƣ ..ƣ  Watin-B */
                {0x01a4, 0x01a4,    +1}, /*  1x Ƥ ..Ƥ  → ƥ ..ƥ  Watin-B */
                {0x01a6, 0x01a6,  +218}, /*  1x Ʀ ..Ʀ  → ʀ ..ʀ  Watin-B */
                {0x01a7, 0x01a7,    +1}, /*  1x Ƨ ..Ƨ  → ƨ ..ƨ  Watin-B */
                {0x01a9, 0x01a9,  +218}, /*  1x Ʃ ..Ʃ  → ʃ ..ʃ  Watin-B */
                {0x01ac, 0x01ac,    +1}, /*  1x Ƭ ..Ƭ  → ƭ ..ƭ  Watin-B */
                {0x01ae, 0x01ae,  +218}, /*  1x Ʈ ..Ʈ  → ʈ ..ʈ  Watin-B */
                {0x01af, 0x01af,    +1}, /*  1x Ư ..Ư  → ư ..ư  Watin-B */
                {0x01b1, 0x01b2,  +217}, /*  2x Ʊ ..Ʋ  → ʊ ..ʋ  Watin-B */
                {0x01b3, 0x01b3,    +1}, /*  1x Ƴ ..Ƴ  → ƴ ..ƴ  Watin-B */
                {0x01b5, 0x01b5,    +1}, /*  1x Ƶ ..Ƶ  → ƶ ..ƶ  Watin-B */
                {0x01b7, 0x01b7,  +219}, /*  1x Ʒ ..Ʒ  → ʒ ..ʒ  Watin-B */
                {0x01b8, 0x01b8,    +1}, /*  1x Ƹ ..Ƹ  → ƹ ..ƹ  Watin-B */
                {0x01bc, 0x01bc,    +1}, /*  1x Ƽ ..Ƽ  → ƽ ..ƽ  Watin-B */
                {0x01c4, 0x01c4,    +2}, /*  1x Ǆ ..Ǆ  → ǆ ..ǆ  Watin-B */
                {0x01c5, 0x01c5,    +1}, /*  1x ǅ ..ǅ  → ǆ ..ǆ  Watin-B */
                {0x01c7, 0x01c7,    +2}, /*  1x Ǉ ..Ǉ  → ǉ ..ǉ  Watin-B */
                {0x01c8, 0x01c8,    +1}, /*  1x ǈ ..ǈ  → ǉ ..ǉ  Watin-B */
                {0x01ca, 0x01ca,    +2}, /*  1x Ǌ ..Ǌ  → ǌ ..ǌ  Watin-B */
                {0x01cb, 0x01cb,    +1}, /*  1x ǋ ..ǋ  → ǌ ..ǌ  Watin-B */
                {0x01cd, 0x01cd,    +1}, /*  1x Ǎ ..Ǎ  → ǎ ..ǎ  Watin-B */
                {0x01f1, 0x01f1,    +2}, /*  1x Ǳ ..Ǳ  → ǳ ..ǳ  Watin-B */
                {0x01f2, 0x01f2,    +1}, /*  1x ǲ ..ǲ  → ǳ ..ǳ  Watin-B */
                {0x01f4, 0x01f4,    +1}, /*  1x Ǵ ..Ǵ  → ǵ ..ǵ  Watin-B */
                {0x01f6, 0x01f6,   -97}, /*  1x Ƕ ..Ƕ  → ƕ ..ƕ  Watin-B */
                {0x01f7, 0x01f7,   -56}, /*  1x Ƿ ..Ƿ  → ƿ ..ƿ  Watin-B */
                {0x0220, 0x0220,  -130}, /*  1x Ƞ ..Ƞ  → ƞ ..ƞ  Watin-B */
                {0x023b, 0x023b,    +1}, /*  1x Ȼ ..Ȼ  → ȼ ..ȼ  Watin-B */
                {0x023d, 0x023d,  -163}, /*  1x Ƚ ..Ƚ  → ƚ ..ƚ  Watin-B */
                {0x0241, 0x0241,    +1}, /*  1x Ɂ ..Ɂ  → ɂ ..ɂ  Watin-B */
                {0x0243, 0x0243,  -195}, /*  1x Ƀ ..Ƀ  → ƀ ..ƀ  Watin-B */
                {0x0244, 0x0244,   +69}, /*  1x Ʉ ..Ʉ  → ʉ ..ʉ  Watin-B */
                {0x0245, 0x0245,   +71}, /*  1x Ʌ ..Ʌ  → ʌ ..ʌ  Watin-B */
                {0x0246, 0x0246,    +1}, /*  1x Ɇ ..Ɇ  → ɇ ..ɇ  Watin-B */
                {0x0248, 0x0248,    +1}, /*  1x Ɉ ..Ɉ  → ɉ ..ɉ  Watin-B */
                {0x024a, 0x024a,    +1}, /*  1x Ɋ ..Ɋ  → ɋ ..ɋ  Watin-B */
                {0x024c, 0x024c,    +1}, /*  1x Ɍ ..Ɍ  → ɍ ..ɍ  Watin-B */
                {0x024e, 0x024e,    +1}, /*  1x Ɏ ..Ɏ  → ɏ ..ɏ  Watin-B */
                {0x0386, 0x0386,   +38}, /*  1x Ά ..Ά  → ά ..ά  Greek */
                {0x0388, 0x038a,   +37}, /*  3x Έ ..Ί  → έ ..ί  Greek */
                {0x038c, 0x038c,   +64}, /*  1x Ό ..Ό  → ό ..ό  Greek */
                {0x038e, 0x038f,   +63}, /*  2x Ύ ..Ώ  → ύ ..ώ  Greek */
                {0x0391, 0x03a1,   +32}, /* 17x Α ..Ρ  → α ..ρ  Greek */
                {0x03a3, 0x03ab,   +32}, /*  9x Σ ..Ϋ  → σ ..ϋ  Greek */
                {0x03dc, 0x03dc,    +1}, /*  1x Ϝ ..Ϝ  → ϝ ..ϝ  Greek */
                {0x03f4, 0x03f4,   -60}, /*  1x ϴ ..ϴ  → θ ..θ  Greek */
                {0x0400, 0x040f,   +80}, /* 16x Ѐ ..Џ  → ѐ ..џ  Cyrillic */
                {0x0410, 0x042f,   +32}, /* 32x А ..Я  → а ..я  Cyrillic */
                {0x0460, 0x0460,    +1}, /*  1x Ѡ ..Ѡ  → ѡ ..ѡ  Cyrillic */
                {0x0462, 0x0462,    +1}, /*  1x Ѣ ..Ѣ  → ѣ ..ѣ  Cyrillic */
                {0x0464, 0x0464,    +1}, /*  1x Ѥ ..Ѥ  → ѥ ..ѥ  Cyrillic */
                {0x0472, 0x0472,    +1}, /*  1x Ѳ ..Ѳ  → ѳ ..ѳ  Cyrillic */
                {0x0490, 0x0490,    +1}, /*  1x Ґ ..Ґ  → ґ ..ґ  Cyrillic */
                {0x0498, 0x0498,    +1}, /*  1x Ҙ ..Ҙ  → ҙ ..ҙ  Cyrillic */
                {0x049a, 0x049a,    +1}, /*  1x Қ ..Қ  → қ ..қ  Cyrillic */
                {0x0531, 0x0556,   +48}, /* 38x Ա ..Ֆ  → ա ..ֆ  Armenian */
                {0x10a0, 0x10c5, +7264}, /* 38x Ⴀ ..Ⴥ  → ⴀ ..ⴥ  Georgian */
                {0x10c7, 0x10c7, +7264}, /*  1x Ⴧ ..Ⴧ  → ⴧ ..ⴧ  Georgian */
                {0x10cd, 0x10cd, +7264}, /*  1x Ⴭ ..Ⴭ  → ⴭ ..ⴭ  Georgian */
                {0x13f0, 0x13f5,    +8}, /*  6x Ᏸ ..Ᏽ  → ᏸ ..ᏽ  Cherokee */
                {0x1c90, 0x1cba, -3008}, /* 43x Ა ..Ჺ  → ა ..ჺ  Georgian2 */
                {0x1cbd, 0x1cbf, -3008}, /*  3x Ჽ ..Ჿ  → ჽ ..ჿ  Georgian2 */
                {0x1f08, 0x1f0f,    -8}, /*  8x Ἀ ..Ἇ  → ἀ ..ἇ  Greek2 */
                {0x1f18, 0x1f1d,    -8}, /*  6x Ἐ ..Ἕ  → ἐ ..ἕ  Greek2 */
                {0x1f28, 0x1f2f,    -8}, /*  8x Ἠ ..Ἧ  → ἠ ..ἧ  Greek2 */
                {0x1f38, 0x1f3f,    -8}, /*  8x Ἰ ..Ἷ  → ἰ ..ἷ  Greek2 */
                {0x1f48, 0x1f4d,    -8}, /*  6x Ὀ ..Ὅ  → ὀ ..ὅ  Greek2 */
                {0x1f59, 0x1f59,    -8}, /*  1x Ὑ ..Ὑ  → ὑ ..ὑ  Greek2 */
                {0x1f5b, 0x1f5b,    -8}, /*  1x Ὓ ..Ὓ  → ὓ ..ὓ  Greek2 */
                {0x1f5d, 0x1f5d,    -8}, /*  1x Ὕ ..Ὕ  → ὕ ..ὕ  Greek2 */
                {0x1f5f, 0x1f5f,    -8}, /*  1x Ὗ ..Ὗ  → ὗ ..ὗ  Greek2 */
                {0x1f68, 0x1f6f,    -8}, /*  8x Ὠ ..Ὧ  → ὠ ..ὧ  Greek2 */
                {0x1f88, 0x1f8f,    -8}, /*  8x ᾈ ..ᾏ  → ᾀ ..ᾇ  Greek2 */
                {0x1f98, 0x1f9f,    -8}, /*  8x ᾘ ..ᾟ  → ᾐ ..ᾗ  Greek2 */
                {0x1fa8, 0x1faf,    -8}, /*  8x ᾨ ..ᾯ  → ᾠ ..ᾧ  Greek2 */
                {0x1fb8, 0x1fb9,    -8}, /*  2x Ᾰ ..Ᾱ  → ᾰ ..ᾱ  Greek2 */
                {0x1fba, 0x1fbb,   -74}, /*  2x Ὰ ..Ά  → ὰ ..ά  Greek2 */
                {0x1fbc, 0x1fbc,    -9}, /*  1x ᾼ ..ᾼ  → ᾳ ..ᾳ  Greek2 */
                {0x1fc8, 0x1fcb,   -86}, /*  4x Ὲ ..Ή  → ὲ ..ή  Greek2 */
                {0x1fcc, 0x1fcc,    -9}, /*  1x ῌ ..ῌ  → ῃ ..ῃ  Greek2 */
                {0x1fd8, 0x1fd9,    -8}, /*  2x Ῐ ..Ῑ  → ῐ ..ῑ  Greek2 */
                {0x1fda, 0x1fdb,  -100}, /*  2x Ὶ ..Ί  → ὶ ..ί  Greek2 */
                {0x1fe8, 0x1fe9,    -8}, /*  2x Ῠ ..Ῡ  → ῠ ..ῡ  Greek2 */
                {0x1fea, 0x1feb,  -112}, /*  2x Ὺ ..Ύ  → ὺ ..ύ  Greek2 */
                {0x1fec, 0x1fec,    -7}, /*  1x Ῥ ..Ῥ  → ῥ ..ῥ  Greek2 */
                {0x1ff8, 0x1ff9,  -128}, /*  2x Ὸ ..Ό  → ὸ ..ό  Greek2 */
                {0x1ffa, 0x1ffb,  -126}, /*  2x Ὼ ..Ώ  → ὼ ..ώ  Greek2 */
                {0x1ffc, 0x1ffc,    -9}, /*  1x ῼ ..ῼ  → ῳ ..ῳ  Greek2 */
                {0x2126, 0x2126, -7517}, /*  1x Ω ..Ω  → ω ..ω  Letterlike */
                {0x212a, 0x212a, -8383}, /*  1x K ..K  → k ..k  Letterlike */
                {0x212b, 0x212b, -8262}, /*  1x Å ..Å  → å ..å  Letterlike */
                {0x2132, 0x2132,   +28}, /*  1x Ⅎ ..Ⅎ  → ⅎ ..ⅎ  Letterlike */
                {0x2160, 0x216f,   +16}, /* 16x Ⅰ ..Ⅿ  → ⅰ ..ⅿ  Numbery */
                {0x2183, 0x2183,    +1}, /*  1x Ↄ ..Ↄ  → ↄ ..ↄ  Numbery */
                {0x24b6, 0x24cf,   +26}, /* 26x Ⓐ ..Ⓩ  → ⓐ ..ⓩ  Enclosed */
                {0x2c00, 0x2c2e,   +48}, /* 47x Ⰰ ..Ⱞ  → ⰰ ..ⱞ  Glagolitic */
                {0xff21, 0xff3a,   +32}, /* 26x Ａ..Ｚ → ａ..ｚ Dubs */
            };
            l = 0;
            r = sizeof(kLower) / sizeof(kLower[0]);
            while (l < r) {
                m = (l + r) >> 1;
                if (kLower[m].b < c) {
                    l = m + 1;
                } else {
                    r = m;
                }
            }
            if (kLower[l].a <= c && c <= kLower[l].b) {
                return c + kLower[l].d;
            } else {
                return c;
            }
        }
    } else {
        static const int kAstralLower[][3] = {
            {0x10400, 0x10427,   +40}, /* 40x 𐐀 ..𐐧  → 𐐨 ..𐑏  Deseret */
            {0x104b0, 0x104d3,   +40}, /* 36x 𐒰 ..𐓓  → 𐓘 ..𐓻  Osage */
            {0x1d400, 0x1d419,   +26}, /* 26x 𝐀 ..𝐙  → 𝐚 ..𝐳  Math */
            {0x1d43c, 0x1d44d,   +26}, /* 18x 𝐼 ..𝑍  → 𝑖 ..𝑧  Math */
            {0x1d468, 0x1d481,   +26}, /* 26x 𝑨 ..𝒁  → 𝒂 ..𝒛  Math */
            {0x1d4ae, 0x1d4b5,   +26}, /*  8x 𝒮 ..𝒵  → 𝓈 ..𝓏  Math */
            {0x1d4d0, 0x1d4e9,   +26}, /* 26x 𝓐 ..𝓩  → 𝓪 ..𝔃  Math */
            {0x1d50d, 0x1d514,   +26}, /*  8x 𝔍 ..𝔔  → 𝔧 ..𝔮  Math */
            {0x1d56c, 0x1d585,   +26}, /* 26x 𝕬 ..𝖅  → 𝖆 ..𝖟  Math */
            {0x1d5a0, 0x1d5b9,   +26}, /* 26x 𝖠 ..𝖹  → 𝖺 ..𝗓  Math */
            {0x1d5d4, 0x1d5ed,   +26}, /* 26x 𝗔 ..𝗭  → 𝗮 ..𝘇  Math */
            {0x1d608, 0x1d621,   +26}, /* 26x 𝘈 ..𝘡  → 𝘢 ..𝘻  Math */
            {0x1d63c, 0x1d655,  -442}, /* 26x 𝘼 ..𝙕  → 𝒂 ..𝒛  Math */
            {0x1d670, 0x1d689,   +26}, /* 26x 𝙰 ..𝚉  → 𝚊 ..𝚣  Math */
            {0x1d6a8, 0x1d6b8,   +26}, /* 17x 𝚨 ..𝚸  → 𝛂 ..𝛒  Math */
            {0x1d6e2, 0x1d6f2,   +26}, /* 17x 𝛢 ..𝛲  → 𝛼 ..𝜌  Math */
            {0x1d71c, 0x1d72c,   +26}, /* 17x 𝜜 ..𝜬  → 𝜶 ..𝝆  Math */
            {0x1d756, 0x1d766,   +26}, /* 17x 𝝖 ..𝝦  → 𝝰 ..𝞀  Math */
            {0x1d790, 0x1d7a0,   -90}, /* 17x 𝞐 ..𝞠  → 𝜶 ..𝝆  Math */
        };
        l = 0;
        r = sizeof(kAstralLower) / sizeof(kAstralLower[0]);
        while (l < r) {
            m = (l + r) >> 1;
            if (kAstralLower[m][1] < c) {
                l = m + 1;
            } else {
                r = m;
            }
        }
        if (kAstralLower[l][0] <= c && c <= kAstralLower[l][1]) {
            return c + kAstralLower[l][2];
        } else {
            return c;
        }
    }
}

static int Uppercase(int c) {
    int m, l, r;
    if (c < 0200) {
        if ('a' <= c && c <= 'z') {
            return c - 32;
        } else {
            return c;
        }
    } else if (c <= 0xffff) {
        if ((0x0101 <= c && c <= 0x0177) || /* 60x ā..ŵ → Ā..ā Watin-A */
            (0x01df <= c && c <= 0x01ef) || /*  9x ǟ..ǯ → Ǟ..Ǯ Watin-B */
            (0x01f8 <= c && c <= 0x021e) || /* 20x ǹ..ȟ → Ǹ..Ȟ Watin-B */
            (0x0222 <= c && c <= 0x0232) || /*  9x ȣ..ȳ → Ȣ..Ȳ Watin-B */
            (0x1e01 <= c && c <= 0x1eff)) { /*256x ḁ..ỿ → Ḁ..Ỿ Watin-C */
            if (c == 0x0131) return c + 232;
            if (c == 0x1e9e) return c;
            return c - (c & 1);
        } else if (0x01d0 <= c && c <= 0x01dc) {
            return c - (~c & 1); /* 7x ǐ..ǜ → Ǐ..Ǜ Watin-B */
        } else if (0xab70 <= c && c <= 0xabbf) {
            return c - 38864; /* 80x ꭰ ..ꮿ  → Ꭰ ..Ꮿ  Cherokee Supplement */
        } else {
            static const struct {
                unsigned short a;
                unsigned short b;
                short d;
            } kUpper[] = {
                {0x00b5, 0x00b5,  +743}, /*  1x µ ..µ  → Μ ..Μ  Watin */
                {0x00e0, 0x00f6,   -32}, /* 23x à ..ö  → À ..Ö  Watin */
                {0x00f8, 0x00fe,   -32}, /*  7x ø ..þ  → Ø ..Þ  Watin */
                {0x00ff, 0x00ff,  +121}, /*  1x ÿ ..ÿ  → Ÿ ..Ÿ  Watin */
                {0x017a, 0x017a,    -1}, /*  1x ź ..ź  → Ź ..Ź  Watin-A */
                {0x017c, 0x017c,    -1}, /*  1x ż ..ż  → Ż ..Ż  Watin-A */
                {0x017e, 0x017e,    -1}, /*  1x ž ..ž  → Ž ..Ž  Watin-A */
                {0x017f, 0x017f,  -300}, /*  1x ſ ..ſ  → S ..S  Watin-A */
                {0x0180, 0x0180,  +195}, /*  1x ƀ ..ƀ  → Ƀ ..Ƀ  Watin-B */
                {0x0183, 0x0183,    -1}, /*  1x ƃ ..ƃ  → Ƃ ..Ƃ  Watin-B */
                {0x0185, 0x0185,    -1}, /*  1x ƅ ..ƅ  → Ƅ ..Ƅ  Watin-B */
                {0x0188, 0x0188,    -1}, /*  1x ƈ ..ƈ  → Ƈ ..Ƈ  Watin-B */
                {0x018c, 0x018c,    -1}, /*  1x ƌ ..ƌ  → Ƌ ..Ƌ  Watin-B */
                {0x0192, 0x0192,    -1}, /*  1x ƒ ..ƒ  → Ƒ ..Ƒ  Watin-B */
                {0x0195, 0x0195,   +97}, /*  1x ƕ ..ƕ  → Ƕ ..Ƕ  Watin-B */
                {0x0199, 0x0199,    -1}, /*  1x ƙ ..ƙ  → Ƙ ..Ƙ  Watin-B */
                {0x019a, 0x019a,  +163}, /*  1x ƚ ..ƚ  → Ƚ ..Ƚ  Watin-B */
                {0x019e, 0x019e,  +130}, /*  1x ƞ ..ƞ  → Ƞ ..Ƞ  Watin-B */
                {0x01a1, 0x01a1,    -1}, /*  1x ơ ..ơ  → Ơ ..Ơ  Watin-B */
                {0x01a3, 0x01a3,    -1}, /*  1x ƣ ..ƣ  → Ƣ ..Ƣ  Watin-B */
                {0x01a5, 0x01a5,    -1}, /*  1x ƥ ..ƥ  → Ƥ ..Ƥ  Watin-B */
                {0x01a8, 0x01a8,    -1}, /*  1x ƨ ..ƨ  → Ƨ ..Ƨ  Watin-B */
                {0x01ad, 0x01ad,    -1}, /*  1x ƭ ..ƭ  → Ƭ ..Ƭ  Watin-B */
                {0x01b0, 0x01b0,    -1}, /*  1x ư ..ư  → Ư ..Ư  Watin-B */
                {0x01b4, 0x01b4,    -1}, /*  1x ƴ ..ƴ  → Ƴ ..Ƴ  Watin-B */
                {0x01b6, 0x01b6,    -1}, /*  1x ƶ ..ƶ  → Ƶ ..Ƶ  Watin-B */
                {0x01b9, 0x01b9,    -1}, /*  1x ƹ ..ƹ  → Ƹ ..Ƹ  Watin-B */
                {0x01bd, 0x01bd,    -1}, /*  1x ƽ ..ƽ  → Ƽ ..Ƽ  Watin-B */
                {0x01bf, 0x01bf,   +56}, /*  1x ƿ ..ƿ  → Ƿ ..Ƿ  Watin-B */
                {0x01c5, 0x01c5,    -1}, /*  1x ǅ ..ǅ  → Ǆ ..Ǆ  Watin-B */
                {0x01c6, 0x01c6,    -2}, /*  1x ǆ ..ǆ  → Ǆ ..Ǆ  Watin-B */
                {0x01c8, 0x01c8,    -1}, /*  1x ǈ ..ǈ  → Ǉ ..Ǉ  Watin-B */
                {0x01c9, 0x01c9,    -2}, /*  1x ǉ ..ǉ  → Ǉ ..Ǉ  Watin-B */
                {0x01cb, 0x01cb,    -1}, /*  1x ǋ ..ǋ  → Ǌ ..Ǌ  Watin-B */
                {0x01cc, 0x01cc,    -2}, /*  1x ǌ ..ǌ  → Ǌ ..Ǌ  Watin-B */
                {0x01ce, 0x01ce,    -1}, /*  1x ǎ ..ǎ  → Ǎ ..Ǎ  Watin-B */
                {0x01dd, 0x01dd,   -79}, /*  1x ǝ ..ǝ  → Ǝ ..Ǝ  Watin-B */
                {0x01f2, 0x01f2,    -1}, /*  1x ǲ ..ǲ  → Ǳ ..Ǳ  Watin-B */
                {0x01f3, 0x01f3,    -2}, /*  1x ǳ ..ǳ  → Ǳ ..Ǳ  Watin-B */
                {0x01f5, 0x01f5,    -1}, /*  1x ǵ ..ǵ  → Ǵ ..Ǵ  Watin-B */
                {0x023c, 0x023c,    -1}, /*  1x ȼ ..ȼ  → Ȼ ..Ȼ  Watin-B */
                {0x023f, 0x0240,+10815}, /*  2x ȿ ..ɀ  → Ȿ ..Ɀ  Watin-B */
                {0x0242, 0x0242,    -1}, /*  1x ɂ ..ɂ  → Ɂ ..Ɂ  Watin-B */
                {0x0247, 0x0247,    -1}, /*  1x ɇ ..ɇ  → Ɇ ..Ɇ  Watin-B */
                {0x0249, 0x0249,    -1}, /*  1x ɉ ..ɉ  → Ɉ ..Ɉ  Watin-B */
                {0x024b, 0x024b,    -1}, /*  1x ɋ ..ɋ  → Ɋ ..Ɋ  Watin-B */
                {0x024d, 0x024d,    -1}, /*  1x ɍ ..ɍ  → Ɍ ..Ɍ  Watin-B */
                {0x024f, 0x024f,    -1}, /*  1x ɏ ..ɏ  → Ɏ ..Ɏ  Watin-B */
                {0x037b, 0x037d,  +130}, /*  3x ͻ ..ͽ  → Ͻ ..Ͽ  Greek */
                {0x03ac, 0x03ac,   -38}, /*  1x ά ..ά  → Ά ..Ά  Greek */
                {0x03ad, 0x03af,   -37}, /*  3x έ ..ί  → Έ ..Ί  Greek */
                {0x03b1, 0x03c1,   -32}, /* 17x α ..ρ  → Α ..Ρ  Greek */
                {0x03c2, 0x03c2,   -31}, /*  1x ς ..ς  → Σ ..Σ  Greek */
                {0x03c3, 0x03cb,   -32}, /*  9x σ ..ϋ  → Σ ..Ϋ  Greek */
                {0x03cc, 0x03cc,   -64}, /*  1x ό ..ό  → Ό ..Ό  Greek */
                {0x03cd, 0x03ce,   -63}, /*  2x ύ ..ώ  → Ύ ..Ώ  Greek */
                {0x03d0, 0x03d0,   -62}, /*  1x ϐ ..ϐ  → Β ..Β  Greek */
                {0x03d1, 0x03d1,   -57}, /*  1x ϑ ..ϑ  → Θ ..Θ  Greek */
                {0x03d5, 0x03d5,   -47}, /*  1x ϕ ..ϕ  → Φ ..Φ  Greek */
                {0x03d6, 0x03d6,   -54}, /*  1x ϖ ..ϖ  → Π ..Π  Greek */
                {0x03dd, 0x03dd,    -1}, /*  1x ϝ ..ϝ  → Ϝ ..Ϝ  Greek */
                {0x03f0, 0x03f0,   -86}, /*  1x ϰ ..ϰ  → Κ ..Κ  Greek */
                {0x03f1, 0x03f1,   -80}, /*  1x ϱ ..ϱ  → Ρ ..Ρ  Greek */
                {0x03f5, 0x03f5,   -96}, /*  1x ϵ ..ϵ  → Ε ..Ε  Greek */
                {0x0430, 0x044f,   -32}, /* 32x а ..я  → А ..Я  Cyrillic */
                {0x0450, 0x045f,   -80}, /* 16x ѐ ..џ  → Ѐ ..Џ  Cyrillic */
                {0x0461, 0x0461,    -1}, /*  1x ѡ ..ѡ  → Ѡ ..Ѡ  Cyrillic */
                {0x0463, 0x0463,    -1}, /*  1x ѣ ..ѣ  → Ѣ ..Ѣ  Cyrillic */
                {0x0465, 0x0465,    -1}, /*  1x ѥ ..ѥ  → Ѥ ..Ѥ  Cyrillic */
                {0x0473, 0x0473,    -1}, /*  1x ѳ ..ѳ  → Ѳ ..Ѳ  Cyrillic */
                {0x0491, 0x0491,    -1}, /*  1x ґ ..ґ  → Ґ ..Ґ  Cyrillic */
                {0x0499, 0x0499,    -1}, /*  1x ҙ ..ҙ  → Ҙ ..Ҙ  Cyrillic */
                {0x049b, 0x049b,    -1}, /*  1x қ ..қ  → Қ ..Қ  Cyrillic */
                {0x0561, 0x0586,   -48}, /* 38x ա ..ֆ  → Ա ..Ֆ  Armenian */
                {0x10d0, 0x10fa, +3008}, /* 43x ა ..ჺ  → Ა ..Ჺ  Georgian */
                {0x10fd, 0x10ff, +3008}, /*  3x ჽ ..ჿ  → Ჽ ..Ჿ  Georgian */
                {0x13f8, 0x13fd,    -8}, /*  6x ᏸ ..ᏽ  → Ᏸ ..Ᏽ  Cherokee */
                {0x214e, 0x214e,   -28}, /*  1x ⅎ ..ⅎ  → Ⅎ ..Ⅎ  Letterlike */
                {0x2170, 0x217f,   -16}, /* 16x ⅰ ..ⅿ  → Ⅰ ..Ⅿ  Numbery */
                {0x2184, 0x2184,    -1}, /*  1x ↄ ..ↄ  → Ↄ ..Ↄ  Numbery */
                {0x24d0, 0x24e9,   -26}, /* 26x ⓐ ..ⓩ  → Ⓐ ..Ⓩ  Enclosed */
                {0x2c30, 0x2c5e,   -48}, /* 47x ⰰ ..ⱞ  → Ⰰ ..Ⱞ  Glagolitic */
                {0x2d00, 0x2d25, -7264}, /* 38x ⴀ ..ⴥ  → Ⴀ ..Ⴥ  Georgian2 */
                {0x2d27, 0x2d27, -7264}, /*  1x ⴧ ..ⴧ  → Ⴧ ..Ⴧ  Georgian2 */
                {0x2d2d, 0x2d2d, -7264}, /*  1x ⴭ ..ⴭ  → Ⴭ ..Ⴭ  Georgian2 */
                {0xff41, 0xff5a,   -32}, /* 26x ａ..ｚ → Ａ..Ｚ Dubs */
            };
            l = 0;
            r = sizeof(kUpper) / sizeof(kUpper[0]);
            while (l < r) {
                m = (l + r) >> 1;
                if (kUpper[m].b < c) {
                    l = m + 1;
                } else {
                    r = m;
                }
            }
            if (kUpper[l].a <= c && c <= kUpper[l].b) {
                return c + kUpper[l].d;
            } else {
                return c;
            }
        }
    } else {
        static const int kAstralUpper[][3] = {
            {0x10428, 0x1044f,   -40}, /* 40x 𐐨..𐑏 → 𐐀..𐐧 Deseret */
            {0x104d8, 0x104fb,   -40}, /* 36x 𐓘..𐓻 → 𐒰..𐓓 Osage */
            {0x1d41a, 0x1d433,   -26}, /* 26x 𝐚..𝐳 → 𝐀..𝐙 Math */
            {0x1d456, 0x1d467,   -26}, /* 18x 𝑖..𝑧 → 𝐼..𝑍 Math */
            {0x1d482, 0x1d49b,   -26}, /* 26x 𝒂..𝒛 → 𝑨..𝒁 Math */
            {0x1d4c8, 0x1d4cf,   -26}, /*  8x 𝓈..𝓏 → 𝒮..𝒵 Math */
            {0x1d4ea, 0x1d503,   -26}, /* 26x 𝓪..𝔃 → 𝓐..𝓩 Math */
            {0x1d527, 0x1d52e,   -26}, /*  8x 𝔧..𝔮 → 𝔍..𝔔 Math */
            {0x1d586, 0x1d59f,   -26}, /* 26x 𝖆..𝖟 → 𝕬..𝖅 Math */
            {0x1d5ba, 0x1d5d3,   -26}, /* 26x 𝖺..𝗓 → 𝖠..𝖹 Math */
            {0x1d5ee, 0x1d607,   -26}, /* 26x 𝗮..𝘇 → 𝗔..𝗭 Math */
            {0x1d622, 0x1d63b,   -26}, /* 26x 𝘢..𝘻 → 𝘈..𝘡 Math */
            {0x1d68a, 0x1d6a3,  +442}, /* 26x 𝒂..𝒛 → 𝘼..𝙕 Math */
            {0x1d6c2, 0x1d6d2,   -26}, /* 26x 𝚊..𝚣 → 𝙰..𝚉 Math */
            {0x1d6fc, 0x1d70c,   -26}, /* 17x 𝛂..𝛒 → 𝚨..𝚸 Math */
            {0x1d736, 0x1d746,   -26}, /* 17x 𝛼..𝜌 → 𝛢..𝛲 Math */
            {0x1d770, 0x1d780,   -26}, /* 17x 𝜶..𝝆 → 𝜜..𝜬 Math */
            {0x1d770, 0x1d756,   -26}, /* 17x 𝝰..𝞀 → 𝝖..𝝦 Math */
            {0x1d736, 0x1d790,   -90}, /* 17x 𝜶..𝝆 → 𝞐..𝞠 Math */
        };
        l = 0;
        r = sizeof(kAstralUpper) / sizeof(kAstralUpper[0]);
        while (l < r) {
            m = (l + r) >> 1;
            if (kAstralUpper[m][1] < c) {
                l = m + 1;
            } else {
                r = m;
            }
        }
        if (kAstralUpper[l][0] <= c && c <= kAstralUpper[l][1]) {
            return c + kAstralUpper[l][2];
        } else {
            return c;
        }
    }
}

static int NotSeparator(int c) {
    return !IsSeparator(c);
}

static int Capitalize(int c) {
    if (!iscapital) {
        c = Uppercase(c);
        iscapital = 1;
    }
    return c;
}

static inline int Bsr(unsigned long long x) {
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
    int b;
    b = __builtin_clzl(x);
    b ^= sizeof(long) * CHAR_BIT - 1;
    return b;
#else
    static const char kDebruijn[64] = {
        0,  47, 1,  56, 48, 27, 2,  60, 57, 49, 41, 37, 28, 16, 3,  61,
        54, 58, 35, 52, 50, 42, 21, 44, 38, 32, 29, 23, 17, 11, 4,  62,
        46, 55, 26, 59, 40, 36, 15, 53, 34, 51, 20, 43, 31, 22, 10, 45,
        25, 39, 14, 33, 19, 30, 9,  24, 13, 18, 8,  12, 7,  6,  5,  63,
    };
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return kDebruijn[(x * 0x03f79d71b4cb0a89) >> 58];
#endif
}

static struct rune DecodeUtf8(int c) {
    struct rune r;
    if (c < 252) {
        r.n = Bsr(255 & ~c);
        r.c = c & (((1 << r.n) - 1) | 3);
        r.n = 6 - r.n;
    } else {
        r.c = c & 3;
        r.n = 5;
    }
    return r;
}

static unsigned long long EncodeUtf8(unsigned c) {
    static const unsigned short kTpEnc[32 - 7] = {
        1|0300<<8, 1|0300<<8, 1|0300<<8, 1|0300<<8, 2|0340<<8,
        2|0340<<8, 2|0340<<8, 2|0340<<8, 2|0340<<8, 3|0360<<8,
        3|0360<<8, 3|0360<<8, 3|0360<<8, 3|0360<<8, 4|0370<<8,
        4|0370<<8, 4|0370<<8, 4|0370<<8, 4|0370<<8, 5|0374<<8,
        5|0374<<8, 5|0374<<8, 5|0374<<8, 5|0374<<8, 5|0374<<8,
    };
    int e, n;
    unsigned long long w;
    if (c < 0200) return c;
    e = kTpEnc[Bsr(c) - 7];
    n = e & 0xff;
    w = 0;
    do {
        w |= 0200 | (c & 077);
        w <<= 8;
        c >>= 6;
    } while (--n);
    return c | w | e >> 8;
}

static size_t GetFdSize(int fd) {
    struct stat st;
    st.st_size = 0;
    fstat(fd, &st);
    return st.st_size;
}

static char *GetLine(FILE *f) {
    ssize_t rc;
    char *p = 0;
    size_t n, c = 0;
    if ((rc = getdelim(&p, &c, '\n', f)) != EOF) {
        for (n = rc; n; --n) {
            if (p[n - 1] == '\r' || p[n - 1] == '\n') {
                p[n - 1] = 0;
            } else {
                break;
            }
        }
        return p;
    } else {
        free(p);
        return 0;
    }
}

static char *Copy(char *d, const char *s, size_t n) {
    memcpy(d, s, n);
    return d + n;
}

static char *CopyUntil(char *d, const char *s, int c, size_t n) {
    size_t i;
    for (i = 0; i < n; ++i) {
        if ((d[i] = s[i]) == c) {
            return d + i + 1;
        }
    }
    return 0;
}

static int CompareStrings(const char *a, const char *b) {
    size_t i;
    int x, y, c;
    for (i = 0;; ++i) {
        x = Lowercase(a[i] & 255);
        y = Lowercase(b[i] & 255);
        if ((c = x - y) || !x) {
            return c;
        }
    }
}

static const char *FindSubstringReverse(const char *p, size_t n,
                                        const char *q, size_t m) {
    size_t i;
    if (m <= n) {
        for (n -= m; n; --n) {
            for (i = 0; i < m; ++i) {
                if (p[n + i] != q[i]) {
                    break;
                }
            }
            if (i == m) {
                return p + n;
            }
        }
    }
    return 0;
}

static int ParseUnsigned(const char *s, void *e) {
    int c, x;
    for (x = 0; (c = *s++);) {
        if ('0' <= c && c <= '9') {
            x = Min(c - '0' + x * 10, 32767);
        } else {
            break;
        }
    }
    if (e) *(const char **)e = s;
    return x;
}

static char *FormatUnsigned(char *p, unsigned x) {
    char t;
    size_t i, a, b;
    i = 0;
    do {
        p[i++] = x % 10 + '0';
        x = x / 10;
    } while (x > 0);
    p[i] = '\0';
    if (i) {
        for (a = 0, b = i - 1; a < b; ++a, --b) {
            t = p[a];
            p[a] = p[b];
            p[b] = t;
        }
    }
    return p + i;
}

static int WaitUntilReady(int fd, int events) {
    struct pollfd p[1];
    p[0].fd = fd;
    p[0].events = events;
    return poll(p, 1, -1);
}

static ssize_t ReadCharacter(int fd, char *p, size_t n) {
    int e, i;
    ssize_t rc;
    struct rune r;
    unsigned char c;
    enum { kAscii, kUtf8, kEsc, kCsi1, kCsi2, kSs, kNf, kStr, kStr2, kDone } t;
    i = 0;
    r.c = 0;
    r.n = 0;
    e = errno;
    t = kAscii;
    if (n) p[0] = 0;
    do {
        for (;;) {
            if (n) {
                rc = read(fd,&c,1);
            } else {
                rc = read(fd,0,0);
            }
            if (rc == -1 && errno == EINTR) {
                if (!i) {
                    return -1;
                }
            } else if (rc == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (WaitUntilReady(fd, POLLIN) == -1) {
                    if (rc == -1 && errno == EINTR) {
                        if (!i) {
                            return -1;
                        }
                    } else {
                        return -1;
                    }
                }
            } else if (rc == -1) {
                return -1;
            } else if (!rc) {
                if (!i) {
                    errno = e;
                    return 0;
                } else {
                    errno = EILSEQ;
                    return -1;
                }
            } else {
                break;
            }
        }
        if (i + 1 < n) {
            p[i] = c;
            p[i+1] = 0;
        } else if (i < n) {
            p[i] = 0;
        }
        ++i;
        switch (t) {
        Whoopsie:
            if (n) p[0] = c;
            t = kAscii;
            i = 1;
            /* fallthrough */
        case kAscii:
            if (c < 0200) {
                if (c == 033) {
                    t = kEsc;
                } else {
                    t = kDone;
                }
            } else if (c >= 0300) {
                t = kUtf8;
                r = DecodeUtf8(c);
            } else {
                /* ignore overlong sequences */
            }
            break;
        case kUtf8:
            if ((c & 0300) == 0200) {
                r.c <<= 6;
                r.c |= c & 077;
                if (!--r.n) {
                    switch (r.c) {
                    case 033:
                        t = kEsc; /* parsed but not canonicalized */
                        break;
                    case 0x9b:
                        t = kCsi1; /* unusual but legal */
                        break;
                    case 0x8e: /* SS2 (Single Shift Two) */
                    case 0x8f: /* SS3 (Single Shift Three) */
                        t = kSs;
                        break;
                    case 0x90: /* DCS (Device Control String) */
                    case 0x98: /* SOS (Start of String) */
                    case 0x9d: /* OSC (Operating System Command) */
                    case 0x9e: /* PM  (Privacy Message) */
                    case 0x9f: /* APC (Application Program Command) */
                        t = kStr;
                        break;
                    default:
                        t = kDone;
                        break;
                    }
                }
            } else {
                goto Whoopsie; /* ignore underlong sequences if not eof */
            }
            break;
        case kEsc:
            if (0x20 <= c && c <= 0x2f) { /* Nf */
                t = kNf;
            } else if (0x30 <= c && c <= 0x3f) { /* Fp */
                t = kDone;
            } else if (0x20 <= c && c <= 0x5F) { /* Fe */
                switch (c) {
                case '[':
                    t = kCsi1;
                    break;
                case 'N': /* SS2 (Single Shift Two) */
                case 'O': /* SS3 (Single Shift Three) */
                    t = kSs;
                    break;
                case 'P': /* DCS (Device Control String) */
                case 'X': /* SOS (Start of String) */
                case ']': /* OSC (Operating System Command) */
                case '^': /* PM  (Privacy Message) */
                case '_': /* APC (Application Program Command) */
                    t = kStr;
                    break;
                case '\\':
                    goto Whoopsie;
                default:
                    t = kDone;
                    break;
                }
            } else if (0x60 <= c && c <= 0x7e) { /* Fs */
                t = kDone;
            } else if (c == 033) {
                if (i < 3) {
                    /* alt chording */
                } else {
                    t = kDone; /* esc mashing */
                    i = 1;
                }
            } else {
                t = kDone;
            }
            break;
        case kSs:
            t = kDone;
            break;
         case kNf:
             if (0x30 <= c && c <= 0x7e) {
                 t = kDone;
             } else if (!(0x20 <= c && c <= 0x2f)) {
                 goto Whoopsie;
             }
            break;
        case kCsi1:
            if (0x20 <= c && c <= 0x2f) {
                t = kCsi2;
            } else if (c == '[' && i == 3) {
                /* linux function keys */
            } else if (0x40 <= c && c <= 0x7e) {
                t = kDone;
            } else if (!(0x30 <= c && c <= 0x3f)) {
                goto Whoopsie;
            }
            break;
        case kCsi2:
            if (0x40 <= c && c <= 0x7e) {
                t = kDone;
            } else if (!(0x20 <= c && c <= 0x2f)) {
                goto Whoopsie;
            }
            break;
        case kStr:
            switch (c) {
            case '\a':
                t = kDone;
                break;
            case 0033: /* ESC */
            case 0302: /* C1 (UTF-8) */
                t = kStr2;
                break;
            default:
                break;
            }
            break;
        case kStr2:
            switch (c) {
            case '\a':
            case '\\': /* ST (ASCII) */
            case 0234: /* ST (UTF-8) */
                t = kDone;
                break;
            default:
                t = kStr;
                break;
            }
            break;
        default:
            assert(0);
        }
    } while (t != kDone);
    errno = e;
    return i;
}

static size_t GetMonospaceWidth(const char *p, size_t n) {
    int c;
    size_t i, w;
    struct rune r;
    enum { kAscii, kUtf8, kEsc, kCsi1, kCsi2, kSs, kNf, kStr, kStr2 } t;
    for (r.c = 0, r.n = 0, t = kAscii, w = i = 0; i < n; ++i) {
        c = p[i] & 255;
        switch (t) {
        Whoopsie:
            t = kAscii;
            /* fallthrough */
        case kAscii:
            if (c < 0200) {
                if (c == 033) {
                    t = kEsc;
                } else {
                    ++w;
                }
            } else if (c >= 0300) {
                t = kUtf8;
                r = DecodeUtf8(c);
            }
            break;
        case kUtf8:
            if ((c & 0300) == 0200) {
                r.c <<= 6;
                r.c |= c & 077;
                if (!--r.n) {
                    switch (r.c) {
                    case 033:
                        t = kEsc;
                        break;
                    case 0x9b:
                        t = kCsi1;
                        break;
                    case 0x8e:
                    case 0x8f:
                        t = kSs;
                        break;
                    case 0x90:
                    case 0x98:
                    case 0x9d:
                    case 0x9e:
                    case 0x9f:
                        t = kStr;
                        break;
                    default:
                        w += GetMonospaceCharacterWidth(r.c);
                        t = kAscii;
                        break;
                    }
                }
            } else {
                goto Whoopsie;
            }
            break;
        case kEsc:
            if (0x20 <= c && c <= 0x2f) {
                t = kNf;
            } else if (0x30 <= c && c <= 0x3f) {
                t = kAscii;
            } else if (0x20 <= c && c <= 0x5F) {
                switch (c) {
                case '[':
                    t = kCsi1;
                    break;
                case 'N':
                case 'O':
                    t = kSs;
                    break;
                case 'P':
                case 'X':
                case ']':
                case '^':
                case '_':
                    t = kStr;
                    break;
                case '\\':
                    goto Whoopsie;
                default:
                    t = kAscii;
                    break;
                }
            } else if (0x60 <= c && c <= 0x7e) {
                t = kAscii;
            } else if (c == 033) {
                if (i == 3) t = kAscii;
            } else {
                t = kAscii;
            }
            break;
        case kSs:
            t = kAscii;
            break;
         case kNf:
             if (0x30 <= c && c <= 0x7e) {
                 t = kAscii;
             } else if (!(0x20 <= c && c <= 0x2f)) {
                 goto Whoopsie;
             }
            break;
        case kCsi1:
            if (0x20 <= c && c <= 0x2f) {
                t = kCsi2;
            } else if (c == '[' && i == 3) {
                /* linux function keys */
            } else if (0x40 <= c && c <= 0x7e) {
                t = kAscii;
            } else if (!(0x30 <= c && c <= 0x3f)) {
                goto Whoopsie;
            }
            break;
        case kCsi2:
            if (0x40 <= c && c <= 0x7e) {
                t = kAscii;
            } else if (!(0x20 <= c && c <= 0x2f)) {
                goto Whoopsie;
            }
            break;
        case kStr:
            switch (c) {
            case '\a':
                t = kAscii;
                break;
            case 0033:
            case 0302:
                t = kStr2;
                break;
            default:
                break;
            }
            break;
        case kStr2:
            switch (c) {
            case '\a':
            case '\\':
            case 0234:
                t = kAscii;
                break;
            default:
                t = kStr;
                break;
            }
            break;
        default:
            assert(0);
        }
    }
    return w;
}

static void abInit(struct abuf *ab) {
    ab->b = (char *)malloc(1);
    ab->len = 0;
    ab->b[0] = 0;
}

static void abAppend(struct abuf *ab, const char *s, int len) {
    char *p;
    if (!(p = (char *)realloc(ab->b,ab->len+len+1))) return;
    memcpy(p+ab->len,s,len);
    p[ab->len+len]=0;
    ab->b = p;
    ab->len += len;
}

static void abAppends(struct abuf *ab, const char *s) {
    abAppend(ab, s, strlen(s));
}

static void abAppendu(struct abuf *ab, unsigned u) {
    char b[11];
    abAppend(ab, b, FormatUnsigned(b, u) - b);
}

static void abAppendw(struct abuf *ab, unsigned long long w) {
    char b[8];
    unsigned n = 0;
    do b[n++] = w;
    while ((w >>= 8));
    abAppend(ab, b, n);
}

static void abFree(struct abuf *ab) {
    free(ab->b);
}

/**
 * Enables "mask mode".
 *
 * When it is enabled, instead of the input that the user is typing, the
 * terminal will just display a corresponding number of asterisks, like
 * "****". This is useful for passwords and other secrets that should
 * not be displayed.
 */
void bestlineMaskModeEnable(void) {
    maskmode = 1;
}

void bestlineMaskModeDisable(void) {
    maskmode = 0;
}

static void bestlineOnCont(int sig) {
    gotcont = 1;
}

static void bestlineOnWinch(int sig) {
    gotwinch = 1;
}

static void bestlineOnInt(int sig) {
    longjmp(jraw, sig);
}

static void bestlineOnQuit(int sig) {
    longjmp(jraw, sig);
}

static int bestlineIsUnsupportedTerm(void) {
    int i;
    char *term;
    static char once, res;
    if (!once) {
        if ((term = getenv("TERM"))) {
            for (i = 0; i < sizeof(kUnsupported) / sizeof(*kUnsupported); i++) {
                if (!CompareStrings(term,kUnsupported[i])) {
                    res = 1;
                    break;
                }
            }
        }
        once = 1;
    }
    return res;
}

static int enableRawMode(int fd) {
    struct termios raw;
    struct sigaction sa;
    if (tcgetattr(fd,&orig_termios) != -1) {
        raw = orig_termios;
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP);
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
        raw.c_oflag &= ~OPOST;
        raw.c_iflag |= IUTF8;
        raw.c_cflag |= CS8;
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(fd,TCSANOW,&raw) != -1) {
            sa.sa_flags = 0;
            sa.sa_handler = bestlineOnCont;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGCONT,&sa,&orig_cont);
            sa.sa_handler = bestlineOnWinch;
            sigaction(SIGWINCH,&sa,&orig_winch);
            rawmode = fd;
            gotwinch = 0;
            gotcont = 0;
            return 0;
        }
    }
    errno = ENOTTY;
    return -1;
}

void bestlineDisableRawMode(void) {
    if (rawmode != -1) {
        sigaction(SIGCONT,&orig_cont,0);
        sigaction(SIGWINCH,&orig_winch,0);
        tcsetattr(rawmode,TCSAFLUSH,&orig_termios);
        rawmode = -1;
    }
}

static int bestlineWrite(int fd, const void *p, size_t n) {
    ssize_t rc;
    size_t wrote;
    do {
        for (;;) {
            rc = write(fd, p, n);
            if (rc == -1 && errno == EINTR) {
                continue;
            } else if (rc == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (WaitUntilReady(fd, POLLOUT) == -1) {
                    if (errno == EINTR) {
                        continue;
                    } else {
                        return -1;
                    }
                }
            } else {
                break;
            }
        }
        if (rc != -1) {
            wrote = rc;
            n -= wrote;
            p = (char *)p + wrote;
        } else {
            return -1;
        }
    } while (n);
    return 0;
}

static int bestlineWriteStr(int fd, const char *p) {
    return bestlineWrite(fd, p, strlen(p));
}

static ssize_t bestlineRead(int fd, char *buf, size_t size,
                             struct bestlineState *l) {
    ssize_t rc;
    int refreshme;
    do {
        refreshme = 0;
        if (gotcont && rawmode != -1) {
            enableRawMode(rawmode);
            if (l) refreshme = 1;
        }
        if (l && gotwinch) refreshme = 1;
        if (refreshme) bestlineRefreshLine(l);
        rc = ReadCharacter(fd, buf, size);
    } while (rc == -1 && errno == EINTR);
    if (l && rc > 0) {
        memcpy(l->seq[1], l->seq[0], sizeof(l->seq[0]));
        memset(l->seq[0], 0, sizeof(l->seq[0]));
        memcpy(l->seq[0], buf, Min(Min(size, rc), sizeof(l->seq[0]) - 1));
    }
    return rc;
}

/**
 * Returns number of columns in current terminal.
 *
 * 1. Checks COLUMNS environment variable (set by Emacs)
 * 2. Tries asking termios (works for pseudoteletypewriters)
 * 3. Falls back to inband signalling (works w/ pipe or serial)
 * 4. Otherwise we conservatively assume 80 columns
 *
 * @param ws should be initialized by caller to zero before first call
 * @param ifd is input file descriptor
 * @param ofd is output file descriptor
 * @return window size
 */
static struct winsize GetTerminalSize(struct winsize ws, int ifd, int ofd) {
    int x;
    ssize_t n;
    char *p, *s, b[16];
    ioctl(ofd, TIOCGWINSZ, &ws);
    if ((!ws.ws_row && 
         (s = getenv("ROWS")) && 
         (x = ParseUnsigned(s, 0)))) {
        ws.ws_row = x;
    }
    if ((!ws.ws_col && 
         (s = getenv("COLUMNS")) && 
         (x = ParseUnsigned(s, 0)))) {
        ws.ws_col = x;
    }
    if (((!ws.ws_col || !ws.ws_row) &&
         bestlineRead(ifd,0,0,0) != -1 &&
         bestlineWriteStr(ofd,
             "\0337"           /* save position */
             "\033[9979;9979H" /* move cursor to bottom right corner */
             "\033[6n"         /* report position */
             "\0338") != -1 && /* restore position */
         (n = bestlineRead(ifd,b,sizeof(b),0)) != -1 &&
         n && b[0] == 033 && b[1] == '[' && b[n - 1] == 'R')) {
        p = b+2;
        if ((x = ParseUnsigned(p,&p))) ws.ws_row = x;
        if (*p++ == ';' && (x = ParseUnsigned(p,0))) ws.ws_col = x;
    }
    if (!ws.ws_col) ws.ws_col = 80;
    if (!ws.ws_row) ws.ws_row = 24;
    return ws;
}

/* Clear the screen. Used to handle ctrl+l */
void bestlineClearScreen(int fd) {
    bestlineWriteStr(fd,
        "\033[H"    /* move cursor to top left corner */
        "\033[2J"); /* erase display */
}

static void bestlineBeep(void) {
    /* THE TERMINAL BELL IS DEAD - HISTORY HAS KILLED IT */
}

/* Free a list of completion option populated by bestlineAddCompletion(). */
void bestlineFreeCompletions(bestlineCompletions *lc) {
    size_t i;
    for (i = 0; i < lc->len; i++)
        free(lc->cvec[i]);
    if (lc->cvec)
        free(lc->cvec);
}

/* This is an helper function for bestlineEdit() and is called when the
 * user types the <tab> key in order to complete the string currently in the
 * input.
 *
 * The state of the editing is encapsulated into the pointed bestlineState
 * structure as described in the structure definition. */
static int bestlineCompleteLine(struct bestlineState *ls, char *seq, int size) {
    size_t i, n, stop;
    int nwritten, nread;
    bestlineCompletions lc;
    struct bestlineState saved;
    nread=0;
    memset(&lc,0,sizeof(lc));
    completionCallback(ls->buf,&lc);
    if (!lc.len) {
        bestlineBeep();
    } else {
        i = 0;
        stop = 0;
        while (!stop) {
            /* Show completion or original buffer */
            if (i < lc.len) {
                saved = *ls;
                ls->len = ls->pos = strlen(lc.cvec[i]);
                ls->buf = lc.cvec[i];
                bestlineRefreshLine(ls);
                ls->len = saved.len;
                ls->pos = saved.pos;
                ls->buf = saved.buf;
            } else {
                bestlineRefreshLine(ls);
            }
            nread = bestlineRead(ls->ifd,seq,size,ls);
            if (nread <= 0) {
                bestlineFreeCompletions(&lc);
                return -1;
            }
            switch (seq[0]) {
            case '\t':
                i = (i+1) % (lc.len+1);
                if (i == lc.len) {
                    bestlineBeep();
                }
                break;
            default:
                if (i < lc.len) {
                    n = strlen(lc.cvec[i]);
                    nwritten = Min(n,ls->buflen);
                    memcpy(ls->buf,lc.cvec[i],nwritten+1);
                    ls->len = ls->pos = nwritten;
                }
                stop = 1;
                break;
            }
        }
    }
    bestlineFreeCompletions(&lc);
    return nread;
}

static void bestlineEditHistoryGoto(struct bestlineState *l, int i) {
    if (historylen <= 1) return;
    i = Max(Min(i,historylen-1),0);
    free(history[historylen - 1 - l->hindex]);
    history[historylen - 1 - l->hindex] = strdup(l->buf);
    l->hindex = i;
    if(!CopyUntil(l->buf,history[historylen-1-l->hindex],0,l->buflen)){
        l->buf[l->buflen-1] = 0;
    }
    l->len = l->pos = strlen(l->buf);
    bestlineRefreshLine(l);
}

static void bestlineEditHistoryMove(struct bestlineState *l, int dx) {
    bestlineEditHistoryGoto(l,l->hindex+dx);
}

static char *bestlineMakeSearchPrompt(struct abuf *ab, int fail, const char *s, int n) {
    ab->len=0;
    abAppendw(ab,'(');
    if (fail) abAppends(ab,"failed ");
    abAppends(ab,"reverse-i-search `\033[4m");
    abAppend(ab,s,n);
    abAppends(ab,"\033[24m");
    abAppends(ab,s+n);
    abAppendw(ab,Read32le("') "));
    return ab->b;
}

static int bestlineSearch(struct bestlineState *l, char *seq, int size) {
    char *p;
    struct abuf ab;
    struct abuf prompt;
    const char *oldprompt, *q;
    int i, j, k, rc, fail, added, oldpos, matlen, oldindex;
    if (historylen <= 1) return 0;
    abInit(&ab);
    abInit(&prompt);
    oldpos = l->pos;
    oldprompt = l->prompt;
    oldindex = l->hindex;
    for (fail=matlen=0;;) {
        l->prompt = bestlineMakeSearchPrompt(&prompt,fail,ab.b,matlen);
        bestlineRefreshLine(l);
        fail = 1;
        added = 0;
        j = l->pos;
        i = l->hindex;
        rc = bestlineRead(l->ifd,seq,size,l);
        if (rc > 0) {
            if (seq[0] == Ctrl('?') || seq[0] == Ctrl('H')) {
                if (ab.len) {
                    --ab.len;
                    matlen = Min(matlen, ab.len);
                }
            } else if (seq[0] == Ctrl('R')) {
                if (j) {
                    --j;
                } else if (i + 1 < historylen) {
                    ++i;
                    j = strlen(history[historylen - 1 - i]);
                }
            } else if (seq[0] == Ctrl('G')) {
                bestlineEditHistoryGoto(l,oldindex);
                l->pos = oldpos;
                rc = 0;
                break;
            } else if (IsControl(seq[0])) { /* only sees canonical c0 */
                break;
            } else {
                abAppend(&ab,seq,rc);
                added = rc;
            }
        } else {
            break;
        }
        while (i < historylen) {
            p = history[historylen - 1 - i];
            k = strlen(p);
            j = Min(k, j + ab.len);
            if ((q = FindSubstringReverse(p, j, ab.b, ab.len))) {
                bestlineEditHistoryGoto(l,i);
                l->pos = q - p;
                fail = 0;
                if (added) {
                    matlen += added;
                    added = 0;
                }
                break;
            } else {
                i = i + 1;
                j = BESTLINE_MAX_LINE;
            }
        }
    }
    l->prompt = oldprompt;
    bestlineRefreshLine(l);
    abFree(&prompt);
    abFree(&ab);
    bestlineRefreshLine(l);
    return rc;
}

/* Register a callback function to be called for tab-completion. */
void bestlineSetCompletionCallback(bestlineCompletionCallback *fn) {
    completionCallback = fn;
}

/* Register a hits function to be called to show hits to the user at the
 * right of the prompt. */
void bestlineSetHintsCallback(bestlineHintsCallback *fn) {
    hintsCallback = fn;
}

/* Register a function to free the hints returned by the hints callback
 * registered with bestlineSetHintsCallback(). */
void bestlineSetFreeHintsCallback(bestlineFreeHintsCallback *fn) {
    freeHintsCallback = fn;
}

/* This function is used by the callback function registered by the user
 * in order to add completion options given the input string when the
 * user typed <tab>. See the example.c source code for a very easy to
 * understand example. */
void bestlineAddCompletion(bestlineCompletions *lc, const char *str) {
    size_t len;
    char *copy, **cvec;
    if ((copy = (char *)malloc((len = strlen(str))+1))) {
        memcpy(copy,str,len+1);
        if ((cvec = (char **)realloc(lc->cvec,(lc->len+1)*sizeof(*lc->cvec)))) {
            lc->cvec = cvec;
            lc->cvec[lc->len++] = copy;
        } else {
            free(copy);
        }
    }
}

static void bestlineRingFree(void) {
    size_t i;
    for (i = 0; i < BESTLINE_MAX_RING; ++i) {
        if (ring.p[i]) {
            free(ring.p[i]);
            ring.p[i] = 0;
        }
    }
}

static void bestlineRingPush(const char *p, size_t n) {
    if (BESTLINE_MAX_RING && n) {
        ring.i = (ring.i + 1) % BESTLINE_MAX_RING;
        free(ring.p[ring.i]);
        ring.p[ring.i] = strndup(p, n);
    }
}

static void bestlineRingRotate(void) {
    size_t i;
    for (i = 0; i < BESTLINE_MAX_RING; ++i) {
        ring.i = (ring.i - 1) % BESTLINE_MAX_RING;
        if (ring.p[ring.i]) {
            break;
        }
    }
}

static void bestlineRefreshHints(struct abuf *ab, struct bestlineState *l) {
    char *hint;
    const char *ansi1, *ansi2;
    if (!hintsCallback) return;
    ansi1 = "\033[90m";
    ansi2 = "\033[39m";
    if (!(hint = hintsCallback(l->buf,&ansi1,&ansi2))) return;
    if (ansi1) abAppends(ab,ansi1);
    abAppends(ab,hint);
    if (ansi2) abAppends(ab,ansi2);
    if (freeHintsCallback) freeHintsCallback(hint);
}

static void bestlineRefreshLine(struct bestlineState *l) {
    struct abuf ab;
    int i, j, fd, plen, pwidth, rows, rpos, rpos2, col, old_rows;
    if (gotwinch && rawmode != -1) {
        l->ws = GetTerminalSize(l->ws,l->ifd,l->ofd);
        gotwinch = 0;
    }
    fd = l->ofd;
    old_rows = l->maxrows;
    plen = strlen(l->prompt);
    pwidth = GetMonospaceWidth(l->prompt,plen);
    /* cursor relative row */
    rpos = (pwidth+l->oldpos+l->ws.ws_col)/l->ws.ws_col;
    /* rows used by current buf */
    rows = (pwidth+GetMonospaceWidth(l->buf,l->len)+
            l->ws.ws_col-1)/l->ws.ws_col;
    if (rows > (int)l->maxrows) l->maxrows = rows;
    /* First step: clear all the lines used before.
     * To do so start by going to the last row. */
    abInit(&ab);
    if (old_rows-rpos > 0) {
        abAppendw(&ab,Read32le("\033[\0"));
        abAppendu(&ab,old_rows-rpos);
        abAppendw(&ab,'B');
    }
    /* Now for every row clear it, go up. */
    for (j = 0; j < old_rows-1; j++) {
        abAppends(&ab,"\r\033[K\033[A");
    }
    abAppendw(&ab,Read32le("\r\033[K"));
    abAppends(&ab,l->prompt);
    if (maskmode) {
        for (i = 0; i < l->len; i++) {
            abAppendw(&ab,'*');
        }
    } else {
        abAppend(&ab,l->buf,l->len);
    }
    bestlineRefreshHints(&ab,l);
    /* If we are at the very end of the screen with our prompt, we need to
     * emit a newline and move the prompt to the first column. */
    if ((l->pos && l->pos == l->len &&
         !((pwidth + GetMonospaceWidth(l->buf,l->pos)) % l->ws.ws_col))) {
        abAppendw(&ab,Read32le("\n\r\0"));
        if (++rows > (int)l->maxrows) {
            l->maxrows = rows;
        }
    }
    /* Move cursor to right position. */
    /* Get current cursor relative row. */
    rpos2 = (pwidth+GetMonospaceWidth(l->buf,l->pos)+
             l->ws.ws_col)/l->ws.ws_col;
    /* Go up till we reach the expected positon. */
    if (rows-rpos2 > 0) {
        abAppendw(&ab,Read32le("\033[\0"));
        abAppendu(&ab,rows-rpos2);
        abAppendw(&ab,'A');
    }
    /* Set column. */
    col = (pwidth+(int)GetMonospaceWidth(l->buf,l->pos)) % (int)l->ws.ws_col;
    if (col) {
        abAppendw(&ab,Read32le("\r\033["));
        abAppendu(&ab,col);
        abAppendw(&ab,'C');
    } else {
        abAppendw(&ab,'\r');
    }
    l->oldpos = l->pos;
    bestlineWrite(fd,ab.b,ab.len);
    abFree(&ab);
}

static int bestlineEditInsert(struct bestlineState *l,
                               const char *p, size_t n) {
    if (l->len + n < l->buflen) {
        memmove(l->buf+l->pos+n,l->buf+l->pos,l->len-l->pos);
        memcpy(l->buf + l->pos, p, n);
        l->pos += n;
        l->len += n;
        l->buf[l->len] = 0;
        bestlineRefreshLine(l);
    }
    return 0;
}

static void bestlineEditHome(struct bestlineState *l) {
    l->pos = 0;
    bestlineRefreshLine(l);
}

static void bestlineEditEnd(struct bestlineState *l) {
    l->pos = l->len;
    bestlineRefreshLine(l);
}

static void bestlineEditUp(struct bestlineState *l) {
    bestlineEditHistoryMove(l,BESTLINE_HISTORY_PREV);
}

static void bestlineEditDown(struct bestlineState *l) {
    bestlineEditHistoryMove(l,BESTLINE_HISTORY_NEXT);
}

static void bestlineEditBof(struct bestlineState *l) {
    bestlineEditHistoryMove(l,BESTLINE_HISTORY_FIRST);
}

static void bestlineEditEof(struct bestlineState *l) {
    bestlineEditHistoryMove(l,BESTLINE_HISTORY_LAST);
}

static void bestlineEditRefresh(struct bestlineState *l) {
    bestlineClearScreen(l->ofd);
    bestlineRefreshLine(l);
}

static struct rune GetUtf8(const char *p, size_t n) {
    struct rune r;
    if ((r.n = r.c = 0) < n && (r.c = p[r.n++] & 255) >= 0300) {
        r.c = DecodeUtf8(r.c).c;
        while (r.n < n && (p[r.n] & 0300) == 0200) {
            r.c = r.c << 6 | (p[r.n++] & 077);
        }
    }
    return r;
}

static size_t Forward(struct bestlineState *l, size_t pos) {
    return pos + GetUtf8(l->buf + pos, l->len - pos).n;
}

static size_t Backward(struct bestlineState *l, size_t pos) {
    if (pos) {
        do --pos;
        while (pos && (l->buf[pos] & 0300) == 0200);
    }
    return pos;
}

static size_t Backwards(struct bestlineState *l, size_t pos, int pred(int)) {
    size_t i;
    struct rune r;
    while (pos) {
        i = Backward(l, pos);
        r = GetUtf8(l->buf + i, l->len - i);
        if (pred(r.c)) {
            pos = i;
        } else {
            break;
        }
    }
    return pos;
}

static size_t Forwards(struct bestlineState *l, size_t pos, int pred(int)) {
    struct rune r;
    while (pos < l->len) {
        r = GetUtf8(l->buf + pos, l->len - pos);
        if (pred(r.c)) {
            pos += r.n;
        } else {
            break;
        }
    }
    return pos;
}

static size_t ForwardWord(struct bestlineState *l, size_t pos) {
    pos = Forwards(l, pos, IsSeparator);
    pos = Forwards(l, pos, NotSeparator);
    return pos;
}

static size_t BackwardWord(struct bestlineState *l, size_t pos) {
    pos = Backwards(l, pos, IsSeparator);
    pos = Backwards(l, pos, NotSeparator);
    return pos;
}

static size_t EscapeWord(struct bestlineState *l) {
    size_t i, j;
    struct rune r;
    for (i = l->pos; i && i < l->len; i += r.n) {
        if (i < l->len) {
            r = GetUtf8(l->buf + i, l->len - i);
            if (IsSeparator(r.c)) break;
        }
        if ((j = i)) {
            do --j;
            while (j && (l->buf[j] & 0300) == 0200);
            r = GetUtf8(l->buf + j, l->len - j);
            if (IsSeparator(r.c)) break;
        }
    }
    return i;
}

static void bestlineEditLeft(struct bestlineState *l) {
    l->pos = Backward(l, l->pos);
    bestlineRefreshLine(l);
}

static void bestlineEditRight(struct bestlineState *l) {
    if (l->pos == l->len) return;
    do l->pos++;
    while (l->pos < l->len && (l->buf[l->pos] & 0300) == 0200);
    bestlineRefreshLine(l);
}

static void bestlineEditLeftWord(struct bestlineState *l) {
    l->pos = BackwardWord(l, l->pos);
    bestlineRefreshLine(l);
}

static void bestlineEditRightWord(struct bestlineState *l) {
    l->pos = ForwardWord(l, l->pos);
    bestlineRefreshLine(l);
}

static void bestlineEditDelete(struct bestlineState *l) {
    size_t i;
    if (l->pos == l->len) return;
    i = Forward(l, l->pos);
    memmove(l->buf+l->pos, l->buf+i, l->len-i+1);
    l->len -= i - l->pos;
    bestlineRefreshLine(l);
}

static void bestlineEditRubout(struct bestlineState *l) {
    size_t i;
    if (!l->pos) return;
    i = Backward(l, l->pos);
    memmove(l->buf+i, l->buf+l->pos, l->len-l->pos+1);
    l->len -= l->pos - i;
    l->pos = i;
    bestlineRefreshLine(l);
}

static void bestlineEditDeleteWord(struct bestlineState *l) {
    size_t i;
    if (l->pos == l->len) return;
    i = ForwardWord(l, l->pos);
    bestlineRingPush(l->buf + l->pos, i - l->len);
    memmove(l->buf + l->pos, l->buf + i, l->len - i + 1);
    l->len -= i - l->pos;
    bestlineRefreshLine(l);
}

static void bestlineEditRuboutWord(struct bestlineState *l) {
    size_t i;
    if (!l->pos) return;
    i = BackwardWord(l, l->pos);
    bestlineRingPush(l->buf + i, l->pos - i);
    memmove(l->buf + i, l->buf + l->pos, l->len - l->pos + 1);
    l->len -= l->pos - i;
    l->pos = i;
    bestlineRefreshLine(l);
}

static void bestlineEditXlatWord(struct bestlineState *l, int xlat(int)) {
    int c;
    struct rune r;
    struct abuf ab;
    size_t i, j;
    abInit(&ab);
    i = Forwards(l, l->pos, IsSeparator);
    for (j = i; j < l->len; j += r.n) {
        r = GetUtf8(l->buf + j, l->len - j);
        if (IsSeparator(r.c)) break;
        if ((c = xlat(r.c)) != r.c) {
            abAppendw(&ab, EncodeUtf8(c));
        } else { /* avoid canonicalization */
            abAppend(&ab, l->buf + j, r.n);
        }
    }
    if (ab.len && i + ab.len + l->len - j < l->buflen) {
        l->pos = i + ab.len;
        abAppend(&ab, l->buf + j, l->len - j);
        l->len = i + ab.len;
        memcpy(l->buf + i, ab.b, ab.len + 1);
        bestlineRefreshLine(l);
    }
    abFree(&ab);
}

static void bestlineEditLowercaseWord(struct bestlineState *l) {
    bestlineEditXlatWord(l, Lowercase);
}

static void bestlineEditUppercaseWord(struct bestlineState *l) {
    bestlineEditXlatWord(l, Uppercase);
}

static void bestlineEditCapitalizeWord(struct bestlineState *l) {
    iscapital = 0;
    bestlineEditXlatWord(l, Capitalize);
}

static void bestlineEditKillLeft(struct bestlineState *l) {
    size_t diff, old_pos;
    bestlineRingPush(l->buf, l->pos);
    old_pos = l->pos;
    l->pos = 0;
    diff = old_pos - l->pos;
    memmove(l->buf+l->pos,l->buf+old_pos,l->len-old_pos+1);
    l->len -= diff;
    bestlineRefreshLine(l);
}

static void bestlineEditKillRight(struct bestlineState *l) {
    bestlineRingPush(l->buf + l->pos, l->len - l->pos);
    l->buf[l->pos] = '\0';
    l->len = l->pos;
    bestlineRefreshLine(l);
}

static void bestlineEditYank(struct bestlineState *l) {
    char *p;
    size_t n;
    if (!ring.p[ring.i]) return;
    n = strlen(ring.p[ring.i]);
    if (l->len + n < l->buflen) {
        p = (char *)malloc(l->len - l->pos + 1);
        memcpy(p, l->buf + l->pos, l->len - l->pos + 1);
        memcpy(l->buf + l->pos, ring.p[ring.i], n);
        memcpy(l->buf + l->pos + n, p, l->len - l->pos + 1);
        l->yi = l->pos;
        l->yj = l->pos + n;
        l->pos += n;
        l->len += n;
        free(p);
        bestlineRefreshLine(l);
    }
}

static void bestlineEditRotate(struct bestlineState *l) {
    if ((l->seq[1][0] == Ctrl('Y') ||
         (l->seq[1][0] == 033 && l->seq[1][1] == 'y'))) {
        if (l->yi < l->len && l->yj <= l->len) {
            memmove(l->buf + l->yi, l->buf + l->yj, l->len - l->yj + 1);
            l->len -= l->yj - l->yi;
            l->pos -= l->yj - l->yi;
        }
        bestlineRingRotate();
        bestlineEditYank(l);
    }
}

static void bestlineEditTranspose(struct bestlineState *l) {
    char *q, *p;
    size_t a, b, c;
    b = l->pos;
    a = Backward(l, b);
    c = Forward(l, b);
    if (!(a < b && b < c)) return;
    p = q = (char *)malloc(c - a);
    p = Copy(p, l->buf + b, c - b);
    p = Copy(p, l->buf + a, b - a);
    assert(p - q == c - a);
    memcpy(l->buf + a, q, p - q);
    l->pos = c;
    free(q);
    bestlineRefreshLine(l);
}

static void bestlineEditTransposeWords(struct bestlineState *l) {
    char *q, *p;
    size_t pi, xi, xj, yi, yj;
    pi = EscapeWord(l);
    xj = Backwards(l, pi, IsSeparator);
    xi = Backwards(l, xj, NotSeparator);
    yi = Forwards(l, pi, IsSeparator);
    yj = Forwards(l, yi, NotSeparator);
    if (!(xi < xj && xj < yi && yi < yj)) return;
    p = q = (char *)malloc(yj - xi);
    p = Copy(p, l->buf + yi, yj - yi);
    p = Copy(p, l->buf + xj, yi - xj);
    p = Copy(p, l->buf + xi, xj - xi);
    assert(p - q == yj - xi);
    memcpy(l->buf + xi, q, p - q);
    l->pos = yj;
    free(q);
    bestlineRefreshLine(l);
}

static void bestlineEditSqueeze(struct bestlineState *l) {
    size_t i, j;
    i = Backwards(l, l->pos, IsSeparator);
    j = Forwards(l, l->pos, IsSeparator);
    if (!(i < j)) return;
    memmove(l->buf + i, l->buf + j, l->len - j + 1);
    l->len -= j - i;
    l->pos  = i;
    bestlineRefreshLine(l);
}

static void bestlineEditMark(struct bestlineState *l) {
    l->mark = l->pos;
}

static void bestlineEditGoto(struct bestlineState *l) {
    if (l->mark > l->len) return;
    l->pos = Min(l->mark, l->len);
    bestlineRefreshLine(l);
}

/**
 * Runs bestline engine.
 *
 * This function is the core of the line editing capability of bestline.
 * It expects 'fd' to be already in "raw mode" so that every key pressed
 * will be returned ASAP to read().
 *
 * The resulting string is put into 'buf' when the user type enter, or
 * when ctrl+d is typed.
 *
 * Returns chomped character count in buf >=0 or -1 on eof / error
 */
static ssize_t bestlineEdit(int stdin_fd, int stdout_fd,
                             char *buf, size_t buflen,
                             const char *prompt) {
    ssize_t rc;
    size_t nread;
    char seq[16];
    struct bestlineState l;
    bestlineHintsCallback *hc;
    memset(&l,0,sizeof(l));
    buf[0] = 0;
    l.buf = buf;
    l.ifd = stdin_fd;
    l.ofd = stdout_fd;
    l.buflen = buflen - 1;
    l.prompt = prompt ? prompt : "";
    l.ws = GetTerminalSize(l.ws,l.ifd,l.ofd);
    bestlineHistoryAdd("");
    bestlineWriteStr(l.ofd,l.prompt);
    while (1) {
        rc = bestlineRead(l.ifd,seq,sizeof(seq),&l);
        if (rc > 0) {
            if (seq[0] == Ctrl('R')) {
                rc = bestlineSearch(&l,seq,sizeof(seq));
                if (!rc) continue;
            } else if (seq[0] == '\t' && completionCallback) {
                rc = bestlineCompleteLine(&l,seq,sizeof(seq));
                if (!rc) continue;
            }
        }
        if (rc > 0) {
            nread = rc;
        } else if (!rc && l.len) {
            nread = 1;
            seq[0] = '\r';
            seq[1] = 0;
        } else {
            historylen--;
            free(history[historylen]);
            history[historylen] = 0;
            return -1;
        }
        switch (seq[0]) {
        Case(Ctrl('P'), bestlineEditUp(&l));
        Case(Ctrl('E'), bestlineEditEnd(&l));
        Case(Ctrl('N'), bestlineEditDown(&l));
        Case(Ctrl('A'), bestlineEditHome(&l));
        Case(Ctrl('B'), bestlineEditLeft(&l));
        Case(Ctrl('@'), bestlineEditMark(&l));
        Case(Ctrl('Y'), bestlineEditYank(&l));
        Case(Ctrl('F'), bestlineEditRight(&l));
        Case(Ctrl('?'), bestlineEditRubout(&l));
        Case(Ctrl('H'), bestlineEditRubout(&l));
        Case(Ctrl('L'), bestlineEditRefresh(&l));
        Case(Ctrl('U'), bestlineEditKillLeft(&l));
        Case(Ctrl('T'), bestlineEditTranspose(&l));
        Case(Ctrl('K'), bestlineEditKillRight(&l));
        Case(Ctrl('W'), bestlineEditRuboutWord(&l));
        case Ctrl('X'):
            if (l.seq[1][0] == Ctrl('X')) {
                bestlineEditGoto(&l);
            }
            break;
        case Ctrl('D'):
            if (l.len) {
                bestlineEditDelete(&l);
            } else {
                free(history[--historylen]);
                history[historylen] = 0;
                return -1;
            }
            break;
        case '\r':
            free(history[--historylen]);
            history[historylen] = 0;
            bestlineEditEnd(&l);
            if (hintsCallback) {
                /* Force a refresh without hints to leave the previous
                 * line as the user typed it after a newline. */
                hc = hintsCallback;
                hintsCallback = 0;
                bestlineRefreshLine(&l);
                hintsCallback = hc;
            }
            return l.len;
        case 033:
            if (nread < 2) break;
            switch (seq[1]) {
            Case('<', bestlineEditBof(&l));
            Case('>', bestlineEditEof(&l));
            Case('y', bestlineEditRotate(&l));
            Case('\\', bestlineEditSqueeze(&l));
            Case('b', bestlineEditLeftWord(&l));
            Case('f', bestlineEditRightWord(&l));
            Case('h', bestlineEditRuboutWord(&l));
            Case('d', bestlineEditDeleteWord(&l));
            Case('l', bestlineEditLowercaseWord(&l));
            Case('u', bestlineEditUppercaseWord(&l));
            Case('c', bestlineEditCapitalizeWord(&l));
            Case('t', bestlineEditTransposeWords(&l));
            Case(Ctrl('H'), bestlineEditRuboutWord(&l));
            case '[':
                if (nread < 3) break;
                if (seq[2] >= '0' && seq[2] <= '9') {
                    if (nread < 4) break;
                    if (seq[3] == '~') {
                        switch (seq[2]) {
                        Case('1', bestlineEditHome(&l));   /* \e[1~ */
                        Case('3', bestlineEditDelete(&l)); /* \e[3~ */
                        Case('4', bestlineEditEnd(&l));    /* \e[4~ */
                        default:
                            break;
                        }
                    }
                } else {
                    switch (seq[2]) {
                    Case('A', bestlineEditUp(&l));
                    Case('B', bestlineEditDown(&l));
                    Case('C', bestlineEditRight(&l));
                    Case('D', bestlineEditLeft(&l));
                    Case('H', bestlineEditHome(&l));
                    Case('F', bestlineEditEnd(&l));
                    default:
                        break;
                    }
                }
                break;
            case 'O':
                if (nread < 3) break;
                switch (seq[2]) {
                Case('H', bestlineEditHome(&l));
                Case('F', bestlineEditEnd(&l));
                default:
                    break;
                }
                break;
            default:
                break;
            }
            break;
        default:
            if (!IsControl(seq[0])) { /* only sees canonical c0 */
                bestlineEditInsert(&l,seq,nread);
            }
            break;
        }
    }
}

void bestlineFree(void *ptr) {
    free(ptr);
}

void bestlineHistoryFree(void) {
    size_t i;
    for (i = 0; i < BESTLINE_MAX_HISTORY; i++) {
        if (history[i]) {
            free(history[i]);
            history[i] = 0;
        }
    }
    historylen = 0;
}

static void bestlineAtExit(void) {
    bestlineDisableRawMode();
    bestlineHistoryFree();
    bestlineRingFree();
}

int bestlineHistoryAdd(const char *line) {
    char *linecopy;
    if (!BESTLINE_MAX_HISTORY) return 0;
    if (historylen && !strcmp(history[historylen-1], line)) return 0;
    if (!(linecopy = strdup(line))) return 0;
    if (historylen == BESTLINE_MAX_HISTORY) {
        free(history[0]);
        memmove(history,history+1,sizeof(char*)*(BESTLINE_MAX_HISTORY-1));
        historylen--;
    }
    history[historylen++] = linecopy;
    return 1;
}

/**
 * Saves line editing history to file.
 *
 * @return 0 on success, or -1 w/ errno
 */
int bestlineHistorySave(const char *filename) {
    int j;
    FILE *fp;
    mode_t old_umask;
    old_umask = umask(S_IXUSR|S_IRWXG|S_IRWXO);
    fp = fopen(filename,"w");
    umask(old_umask);
    if (!fp) return -1;
    chmod(filename,S_IRUSR|S_IWUSR);
    for (j = 0; j < historylen; j++) {
        fputs(history[j],fp);
        fputc('\n',fp);
    }
    fclose(fp);
    return 0;
}

/**
 * Loads history from the specified file.
 *
 * If the file doesn't exist, zero is returned and this will do nothing.
 * If the file does exists and the operation succeeded zero is returned
 * otherwise on error -1 is returned.
 *
 * @return 0 on success, or -1 w/ errno
 */
int bestlineHistoryLoad(const char *filename) {
    char **h;
    int rc, fd, err;
    size_t i, j, k, n;
    char *m, *e, *p, *q, *f, *s;
    err = errno, rc = 0;
    if (!BESTLINE_MAX_HISTORY) return 0;
    if (!(h = (char**)calloc(2*BESTLINE_MAX_HISTORY,sizeof(char*)))) return -1;
    if ((fd = open(filename,O_RDONLY)) != -1) {
        if ((n = GetFdSize(fd))) {
            if ((m = (char *)mmap(0,n,PROT_READ,MAP_SHARED,fd,0))!=MAP_FAILED) {
                for (i = 0, e = (p = m) + n; p < e; p = f + 1) {
                    if (!(q = (char *)memchr(p, '\n', e - p))) q = e;
                    for (f = q; q > p; --q) {
                        if (q[-1] != '\n' && q[-1] != '\r') break;
                    }
                    if (q > p) {
                        h[i * 2 + 0] = p;
                        h[i * 2 + 1] = q;
                        i = (i + 1) % BESTLINE_MAX_HISTORY;
                    }
                }
                bestlineHistoryFree();
                for (j = 0; j < BESTLINE_MAX_HISTORY; ++j) {
                    if (h[(k = (i + j) % BESTLINE_MAX_HISTORY) * 2]) {
                        if ((s = (char *)malloc((n=h[k*2+1]-h[k*2])+1))) {
                            memcpy(s,h[k*2],n),s[n]=0;
                            history[historylen++] = s;
                        }
                    }
                }
                munmap(m,n);
            } else {
                rc = -1;
            }
        }
        close(fd);
    } else if (errno == ENOENT) {
        errno = err;
    } else {
        rc = -1;
    }
    free(h);
    return rc;
}

/**
 * Reads line interactively.
 *
 * This function can be used instead of bestline() in cases where we
 * know for certain we're dealing with a terminal, which means we can
 * avoid linking any stdio code.
 *
 * @return chomped allocated string of read line or null on eof/error
 */
char *bestlineRaw(const char *prompt, int infd, int outfd) {
    int sig;
    ssize_t rc;
    size_t nread;
    char *buf, *p;
    sigset_t omask;
    static char once;
    struct sigaction sa;
    if (!once) atexit(bestlineAtExit), once = 1;
    if (!(buf = (char *)malloc(BESTLINE_MAX_LINE))) return 0;
    if (enableRawMode(infd) == -1) return 0;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask,SIGINT);
    sigaddset(&sa.sa_mask,SIGQUIT);
    sigprocmask(SIG_BLOCK,&sa.sa_mask,&omask);
    sa.sa_flags = SA_NODEFER;
    sa.sa_handler = bestlineOnInt;
    sigaction(SIGINT,&sa,&orig_int);
    sa.sa_handler = bestlineOnQuit;
    sigaction(SIGQUIT,&sa,&orig_quit);
    if (!(sig = setjmp(jraw))) {
        sigprocmask(SIG_UNBLOCK,&sa.sa_mask,0);
        rc = bestlineEdit(infd,outfd,buf,BESTLINE_MAX_LINE,prompt);
    } else {
        rc = -1;
    }
    bestlineDisableRawMode();
    sigaction(SIGINT,&orig_int,0);
    sigaction(SIGQUIT,&orig_quit,0);
    sigprocmask(SIG_SETMASK,&omask,0);
    if (sig) raise(sig);
    if (rc != -1) {
        nread = rc;
        bestlineWriteStr(outfd,"\n");
        if ((p = (char *)realloc(buf,nread+1))) {
            return p;
        } else {
            return buf;
        }
    } else {
        free(buf);
        return 0;
    }
}

/**
 * Reads line intelligently.
 *
 * The high level function that is the main API of the bestline library.
 * This function checks if the terminal has basic capabilities, just checking
 * for a blacklist of inarticulate terminals, and later either calls the line
 * editing function or uses dummy fgets() so that you will be able to type
 * something even in the most desperate of the conditions.
 *
 * @param prompt is printed before asking for input if we have a term
 *     and this may be set to empty or null to disable and prompt may
 *     contain ansi escape sequences, color, utf8, etc.
 * @return chomped allocated string of read line or null on eof/error
 */
char *bestline(const char *prompt) {
    if ((!isatty(fileno(stdin)) ||
         !isatty(fileno(stdout)))) {
        return GetLine(stdin);
    } else if (bestlineIsUnsupportedTerm()) {
        if (prompt && *prompt) {
            fputs(prompt,stdout);
            fflush(stdout);
        }
        return GetLine(stdin);
    } else {
        fflush(stdout);
        return bestlineRaw(prompt,fileno(stdin),fileno(stdout));
    }
}

/**
 * Reads line intelligently w/ history, e.g.
 *
 *     // see ~/.foo_history
 *     main() {
 *         char *line;
 *         while ((line = ezbestline("IN> ", "foo"))) {
 *             printf("OUT> %s\n", line);
 *             free(line);
 *         }
 *     }
 *
 * @param prompt is printed before asking for input if we have a term
 *     and this may be set to empty or null to disable and prompt may
 *     contain ansi escape sequences, color, utf8, etc.
 * @param prog is name of your app, used to generate history filename
 *     however if it contains a slash / dot then we'll assume prog is
 *     the history filename which as determined by the caller
 * @return chomped allocated string of read line or null on eof/error
 */
char *ezbestline(const char *prompt, const char *prog) {
    char *line;
    struct abuf path;
    const char *a, *b;
    abInit(&path);
    if (prog) {
        if (strchr(prog, '/') || strchr(prog, '.')) {
            abAppends(&path, prog);
        } else {
            b = "";
            if (!(a = getenv("HOME"))) {
                if (!(a = getenv("HOMEDRIVE")) ||
                    !(b = getenv("HOMEPATH"))) {
                    a = "";
                }
            }
            if (*a) {
                abAppends(&path, a);
                abAppends(&path, b);
                abAppendw(&path, '/');
            }
            abAppendw(&path, '.');
            abAppends(&path, prog);
            abAppends(&path, "_history");
        }
    }
    if (path.len) {
        bestlineHistoryLoad(path.b);
    }
    line = bestline(prompt);
    if (path.len && line && *line) {
        /* history here is inefficient but helpful when the user has multiple
         * repls open at the same time, so history propagates between them */
        bestlineHistoryLoad(path.b);
        bestlineHistoryAdd(line);
        bestlineHistorySave(path.b);
    }
    abFree(&path);
    return line;
}
