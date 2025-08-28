
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
    char eof;
} IOFile;

typedef struct DocElmProperties
{
    unsigned int font_size;
} DocElmProperties;


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
    if(file->loc >= file->size)
    {
        file->eof = 1;
        return 0;
    }

    while(file->loc < file->size && file->content[file->loc] != ']'){
        sb_add(sb, file->content[file->loc])
        file->loc++;
    }
    if(file->loc >= file->size)  {
        file->eof = 1;
        return 0;
    };
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
    if(fread(input.content, sizeof(char), input.size,input.fd) <= 0){
        printf("Error on reading input file!");
        return 0;
    }


    IOFile output = {0};
    output.fd = fopen(output_file, "w");
    if(!output.fd) {
        printf("Couldn't open output file %s\n", output_file);
        return 1;
    }

    //gen boilerplate
    fwrite("<!DOCTYPE html>\n",sizeof(char), 16, output.fd);
    fwrite("<html lang=\"en\">\n",sizeof(char), 17, output.fd);
    fwrite("<head>\n",sizeof(char), 7, output.fd);
    fwrite("<meta charset=\"UTF-8\">\n",sizeof(char), 23, output.fd);
    fwrite("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n",sizeof(char), 71, output.fd);
    
    
    DocElmProperties default_prop = {
        .font_size = 12
    };
    
    StringBuilder sb = {0};
    char title_found = 0;
    while(get_next_token(&input, &sb)){
        if(strcmp(sb.data, document_elements[ELM_TITLE]) == 0) {
            title_found = 1;
            break;
        }
        sb.count = 0;
    }
    if(input.eof || !title_found){
        printf("Expected a title but coundl't find one!");
        return 1;
    }

    //gen title
    fwrite("<title>",sizeof(char), 7, output.fd);
    fwrite(sb.data,sizeof(char), sb.count-1, output.fd);
    //gen boilerplate
    fwrite("</title>\n",sizeof(char), 9, output.fd);
    fwrite("</head>\n<body>\n",sizeof(char), 15, output.fd);
    fwrite("</body>\n</html>",sizeof(char), 15, output.fd);
    
}