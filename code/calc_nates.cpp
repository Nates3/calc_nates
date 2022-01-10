#include "calc_nates.h"
#include "token.cpp"

internal u32
StringSize(char *Text)
{
    u32 Count = 0;
    while(*(Text + Count) != 0)
    {
        ++Count;
    }
    return(Count);
}

internal void
CopyArgument(char *Dest, char *Source, u32 Count)
{
    for(u32 Index = 0;
        Index < Count;
        ++Index)
    {
        *Dest++ = *Source++;
    }
    
    *Dest = 32;
}

enum operator_type
{
    OP_TYPE_INVAL = (1 << 0),
    OP_TYPE_ADD = (1 << 1),
    OP_TYPE_SUB = (1 << 2),
    OP_TYPE_MUL = (1 << 3),
    OP_TYPE_DIV = (1 << 4),
};

internal operator_type
GetOperatorType(char Value)
{
    operator_type Result = OP_TYPE_INVAL;
    switch(Value)
    {
        case 0x2b:
        {
            Result = OP_TYPE_ADD;
        } break;
        
        case 0x2d:
        {
            Result = OP_TYPE_SUB;
        } break;
        
        case 0x2a:
        {
            Result = OP_TYPE_MUL;
        } break;
        
        case 0x2f:
        {
            Result = OP_TYPE_DIV;
        } break;
    }
    
    return(Result);
}


struct expression;
struct expression_piece
{
    // TODO(nates): Change this into a flag if it's an identifier
    b32 IsPointer;
    union
    {
        expression *SubExpr;
        r32 Value;
    };
};

struct expression
{
    expression_piece P1;
    operator_type Operator;
    expression_piece P2;
};


internal r32
EvalExpr(expression *Expr)
{
    r32 Result = 0;
    
    r32 Value1 = 0.0f;
    if(Expr->P1.IsPointer)
    {
        Value1 = EvalExpr(Expr->P1.SubExpr);
    }
    else
    {
        Value1 = Expr->P1.Value;
    }
    
    r32 Value2 = 0.0f;
    if(Expr->P2.IsPointer)
    {
        Value2 = EvalExpr(Expr->P2.SubExpr);
    }
    else
    {
        Value2 = Expr->P2.Value;
    }
    
    switch(Expr->Operator)
    {
        case OP_TYPE_ADD:
        {
            Result = Value1 + Value2;
        } break;
        
        case OP_TYPE_SUB:
        {
            Result = Value1 - Value2;
        } break;
        
        case OP_TYPE_MUL:
        {
            Result = Value1*Value2;
        } break;
        
        case OP_TYPE_DIV:
        {
            Result = Value1/Value2;
        }
    }
    
    return(Result);
}


internal r32
R32Power(r32 Number, s32 Exponent)
{
    r32 Result = 1;
    
    b32 Inverse = false;
    if(Exponent < 0)
    {
        Exponent *= -1;
        Inverse = true;
    }
    
    if(Exponent != 0)
    {
        Result = Number;
        --Exponent;
        while(Exponent)
        {
            Result *= Number;
            --Exponent;
        };
    }
    
    if(Inverse)
    {
        Result = (1.0f / Result);
    }
    
    return(Result);
}
struct alloc_memory
{
    void *Base;
    u32 Used;
    u32 Size;
};


internal memory_arena
GetMemoryArena(alloc_memory *Alloc, u32 Size)
{
    memory_arena Result = {};
    
    if((Alloc->Used + Size) < Alloc->Size)
    {
        Result.Base = ((u8 *)Alloc->Base + Alloc->Used);
        Result.Size = Size;
        Alloc->Used += Size;
    }
    return(Result);
}

internal void
FlushArena(memory_arena *Arena)
{
    Arena->Used = 0;
}


internal r32
GetR32FromLiteral(char *Chars, u32 Size)
{
    r32 Result = 0.0f;
    s32 Exponent = -1;
    {
        char *TempChar = Chars;
        while((InRange(0x30, *TempChar, 0x39)))
        {
            ++Exponent;
            ++TempChar;
        }
    }
    
    for(u32 Index = 0;
        Index < Size;
        ++Index)
    {
        char *CurrentChar = (Chars + Index);
        if(*CurrentChar != 0x2e)
        {
            u32 Value = *CurrentChar - 0x30;
            
            Result += (Value*R32Power(10, Exponent--));
        }
    }
    
    
    return(Result);
    
}

global_variable b32 NoError;
#define ERRORPATH NoError = false; break

