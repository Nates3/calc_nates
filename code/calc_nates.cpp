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
CopyArgument(char *Dest, char *Source, u32 Count, u32 *CharCount)
{
    for(u32 Index = 0;
        Index < Count;
        ++Index)
    {
        *Dest++ = *Source++;
        ++(*CharCount);
    }
    
    ++(*CharCount);
    *Dest = 32;
}

enum operator_type
{
    OP_TYPE_NULL = (1 << 1),
    OP_TYPE_ADD = (1 << 2),
    OP_TYPE_SUB = (1 << 3),
    OP_TYPE_MUL = (1 << 4),
    OP_TYPE_DIV = (1 << 5),
};

internal operator_type
GetOperatorType(char Value)
{
    operator_type Result = OP_TYPE_NULL;
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

internal u32
GetPriority(operator_type Type)
{
    u32 Result = 0;
    switch(Type)
    {
        case OP_TYPE_ADD:
        case OP_TYPE_SUB:
        {
            Result = 1;
        } break;
        
        case OP_TYPE_MUL:
        case OP_TYPE_DIV:
        {
            Result = 2;
        } break;
    }
    
    return(Result);
}


struct expression;
struct expression_piece
{
    expression *Expr;
    r32 Value;
};

struct expression
{
    b32 Evaluated;
    r32 Result;
    
    expression_piece P1;
    operator_type Operator;
    u32 OperatorPriority;
    expression_piece P2;
    
};


internal r32
EvalExpression(expression **Start, u32 ExpressionCount)
{
    r32 Result = 0;
    
    for(u32 Index = 0;
        Index < ExpressionCount;
        ++Index)
    {
        expression *Current = *(Start + Index);
        expression *Prev = Current->P1.Expr;
        expression *Next = Current->P2.Expr;
        
        
        r32 Value1 = 0.0f;
        b32 PrevExists = (Prev) ? true : false;
        if(PrevExists && Prev->Evaluated)
        {
            Value1 = Prev->Result;
        }
        else
        {
            Value1 = Current->P1.Value;
        }
        
        r32 Value2 = 0.0f;
        b32 NextExists = (Next) ? true : false;
        if(NextExists && Next->Evaluated)
        {
            Value2 = Next->Result;
        }
        else
        {
            Value2 = Current->P2.Value;
        }
        
        switch(Current->Operator)
        {
            case OP_TYPE_NULL:
            {
                Current->Result = Value1 + Value2; // NOTE(nates): If null assume add
            } break;
            
            case OP_TYPE_ADD:
            {
                Current->Result = Value1 + Value2;
            } break;
            
            case OP_TYPE_SUB:
            {
                Current->Result = Value1 - Value2;
            } break;
            
            case OP_TYPE_MUL:
            {
                Current->Result = Value1*Value2;
            } break;
            
            case OP_TYPE_DIV:
            {
                Current->Result = Value1/Value2;
            } break;
        }
        
        Current->Evaluated = true;
        Result = Current->Result;
        
        if(PrevExists)
        {
            Prev->P2.Value = Current->Result;
            Prev->P2.Expr = Next;
        }
        
        if(NextExists)
        {
            Next->P1.Value = Current->Result;
            Next->P1.Expr = Prev;
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

// TODO(nates): Build expresions using the new double linked list structure and make a function that
// sorts these expressions for which ones should be operated on first update EvalExpression to use
// the double linked list for handling operator precedence

internal expression *
BuildExpression(memory_arena *Arena, token *TokenList, u32 TokenCount, 
                u32 *ExpressionCount, expression *PrevExpr = 0)
{
    expression *Result = PushStruct(Arena, expression);
    (*ExpressionCount)++;
    
    expression_piece *CurrentPiece = &Result->P1;
    token *LastLiteral = 0;
    u32 LastLiteralCount = TokenCount;
    b32 Skip = false;
    
    if(PrevExpr)
    {
        CurrentPiece->Expr = PrevExpr;
    }
    
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
                    Result->OperatorPriority = GetPriority(Result->Operator);
                }
                else
                {
                    CurrentPiece->Expr = BuildExpression(Arena, LastLiteral, LastLiteralCount, 
                                                         ExpressionCount, Result);
                    Skip = true;
                }
            } break;
            
            case TOKEN_TYPE_ERR:
            {
                ERRORPATH;
            } break;
        }
    }
    
    if(!Result->Operator)
    {
        Result->Operator = OP_TYPE_NULL;
    }
    
    return(Result);
}

