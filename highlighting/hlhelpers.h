#ifndef HLHELPERS_H
#define HLHELPERS_H

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

#define HL_RESET "\x1b[39m"

#define HL_BACKGROUND "48;5;237m"

enum hl {
    HL_NORMAL = 0,
    HL_COMMENT,
    HL_MLCOMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2,
    HL_STRING,
    HL_NUMBER,
    HL_MATCH
};

#endif /* HIGHLIGHT_H */