
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "ccloc.h"

#define FILE_BUF_SIZE 65536 // Something from SOF
#define STRING_BUF_SIZE 4097 // Max file path in EXT4 + 1
#define EXTENSION_BUF_SIZE 10 // We realistically shouldn't have a long extension
#define DIR_STACK_SIZE 15 // This never goes above like 8 in testing
#define THREAD_POOL_SIZE 16 // I just want 10 threads

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

    Files files;
    files.capacity = 10;
    files.count = 0;
    files.items = (char**) malloc(sizeof(char*) * files.capacity);
    listdir(root_path, &files);

    int filesLeft = files.count;
    int batchSize = filesLeft / THREAD_POOL_SIZE;
    batchSize = batchSize <= 0 ? filesLeft : batchSize;

    // TODO: dynamically allocate thread pool based on thread count avaliable
    pthread_t threads[THREAD_POOL_SIZE];
    for (int thread = 0; thread < THREAD_POOL_SIZE; ++thread) { 
        ThreadArgs* args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
        int start = filesLeft - batchSize;
        if (start < batchSize) start = 0; 
        *args = (ThreadArgs) { &files, start, filesLeft };
        filesLeft -= (filesLeft - start);
        pthread_create(&threads[thread], NULL, batch_process, (void*)args);
    }

    LanguageCount sumCounts[SUPPORTED_LANGUAGES];

    for (int i = 0; i < SUPPORTED_LANGUAGES; ++i) sumCounts[i] = (LanguageCount) { i , 0, 0, 0, 0, 0 };
    for (int thread = 0; thread < THREAD_POOL_SIZE; ++thread) {
        LanguageCount** counts;
        pthread_join(threads[thread], (void*) &counts);
        for (int language = 0; language < SUPPORTED_LANGUAGES; ++language) {
            LanguageCount* count = counts[language];
            if (count) {
                sumCounts[language].code += count->code;
                sumCounts[language].comment += count->comment;
                sumCounts[language].blank += count->blank;
                sumCounts[language].files += count->files;
                sumCounts[language].lines += count->lines;
                free(count);
            }
        }
        free(counts);
    }

    for (int i = 0; i < files.count; i++) 
        free(files.items[i]);
    free(files.items);

    int totalFiles = 0, totalLines = 0, totalComment = 0, totalBlank = 0, totalCode = 0;
    printf("----------------------------------------------------------------------------------------\n");
    printf(" %-16s%-16s%-16s%-16s%-16s%-16s\n", "Language", "Files", "Lines", "Blank", "Comment", "Code");
    printf("----------------------------------------------------------------------------------------\n");
    for (int i = 0; i < SUPPORTED_LANGUAGES; i++) {
        LanguageCount count = sumCounts[i];
        if (count.files > 0) {
            int code = count.code - count.comment;
            printf(" %-16s%-16d%-16d%-16d%-16d%-16d\n", LANGUAGES[count.language].name, count.files, count.lines, count.blank, count.comment, code);
            totalFiles += count.files;
            totalLines += count.lines;
            totalComment += count.comment;
            totalBlank += count.blank;
            totalCode += code;
        }
    }
    printf("----------------------------------------------------------------------------------------\n");
    printf(" %-16s%-16d%-16d%-16d%-16d%-16d\n", "Total", totalFiles, totalLines, totalBlank, totalComment,  totalCode);
    printf("----------------------------------------------------------------------------------------\n");

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
                for (int i = 0; i < files->count; ++i) 
                    temp[i] = files->items[i];
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

void* batch_process(void* hargs) {

    ThreadArgs args = *(ThreadArgs*) hargs;
    Files* files = args.files;
    char fileBuf[FILE_BUF_SIZE] = {0};

    LanguageCount** counts = (LanguageCount**) malloc(sizeof(LanguageCount*) * SUPPORTED_LANGUAGES);
    for (int i = 0; i < SUPPORTED_LANGUAGES; ++i) counts[i] = NULL;

    for (int i = args.start; i < args.end; ++i) {
        char* file_name = files->items[i];

        // Get file extension
        char* last_slash = strrchr(file_name, '/');
        if (last_slash == NULL) last_slash = file_name;
        else last_slash += 1;

        char* extension = strrchr(last_slash, '.');
        if (extension == NULL) extension = last_slash;
        else extension += 1;
        int extension_size = strlen(extension);

        // Get language from list of languages
        int language = -1;
        for (int j = 0; j < SUPPORTED_LANGUAGES; ++j) {
            if (strncmp(LANGUAGES[j].extension, extension, extension_size) == 0) {
                language = j;
                break;
            }
        }
        if (language == -1) continue; // Not a language. Skip it.

        LanguageCount* count = counts[language];
        if (!count) {
            count = counts[language] = (LanguageCount*) malloc(sizeof(LanguageCount));
            *count = (LanguageCount) { language, 0, 0, 0, 0, 0 };
        }

        // Open file
        FILE* file = fopen(file_name, "r");
        if (file == NULL) continue;

        // If we don't find it, create a new one
        // we also might have to expand our dynamic 
        // array.
        count->files++;

        int whitespace = 0;
        int characters = 0;
        // int comment = 0;
        int nread = 0;
        while ((nread = fread(fileBuf, sizeof(char), FILE_BUF_SIZE, file)) > 0) {
            for (int i = 0 ; i < nread; i++) {
                char current = fileBuf[i];
                if (current == '\n') {
                    if (whitespace == characters) count->blank++;
                    else count->code++;
                    count->lines++;

                    whitespace = 0;
                    characters = 0;
                    continue;
                }

                if (characters - whitespace == 0) {
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

                if (isblank(current)) whitespace++;
                characters++;
            }   
        }

        fclose(file);
    }

    free(hargs);
    return counts;
}

// https://stackoverflow.com/questions/21880730/c-what-is-the-best-and-fastest-way-to-concatenate-strings
char* fstrcat( char* dest, char* src ) {
     while (*dest) dest++;
     while ((*dest++ = *src++));
     return --dest;
}