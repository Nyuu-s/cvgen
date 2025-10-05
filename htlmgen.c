
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define sb_add(sb, c)\
do{\
    if(sb.capacity <= sb.count){\
        if(sb.capacity == 0) sb.capacity = 64;\
        sb.capacity *= 2;\
        sb.data = realloc(sb.data, sb.capacity * sizeof(*sb.data));\
    }\
    sb.data[sb.count++] = c;\
} while (0);

#define INITIAL_STACK_CAPACITY 100
#define NESTING_THRESHOLD 3
#define NESTING_IGNORED 1

typedef struct DocElmProperties
{
    unsigned int font_size;
} DocElmProperties;



typedef struct Token
{
    union {
        char* value;
        struct {
            char* data;
            size_t capacity;
            size_t count;
        } sb;
    };
    char type;
    
} Token;



typedef struct NestingElement
{
    char* name;
    size_t level;
} NestingElement;

typedef struct NestingStack
{
    NestingElement* elements;
    size_t top;
    size_t capacity;
} NestingStack;


typedef struct Lexer
{
    char* content;
    size_t input_size;
    size_t loc;
    size_t line;
    FILE* fd;
    // StringBuilder* sb;
} Lexer;

// #define TOKEN_START_ELEM "#["
// #define TOKEN_END_ELEM "]"
// #define TOKEN_START_SPANSTYLE "@("
// #define TOKEN_END_SPANSTYLE ")"
// #define TOKEN_ELEM_CLASSSEP "|"
// #define TOKEN_SPANSTYLE_CLASSSEP "+"
// #define TOKEN_SPANSTYLE_CLASSEND ":"
// #define TOKEN_ELEM_ATTR "="

typedef struct ParserState{
    int state;
} ParserState;

enum STATES {
    STATE_TEXT,
    STATE_ELEMENT,
};

enum TOKEN_TYPES {

    TYPE_PUNCT,
    TYPE_SEPARATOR,
    TYPE_IDENTIFIER,
    TYPE_COUNT
};

const char* token_type_strings[] = {
    "PUNCT",
    "SEPARATOR",
    "IDENTIFIER"
    
};
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


size_t get_left_nesting_distance(Lexer* lexer){

    size_t cursor = lexer->loc;
    char is_left_blank = 0;
    while(cursor != 0  )
    {
        cursor--;
        if(lexer->content[cursor] != ' ' && lexer->content[cursor] != '\t'){ 
            // printf("%d hit! stoping ", lexer->content[cursor]);
            //if \n is found then all left must be blank, if any non blank char is found than its not all blank on left
            is_left_blank = lexer->content[cursor] == '\n';
            cursor++;
            // printf("is blank %d\n", is_left_blank);
            break;
        }
    }

    size_t res = ((lexer->loc - cursor) + NESTING_THRESHOLD) & ~NESTING_THRESHOLD;
    return is_left_blank ? res : NESTING_IGNORED;

}
char lexer_read_char(Lexer* lexer){
    if (lexer->loc < lexer->input_size)
    {
        return lexer->content[lexer->loc++];
    }
    return EOF;
};
char lexer_peek_char(Lexer* lexer){
    if (lexer->loc < lexer->input_size)
    {
        return lexer->content[lexer->loc];
    }
    return EOF;
}
int peek_next_token(Lexer* lexer, Token* token){
    Lexer back = *lexer;
    int res = get_next_token(lexer, token);
    *lexer = back;
    return res;
}

