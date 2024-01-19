#include "highlighting/hl.h"

#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "src/row/erow.h"
#include "src/highlighting/hlhelpers.h"
#include "src/highlighting/hldb.h"
#include "src/system/config.h"

int is_separator(int c)
{
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void updateSyntax(erow *row)
{
    row->highlight = realloc(row->highlight, row->rendersize);
    memset(row->highlight, HL_NORMAL, row->rendersize);
    
    if (E.syntax == NULL) return;
    
    char **keywords = E.syntax->;

    const char *scs = E.syntax->singleline_comment_start;
    const char *mcs = E.syntax->multiline_comment_start;
    const char *mce = E.syntax->multiline_comment_end;

    const uint32_t scs_len = scs ? strlen(scs) : 0;
    const uint32_t mcs_len = mcs ? strlen(mcs) : 0;
    const uint32_t mce_len = mce ? strlen(mcs) : 0;

    int prev_separator = 1, in_string = 0, in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);
    int i = 0;

    while (i < row->rendersize) {
        const char symbol = row->render[i];
        unsigned char prev_highlight = (i > 0) ? row->highlight[i - 1] : HL_NORMAL;

        if (scs_len && !in_string && !in_comment)  {
            if (!strncmp(&row->render[i], scs, scs_len))    {
                memset(&row->highlight[i], HL_COMMENT, row->rendersize - i);
                break;
            }
        }

        if (mcs_len && mce_len && !in_string)   {
            if (in_comment) {
                row->highlight[i] = HL_MLCOMMENT;
                if (!strncmp(&row->render[i], mce, mce_len))    {
                    memset(&row->highlight[i], HL_MLCOMMENT, mce_len);
                    i += mce_len;
                    in_comment = 0;
                    prev_separator = 1;
                    continue;
                } else {
                    i++;
                    continue;
                }
            } else if (!strncmp(&row->render[i], mcs, mcs_len)) {
                memset(&row->highlight[i], HL_MLCOMMENT, mcs_len);
                i += mcs_len;
                in_comment = 1;
                continue;
            }
        }

        if (E.syntax->flags && HL_HIGHLIGHT_STRINGS)    {
            if (in_string)  {
                row->highlight[i] = HL_STRING;

                if (symbol == '\\' && i + 1 < row->rendersize)   {
                    row->highlight[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }

                if (symbol == in_string) in_string = 0;

                prev_separator = 1;
                i++;
                continue;
            } else {
                if (symbol == '"' || symbol == '\\')   {
                    in_string = symbol;
                    row->highlight[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }

        if (E.syntax->flags && HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(symbol) && (prev_separator || prev_highlight == HL_NUMBER)) || (symbol == '.' && prev_highlight == HL_NUMBER))   {
                row->highlight[i] = HL_NUMBER;
                prev_separator = 0;
                i++;
                continue;
            }
        }

        if (prev_separator) {
            uint16_t j = 0;

            for (; keywords[j]; ++j)  {
                int32_t keyword_len = strlen(keywords[j]);
                const int32_t is_keyword2 = keywords[j][keyword_len - 1] == '|';

                if (is_keyword2) keyword_len--;

                if (!strncmp(&row->render[i], keywords[j], keyword_len) && is_separator(row->render[i + keyword_len]))  {
                    memset(&row->highlight[i], is_keyword2 ? HL_KEYWORD2 : HL_KEYWORD1, keyword_len);
                    i += keyword_len;
                    break;
                }
            }

            if (keywords[j] != NULL)    {
                prev_separator = 0;
                continue;
            }
        }

        prev_separator = is_separator(symbol);
        i++;
    }

    int changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;
    if (changed && row->idx + 1 < E.rowsnum)
        updateSyntax(&E.row[row->idx + 1]);
}

int mapSyntaxToColor(const int highlight)
{
    switch (highlight)  {
        case HL_COMMENT:
        case HL_MLCOMMENT: return 244;
        case HL_KEYWORD1: return 202;
        case HL_KEYWORD2: return 202;
        case HL_NUMBER: return 251; //TODO: 8-bit colors!!!
        case HL_MATCH: return 196;
        case HL_STRING: return 70;
        default: return 37;
    }
}

void selectSyntaxHighlight()
{
    E.syntax = NULL;
    if (E.filename == NULL) return;

    char *filetype = strchr(E.filename, '.');

    for (uint32_t j = 0; j < HLDB_ENTRIES; ++j) {
        struct editorSyntax *s = &HLDB[j];
        uint32_t i = 0;

        while (s->filematch[i]) {
            int is_filetype = (s->filematch[i][0] == '.');
            if ((is_filetype && filetype && !strcmp(filetype, s->filematch[i])) || (!is_filetype && strstr(E.filename, s->filematch[i])))   {
                E.syntax = s;

                for (int32_t filerow = 0; filerow < E.rowsnum; ++filerow)   {
                    updateSyntax(&E.row[filerow]);
                }

                return;
            }

            i++;
        }
    }
}