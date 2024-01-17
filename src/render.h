#ifndef RENDER_H
#define RENDER_H

void scroll();

void renderRows(struct stringbuf *buf);

void renderStatusBar(struct stringbuf *buf);

void renderMessageBar(struct stringbuf *buf);

void refreshScreen(void);

void setStatusMessage(const char *fmt, ...);

#endif /* RENDER_H */