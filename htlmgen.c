
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
    char future_token_type;
} IOFile;

typedef struct StringBuilder {
    char* data;
    size_t capacity;
    size_t count;
} StringBuilder;

typedef struct Token
{
    StringBuilder* sb;
    char type;
    } Token;


enum TOKEN_TYPES {
    TYPE_TEXT,
    TYPE_ELEMENT,
    TYPE_PROPERTY,
    TYPE_COUNT
};

const char* token_type_strings[] = {"TEXT", "ELEMENT", "PROPERTY"};

size_t get_file_size(FILE* fd){
    if(!fd) return 0;
    fseek(fd, 0, SEEK_END);
    size_t res = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    return res;
}
// todo pass sb as pointer
int get_next_token(IOFile* file, Token* token){

    //todo while loop doesn't detect #[ to exit
    token->type = file->future_token_type;
    token->sb->count = 0;
    char is_new_elem = 0;
    while (file->loc < file->size)
    {
        if (file->content[file->loc] == '#' && !is_new_elem)
        {
            is_new_elem = 1;
            file->loc++;
            continue;
        }
        
        if (is_new_elem)
        {
            is_new_elem = 0;
            if(file->content[file->loc] == '['){
                file->loc++;
                if(token->sb->count > 0){
                    token->sb->data[token->sb->count] = '\0';
                    file->future_token_type = TYPE_ELEMENT;
                    
                    return 1;
                }
                token->type = TYPE_ELEMENT;
            }else{
                //'#' was not an identifier, add the previous #
                sb_add(token->sb, file->content[file->loc-1])
            }
        }

        if((token->type == TYPE_ELEMENT || token->type == TYPE_PROPERTY) && file->content[file->loc] == '|'){
            file->loc++;
            file->future_token_type = TYPE_PROPERTY;
            token->sb->data[token->sb->count] = '\0';
            return 1;
        }

        if((token->type == TYPE_ELEMENT || token->type == TYPE_PROPERTY) && file->content[file->loc] == ']'){
            file->loc++;
            file->future_token_type = 0;
            token->sb->data[token->sb->count] = '\0';
            return 1;
        }
        sb_add(token->sb, file->content[file->loc++]);

        
        
    }
    
    return 0;
    // while (file->loc < file->size && !(file->loc+1 < file->size && file->content[file->loc] == '#' && file->content[file->loc+1] == '[')){
    //     file->loc++;
    // }
    // if(file->loc >= file->size) return 0;

    // while(file->loc < file->size && file->content[file->loc] != ']'){
    //     sb_add(sb, file->content[file->loc])
    //     file->loc++;
    // }
    // if(file->loc >= file->size) return 0;
    // sb_add(sb, file->content[file->loc++]);
    // sb_add(sb, '\0');
    // printf("token: %s\n", sb->data);
    // return 1;
    

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
    Token token = {
        .sb = &sb,
        .type = 0,
    };
    while(get_next_token(&input, &token)){
        // do stuff with token value in sb
        printf("token: %s | type: %s\n", token.sb->data, token.type > TYPE_COUNT ? "UNKNOWN": token_type_strings[token.type]);
    }

}