# Bestline

Library for interactive pseudoteletypewriter command
sessions using ANSI Standard X3.64 control sequences.

![Bestline Demo Video GIF](bestline.gif)

## Overview

This is a single-file, no-dependencies C or C++ library that makes it as
easy as possible to display a command prompt that asks the user for
input. It supports Emacs editing shortcuts, history search, completion /
hint callback, and UTF-8 editing under a BSD-2 license.

Bestline is a fork of [linenoise](https://github.com/antirez/linenoise/)
(a popular GNU Readline alternative) that fixes its bugs and adds the
missing features while reducing binary footprint (surprisingly) by
removing bloated dependencies, which means you can finally have a
permissively-licensed command prompt w/ a 38kb footprint that's nearly
as good as GNU Readline.

```
$ CC="cc -s -static -Os -DNDEBUG" make
$ ls -hal bestline_example
-rwxr-xr-x 1 jart jart 38K Sep 19 21:41 bestline_example
```

# Example

This example will save history to `~/.foo_history`. It's 50kb when
statically linked with Cosmopolitan Libc.

```c
#include <stdio.h>
#include "bestline.h"
main() {
    char *line;
    while ((line = bestlineWithHistory("IN> ", "foo"))) {
        fputs("OUT> ", stdout);
        fputs(line, stdout);
        fputs("\n", stdout);
        free(line);
    }
}
```

## Shortcuts

```
CTRL-E         END
CTRL-A         START
CTRL-B         BACK
CTRL-F         FORWARD
CTRL-L         CLEAR
CTRL-H         BACKSPACE
CTRL-D         DELETE
CTRL-Y         YANK
CTRL-D         EOF (IF EMPTY)
CTRL-N         NEXT HISTORY
CTRL-P         PREVIOUS HISTORY
CTRL-R         SEARCH HISTORY
CTRL-G         CANCEL SEARCH
ALT-<          BEGINNING OF HISTORY
ALT->          END OF HISTORY
ALT-F          FORWARD WORD
ALT-B          BACKWARD WORD
CTRL-ALT-F     FORWARD EXPR
CTRL-ALT-B     BACKWARD EXPR
ALT-RIGHT      FORWARD EXPR
ALT-LEFT       BACKWARD EXPR
CTRL-K         KILL LINE FORWARDS
CTRL-U         KILL LINE BACKWARDS
ALT-H          KILL WORD BACKWARDS
CTRL-W         KILL WORD BACKWARDS
CTRL-ALT-H     KILL WORD BACKWARDS
ALT-D          KILL WORD FORWARDS
ALT-Y          ROTATE KILL RING AND YANK AGAIN
ALT-\          SQUEEZE ADJACENT WHITESPACE
CTRL-T         TRANSPOSE
ALT-T          TRANSPOSE WORD
ALT-U          UPPERCASE WORD
ALT-L          LOWERCASE WORD
ALT-C          CAPITALIZE WORD
CTRL-C         INTERRUPT PROCESS
CTRL-Z         SUSPEND PROCESS
CTRL-\         QUIT PROCESS
CTRL-S         PAUSE OUTPUT
CTRL-Q         UNPAUSE OUTPUT (IF PAUSED)
CTRL-Q         ESCAPED INSERT
CTRL-SPACE     SET MARK
CTRL-X CTRL-X  GOTO MARK
CTRL-Z         SUSPEND PROCESS
```

### ProTip

Remap CAPS LOCK to CTRL.

## Requirements

You have to use an ANSI UTF-8 terminal that supports VT100 codes.

## Changes

Here's what we've changed compared to
[linenoise](https://github.com/antirez/linenoise/):

- Remove bell
- Add kill ring
- Fix flickering
- Add UTF-8 editing
- Add CTRL-R search
- React to terminal resizing
- Don't generate .data section
- Support terminal flow control
- Make history loading 10x faster
- Make multiline mode the only mode
- Support unlimited input line length
- Accommodate O_NONBLOCK file descriptors
- Restore raw mode on process foregrounding
- Make source code compatible with C++ compilers
- Fix corruption issues by using generalized parsing
- Implement nearly all GNU Readline editing shortcuts
- Remove heavyweight dependencies like printf/sprintf
- Remove ISIG→^C→EAGAIN hack and catch signals properly
- Support running on Windows in MinTTY or CMD.EXE on Win10+
- Support diacritics, русский, Ελληνικά, 漢字, 仮名, 한글

## Readability

This codebase aims to follow in antirez's tradition of writing beautiful
programs, that solve extremely difficult technical problems in the most
elegant way possible. The original Linenoise source code is sort of like
an old DeLorean where it's simple and beautiful, but has a lot of things
broken about it that need to be fixed, which gives you plenty of reasons
to sit down and fix things to fully appreciate its beauty.

There are, however, some differences in style. antirez generally optimizes
for fewer lines of code even if it makes the binary footprint larger and
with poor edge case handling and cultural biases presumably to preserve
its accessibility and value as an educational tool. For example, one of
the biggest issues with Linenoise, was that pressing the wrong key on
the keyboard would mess with the state and garble input since it didn't
actually parse ANSI codes or even multibyte characters.

While this project has addressed many of Linenoise's shortcomings, we've
sought to do it in a way that carries on the antirez's tradition of simple
elegant hackable code. It is our hope that should you find opportunities
for improvement in this codebase that you'll find it equally pleasurable
to work with.

## Portability

Bestline is written in portable ANSI C99 that conforms to POSIX. We
recommend using Cosmopolitan Libc since it produces binaries that work
on any operating system including Windows.

Portability across terminals is achieved because literally everything
these days supports VT100 control codes which were standardized by ANSI
back in the 1970s. This library ignores platform-specific norms for
multibyte encoding and it also ignores antiquated terminal capability
databases. Libraries like ncurses were designed to reduce bandwidth on
300 bit per second modems. They're bloated and huge because they needed
to implement workarounds to all the "incompatible by design" engineering
practices used by terminal platforms in the '70s in '80s.

Corporate America has long since moved on to making GUI platforms
incompatible instead. Even the Windows command prompt supports VT100 and
XTERM sequences these days. Seriously. It's 2021 and everyone in the
world finally agrees on UTF-8 and ANSI VT100 style command sequences.
That's why Bestline is now, for the first time in history, able to offer
you a fully featured experience using simple bloat-free code.

## Contributing

We'd love to accept your pull requests! Please send an email beforehand
to Justine Tunney <jtunney@gmail.com> saying that you intend to assign
her the copyright to the changes you contribute to Bestline.

Please do not contribute changes that have `#ifdef` statements. We don't
care if MSVC printed a warning, and we won't accept Windows torture code
since Windows compatibility can be abstracted by Cosmopolitan Libc which
does C on Windows better than Windows does considering how there's about
ten different incompatible libc implementations, provided by the Windows
platform, and they have a history of doing things like adding telemetry.

## License

Bestline is released under the 2-clause BSD license. You have the
freedom to copy the Bestline source code into your codebase, but you
have to keep the license notice at the top of the file. You also have
the freedom to distribute your app as a closed-source binary, but you
have to embed the copyright notice in the executable. We've added an
`.ident` assembly directive to the top of the source code file which
should automatically take care of binary notice compliance.

```
The BSD-2 License

Copyright 2018-2021 Justine Tunney <jtunney@gmail.com>
Copyright 2010-2016 Salvatore Sanfilippo <antirez@gmail.com>
Copyright 2010-2013 Pieter Noordhuis <pcnoordhuis@gmail.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

## Donations

Please consider tipping the author at <https://github.com/sponsors/jart>
since she needs your support in order to keep going building cool tools
and libraries that serve the public interest. So if you like what you've
seen and want to encourage more, please consider granting recognition.
