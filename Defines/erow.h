typedef struct erow {
    int idx;
    int size;
    int rendersize;
    char *chars;
    char *render;
    char *highlight;
    int hl_open_comment;
} erow;