int get_next_token(Lexer* lexer, Token* token){

    token->sb.count = 0;
    char currchar = lexer_read_char(lexer);
    while (lexer->loc <= lexer->input_size && currchar != EOF)
    {
        if(currchar == '\n'){
            lexer->line++;
            currchar = lexer_read_char(lexer);
        }

        if(currchar == '#' && lexer_peek_char(lexer) == '['){
            currchar = lexer_read_char(lexer);
            token->type = TYPE_PUNCT;
            sb_add(token->sb, '#');
            sb_add(token->sb, currchar);
            token->sb.data[token->sb.count] = '\0';
            return 1;
        }
        if(currchar == '@' && lexer_peek_char(lexer) == '('){
            currchar = lexer_read_char(lexer);
            token->type = TYPE_PUNCT;
            sb_add(token->sb, '@');
            sb_add(token->sb, currchar);
            token->sb.data[token->sb.count] = '\0';
            return 1;
        }
        if(currchar == ']' || currchar == ')' || currchar == '=' || currchar == ':' ){
            token->type = TYPE_PUNCT;
            sb_add(token->sb, currchar);
            token->sb.data[token->sb.count] = '\0';
            return 1;
        }
        if(currchar == '|' || currchar == '+'){
            token->type = TYPE_SEPARATOR;
            sb_add(token->sb, currchar);
            token->sb.data[token->sb.count] = '\0';
            return 1;
        }
        
        if(is_alpha_num(currchar)){
            sb_add(token->sb, currchar);
            currchar = lexer_read_char(lexer);
            while (is_alpha_num(currchar)) {
                sb_add(token->sb, currchar);
                currchar = lexer_read_char(lexer);
            }
            token->type = TYPE_IDENTIFIER;
            token->sb.data[token->sb.count] = '\0';
            lexer->loc--;
            return 1;
        } 

        if(token->sb.count > 0 && (is_control_char(currchar) || currchar == ' ')){
            token->sb.data[token->sb.count] = '\0';
            token->type = TYPE_IDENTIFIER;
            return 1;
        } else {
            currchar = lexer_read_char(lexer);

        }
        

    }
    return 0;

};



int stack_init(NestingStack* stack, int default_cap){
    if(!stack) return 0;
    stack->capacity = default_cap;
    stack->top = -1;
    stack->elements = (NestingElement*) malloc(sizeof(NestingElement) * default_cap);
    return 1;
}
int stack_push(NestingStack* stack, const char* name, size_t level){
    if(!stack || !name) return 0;
    
    if(stack->top >= stack->capacity){
        stack->capacity = stack->capacity == 0 ? INITIAL_STACK_CAPACITY : stack->capacity * 2;
        NestingElement* new_elements = realloc(stack->elements, sizeof(NestingElement) * stack->capacity);
        if(!new_elements) return 0;
        stack->elements = new_elements; 
        
    }
    stack->top++;
    stack->elements[stack->top].name = (char*)malloc(sizeof(char) * strlen(name)+1);
    if(!stack->elements[stack->top].name) return 0;
    strcpy(stack->elements[stack->top].name, name);
    stack->elements[stack->top].level = level;
    return 1;
}
NestingElement stack_pop(NestingStack* stack){
    if(!stack) return (NestingElement){0};
    if(stack->top == -1) return (NestingElement){0};
    NestingElement top_elem = stack->elements[stack->top];
    stack->top--;
    return top_elem;
}

NestingElement* stack_peek(NestingStack* stack){
    if (stack->top != -1)
    {
        return &stack->elements[stack->top];
    }
    return NULL;
}

int lexer_register_punctuation(Lexer* lexer, const char** strarr, size_t amount){
    for (int i=0; i<amount; i++) {
        if(strlen(strarr[i]) > 1)
            printf("TODO: register multi-char punct: %s\n", strarr[i]);
        else
            printf("TODO: register single-char punct: %s\n", strarr[i]);
    }
}
int lexer_register_separators(Lexer* lexer, const char** strarr, size_t amount){
    for (int i=0; i<amount; i++) {
        if(strlen(strarr[i]) > 1)
            printf("TODO: register multi-char separator: %s\n", strarr[i]);
        else
            printf("TODO: register multi-char punct: %s\n", strarr[i]);
    }
}

void handle_title(Lexer* lexer, Token* token, FILE* output){
    Lexer back = *lexer;
    char is_valid = 0;
    fwrite("<title>",sizeof(char), 7, output);
    get_next_token(lexer, token);
    if(token->type == TYPE_PUNCT && strcmp(token->value, "#[") == 0){
        get_next_token(lexer, token);
        if (strcmp(token->value, "title") == 0) {
            get_next_token(lexer, token);
            if (strcmp(token->value, "]") == 0) {
               while (get_next_token(lexer, token) && token->type== TYPE_IDENTIFIER) {
                    fwrite(token->value,sizeof(char), strlen(token->value), output);
                    is_valid = 1;
                    if(peek_next_token(lexer, token) && token->type == TYPE_IDENTIFIER)
                        fwrite(" ",sizeof(char), 1, output);
                    else
                     break;
               } 
            }
        }
    }
    if(!is_valid){   
        fwrite("default",sizeof(char), 7, output);
        *lexer = back;
    }
    fwrite("</title>\n",sizeof(char), 9, output);
}

