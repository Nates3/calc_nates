

#define InRange(min, value, max) ((value >= min) && (value <= max))
internal token_type_result
GetTypes(memory_arena *TempArena, char Char)
{
    b32 Special = (Char == 0x2e);
    b32 Literal = InRange(0x30, Char, 0x39) || (Char == 0x2e);
    b32 Operator = (Char == 0x2b) || (Char == 0x2d) || (Char == 0x2a) || (Char == 0x2f);
    b32 Empty = (Char == 0x20);
    b32 Error = !(Literal || Operator || Empty || Special);
    
    // NOTE(nates): This memory will be seralized so it doesn't have to be a linked list
    token_type_result Result = {};
    token_type *First = 0;
    token_type *Current = PushStruct(TempArena, token_type);
    First = Current;
    Result.List = First;
    if(Literal || Operator || Empty || Special)
    {
        if(Special)
        {
            ++Result.Count;
            if(!Current)
            {
                Current = PushStruct(TempArena, token_type);
            }
            
            *Current = TOKEN_TYPE_SPECIAL;
            Current = 0;
        }
        
        if(Literal)
        {
            ++Result.Count;
            if(!Current)
            {
                Current = PushStruct(TempArena, token_type);
            }
            
            *Current = TOKEN_TYPE_LITERAL;
            Current = 0;
        }
        
        if(Operator)
        {
            ++Result.Count;
            if(!Current)
            {
                Current = PushStruct(TempArena, token_type);
            }
            
            *Current = TOKEN_TYPE_OPERATOR;
            Current = 0;
        }
        
        if(Empty)
        {
            ++Result.Count;
            if(!Current)
            {
                Current = PushStruct(TempArena, token_type);
            }
            
            *Current = TOKEN_TYPE_EMPTY;
            Current = 0;
        }
    }
    
    if(Error)
    {
        ++Result.Count;
        if(!Current)
        {
            Current = PushStruct(TempArena, token_type);
        }
        
        *Current = TOKEN_TYPE_ERR;
        Current = 0;
    }
    
    
    return(Result);
};
