//
// Created by sheverdin on 10/4/23.
//

#ifndef DM_CLIENT_PARSING_H
#define DM_CLIENT_PARSING_H

#include "dm_mainHeader.h"

#define DM_TOK_BUFSIZE 64

FILE *openPipe(const char *cmdStr);
void closePipe(FILE *pipe);

int split_line(char *line, const char* delim, splited_line_t *splitedLine);
void replaceSymbols(char *str, char oldSym, char newSym);
void removeCharacter(char *str, char removeChar);
void getDeviceType(search_out_msg_t *searchOutMsg, char *modelType);
void extractValue(const char* str, char *extractedStr);


#endif //DM_CLIENT_PARSING_H