void handle_element(Lexer* lexer, Token* token, FILE* output){
    if(!get_next_token(lexer, token)) {
        printf("ERROR: reached EOF before end of element! ");
        return;
    }
    if(token->type != TYPE_IDENTIFIER) {
        printf("ERROR: Expected identifier for element name!");
        return;
    }
    fwrite("<",sizeof(char), 1, output);
    fwrite(token->value,sizeof(char), strlen(token->value), output);
    if(!get_next_token(lexer, token)) {
        printf("ERROR: reached EOF before end of element! ");
        return;
    }
    if(strcmp(token->value, "|") != 0 && strcmp(token->value, "]") != 0 ) {
        printf("ERROR: Unexpected token after element name! ");
        return;
    }
    if(strcmp(token->value, "]") == 0) {
        fwrite(">\n",sizeof(char), 2, output);
        return;
    }
    char mode_class_property = 0;
    char class_flag = 0;
    if(strcmp(token->value, "|") == 0) {
        char prev_type = TYPE_SEPARATOR;
        while (get_next_token(lexer, token) && strcmp(token->value, "]") != 0) {

            printf("token: %s | type: %s | loc: %zu\n", token->value, token->type > TYPE_COUNT ? "UNKNOWN": token_type_strings[token->type], lexer->loc);
            if (prev_type == TYPE_SEPARATOR && token->type != TYPE_IDENTIFIER) {
                printf("ERROR: Unexpected token after element separator! ");
                return;
            }
            if(prev_type == TYPE_IDENTIFIER && strcmp(token->value, "|") == 0){
                prev_type = TYPE_SEPARATOR;
                continue;
            }
            prev_type = TYPE_IDENTIFIER;
            Token peek = {0};
            if(peek_next_token(lexer, &peek) && strcmp(peek.value, "=")==0 ){
                mode_class_property = 1;
            }

            if(mode_class_property){
                //handle properties
                if (class_flag) {
                    fwrite("\"",sizeof(char), 1, output);
                }
                printf("TODO: handle properties");
            }else {
                //handle class names
                if(class_flag == 0) { 
                    fwrite(" class=\"",sizeof(char), 8, output);
                    class_flag = 1;
                }
                else
                    fwrite(",",sizeof(char), 1, output);
                fwrite(token->value,sizeof(char), strlen(token->value), output);
            }
            
        }
        if (class_flag && mode_class_property == 0) {
            fwrite("\"",sizeof(char), 1, output);
        }
        if(strcmp(token->value, "]") == 0) {

            fwrite(">\n",sizeof(char), 2, output);
            return;
        }
    }
}

int main() {
    const char* input_file = "./text.txt";
    const char* output_file = "./out.html";
    const char* PUNCT[] = {"#[", "]", "=",  "@(", ")"};
    const char* SEPS[] = {"|",":","+"};
    Lexer lexer = {0};
    lexer_register_punctuation(&lexer, PUNCT, 5);
    lexer_register_separators(&lexer, SEPS, 3);
    lexer.line = 1;
    lexer.fd = fopen(input_file, "rb");
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

    
    DocElmProperties default_prop = {
        .font_size = 12
    };

    NestingStack stack = {0};
    stack_init(&stack, INITIAL_STACK_CAPACITY);

    // StringBuilder sb = {0};
    Token token = {0};
    // lexer.sb = &sb;
    int span_count = 0;
    handle_title(&lexer, &token, output);

    fwrite("</head>\n",sizeof(char), 8, output);
    fwrite("<body>\n",sizeof(char), 7, output);

    while(get_next_token(&lexer, &token)){
        printf("token: %s | type: %s | loc: %zu\n", token.value, token.type > TYPE_COUNT ? "UNKNOWN": token_type_strings[token.type], lexer.loc);

        switch (token.type) {
            case TYPE_PUNCT:
                if (strcmp(token.value, "#[") == 0) {

                    handle_element(&lexer, &token, output);
 
                }
                else if (strcmp(token.value, "@(") == 0) {
                    // handle_spanstyle();
                }
            break;
            case TYPE_IDENTIFIER:
                fwrite(token.value ,sizeof(char), strlen(token.value), output);
            break;
            default:break;
        }
        
            
        

    }
    fwrite("</body>\n",sizeof(char), 8, output);
    fwrite("</html>\n",sizeof(char), 8, output);

    
}