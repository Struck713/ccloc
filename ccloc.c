
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <string.h>
#include <ctype.h>

#define FILE_BUF_SIZE 65536
#define STRING_BUF_SIZE 300
#define EXTENSION_BUF_SIZE 10
#define DIR_STACK_SIZE 15
#define THREAD_POOL_SIZE 5

typedef struct {
    char* name;
    char* extension;
} Language;

typedef struct {
    int language;
    int lines;
    int code;
    int comment;
    int blank;
    int files;
} LanguageCount;

typedef struct {
    LanguageCount* items;
    int count;
    int capacity;
} LanguageCounts;

typedef struct {
    char** items;
    int count;
    int capacity;
} Files;

typedef struct {
    DIR* dir;
    char* path;
} Directory;

const Language languages[] = {
    { "TypeScript", "ts" },
    { "JavaScript", "js" },
    { "Java", "java" },
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
    { NULL, NULL },
};

void* process_file();
void listdir(char*, Files*);
char* fstrcat(char*, char*);

int main(int argc, char** argv) {

    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        printf("ERROR: Please provide a file path.\n");
        return 1;
    }

    // put this on the heap for later
    char* path_arg = argv[1];
    int path_arg_size = strlen(path_arg);
    char* root_path = (char*) malloc(path_arg_size + 1);
    strncpy(root_path, path_arg, path_arg_size);
    root_path[path_arg_size] = 0;

    LanguageCounts counts;
    counts.capacity = 5;
    counts.count = 0;
    counts.items = (LanguageCount*) malloc(sizeof(LanguageCount) * counts.capacity);

    Files files;
    files.capacity = 10;
    files.count = 0;
    files.items = (char**) malloc(sizeof(char*) * files.capacity);

    listdir(root_path, &files);

    int totalFiles = files.count, totalLines = 0, totalComment = 0, totalBlank = 0, totalCode = 0;
    printf("----------------------------------------------------------------------------------------\n");
    printf("%-16s%-16s%-16s%-16s%-16s%-16s\n", "Language", "Files", "Lines", "Blank", "Comment", "Code");
    printf("----------------------------------------------------------------------------------------\n");
    for (int i = 0; i < counts.count; i++) {
        LanguageCount count = counts.items[i];
        int code = count.code - count.comment;
        printf("%-16s%-16d%-16d%-16d%-16d%-16d\n", languages[count.language].name, count.files, count.lines, count.blank, count.comment, code);
        totalFiles += count.files;
        totalLines += count.lines;
        totalComment += count.comment;
        totalBlank += count.blank;
        totalCode += code;
    }
    printf("----------------------------------------------------------------------------------------\n");
    printf("%-16s%-16d%-16d%-16d%-16d%-16d\n", "Total", totalFiles, totalLines, totalBlank, totalComment,  totalCode);
    printf("----------------------------------------------------------------------------------------\n");

    free(counts.items);
    for (int i = 0; i < files.count; i++) 
        free(files.items[i]);
    free(files.items);
}

void listdir(char* root_path, Files* files) {
    
    DIR* root = opendir(root_path);
    if (root == NULL) {
        printf("ERROR: Failed to open directory, %s\n", root_path);
        return;
    }

    char buffer[STRING_BUF_SIZE] = "";
    int index = 0;
    Directory stack[DIR_STACK_SIZE];
    stack[index] = (Directory){ root, root_path };

    while (index >= 0) {
        struct dirent* entry;
        while ((entry = readdir(stack[index].dir))) {

            char* file_name = entry->d_name;
            if (file_name[0] == '.') continue;

            buffer[0] = 0;
            char* current = buffer;
            current = fstrcat(current, stack[index].path);
            current = fstrcat(current, "/");
            current = fstrcat(current, file_name);
            int buffer_size = current - buffer;

            char* path = (char*) malloc(buffer_size + 1);
            strncpy(path, buffer, buffer_size);
            path[buffer_size] = 0;

            // Handle directory, open and push it to the stack
            if (entry->d_type == DT_DIR) {
                
                DIR* dir = opendir(path); 
                if (dir == NULL) {
                    free(path);
                    continue;
                }

                stack[++index] = (Directory){ dir, path };
                continue;
            }

            // append file
            if (files->count + 1 >= files->capacity) {
                files->capacity *= 2;
                
                char** temp = (char**) malloc(sizeof(char*) * files->capacity);
                for (int i = 0; i < files->count; ++i) {
                    temp[i] = files->items[i];
                }
                free(files->items);
                files->items = temp;
            }
            
            files->items[files->count] = path;
            files->count++;
        }

        free(stack[index].path);
        closedir(stack[index].dir);
        --index;
    }
}

