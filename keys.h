#ifndef ARROWKEYS_H
#define ARROWKEYS_H

#define CTRL_KEY(k) ((k) & 0x1f)

enum arrowsKeys {
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DELETE_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

#endif /* ARROWKEYS_H */