internal expression *
BuildExpr(memory_arena *TokenArena, token *TokenList, u32 TokenCount)
{
    expression *Result = PushStruct(TokenArena, expression);
    
    expression_piece *CurrentPiece = &Result->P1;
    token *LastLiteral = 0;
    u32 LastLiteralCount = TokenCount;
    b32 Skip = false;
    for(u32 TokenIndex = 0;
        (TokenIndex < TokenCount) && NoError && !Skip;
        ++TokenIndex)
    {
        token *CurrentToken = TokenList + TokenIndex;
        
        switch(CurrentToken->Type)
        {
            case TOKEN_TYPE_LITERAL:
            {
                CurrentPiece->Value = GetR32FromLiteral(CurrentToken->Chars, 
                                                        CurrentToken->Size);
                LastLiteral = CurrentToken;
                LastLiteralCount = TokenCount - TokenIndex;
            } break;
            
            case TOKEN_TYPE_OPERATOR:
            {
                CurrentPiece = &Result->P2;
                if(!Result->Operator)
                {
                    Result->Operator = GetOperatorType(*CurrentToken->Chars);
                }
                else
                {
                    CurrentPiece->IsPointer = true;
                    CurrentPiece->SubExpr = BuildExpr(TokenArena, LastLiteral, LastLiteralCount);
                    Skip = true;
                }
            } break;
            
            case TOKEN_TYPE_ERR:
            {
                ERRORPATH;
            } break;
        }
        
    }
    
    return(Result);
}


int main(int ArgCount, char **ArgV_)
{
    NoError = true;
    
    u32 MainMemorySize = MegaBytes(1);
    alloc_memory MainMemory = {};
    MainMemory.Base = VirtualAlloc(0, MainMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    MainMemory.Size = MainMemorySize;
    
    memory_arena ParseArena = GetMemoryArena(&MainMemory, KiloBytes(64));
    memory_arena TokenArena = GetMemoryArena(&MainMemory, KiloBytes(64));
    
    memory_arena TempArena = GetMemoryArena(&MainMemory, KiloBytes(64));
    
    char **ArgVList = (ArgV_ + 1);
    if(ArgCount > 1)
    {
        u32 ArgVCount = ArgCount - 1;
        
        for(u32 Index = 0;
            Index < ArgVCount;
            ++Index)
        {
            char *Arg = *(ArgVList + Index);
            u32 SizeOfArg = StringSize(Arg);
            
            char *Dest = PushArray(&ParseArena, (SizeOfArg + 1), char);
            
            CopyArgument(Dest, Arg, SizeOfArg);
        }
    }
    
    // TODO(nates): Can we do this differently?
    char *Base = (char *)ParseArena.Base;
    u32 TokenCount = 1;
    u32 TokenListSize = 256;
    token *TokenList = PushArray(&TokenArena, TokenListSize, token);
    
    // NOTE(nates): LEXER
    {
        token *CurrentToken = TokenList;
        for(u32 CharIndex = 0;
            (CharIndex < ParseArena.Used) && (NoError);
            )
        {
            char *CurrentChar = (Base + CharIndex);
            token_type_result Types = GetTypes(&TempArena, *CurrentChar);
            
            token_type *Type = Types.List;
            
            if(!CurrentToken->Type)
            {
                for(u32 tIndex = 0;
                    tIndex < Types.Count;
                    )
                {
                    token_type CurrentType = *(Types.List + tIndex);
                    
                    if(CurrentType != TOKEN_TYPE_SPECIAL)
                    {
                        CurrentToken->Type = CurrentType;
                        break;
                    }
                    else
                    {
                        ++tIndex;
                    }
                }
            }
            
            if(*Type == TOKEN_TYPE_SPECIAL)
            {
                Type += 1;
            }
            
            if(*Type == TOKEN_TYPE_ERR)
            {
                ERRORPATH;
            }
            else
            {
                if(*Type == TOKEN_TYPE_EMPTY)
                {
                    ++CharIndex;
                }
                else
                {
                    if(CurrentToken->Type == *Type)
                    {
                        // TODO(nates): Lexer should handle operators properly
                        char *Char = PushStruct(&TokenArena, char);
                        *Char = *CurrentChar;
                        if(!CurrentToken->Chars)
                        {
                            CurrentToken->Chars = Char;
                        }
                        ++CurrentToken->Size;
                        ++CharIndex;
                    }
                    else
                    {
                        Assert(TokenCount < TokenListSize);
                        ++CurrentToken;
                        ++TokenCount;
                    }
                }
                
                FlushArena(&TempArena);
            }
        }
    }
    
    expression *Start = BuildExpr(&TokenArena, TokenList, TokenCount);
    
    
    char Buffer[64] = {};
    if(NoError)
    {
        r32 Result = EvalExpr(Start);
        sprintf(Buffer, "%0.8f\n", Result);
        
#define DEBUG 0
#if DEBUG
        OutputDebugStringA(Buffer);
#else
        printf(Buffer);
#endif
    }
    else
    {
        sprintf(Buffer, "there was an error\n");
#if DEBUG
        OutputDebugStringA(Buffer);
#else
        printf(Buffer);
#endif
    }
    
    return 0;
}

