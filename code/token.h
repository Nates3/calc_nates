/* date = January 9th 2022 11:04 am */
#ifndef TOKEN_H

enum token_type
{
    TOKEN_TYPE_ERR = 0x0,
    TOKEN_TYPE_EMPTY = (1 << 0),
    TOKEN_TYPE_LITERAL = (1 << 1),
    TOKEN_TYPE_OPERATOR = (1 << 2),
    TOKEN_TYPE_SPECIAL = (1 << 4),
};

struct token
{
    char *Chars;
    token_type Type;
    u32 Size;
};

struct token_type_result
{
    token_type *List;
    u32 Count;
};


#define TOKEN_H
#endif //TOKEN_H
