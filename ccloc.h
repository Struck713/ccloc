#include <dirent.h>

#ifndef CCLOC_H_
#define CCLOC_H_

typedef struct {
    char* name;
    char* extension;
} Language;

typedef struct {
    DIR* dir;
    char* path;
} Directory;

typedef struct {
    int language;
    int lines;
    int code;
    int comment;
    int blank;
    int files;
} LanguageCount;

typedef struct {
    char** items;
    int count;
    int capacity;
} Files;

typedef struct {
    Files* files;
    int start;
    int end;
} ThreadArgs;

#define SUPPORTED_LANGUAGES 17
const Language LANGUAGES[SUPPORTED_LANGUAGES] = {
    { "TypeScript", "ts" },
    { "JavaScript", "js" },
    { "Java", "java" },
    { "C", "c" },
    { "Go", "go" },
    { "D", "d" },
    { "C", "c" },
    { "C/C++ Header", "h" },
    { "C++", "cpp" },
    { "Python", "py" },
    { "Perl", "perl" },
    { "CSS", "css" },
    { "Plain Text", "txt" },
    { "Markdown", "md" },
    { "Bourne shell", "sh" },
    { "Assembly", "asm" },
    { "R", "r" },
    { "Rust", "rs" },
    { "Makefile", "Makefile" },
    { "SQL", "sql" },
};

void* batch_process(void* arg);
void listdir(char*, Files*);
char* fstrcat(char*, char*);

#endif