internal expression **
SortExpressions(memory_arena *Arena, expression *Start, u32 ExpressionCount)
{
    // TODO(nates): I don't like this, it's janky, is there a simplier way to do the sort or eleminate the
    // sort completely
    u32 *Map = PushArray(Arena, ExpressionCount, u32);
    // NOTE(nates): Fill map with default values
    {
        u32 MapIndex = 0;
        while(MapIndex < ExpressionCount)
        {
            *(Map + MapIndex) = MapIndex++;
        }
    }
    
    // TODO(nates): Impliment radix/counter sort and remove bubble sort?
    // NOTE(nates): Sort the map using bubble sort
    {
        u32 MapIndex = 0;
        b32 NotSorted = true;
        b32 Failed = false;
        while(NotSorted)
        {
            if((MapIndex + 1) < (ExpressionCount))
            {
                u32 *CurrentMap = (Map + MapIndex);
                u32 *NextMap = (Map + (MapIndex + 1));
                expression *Current = Start + *CurrentMap;
                expression *Next = Start + *NextMap;
                
                if(Current->OperatorPriority < Next->OperatorPriority)
                {
                    u32 Temp = *CurrentMap;
                    *CurrentMap = *NextMap;
                    *NextMap = Temp;
                    Failed = true;
                }
            }
            
            b32 End = ((MapIndex + 1) == ExpressionCount);
            if(End && Failed)
            {
                MapIndex = 0;
                Failed = false;
            }
            else if(End)
            {
                NotSorted = false;
            }
            else
            {
                ++MapIndex;
            }
        }
    }
    
    // NOTE(nates): Build Expression pointer list
    expression **Result = PushStruct(Arena, expression *);
    {
        *Result = Start + *(Map + 0);
        for(u32 MapIndex = 1;
            MapIndex < ExpressionCount;
            ++MapIndex)
        {
            expression **Add = PushStruct(Arena, expression *);
            *Add = Start + *(Map + MapIndex);
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
    
    memory_arena MainArena = GetMemoryArena(&MainMemory, KiloBytes(128));
    memory_arena TempArena = GetMemoryArena(&MainMemory, KiloBytes(64));
    
    u32 CharCount = 0;
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
            
            char *Dest = PushArray(&MainArena, (SizeOfArg + 1), char);
            
            CopyArgument(Dest, Arg, SizeOfArg, &CharCount);
        }
        
        
        u32 TokenCount = 1;
        u32 TokenListSize = 512;
        token *TokenList = PushArray(&MainArena, TokenListSize, token);
        
        u32 MaxTokenSize = 7;
        
        // NOTE(nates): LEXER
        {
            token *CurrentToken = TokenList;
            for(u32 CharIndex = 0;
                (CharIndex < CharCount) && (NoError);
                )
            {
                char *CurrentChar = ((char *)MainArena.Base + CharIndex);
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
                            if(CurrentToken->Size < MaxTokenSize)
                            {
                                char *Char = PushStruct(&MainArena, char);
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
                                ERRORPATH;
                            }
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
        
        char Buffer[256] = {};
        if(NoError)
        {
            u32 ExpressionCount = 0;
            expression *Start = BuildExpression(&MainArena, TokenList, TokenCount, &ExpressionCount);
            expression **Sorted = SortExpressions(&TempArena, Start, ExpressionCount);
            
            // TODO(nates): IMPORTANT(nates): Do the math _not_ using floating point, need to change
            // build expression and eval expression
            r32 Result = EvalExpression(Sorted, ExpressionCount);
            sprintf_s(Buffer, "%0.8f\n", Result);
            
#define DEBUG 0
#if DEBUG
            OutputDebugStringA(Buffer);
#else
            printf(Buffer);
#endif
        }
        else
        {
            sprintf_s(Buffer, "There was an error\nUSAGE: Calc.exe Number Operator Number ...\nPossible Too Many Digits, Max Digit Count: %d\n", MaxTokenSize);
            //sprintf_s(Buffer, "There was an error\nUSAGE: Calc.exe Number: 2.0 Operator: (* ) \n");
#if DEBUG
            OutputDebugStringA(Buffer);
#else
            printf(Buffer);
#endif
        }
    }
    
    return 0;
}

