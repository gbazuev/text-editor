#ifndef RENDER_H
#define RENDER_H

#include "renderbuf.h"

void scroll(void);

void renderCurrentRow(struct renderbuf *buf);

//void renderRows(struct renderbuf *buf); TODO

void renderStatusBar(struct renderbuf *buf);

void renderMessageBar(struct renderbuf *buf);

void refreshScreen(void);

void setStatusMessage(const char *fmt, ...);

#endif /* RENDER_H */
