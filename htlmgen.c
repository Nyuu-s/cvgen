
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define sb_add(sb, c)\
do{\
    if(sb->capacity <= sb->count){\
        if(sb->capacity == 0) sb->capacity = 64;\
        sb->capacity *= 2;\
        sb->data = realloc(sb->data, sb->capacity * sizeof(*sb->data));\
    }\
    sb->data[sb->count++] = c;\
} while (0);

typedef struct IOFile {
    char* content;
    size_t size;
    size_t loc;
    FILE* fd;
} IOFile;

typedef struct StringBuilder {
    char* data;
    size_t capacity;
    size_t count;
} StringBuilder;

enum DOCELM {
    ELM_TITLE,
    ELM_SECTION,
    ELM_PROPERTIES,
    ELM_COUNT
};

const char* document_elements[] = {"#[title]", "#[section]", "#[prop]"};

size_t get_file_size(FILE* fd){
    if(!fd) return 0;
    fseek(fd, 0, SEEK_END);
    size_t res = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    return res;
}
// todo pass sb as pointer
int get_next_token(IOFile* file, StringBuilder* sb){

    //todo while loop doesn't detect #[ to exit
    while (file->loc < file->size && !(file->loc+1 < file->size && file->content[file->loc] == '#' && file->content[file->loc+1] == '[')){
        file->loc++;
    }
    if(file->loc >= file->size) return 0;

    while(file->loc < file->size && file->content[file->loc] != ']'){
        sb_add(sb, file->content[file->loc])
        file->loc++;
    }
    if(file->loc >= file->size) return 0;
    sb_add(sb, file->content[file->loc++]);
    sb_add(sb, '\0');
    printf("token: %s\n", sb->data);
    return 1;
    

};

int main() {
    const char* input_file = "./text.txt";
    const char* output_file = "./out.html";
    IOFile input = {0};
    input.fd = fopen(input_file, "r");
    if(!input.fd){
        printf("Couldn't open input file %s\n", input_file);
        return 1;
    }
    input.size = get_file_size(input.fd);
    input.content = (char*) malloc(sizeof(char) * input.size);
    if(fread(input.content, input.size, sizeof(char), input.fd) <= 0){
        printf("Error on reading input file!");
        return 0;
    }


    IOFile output = {0};
    output.fd = fopen(output_file, "w");
    if(!output.fd) {
        printf("Couldn't open output file %s\n", output_file);
        return 1;
    }

    StringBuilder sb = {0};
    while(get_next_token(&input, &sb)){
        // do stuff with token value in sb
        if(strcmp(sb.data, document_elements[ELM_TITLE]) == 0){
            printf("FOUND TITLE");
            break;
        }
    }

}