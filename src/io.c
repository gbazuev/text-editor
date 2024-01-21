#include "io.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "system/config.h"
#include "highlighting/hl.h"
#include "terminal/terminal.h"
#include "render.h"
#include "input.h"

char *convertRowsToSingleString(int32_t *buflen)
{
    int32_t totallen = 0, j;
    for (j = 0; j < E.rowsnum; ++j) {
        totallen += E.row[j].size + 1;
    }
    *buflen = totallen;

    char *buf = malloc(totallen);
    char *p = buf;
    for (j = 0; j < E.rowsnum; ++j) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }

    return buf;
}

void openFile(const char *filename)
{   
    free(E.filename);
    E.filename = strdup(filename);

    selectSyntaxHighlight();

    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecap, fp)) != -1)  {
        while ((linelen > 0 && (line[linelen - 1] == '\n')) || (line[linelen - 1] == '\r')) {
            linelen--;
        }
        insertRow(E.rowsnum, line, linelen);
    }

    free(line);
    fclose(fp);
    E.dirty = 0;
}

void saveFile()
{
    if (E.filename == NULL) {
        E.filename = prompt("Save as: %s (ESC to cancel)", NULL);
        if (E.filename == NULL) {
            setStatusMessage("Save aborted!");
            return;
        }

        selectSyntaxHighlight();
    }

    int len;
    char *buf = convertRowsToSingleString(&len);

    const int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    if (fd != -1)   {
        if (ftruncate(fd, len) != -1)   {
           if (write(fd, buf, len) == len)  {
            close(fd);
            free(buf);
            E.dirty = 0;
            setStatusMessage("%d bytes written to disk", len);
            return;
           }
        }

        close(fd);
    }

    free(buf);
    setStatusMessage("I/O Error: %s", strerror(errno));
}