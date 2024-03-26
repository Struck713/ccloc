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
} LanguageCounts;

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

// TODO: add more languages and multi-extension support support per
#define SUPPORTED_LANGUAGES 31
const Language LANGUAGES[SUPPORTED_LANGUAGES] = {
    { "Assembly", "asm" },
    { "Bourne shell", "sh" },
    { "C", "c" },
    { "C/C++ Header", "h" },
    { "C#", "cs" },
    { "C++", "cpp" },
    { "CMake", "cmake" },
    { "CSS", "css" },
    { "D", "d" },
    { "Dart", "dart" },
    { "Docker", "docker" },
    { "Go", "go" },
    { "Groovy", "groovy" },
    { "Haskell", "hs" },
    { "HTML", "html" },
    { "Jai", "jai" },
    { "Java", "java" },
    { "JavaScript", "js" },
    { "JSON", "json" },
    { "JSX", "jsx" },
    { "Lua", "lua" },
    { "Makefile", "Makefile" },
    { "Markdown", "md" },
    { "Perl", "perl" },
    { "PHP", "php" },
    { "Plain Text", "txt" },
    { "Python", "py" },
    { "R", "r" },
    { "Rust", "rs" },
    { "SQL", "sql" },
    { "TypeScript", "ts" }
};

void* batch_process(void* arg);
void listdir(char*, Files*);
char* fstrcat(char*, char*);

#endif