void* process_file(char* file_name, LanguageCounts* counts) {
    char buffer[STRING_BUF_SIZE] = "";
    // Get file extension
    int dot = 0;
    int length = 0;
    for (; file_name[length] != 0; length++) {
        if (file_name[length] == '.')
            dot = length;
    }
    
    char extension[EXTENSION_BUF_SIZE];
    if (dot != 0) {
        for (int i = dot; i < length; i++) extension[i - dot] = file_name[i + 1];
    } else {
        for (int i = dot; i < length; i++) extension[i] = file_name[i];
    }
    int extension_size = length - dot;
    extension[extension_size] = 0;

    // Get language from list of languages
    int language = -1;
    for (int i =  0; languages[i].extension; i++) {
        if (strncmp(languages[i].extension, extension, extension_size) == 0) {
            language = i;
            break;
        }
    }

    // printf("%s => %s\n", language == -1 ? "NONE" : languages[language].name, file_name);

    // Not a language. Skip it.
    if (language == -1) return NULL;

    // Check out dynamic array for the language count
    LanguageCount* count = NULL;
    for (int i = 0; i < counts->count; i++) {
        if (counts->items[i].language == language) {
            count = (counts->items + i);
        }
    }

    // Open file
    FILE* file = fopen(buffer, "r");
    if (file == NULL) return NULL;

    // If we don't find it, create a new one
    // we also might have to expand our dynamic 
    // array.

    if (!count) {
        if (counts->count + 1 >= counts->capacity) {
            counts->capacity *= 2;
            LanguageCount* temp = (LanguageCount*) malloc(sizeof(LanguageCount) * counts->capacity);
            for (int i = 0; i < counts->count; ++i) {
                LanguageCount item = counts->items[i];
                temp[i] = (LanguageCount) {
                    item.language,
                    item.lines,
                    item.code,
                    item.comment,
                    item.blank,
                    item.files
                };
            }
            free(counts->items);
            counts->items = temp;
        }
        counts->items[counts->count] = (LanguageCount){ language, 0, 0, 0, 0 };
        count = counts->items + counts->count;
        counts->count++;
    }

    count->files++;

    // Read all the lines out
    char fileBuf[FILE_BUF_SIZE] = { 0 };

    int whitespaceRead = 0;
    int charactersRead = 0;
    int comment = 0;
    while (fread(fileBuf, sizeof(char), FILE_BUF_SIZE, file) > 0) {
        for (int i = 0 ;; i++) {
            char current = fileBuf[i];
            if (current == '\n' || current == 0) {
                whitespaceRead == charactersRead ? count->blank++ : count->code++;
                count->lines++;

                whitespaceRead = 0;
                charactersRead = 0;
                if (current == 0) break;
                continue;
            }

            if (charactersRead - whitespaceRead == 0) {
                char next = fileBuf[i + 1];
                if ((current == '/' && next == '/') || (current == '#' && next == ' ')) count->comment++;
                // else if ((current == '/' && next == '*') && comment == 0) {
                //     count->comment++;
                //     comment++;
                // } else if ((current == '*' && next == '/') && comment == 1) {
                //     count->comment++;
                //     comment--;
                // } else if (comment == 0) {
                //     count->comment++;
                // }
            }

            if (isblank(current)) {
                whitespaceRead++;
            }
            charactersRead++;
        }   
    }

    fclose(file);
    return NULL;
};

// https://stackoverflow.com/questions/21880730/c-what-is-the-best-and-fastest-way-to-concatenate-strings
char* fstrcat( char* dest, char* src ) {
     while (*dest) dest++;
     while ((*dest++ = *src++));
     return --dest;
}