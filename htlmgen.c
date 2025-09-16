
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

typedef struct DocElmProperties
{
    unsigned int font_size;
} DocElmProperties;


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

typedef struct Lexer
{
    char* content;
    Token* peeked_token; //maybe later
    size_t input_size;
    size_t loc;
    FILE* fd;
    char eof;
    char future_token_type;
} Lexer;


enum TOKEN_TYPES {
    TYPE_TEXT,
    TYPE_ELEMENT,
    TYPE_PROPERTY,
    TYPE_COUNT
};

const char* token_type_strings[] = {"TEXT", "ELEMENT", "PROPERTY"};
int get_next_token(Lexer* lexer, Token* token);
int peek_next_token(Lexer* lexer, Token* token);



size_t get_file_size(FILE* fd){
    if(!fd) return 0;
    fseek(fd, 0, SEEK_END);
    size_t res = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    return res;
}

int is_alpha_num(char c){
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z' ) || (c >= '0' && c <= '9'));
}

int is_control_char(char c){
    return (c <= 0x1F);
}
int peek_next_token(Lexer* lexer, Token* token){
    Lexer backup = *lexer;
    int res = get_next_token(lexer, token);
    *lexer = backup;
    // lexer->peeked_token = token;
    return res;
}

int get_next_token(Lexer* lexer, Token* token){

    // if (lexer->peeked_token != NULL)
    // {
    //    token = lexer->peeked_token;
    //    lexer->peeked_token = NULL;
    //    return 1;
    // }
    
    //todo while loop doesn't detect #[ to exit
    token->type = lexer->future_token_type;
    token->sb->count = 0;
    char is_new_elem = 0;
    while (lexer->loc < lexer->input_size)
    {
        if(is_control_char(lexer->content[lexer->loc])){
            lexer->loc++;
            continue;
        }
        if (lexer->content[lexer->loc] == '#' && !is_new_elem)
        {
            is_new_elem = 1;
            lexer->loc++;
            continue;
        }
        
        if (is_new_elem)
        {
            is_new_elem = 0;
            if(lexer->content[lexer->loc] == '['){
                lexer->loc++;
                if(token->sb->count > 0){
                    token->sb->data[token->sb->count] = '\0';
                    lexer->future_token_type = TYPE_ELEMENT;
                    
                    return 1;
                }
                token->type = TYPE_ELEMENT;
            }else{
                //'#' was not an identifier, add the previous #
                sb_add(token->sb, lexer->content[lexer->loc-1])
            }
        }
        if((token->type == TYPE_ELEMENT || token->type == TYPE_PROPERTY) && lexer->content[lexer->loc] == '|'){
            lexer->loc++;
            lexer->future_token_type = TYPE_PROPERTY;
            token->sb->data[token->sb->count] = '\0';
            return 1;
        }

        if((token->type == TYPE_ELEMENT) && lexer->content[lexer->loc] == '+'){
            if(token->sb->count > 0){
                lexer->future_token_type = TYPE_ELEMENT;
                token->sb->data[token->sb->count] = '\0';
                return 1;
            }
        }
        if((token->type == TYPE_ELEMENT || token->type == TYPE_PROPERTY) && lexer->content[lexer->loc] == ']'){
            if (strncmp("styles", token->sb->data, token->sb->count) == 0)
            {
                // mdoe style
            }
            
            lexer->loc++;
            lexer->future_token_type = 0;
            token->sb->data[token->sb->count] = '\0';
            return 1;
        }
        sb_add(token->sb, lexer->content[lexer->loc++]);

        
        
    }
    token->sb->data[token->sb->count] = '\0';
    // get last token
    if(token->sb->count > 0){ 
        return 1;
    }

    lexer->eof = 1;
    return 0;
};

int main() {
    const char* input_file = "./text.txt";
    const char* output_file = "./out.html";
    Lexer lexer = {0};
    lexer.fd = fopen(input_file, "r");
    if(!lexer.fd){
        printf("Couldn't open input file %s\n", input_file);
        return 1;
    }
    lexer.input_size = get_file_size(lexer.fd);
    lexer.content = (char*) malloc(sizeof(char) * lexer.input_size);
    if(fread(lexer.content, sizeof(char), lexer.input_size,lexer.fd) <= 0){
        printf("Error on reading input file!");
        return 0;
    }


    FILE* output = fopen(output_file, "w");
    if(!output) {
        printf("Couldn't open output file %s\n", output_file);
        return 1;
    }
    
    //gen boilerplate
    fwrite("<!DOCTYPE html>\n",sizeof(char), 16, output);
    fwrite("<html lang=\"en\">\n",sizeof(char), 17, output);
    fwrite("<head>\n",sizeof(char), 7, output);
    fwrite("<meta charset=\"UTF-8\">\n",sizeof(char), 23, output);
    fwrite("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n",sizeof(char), 71, output);
    // TODO handle title
    fwrite("</head>\n",sizeof(char), 8, output);
    fwrite("<body>\n",sizeof(char), 7, output);
    
    DocElmProperties default_prop = {
        .font_size = 12
    };
    
    StringBuilder sb = {0};
    Token token = {
        .sb = &sb,
        .type = 0,
    };
    int span_count = 0;
    while(get_next_token(&lexer, &token)){

        printf("token: %s | type: %s \n", token.sb->data, token.type > TYPE_COUNT ? "UNKNOWN": token_type_strings[token.type]);
        switch (token.type)
        {
            case TYPE_ELEMENT:
            // + mode : span generation
            if(token.sb->data[0] == '+'){
                char class_name[20]; 
                snprintf(class_name, token.sb->count, "%s", token.sb->data+1 ); // +1 to skip the + char
                fwrite("<span class=\"", sizeof(char), 13, output);
                fwrite(class_name, sizeof(char), token.sb->count-1, output);
                StringBuilder peeksb = {0};
                Token peek = { .sb = &peeksb, .type = 0};
                while (peek_next_token(&lexer, &peek) && peek.sb->data[0] == '+')
                {
                     
                    if(!get_next_token(&lexer, &token)) break;

                    snprintf(class_name, token.sb->count+2, ", %s", token.sb->data+1 ); // +1 to skip the + char
                    fwrite(class_name, sizeof(char), token.sb->count+1, output); // count - 1 + 2 to strip \0 and take ", " into account
                }
                fwrite("\">\n", sizeof(char), 3, output);
                span_count++;
            }
            else {
                while (span_count > 0)
                {
                    span_count--;
                    fwrite("</span>\n", sizeof(char), 8, output);
                }
                
            }
            // normal mode, div generation
            break;
        
        default:
            break;
        }
    
    } 
    // end any remaining unclosed spans
    while (span_count > 0)
    {
        span_count--;
        fwrite("</span>\n", sizeof(char), 8, output);
    }

    fwrite("</body>\n",sizeof(char), 8, output);
    fwrite("</html>\n",sizeof(char), 8, output);

    
}