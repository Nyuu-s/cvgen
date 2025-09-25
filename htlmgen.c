
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

#define INITIAL_STACK_CAPACITY 100
#define NESTING_THRESHOLD 3
#define NESTING_IGNORED 1

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
    char* value;
    char type;
    // size_t level;
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
    Token* peeked_token; //maybe later
    size_t input_size;
    size_t loc;
    FILE* fd;
    char eof;
    char future_token_type;
    StringBuilder* sb;
} Lexer;


enum TOKEN_TYPES {
    TYPE_TEXT,
    TYPE_ELEMENT,
    TYPE_PROPERTY,
    TYPE_SPANSTYLE_KEY,
    TYPE_SPANSTYLE_VALUE,
    TYPE_COUNT
};

const char* token_type_strings[] = {"TEXT", "ELEMENT", "PROPERTY", "SPAN_CLASS", "SPAN_TEXT"};
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
    if(lexer->loc >= lexer->input_size) {
        lexer->eof = 1;
        return '\0';
    }
    // printf("readin: %c, loc: %zu \n ", lexer->content[lexer->loc], lexer->loc);
    return lexer->content[lexer->loc++];
}
int get_next_token2(Lexer* lexer, Token* token){


    lexer->sb->count = 0;
    char space_ctr = 0;
    while (!lexer->eof)
    {
        // if (lexer->content[lexer->loc] == ' ' ) 
        // {
        //     space_ctr +=1;
        //     continue;
        // }else if( lexer->content[lexer->loc] == '\t') {
        //     space_ctr += 4;
        //     continue;
        // }

        if(lexer->content[lexer->loc] == '\r' || lexer->content[lexer->loc] == '\n' ) {
            lexer_read_char(lexer);    
            continue;
        }
        if (lexer->content[lexer->loc] == '#' && lexer->loc+1 < lexer->input_size && lexer->content[lexer->loc+1] == '[')
        {
            token->type = TYPE_ELEMENT;
            lexer->loc += 2;
            while (!lexer->eof){
                char curchar = lexer_read_char(lexer);
                if(curchar == '|' || curchar == ']'){
                    lexer->loc--;
                    break;
                }
                sb_add(lexer->sb, curchar);
            }
            sb_add(lexer->sb, '\0');
            token->value = lexer->sb->data;
            return 1;
        }
        if(lexer->content[lexer->loc] == '|' && (token->type == TYPE_ELEMENT || token->type == TYPE_PROPERTY )){
            lexer->loc++;
            while (!lexer->eof){
                char curchar = lexer_read_char(lexer);
                if(curchar == '|' || curchar == ']'){
                    lexer->loc--;
                    break;
                }
                sb_add(lexer->sb, curchar);
            }
            sb_add(lexer->sb, '\0');
            token->value = lexer->sb->data;
            return 1;
            
        }
        if(lexer->content[lexer->loc] == ']' && (token->type == TYPE_ELEMENT || token->type == TYPE_PROPERTY )){
            token->type = TYPE_TEXT;
            lexer->loc++;
            continue;
        }

        if (lexer->content[lexer->loc] == '@' && lexer->loc+1 < lexer->input_size && lexer->content[lexer->loc+1] == '(')
        {
            token->type = TYPE_SPANSTYLE_KEY;
            lexer->loc += 2;
            char curchar;
            while (!lexer->eof){
                curchar = lexer_read_char(lexer);
                if(curchar == '+' || curchar == ':'){
                    lexer->loc--;
                    break;
                }
                sb_add(lexer->sb, curchar);
            }
            sb_add(lexer->sb, '\0');
            token->value = lexer->sb->data;
            return 1;
        }
        if (lexer->content[lexer->loc] == '+' && token->type == TYPE_SPANSTYLE_KEY){
            lexer->loc++;
            while (!lexer->eof){
                char curchar = lexer_read_char(lexer);
                if(curchar == '+' || curchar == ':'){
                    lexer->loc--;
                    break;
                }
                sb_add(lexer->sb, curchar);
            }
            sb_add(lexer->sb, '\0');
            token->value = lexer->sb->data;
            return 1;
        }
        if (lexer->content[lexer->loc] == ':' && token->type == TYPE_SPANSTYLE_KEY){
            token->type = TYPE_SPANSTYLE_VALUE;
            lexer->loc++;
            while (!lexer->eof){
                char curchar = lexer_read_char(lexer);
                if(curchar == ')'){
                    break;
                }
                sb_add(lexer->sb, curchar);
            }
            sb_add(lexer->sb, '\0');
            token->value = lexer->sb->data;
            return 1;
        }
        token->type = TYPE_TEXT;
        while (!lexer->eof)
        {
           char curchar = lexer_read_char(lexer);
           if (curchar == '#' && lexer->loc < lexer->input_size && lexer->content[lexer->loc] == '[')
           {
                lexer->loc--;
                break;
           }
            if (curchar == '@' && lexer->loc < lexer->input_size && lexer->content[lexer->loc] == '(')
           {
                lexer->loc--;
                break;
           }
           if (curchar == '\n' ||curchar == '\r')
           {
                continue;
           }
           
           sb_add(lexer->sb, curchar);           
        }
        sb_add(lexer->sb, '\0');
        token->value = lexer->sb->data;
        return 1;        
    }
    return 0;
    
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
    lexer->sb->count = 0;
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
                if(lexer->sb->count > 0){
                    lexer->sb->data[lexer->sb->count] = '\0';
                    lexer->future_token_type = TYPE_ELEMENT;
                    
                    token->value = lexer->sb->data;
                    return 1;
                }
                token->type = TYPE_ELEMENT;
            }else{
                //'#' was not an identifier, add the previous #
                sb_add(lexer->sb, lexer->content[lexer->loc-1])
            }
        }
        if((token->type == TYPE_ELEMENT || token->type == TYPE_PROPERTY) && lexer->content[lexer->loc] == '|'){
            lexer->loc++;
            lexer->future_token_type = TYPE_PROPERTY;
            lexer->sb->data[lexer->sb->count] = '\0';

            token->value = lexer->sb->data;
            return 1;
        }

        if((token->type == TYPE_ELEMENT) && lexer->content[lexer->loc] == '+'){
            if(lexer->sb->count > 0){
                lexer->future_token_type = TYPE_ELEMENT;
                lexer->sb->data[lexer->sb->count] = '\0';

                token->value = lexer->sb->data;
                return 1;
            }
        }
        if((token->type == TYPE_ELEMENT || token->type == TYPE_PROPERTY) && lexer->content[lexer->loc] == ']'){
            if (strncmp("styles", lexer->sb->data, lexer->sb->count) == 0)
            {
                // mdoe style
            }
            
            lexer->loc++;
            lexer->future_token_type = 0;
            lexer->sb->data[lexer->sb->count] = '\0';
            token->value = lexer->sb->data;
            return 1;
        }
        sb_add(lexer->sb, lexer->content[lexer->loc++]);

        
        
    }
    lexer->sb->data[lexer->sb->count] = '\0';
    // get last token
    if(lexer->sb->count > 0){ 
        return 1;
    }

    lexer->eof = 1;
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

    NestingStack stack = {0};
    stack_init(&stack, INITIAL_STACK_CAPACITY);

    StringBuilder sb = {0};
    Token token = {0};
    lexer.sb = &sb;
    int span_count = 0;
    while(get_next_token2(&lexer, &token)){
        printf("token: %s | type: %s | loc: %zu\n", token.value, token.type > TYPE_COUNT ? "UNKNOWN": token_type_strings[token.type], lexer.loc);
        
        if(token.type == 99){
            //round up spaces count to left margin to multiple of NESTING+1, 4 here, 
            //done to be less strict on amount of space is considered a nesting level
            //when there is 1,2,3,4 space it is considered 4
            //              5,6,7,8 is 8 and so on 
            char tmp[50];
            size_t dist = get_left_nesting_distance(&lexer);
            NestingElement* parent = stack_peek(&stack);
            if(parent == NULL) continue;
            if(dist == NESTING_IGNORED) {
                fwrite("<" , sizeof(char), 1, output);
                fwrite(token.value, sizeof(char), strlen(token.value), output);
                continue;
            }
            if(parent->level > dist){
                while (parent->level > dist)
                {
                    snprintf(tmp, strlen(token.value)+3, "</%s>", token.value);
                    fwrite(tmp, sizeof(char), strlen(tmp), output);

                    stack_pop(&stack);
                    parent = stack_peek(&stack);
                }
                //TODO gen close parent
                snprintf(tmp, strlen(token.value)+3, "</%s>", token.value);
                fwrite(tmp, sizeof(char), strlen(tmp), output);

                //TODO gen open current
                fwrite("<" , sizeof(char), 1, output);
                fwrite(token.value, sizeof(char), strlen(token.value), output);
                stack_push(&stack, token.value, dist);
                continue;
            }
            if(parent->level < dist){
                
                //TODO gen open current
                fwrite("<" , sizeof(char), 1, output);
                fwrite(token.value, sizeof(char), strlen(token.value), output);
                stack_push(&stack, token.value, dist);
                continue;
            }else{
                //TODO close parent
                snprintf(tmp, strlen(token.value)+3, "</%s>", token.value);
                fwrite(tmp, sizeof(char), strlen(tmp), output);
                //TODO open current
                fwrite("<" , sizeof(char), 1, output);
                fwrite(token.value, sizeof(char), strlen(token.value), output);
                stack_push(&stack, token.value, dist);
            }
        }
            
        

    }
    fwrite("</body>\n",sizeof(char), 8, output);
    fwrite("</html>\n",sizeof(char), 8, output);

    
}