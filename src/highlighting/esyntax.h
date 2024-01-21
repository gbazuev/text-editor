#ifndef SYNTAX_H
#define SYNTAX_H

#include <stdint.h>

struct esyntax {
    char *filetype;
    char **filematch;
    char **keywords;
    char *singleline_comment_start;
    char *multiline_comment_start;
    char *multiline_comment_end;
    int32_t flags;
};

#endif /* SYNTAX_H */