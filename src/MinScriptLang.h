#ifndef MIN_SCRIPT_LANG_H
#define MIN_SCRIPT_LANG_H

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//
// Public interface
//
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <exception>

#include <cstdint>

namespace MinScriptLang {

struct PlaceInCode
{
    uint32_t Index, Row, Column;
};

class Error : public std::exception
{
public:
    Error(const PlaceInCode& place) : m_Place{place} { }
    const PlaceInCode& GetPlace() const { return m_Place; }
    virtual const char* what() const override;
    virtual const char* GetMessage() const = 0;
private:
    const PlaceInCode m_Place;
    mutable std::string m_What;
};

class ParsingError : public Error
{
public:
    ParsingError(const PlaceInCode& place, const char* message) : Error{place}, m_Message{message} { }
    virtual const char* GetMessage() const override { return m_Message; }
private:
    const char* const m_Message; // Externally owned
};

class ExecutionError : public Error
{
public:
    ExecutionError(const PlaceInCode& place, std::string&& message) : Error{place}, m_Message{std::move(message)} { }
    virtual const char* GetMessage() const override { return m_Message.c_str(); }
private:
    const std::string m_Message;
};

class EnvironmentPimpl;

class Environment
{
public:
    Environment();
    ~Environment();
    void Execute(const char* code, size_t codeLen);
    const std::string& GetOutput() const;
private:
    EnvironmentPimpl* pimpl;
};

} // namespace MinScriptLang

#endif // #ifndef MIN_SCRIPT_LANG_H



// For Visual Studio IntelliSense.
#ifdef __INTELLISENSE__
#define MIN_SCRIPT_LANG_IMPLEMENTATION
#endif

#ifdef MIN_SCRIPT_LANG_IMPLEMENTATION

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//
// Private implementation
//
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <memory>
#include <algorithm>

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cmath>
#include <ctype.h>

using std::unique_ptr;
using std::shared_ptr;
using std::make_unique;
using std::make_shared;
using std::string;
using std::vector;

// F***k Windows.h macros!
#undef min
#undef max

namespace MinScriptLang {

////////////////////////////////////////////////////////////////////////////////
// Basic facilities

static const char* const ERROR_MESSAGE_PARSING_ERROR = "Parsing error.";
static const char* const ERROR_MESSAGE_INVALID_NUMBER = "Invalid number.";
static const char* const ERROR_MESSAGE_UNRECOGNIZED_TOKEN = "Unrecognized token.";
static const char* const ERROR_MESSAGE_UNEXPECTED_END_OF_FILE_IN_MULTILINE_COMMENT = "Unexpected end of file inside multiline comment.";
static const char* const ERROR_MESSAGE_EXPECTED_EXPRESSION = "Expected expression.";
static const char* const ERROR_MESSAGE_EXPECTED_STATEMENT = "Expected statement.";
static const char* const ERROR_MESSAGE_EXPECTED_LVALUE = "Expected l-value.";
static const char* const ERROR_MESSAGE_EXPECTED_NUMBER = "Expected number.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL                     = "Expected symbol.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_COLON               = "Expected symbol ':'.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON           = "Expected symbol ';'.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN  = "Expected symbol '('.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE = "Expected symbol ')'.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE = "Expected symbol '}'.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_WHILE = "Expected 'while'.";
static const char* const ERROR_MESSAGE_VARIABLE_DOESNT_EXIST = "Variable doesn't exist.";
static const char* const ERROR_MESSAGE_NOT_IMPLEMENTED = "Not implemented.";

struct Constant
{
public:
    enum class Type { Number, String };
    explicit Constant(double number) : m_Type{Type::Number}, m_Number(number) { }
    explicit Constant(string&& s) : m_Type{Type::String}, m_String{std::move(s)} { }
    Type GetType() const { return m_Type; }
    double GetNumber() const { assert(m_Type == Type::Number); return m_Number; }
    const string& GetString() const { assert(m_Type == Type::String); return m_String; }
    void SetNumber(double v) { m_Type = Type::Number; m_Number = v; m_String.clear(); }
    void SetString(string&& s) { m_Type = Type::String; m_String = std::move(s); }
private:
    Type m_Type;
    double m_Number = 0.0;
    string m_String;
};

// Name with underscore because f***g Windows.h defines TokenType as macro!
enum class TokenType_
{
    None, Symbol, Identifier, Number, End
};

enum class Symbol
{
    // Symbols
    Comma,             // ,
    QuestionMark,      // ?
    Colon,             // :
    Semicolon,         // ;
    RoundBracketOpen,  // (
    RoundBracketClose, // )
    CurlyBracketOpen,  // {
    CurlyBracketClose, // }
    Mul,               // *
    Div,               // /
    Mod,               // %
    Add,               // +
    Sub,               // -
    Equal,             // =
    ExclamationMark,   // !
    Tilde,             // ~
    Less,              // <
    Greater,           // >
    // Multiple character symbols
    Incrementation,    // ++
    Decrementation,    // --
    ShiftLeft,         // <<
    ShiftRight,        // >>
    LessEqual,         // <=
    GreaterEqual,      // >=
    // Keywords
    Null, False, True, If, Else, While, Do, For, Count
};
static const char* SYMBOL_STR[] = {
    ",", "?", ":", ";", "(", ")", "{", "}", "*", "/", "%", "+", "-", "=", "!", "~", "<", ">", // Symbols
    "++", "--", "<<", ">>", "<=", ">=", // Multiple character symbols
    "null", "false", "true", "if", "else", "while", "do", "for", // Keywords
};

struct Token
{
    PlaceInCode Place;
    TokenType_ Type;
    union
    {
        Symbol Symbol; // Only when Type == TokenType_::Symbol
        double Number; // Only when Type == TokenType_::Number
    };
    string String; // Only when Type == TokenType::Identifier
};

static inline bool IsDecimalNumber(char ch) { return ch >= '0' && ch <= '9'; }
static inline bool IsAlpha(char ch) { return ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch == '_'; }
static inline bool IsAlphaNumeric(char ch) { return ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9' || ch == '_'; }

static const char* GetDebugPrintIndent(uint32_t indentLevel)
{
    return "                                                                                                                                                                                                                                                                "
        + (256 - std::min<uint32_t>(indentLevel, 128) * 2);
}

static void VFormat(string& str, const char* format, va_list argList)
{
    size_t dstLen = (size_t)_vscprintf(format, argList);
    if(dstLen)
    {
        std::vector<char> buf(dstLen + 1);
        vsprintf_s(&buf[0], dstLen + 1, format, argList);
        str.assign(&buf[0], &buf[dstLen]);
    }
    else
        str.clear();
}

static void Format(string& str, const char* format, ...)
{
    va_list argList;
    va_start(argList, format);
    VFormat(str, format, argList);
    va_end(argList);
}

////////////////////////////////////////////////////////////////////////////////
// class CodeReader definition

class CodeReader
{
public:
    CodeReader(const char* code, size_t codeLen) :
        m_Code{code},
        m_CodeLen{codeLen},
        m_Place{0, 1, 1}
    {
    }

    bool IsEnd() const { return m_Place.Index >= m_CodeLen; }
    const PlaceInCode& GetCurrentPlace() const { return m_Place; }
    const char* GetCurrentCode() const { return m_Code + m_Place.Index; }
    size_t GetCurrentLen() const { return m_CodeLen - m_Place.Index; }
    char GetCurrentChar() const { return m_Code[m_Place.Index]; }
    bool Peek(char ch) const { return m_Place.Index < m_CodeLen && m_Code[m_Place.Index] == ch; }
    bool Peek(const char* s, size_t sLen) const { return m_Place.Index + sLen <= m_CodeLen && memcmp(m_Code + m_Place.Index, s, sLen) == 0; }

    void MoveOneChar()
    {
        if(m_Code[m_Place.Index++] == '\n')
            ++m_Place.Row, m_Place.Column = 1;
        else
            ++m_Place.Column;
    }
    void MoveChars(size_t n)
    {
        for(size_t i = 0; i < n; ++i)
            MoveOneChar();
    }

private:
    const char* const m_Code;
    const size_t m_CodeLen;
    PlaceInCode m_Place;
};

////////////////////////////////////////////////////////////////////////////////
// class Tokenizer definition

class Tokenizer
{
public:
    Tokenizer(const char* code, size_t codeLen);
    void GetNextToken(Token& out);

private:
    static double ParseNumber(const char* s, size_t sLen);
 
    CodeReader m_Code;

    void SkipSpacesAndComments();
};

////////////////////////////////////////////////////////////////////////////////
// class Value definition

class Value
{
public:
    enum class Type { Number, String };

    Value() { }
    Value(double number) : m_Number(number) { }
    Value(string&& str) : m_Type(Type::String), m_String(std::move(str)) { }

    Type GetType() const { return m_Type; }
    double GetNumber() const { assert(m_Type == Type::Number); return m_Number; }
    const string& GetString() const { assert(m_Type == Type::String); return m_String; }

    bool IsTrue() const
    {
        switch(m_Type)
        {
        case Type::Number: return m_Number != 0.f;
        case Type::String: return !m_String.empty();
        default: assert(0); return false;
        }
    }

    void SetNumber(double number) { m_Type = Type::Number; m_Number = number; m_String.clear(); }
    void SetString(string&& str) { m_Type = Type::String; m_String = std::move(str); }

private:
    Type m_Type = Type::Number;
    double m_Number = 0.0;
    string m_String;
};

////////////////////////////////////////////////////////////////////////////////
// class Object definition

class Object
{
public:
    bool HasKey(const Constant& key) const;
    bool HasKey(const string& key) const;
    Value& GetOrCreateValue(const Constant& key); // Creates new null value if doesn't exist.
    Value& GetOrCreateValue(const string& key); // Creates new null value if doesn't exist.
    Value* TryGetValue(const Constant& key); // Returns null if doesn't exist.
    Value* TryGetValue(const string& key); // Returns null if doesn't exist.
    bool Remove(const Constant& key); // Returns true if has been found and removed.

private:
    using NumberMapType = std::map<double, Value>;
    using StringMapType = std::unordered_map<string, Value>;
    NumberMapType m_NumberMap;
    StringMapType m_StringMap;
};

struct LValue
{
    Object& Obj;
    Constant Key;
};

////////////////////////////////////////////////////////////////////////////////
// Abstract Syntax Tree definitions

namespace AST
{

struct ExecuteContext
{
    EnvironmentPimpl& Env;
    Object& GlobalContext;
};

struct Statement
{
    explicit Statement(const PlaceInCode& place) : m_Place{place} { }
    virtual ~Statement() { }
    const PlaceInCode& GetPlace() const { return m_Place; }
    virtual void DebugPrint(uint32_t indentLevel) const = 0;
    virtual void Execute(ExecuteContext& ctx) const = 0;
private:
    const PlaceInCode m_Place;
};

struct EmptyStatement : public Statement
{
    explicit EmptyStatement(const PlaceInCode& place) : Statement{place} { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual void Execute(ExecuteContext& ctx) const { }
};

struct Expression;

struct Condition : public Statement
{
    unique_ptr<Expression> ConditionExpression;
    unique_ptr<Statement> Statements[2]; // [0] executed if true, [1] executed if false, optional.
    explicit Condition(const PlaceInCode& place) : Statement{place} { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

const enum WhileLoopType { While, DoWhile };

struct WhileLoop : public Statement
{
    WhileLoopType Type;
    unique_ptr<Expression> ConditionExpression;
    unique_ptr<Statement> Body;
    explicit WhileLoop(const PlaceInCode& place, WhileLoopType type) : Statement{place}, Type{type} { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

struct ForLoop : public Statement
{
    unique_ptr<Expression> InitExpression; // Optional
    unique_ptr<Expression> ConditionExpression; // Optional
    unique_ptr<Expression> IterationExpression; // Optional
    unique_ptr<Statement> Body;
    explicit ForLoop(const PlaceInCode& place) : Statement{place} { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

struct Block : public Statement
{
    explicit Block(const PlaceInCode& place) : Statement{place} { }
    vector<unique_ptr<Statement>> Statements;
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

struct Script : Block
{
    explicit Script(const PlaceInCode& place) : Block{place} { }
};

struct Expression : Statement
{
    explicit Expression(const PlaceInCode& place) : Statement{place} { }
    virtual Value Evaluate(ExecuteContext& ctx) const = 0;
    virtual LValue GetLValue(ExecuteContext& ctx) const;
    virtual void Execute(ExecuteContext& ctx) const { Evaluate(ctx); }
};

struct ConstantExpression : Expression
{
    explicit ConstantExpression(const PlaceInCode& place) : Expression{place} { }
    virtual void Execute(ExecuteContext& ctx) const { /* Nothing - just ignore its value. */ }
};

struct ConstantValue : ConstantExpression
{
    Value Val;
    ConstantValue(const PlaceInCode& place, Value&& val) : ConstantExpression{place}, Val{std::move(val)} { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual Value Evaluate(ExecuteContext& ctx) const { return Val; }
};

struct Identifier : ConstantExpression
{
    string S;
    Identifier(const PlaceInCode& place, string&& s) : ConstantExpression{place}, S(std::move(s)) { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual Value Evaluate(ExecuteContext& ctx) const;
    virtual LValue GetLValue(ExecuteContext& ctx) const;
};

struct Operator : Expression
{
    explicit Operator(const PlaceInCode& place) : Expression{place} { }
};

enum class UnaryOperatorType
{
    Preincrementation,
    Predecrementation,
    Plus,
    Minus,
    LogicalNot,
    BitwiseNot,
    None,
};

struct UnaryOperator : Operator
{
    UnaryOperatorType Type;
    unique_ptr<Expression> Operand;
    UnaryOperator(const PlaceInCode& place, UnaryOperatorType type) : Operator{place}, Type(type) { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual Value Evaluate(ExecuteContext& ctx) const;

private:
    Value BitwiseNot(Value&& operand) const;
};

enum class BinaryOperatorType
{
    Mul,
    Div,
    Mod,
    Add,
    Sub,
    ShiftLeft,
    ShiftRight,
    Assignment,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
};

struct BinaryOperator : Operator
{
    BinaryOperatorType Type;
    unique_ptr<Expression> Operands[2];
    BinaryOperator(const PlaceInCode& place, BinaryOperatorType type) : Operator{place}, Type(type) { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual Value Evaluate(ExecuteContext& ctx) const;
    
private:
    Value ShiftLeft(Value&& lhs, Value&& rhs) const;
    Value ShiftRight(Value&& lhs, Value&& rhs) const;
    Value Assignment(const LValue& lhs, Value&& rhs) const;
};

struct TernaryOperator : Operator
{
    unique_ptr<Expression> Operands[3];
    explicit TernaryOperator(const PlaceInCode& place) : Operator{place} { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual Value Evaluate(ExecuteContext& ctx) const;
};

enum class MultiOperatorType
{
    None,
    Call,
};

struct MultiOperator : Operator
{
    MultiOperatorType Type;
    vector<unique_ptr<Expression>> Operands;
    MultiOperator(const PlaceInCode& place, MultiOperatorType type) : Operator{place}, Type(type) { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual Value Evaluate(ExecuteContext& ctx) const;

private:
    Value Call(ExecuteContext& ctx) const;
};

} // namespace AST

static inline void CheckNumberOperand(const AST::Expression* operand, const Value& value)
{
    if(value.GetType() != Value::Type::Number)
        throw ExecutionError(operand->GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
}

////////////////////////////////////////////////////////////////////////////////
// class Parser definition

class Parser
{
public:
    Parser(Tokenizer& tokenizer);
    void ParseScript(AST::Script& outScript);

private:
    Tokenizer& m_Tokenizer;
    vector<Token> m_Tokens;
    size_t m_TokenIndex = 0;

    void ParseBlock(AST::Block& outBlock);
    unique_ptr<AST::Statement> TryParseStatement();
    unique_ptr<AST::ConstantExpression> TryParseConstantExpr();
    unique_ptr<AST::Expression> TryParseExpr0();
    unique_ptr<AST::Expression> TryParseExpr2();
    unique_ptr<AST::Expression> TryParseExpr3();
    unique_ptr<AST::Expression> TryParseExpr5();
    unique_ptr<AST::Expression> TryParseExpr6();
    unique_ptr<AST::Expression> TryParseExpr7();
    unique_ptr<AST::Expression> TryParseExpr9();
    unique_ptr<AST::Expression> TryParseExpr16();
    unique_ptr<AST::Expression> TryParseExpr17() { return TryParseExpr16(); }
    bool TryParseSymbol(Symbol symbol);
    const PlaceInCode& GetCurrentTokenPlace() const { return m_Tokens[m_TokenIndex].Place; }
};
  
  ////////////////////////////////////////////////////////////////////////////////
// class EnvironmentPimpl definition

class EnvironmentPimpl
{
public:
    EnvironmentPimpl();
    ~EnvironmentPimpl();
    void Execute(const char* code, size_t codeLen);
    const string& GetOutput() const { return m_Output; }
    void Print(const char* s, size_t sLen) { m_Output.append(s, s + sLen); }

private:
    AST::Script m_Script;
    Object m_GlobalContext;
    string m_Output;
};

////////////////////////////////////////////////////////////////////////////////
// class Error implementation

const char* Error::what() const
{
    if(m_What.empty())
        Format(m_What, "(%u,%u): %s", m_Place.Row, m_Place.Column, GetMessage());
    return m_What.c_str();
}

////////////////////////////////////////////////////////////////////////////////
// class Tokenizer implementation

Tokenizer::Tokenizer(const char* code, size_t codeLen) :
    m_Code{code, codeLen}
{
}

void Tokenizer::GetNextToken(Token& out)
{
    SkipSpacesAndComments();
    
    out.Place = m_Code.GetCurrentPlace();

    // End of input
    if(m_Code.IsEnd())
    {
        out.Type = TokenType_::End;
        return;
    }

    constexpr Symbol firstMultiCharSymbol = Symbol::Incrementation;
    constexpr Symbol firstKeywordSymbol = Symbol::Null;

    const char* const currentCode = m_Code.GetCurrentCode();
    const size_t currentCodeLen = m_Code.GetCurrentLen();
    // Multi char symbol
    for(size_t i = (size_t)firstMultiCharSymbol; i < (size_t)firstKeywordSymbol; ++i)
    {
        const size_t symbolLen = strlen(SYMBOL_STR[i]);
        if(currentCodeLen >= symbolLen && memcmp(SYMBOL_STR[i], currentCode, symbolLen) == 0)
        {
            out.Type = TokenType_::Symbol;
            out.Symbol = (Symbol)i;
            m_Code.MoveChars(symbolLen);
            return;
        }
    }
    // Symbol
    for(size_t i = 0; i < (size_t)firstMultiCharSymbol; ++i)
    {
        if(currentCode[0] == SYMBOL_STR[i][0])
        {
            out.Type = TokenType_::Symbol;
            out.Symbol = (Symbol)i;
            m_Code.MoveOneChar();
            return;
        }
    }
    // Number
    if(IsDecimalNumber(currentCode[0]))
    {
        size_t tokenLen = 1;
        while(tokenLen < currentCodeLen && IsDecimalNumber(currentCode[tokenLen]))
            ++tokenLen;
        // Letters straight after number are invalid.
        if(tokenLen < currentCodeLen && IsAlpha(currentCode[tokenLen]))
            throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_NUMBER);
        out.Type = TokenType_::Number;
        out.Number = ParseNumber(currentCode, tokenLen);
        m_Code.MoveChars(tokenLen);
        return;
    }
    // Identifier or keyword
    if(IsAlpha(currentCode[0]))
    {
        size_t tokenLen = 1;
        while(tokenLen < currentCodeLen && IsAlphaNumeric(currentCode[tokenLen]))
            ++tokenLen;
        // Keyword
        for(size_t i = (size_t)firstKeywordSymbol; i < (size_t)Symbol::Count; ++i)
        {
            const size_t keywordLen = strlen(SYMBOL_STR[i]);
            if(keywordLen == tokenLen && memcmp(SYMBOL_STR[i], currentCode, tokenLen) == 0)
            {
                out.Type = TokenType_::Symbol;
                out.Symbol = (Symbol)i;
                m_Code.MoveChars(keywordLen);
                return;
            }
        }
        // Identifier
        out.Type = TokenType_::Identifier;
        out.String = string{currentCode, currentCode + tokenLen};
        m_Code.MoveChars(tokenLen);
        return;
    }
    throw ParsingError(out.Place, ERROR_MESSAGE_UNRECOGNIZED_TOKEN);
}

double Tokenizer::ParseNumber(const char* s, size_t sLen)
{
    char sz[32];
    assert(sLen < 32);
    memcpy(sz, s, sLen);
    sz[sLen] = 0;
    return atof(sz);
}

void Tokenizer::SkipSpacesAndComments()
{
    while(!m_Code.IsEnd())
    {
        // Whitespace
        if(isspace(m_Code.GetCurrentChar()))
            m_Code.MoveOneChar();
        // Single line comment
        else if(m_Code.Peek("//", 2))
        {
            m_Code.MoveChars(2);
            while(!m_Code.IsEnd() && m_Code.GetCurrentChar() != '\n')
                m_Code.MoveOneChar();
        }
        // Multi line comment
        else if(m_Code.Peek("/*", 2))
        {
            for(m_Code.MoveChars(2);; m_Code.MoveOneChar())
            {
                if(m_Code.IsEnd())
                    throw ParsingError(m_Code.GetCurrentPlace(), ERROR_MESSAGE_UNEXPECTED_END_OF_FILE_IN_MULTILINE_COMMENT);
                else if(m_Code.Peek("*/", 2))
                {
                    m_Code.MoveChars(2);
                    break;
                }
            }
        }
        else
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// class Object implementation

bool Object::HasKey(const Constant& key) const
{
    switch(key.GetType())
    {
    case Constant::Type::Number:
        return m_NumberMap.find(key.GetNumber()) != m_NumberMap.end();
    case Constant::Type::String:
        return m_StringMap.find(key.GetString()) != m_StringMap.end();
    default: assert(0); return false;
    }
}

bool Object::HasKey(const string& key) const
{
    return m_StringMap.find(key) != m_StringMap.end();
}

Value& Object::GetOrCreateValue(const Constant& key)
{
    switch(key.GetType())
    {
    case Constant::Type::Number:
        return m_NumberMap[key.GetNumber()];
    case Constant::Type::String:
        return m_StringMap[key.GetString()];
    default: assert(0); return m_NumberMap[0];
    }
}

Value& Object::GetOrCreateValue(const string& key)
{
    return m_StringMap[key];
}

Value* Object::TryGetValue(const Constant& key)
{
    switch(key.GetType())
    {
    case Constant::Type::Number:
    {
        auto it = m_NumberMap.find(key.GetNumber());
        if(it != m_NumberMap.end())
            return &it->second;
    }
    case Constant::Type::String:
    {
        auto it = m_StringMap.find(key.GetString());
        if(it != m_StringMap.end())
            return &it->second;
    }
    default: assert(0);
    }
    return nullptr;
}

Value* Object::TryGetValue(const string& key)
{
    auto it = m_StringMap.find(key);
    if(it != m_StringMap.end())
        return &it->second;
    return nullptr;
}

bool Object::Remove(const Constant& key)
{
    switch(key.GetType())
    {
    case Constant::Type::Number:
    {
        auto it = m_NumberMap.find(key.GetNumber());
        if(it != m_NumberMap.end())
        {
            m_NumberMap.erase(it);
            return true;
        }
        return false;
    }
    case Constant::Type::String:
    {
        auto it = m_StringMap.find(key.GetString());
        if(it != m_StringMap.end())
        {
            m_StringMap.erase(it);
            return true;
        }
        return false;
    }
    default: assert(0); return false;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Abstract Syntax Tree implementation

namespace AST {

#define DEBUG_PRINT_FORMAT_STR_BEG "(%u,%u) %s"
#define DEBUG_PRINT_ARGS_BEG GetPlace().Row, GetPlace().Column, GetDebugPrintIndent(indentLevel)

void EmptyStatement::DebugPrint(uint32_t indentLevel) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Empty\n", DEBUG_PRINT_ARGS_BEG);
}

void Condition::DebugPrint(uint32_t indentLevel) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Condition\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    ConditionExpression->DebugPrint(indentLevel);
    Statements[0]->DebugPrint(indentLevel);
    if(Statements[1])
        Statements[1]->DebugPrint(indentLevel);
}

void Condition::Execute(ExecuteContext& ctx) const
{
    if(ConditionExpression->Evaluate(ctx).IsTrue())
        Statements[0]->Execute(ctx);
    else if(Statements[1])
        Statements[1]->Execute(ctx);
}

void WhileLoop::DebugPrint(uint32_t indentLevel) const
{
    const char* name = nullptr;
    switch(Type)
    {
    case WhileLoopType::While: name = "While"; break;
    case WhileLoopType::DoWhile: name = "DoWhile"; break;
    default: assert(0);
    }
    printf(DEBUG_PRINT_FORMAT_STR_BEG "%s\n", DEBUG_PRINT_ARGS_BEG, name);
    ++indentLevel;
    ConditionExpression->DebugPrint(indentLevel);
    Body->DebugPrint(indentLevel);
}

void WhileLoop::Execute(ExecuteContext& ctx) const
{
    switch(Type)
    {
    case WhileLoopType::While:
        while(ConditionExpression->Evaluate(ctx).IsTrue())
            Body->Execute(ctx);
        break;
    case WhileLoopType::DoWhile:
        do
            Body->Execute(ctx);
        while(ConditionExpression->Evaluate(ctx).IsTrue());
        break;
    default: assert(0);
    }
}

void ForLoop::DebugPrint(uint32_t indentLevel) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "For\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    if(InitExpression)
        InitExpression->DebugPrint(indentLevel);
    else
        printf(DEBUG_PRINT_FORMAT_STR_BEG "(Init expression empty)\n", DEBUG_PRINT_ARGS_BEG);
    if(ConditionExpression)
        ConditionExpression->DebugPrint(indentLevel);
    else
        printf(DEBUG_PRINT_FORMAT_STR_BEG "(Condition expression empty)\n", DEBUG_PRINT_ARGS_BEG);
    if(IterationExpression)
        IterationExpression->DebugPrint(indentLevel);
    else
        printf(DEBUG_PRINT_FORMAT_STR_BEG "(Iteration expression empty)\n", DEBUG_PRINT_ARGS_BEG);
    Body->DebugPrint(indentLevel);
}

void ForLoop::Execute(ExecuteContext& ctx) const
{
    if(InitExpression)
        InitExpression->Execute(ctx);
    while(ConditionExpression ? ConditionExpression->Evaluate(ctx).IsTrue() : true)
    {
        Body->Execute(ctx);
        if(IterationExpression)
            IterationExpression->Execute(ctx);
    }
}

void Block::DebugPrint(uint32_t indentLevel) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Block\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    for(const auto& stmtPtr : Statements)
        stmtPtr->DebugPrint(indentLevel);
}

void Block::Execute(ExecuteContext& ctx) const
{
    for(const auto& stmtPtr : Statements)
        stmtPtr->Execute(ctx);
}

LValue Expression::GetLValue(ExecuteContext& ctx) const
{
    throw ExecutionError(GetPlace(), string(ERROR_MESSAGE_EXPECTED_LVALUE));
}

void ConstantValue::DebugPrint(uint32_t indentLevel) const
{
    switch(Val.GetType())
    {
    case Value::Type::Number: printf(DEBUG_PRINT_FORMAT_STR_BEG "Constant number: %g\n", DEBUG_PRINT_ARGS_BEG, Val.GetNumber()); break;
    case Value::Type::String: printf(DEBUG_PRINT_FORMAT_STR_BEG "Constant string: %s\n", DEBUG_PRINT_ARGS_BEG, Val.GetString().c_str()); break;
    default: assert(0);
    }
}

void Identifier::DebugPrint(uint32_t indentLevel) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Identifier: %s\n", DEBUG_PRINT_ARGS_BEG, S.c_str());
}

Value Identifier::Evaluate(ExecuteContext& ctx) const
{
    // #TODO local, this, then global
    Value* val = ctx.GlobalContext.TryGetValue(S);
    if(val)
        return *val;
    throw ExecutionError(GetPlace(), string("Variable \"") + S + "\" doesn't exist.");
}

LValue Identifier::GetLValue(ExecuteContext& ctx) const
{
    // #TODO local, this, then global
    return LValue{ctx.GlobalContext, Constant{string(S)}};
}

void UnaryOperator::DebugPrint(uint32_t indentLevel) const
{
    static const char* UNARY_OPERATOR_TYPE_NAMES[] = {
        "Preincrementation", "Predecrementation", "Plus", "Minus", "Logical NOT", "Bitwise NOT" };
    printf(DEBUG_PRINT_FORMAT_STR_BEG "UnaryOperator %s\n", DEBUG_PRINT_ARGS_BEG, UNARY_OPERATOR_TYPE_NAMES[(uint32_t)Type]);
    ++indentLevel;
    Operand->DebugPrint(indentLevel);
}

Value UnaryOperator::Evaluate(ExecuteContext& ctx) const
{
    if(Type == UnaryOperatorType::Preincrementation ||
        Type == UnaryOperatorType::Predecrementation)
    {
        LValue lval = Operand->GetLValue(ctx);
        Value* val = lval.Obj.TryGetValue(lval.Key);
        if(val == nullptr)
            throw ExecutionError(GetPlace(), string(ERROR_MESSAGE_VARIABLE_DOESNT_EXIST));
        if(val->GetType() != Value::Type::Number)
            throw ExecutionError(GetPlace(), string(ERROR_MESSAGE_EXPECTED_NUMBER));
        switch(Type)
        {
        case UnaryOperatorType::Preincrementation: val->SetNumber(val->GetNumber() + 1.0); break;
        case UnaryOperatorType::Predecrementation: val->SetNumber(val->GetNumber() - 1.0); break;
        default: assert(0); return Value{};
        }
        return *val;
    }
    else if(Type == UnaryOperatorType::Plus ||
        Type == UnaryOperatorType::Minus ||
        Type == UnaryOperatorType::LogicalNot ||
        Type == UnaryOperatorType::BitwiseNot)
    {
        Value val = Operand->Evaluate(ctx);
        if(val.GetType() != Value::Type::Number)
            throw ExecutionError(GetPlace(), string(ERROR_MESSAGE_EXPECTED_NUMBER));
        switch(Type)
        {
        case UnaryOperatorType::Plus: return val;
        case UnaryOperatorType::Minus: return Value{-val.GetNumber()};
        case UnaryOperatorType::LogicalNot: return Value{val.IsTrue() ? 0.0 : 1.0};
        case UnaryOperatorType::BitwiseNot: return BitwiseNot(std::move(val));
        default: assert(0); return Value{};
        }
    }
    assert(0); return Value{};
}

Value UnaryOperator::BitwiseNot(Value&& operand) const
{
    const int64_t operandInt = (int64_t)operand.GetNumber();
    const int64_t resultInt = ~operandInt;
    return Value{(double)resultInt};
}

void BinaryOperator::DebugPrint(uint32_t indentLevel) const
{
    static const char* BINARY_OPERATOR_TYPE_NAMES[] = {
        "Mul", "Div", "Mod", "Add", "Sub", "Shift left", "Shift right", "Assignment" };
    printf(DEBUG_PRINT_FORMAT_STR_BEG "BinaryOperator %s\n", DEBUG_PRINT_ARGS_BEG, BINARY_OPERATOR_TYPE_NAMES[(uint32_t)Type]);
    ++indentLevel;
    Operands[0]->DebugPrint(indentLevel);
    Operands[1]->DebugPrint(indentLevel);
}

Value BinaryOperator::Evaluate(ExecuteContext& ctx) const
{
    // This operator is special, requires l-value.
    if(Type == BinaryOperatorType::Assignment)
    {
        LValue lhs = Operands[0]->GetLValue(ctx);
        Value rhs = Operands[1]->Evaluate(ctx);
        return Assignment(lhs, std::move(rhs));
    }

    // Remaining operators use r-values.
    Value lhs = Operands[0]->Evaluate(ctx);
    Value rhs = Operands[1]->Evaluate(ctx);

    // Remaining operators require numbers.
    CheckNumberOperand(Operands[0].get(), lhs);
    CheckNumberOperand(Operands[1].get(), rhs);

    switch(Type)
    {
    case BinaryOperatorType::Mul:          return Value{lhs.GetNumber() * rhs.GetNumber()};
    case BinaryOperatorType::Div:          return Value{lhs.GetNumber() / rhs.GetNumber()};
    case BinaryOperatorType::Mod:          return Value{fmod(lhs.GetNumber(), rhs.GetNumber())};
    case BinaryOperatorType::Add:          return Value{lhs.GetNumber() + rhs.GetNumber()};
    case BinaryOperatorType::Sub:          return Value{lhs.GetNumber() - rhs.GetNumber()};
    case BinaryOperatorType::ShiftLeft:    return ShiftLeft(std::move(lhs), std::move(rhs));
    case BinaryOperatorType::ShiftRight:   return ShiftRight(std::move(lhs), std::move(rhs));
    case BinaryOperatorType::Less:         return Value{lhs.GetNumber() <  rhs.GetNumber() ? 1.0 : 0.0};
    case BinaryOperatorType::LessEqual:    return Value{lhs.GetNumber() <= rhs.GetNumber() ? 1.0 : 0.0};
    case BinaryOperatorType::Greater:      return Value{lhs.GetNumber() >  rhs.GetNumber() ? 1.0 : 0.0};
    case BinaryOperatorType::GreaterEqual: return Value{lhs.GetNumber() >= rhs.GetNumber() ? 1.0 : 0.0};
    }

    assert(0); return Value{};
}

Value BinaryOperator::ShiftLeft(Value&& lhs, Value&& rhs) const
{
    const int64_t lhsInt = (int64_t)lhs.GetNumber();
    const int64_t rhsInt = (int64_t)rhs.GetNumber();
    const int64_t resultInt = lhsInt << rhsInt;
    return Value{(double)resultInt};
}

Value BinaryOperator::ShiftRight(Value&& lhs, Value&& rhs) const
{
    const int64_t lhsInt = (int64_t)lhs.GetNumber();
    const int64_t rhsInt = (int64_t)rhs.GetNumber();
    const int64_t resultInt = lhsInt >> rhsInt;
    return Value{(double)resultInt};
}

Value BinaryOperator::Assignment(const LValue& lhs, Value&& rhs) const
{
    Value& valRef = lhs.Obj.GetOrCreateValue(lhs.Key);
    valRef = std::move(rhs);
    return valRef;
}

void TernaryOperator::DebugPrint(uint32_t indentLevel) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "TernaryOperator\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    Operands[0]->DebugPrint(indentLevel);
    Operands[1]->DebugPrint(indentLevel);
    Operands[2]->DebugPrint(indentLevel);
}

Value TernaryOperator::Evaluate(ExecuteContext& ctx) const
{
    return Operands[0]->Evaluate(ctx).IsTrue() ? Operands[1]->Evaluate(ctx) : Operands[2]->Evaluate(ctx);
}

void MultiOperator::DebugPrint(uint32_t indentLevel) const
{
    static const char* MULTI_OPERATOR_TYPE_NAMES[] = { "None", "Call" };
    printf(DEBUG_PRINT_FORMAT_STR_BEG "MultiOperator %s\n", DEBUG_PRINT_ARGS_BEG, MULTI_OPERATOR_TYPE_NAMES[(uint32_t)Type]);
    ++indentLevel;
    for(const auto& exprPtr : Operands)
        exprPtr->DebugPrint(indentLevel);
}

} // namespace AST

////////////////////////////////////////////////////////////////////////////////
// Built in functions

Value BuiltInFunction_Print(AST::ExecuteContext& ctx, const Value* args, size_t argCount)
{
    string s;
    for(size_t i = 0; i < argCount; ++i)
    {
        const Value& val = args[i];
        switch(val.GetType())
        {
        case Value::Type::Number: Format(s, "%g\n", val.GetNumber()); break;
        case Value::Type::String: Format(s, "%s\n", val.GetString().c_str()); break;
        default: assert(0);
        }
        if(!s.empty())
            ctx.Env.Print(s.data(), s.length());
    }
    return Value{};
}

////////////////////////////////////////////////////////////////////////////////
// AST implementation

namespace AST {

Value MultiOperator::Evaluate(ExecuteContext& ctx) const
{
    switch(Type)
    {
    case MultiOperatorType::Call: return Call(ctx);
    default: assert(0); return Value{};
    }
}

Value MultiOperator::Call(ExecuteContext& ctx) const
{
    Identifier* calleeIdentifier = dynamic_cast<Identifier*>(Operands[0].get()); assert(calleeIdentifier); // #TODO make it flexible!
    const size_t argCount = Operands.size() - 1;
    vector<Value> values(argCount);
    for(size_t i = 0; i < argCount; ++i)
        values[i] = Operands[i + 1]->Evaluate(ctx);
    if(calleeIdentifier->S == "print")
        return BuiltInFunction_Print(ctx, values.data(), values.size());
    else
        throw ExecutionError(GetPlace(), string("Unknown function: ") + calleeIdentifier->S);
}

} // namespace AST

////////////////////////////////////////////////////////////////////////////////
// class Parser implementation

#define MUST_PARSE(result, errorMessage)   do { if(!(result)) throw ParsingError(GetCurrentTokenPlace(), (errorMessage)); } while(false)

Parser::Parser(Tokenizer& tokenizer) :
    m_Tokenizer(tokenizer)
{
}

void Parser::ParseScript(AST::Script& outScript)
{
    for(;;)
    {
        Token token;
        m_Tokenizer.GetNextToken(token);
        m_Tokens.push_back(std::move(token));
        if(token.Type == TokenType_::End)
            break;
    }

    ParseBlock(outScript);
    if(m_Tokens[m_TokenIndex].Type != TokenType_::End)
        throw ParsingError(GetCurrentTokenPlace(), ERROR_MESSAGE_PARSING_ERROR);

    //outScript.DebugPrint(0); // #DELME
}

void Parser::ParseBlock(AST::Block& outBlock)
{
    while(m_Tokens[m_TokenIndex].Type != TokenType_::End)
    {
        unique_ptr<AST::Statement> stmt = TryParseStatement();
        if(!stmt)
            break;
        outBlock.Statements.push_back(std::move(stmt));
    }
}

unique_ptr<AST::Statement> Parser::TryParseStatement()
{
    const PlaceInCode place = GetCurrentTokenPlace();
    
    // Empty statement: ';'
    if(TryParseSymbol(Symbol::Semicolon))
        return make_unique<AST::EmptyStatement>(place);
    
    // Block: '{' Block '}'
    if(TryParseSymbol(Symbol::CurlyBracketOpen))
    {
        unique_ptr<AST::Block> block = make_unique<AST::Block>(GetCurrentTokenPlace());
        ParseBlock(*block);
        MUST_PARSE( TryParseSymbol(Symbol::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE );
        return block;
    }

    // Condition: 'if' '(' Expr17 ')' Statement [ 'else' Statement ]
    if(TryParseSymbol(Symbol::If))
    {
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN );
        unique_ptr<AST::Condition> condition = make_unique<AST::Condition>(place);
        MUST_PARSE( condition->ConditionExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE );
        MUST_PARSE( condition->Statements[0] = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT );
        if(TryParseSymbol(Symbol::Else))
            MUST_PARSE( condition->Statements[1] = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT );
        return condition;
    }

    // Loop: 'while' '(' Expr17 ')' Statement
    if(TryParseSymbol(Symbol::While))
    {
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN );
        unique_ptr<AST::WhileLoop> loop = make_unique<AST::WhileLoop>(place, AST::WhileLoopType::While);
        MUST_PARSE( loop->ConditionExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE );
        MUST_PARSE( loop->Body = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT );
        return loop;
    }

    // Loop: 'do' Statement 'while' '(' Expr17 ')' ';'    - loop
    if(TryParseSymbol(Symbol::Do))
    {
        unique_ptr<AST::WhileLoop> loop = make_unique<AST::WhileLoop>(place, AST::WhileLoopType::DoWhile);
        MUST_PARSE( loop->Body = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT );
        MUST_PARSE( TryParseSymbol(Symbol::While), ERROR_MESSAGE_EXPECTED_SYMBOL_WHILE );
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN );
        MUST_PARSE( loop->ConditionExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE );
        MUST_PARSE( TryParseSymbol(Symbol::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON );
        return loop;
    }

    // Loop: 'for' '(' Expr17? ';' Expr17? ';' Expr17? ')' Statement
    if(TryParseSymbol(Symbol::For))
    {
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN );
        unique_ptr<AST::ForLoop> loop = make_unique<AST::ForLoop>(place);
        if(!TryParseSymbol(Symbol::Semicolon))
        {
            MUST_PARSE( loop->InitExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
            MUST_PARSE( TryParseSymbol(Symbol::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON );
        }
        if(!TryParseSymbol(Symbol::Semicolon))
        {
            MUST_PARSE( loop->ConditionExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
            MUST_PARSE( TryParseSymbol(Symbol::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON );
        }
        if(!TryParseSymbol(Symbol::RoundBracketClose))
        {
            MUST_PARSE( loop->IterationExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
            MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE );
        }
        MUST_PARSE( loop->Body = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT );
        return loop;
    }
    
    // Expression as statement: Expr17 ';'
    unique_ptr<AST::Expression> expr = TryParseExpr17();
    if(expr)
    {
        MUST_PARSE( TryParseSymbol(Symbol::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON );
        return expr;
    }
    
    return unique_ptr<AST::Statement>{};
}

unique_ptr<AST::ConstantExpression> Parser::TryParseConstantExpr()
{
    const Token& t = m_Tokens[m_TokenIndex];
    switch(t.Type)
    {
    case TokenType_::Number:
        ++m_TokenIndex;
        return make_unique<AST::ConstantValue>(t.Place, Value{t.Number});
    case TokenType_::Identifier:
        ++m_TokenIndex;
        return make_unique<AST::Identifier>(t.Place, string(t.String));
    case TokenType_::Symbol:
        switch(t.Symbol)
        {
        case Symbol::Null:
            ++m_TokenIndex;
            return make_unique<AST::ConstantValue>(t.Place, Value{});
        case Symbol::False:
            ++m_TokenIndex;
            return make_unique<AST::ConstantValue>(t.Place, Value{0.0});
        case Symbol::True:
            ++m_TokenIndex;
            return make_unique<AST::ConstantValue>(t.Place, Value{1.0});
        }
        break;
    }
    return unique_ptr<AST::ConstantExpression>{};
}

unique_ptr<AST::Expression> Parser::TryParseExpr0()
{
    // '(' Expr17 ')'
    if(TryParseSymbol(Symbol::RoundBracketOpen))
    {
        unique_ptr<AST::Expression> expr;
        MUST_PARSE( expr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE );
        return expr;
    }
    else
    {
        // Constant
        unique_ptr<AST::ConstantExpression> constant = TryParseConstantExpr();
        if(constant)
            return constant;
    }
    return unique_ptr<AST::Expression>{};
}

unique_ptr<AST::Expression> Parser::TryParseExpr2()
{
    unique_ptr<AST::Expression> expr = TryParseExpr0();
    if(expr)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        // Call: Expr0 '(' [ Expr16 ( ',' Expr16 )* ')'
        if(TryParseSymbol(Symbol::RoundBracketOpen))
        {
            unique_ptr<AST::MultiOperator> multiOperator = make_unique<AST::MultiOperator>(place, AST::MultiOperatorType::Call);
            // Callee
            multiOperator->Operands.push_back(std::move(expr));
            // First argument
            expr = TryParseExpr16();
            if(expr)
            {
                multiOperator->Operands.push_back(std::move(expr));
                // Further arguments
                while(TryParseSymbol(Symbol::Comma))
                {
                    MUST_PARSE( expr = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
                    multiOperator->Operands.push_back(std::move(expr));
                }
            }
            MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE );
            return multiOperator;
        }
        // #TODO Indexing: Expr0 '[' Expr17 ']'
        // #TODO Member access: Expr0 '.' TOKEN_IDENTIFIER
        // Just Expr0
        return expr;
    }
    return unique_ptr<AST::Expression>{};
}

unique_ptr<AST::Expression> Parser::TryParseExpr3()
{
    const PlaceInCode place = GetCurrentTokenPlace();
#define PARSE_UNARY_OPERATOR(symbol, unaryOperatorType) \
    if(TryParseSymbol(symbol)) \
    { \
        unique_ptr<AST::UnaryOperator> op = make_unique<AST::UnaryOperator>(place, (unaryOperatorType)); \
        MUST_PARSE( op->Operand = TryParseExpr3(), ERROR_MESSAGE_EXPECTED_EXPRESSION ); \
        return op; \
    }
    PARSE_UNARY_OPERATOR(Symbol::Incrementation, AST::UnaryOperatorType::Preincrementation)
    PARSE_UNARY_OPERATOR(Symbol::Decrementation, AST::UnaryOperatorType::Predecrementation)
    PARSE_UNARY_OPERATOR(Symbol::Add, AST::UnaryOperatorType::Plus)
    PARSE_UNARY_OPERATOR(Symbol::Sub, AST::UnaryOperatorType::Minus)
    PARSE_UNARY_OPERATOR(Symbol::ExclamationMark, AST::UnaryOperatorType::LogicalNot)
    PARSE_UNARY_OPERATOR(Symbol::Tilde, AST::UnaryOperatorType::BitwiseNot)
#undef PARSE_UNARY_OPERATOR
    // Just Expr2 or null if failed.
    return TryParseExpr2();
}

#define PARSE_BINARY_OPERATOR(binaryOperatorType, exprParseFunc) \
    { \
        unique_ptr<AST::BinaryOperator> op = make_unique<AST::BinaryOperator>(place, (binaryOperatorType)); \
        op->Operands[0] = std::move(expr); \
        MUST_PARSE( op->Operands[1] = (exprParseFunc)(), ERROR_MESSAGE_EXPECTED_EXPRESSION ); \
        expr = std::move(op); \
    }

unique_ptr<AST::Expression> Parser::TryParseExpr5()
{
    unique_ptr<AST::Expression> expr = TryParseExpr3();
    if(expr)
    {
        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Symbol::Mul))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Mul, TryParseExpr3)
            else if(TryParseSymbol(Symbol::Div))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Div, TryParseExpr3)
            else if(TryParseSymbol(Symbol::Mod))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Mod, TryParseExpr3)
            else
                break;
        }
        return expr;
    }
    return unique_ptr<AST::Expression>{};
}

unique_ptr<AST::Expression> Parser::TryParseExpr6()
{
    unique_ptr<AST::Expression> expr = TryParseExpr5();
    if(expr)
    {
        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Symbol::Add))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Add, TryParseExpr5)
            else if(TryParseSymbol(Symbol::Sub))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Sub, TryParseExpr5)
            else
                break;
        }
        return expr;
    }
    return unique_ptr<AST::Expression>{};
}

unique_ptr<AST::Expression> Parser::TryParseExpr7()
{
    unique_ptr<AST::Expression> expr = TryParseExpr6();
    if(expr)
    {
        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Symbol::ShiftLeft))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::ShiftLeft, TryParseExpr6)
            else if(TryParseSymbol(Symbol::ShiftRight))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::ShiftRight, TryParseExpr6)
            else
                break;
        }
        return expr;
    }
    return unique_ptr<AST::Expression>{};
}

unique_ptr<AST::Expression> Parser::TryParseExpr9()
{
    unique_ptr<AST::Expression> expr = TryParseExpr7();
    if(expr)
    {
        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Symbol::Less))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Less, TryParseExpr7)
            else if(TryParseSymbol(Symbol::LessEqual))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::LessEqual, TryParseExpr7)
            else if(TryParseSymbol(Symbol::Greater))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Greater, TryParseExpr7)
            else if(TryParseSymbol(Symbol::GreaterEqual))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::GreaterEqual, TryParseExpr7)
            else
                break;
        }
        return expr;
    }
    return unique_ptr<AST::Expression>{};
}

unique_ptr<AST::Expression> Parser::TryParseExpr16()
{
    unique_ptr<AST::Expression> expr = TryParseExpr9(); // #TODO TryParseExpr15
    if(expr)
    {
        // Ternary operator: Expr15 '?' Expr16 ':' Expr16
        if(TryParseSymbol(Symbol::QuestionMark))
        {
            unique_ptr<AST::TernaryOperator> op = make_unique<AST::TernaryOperator>(GetCurrentTokenPlace());
            op->Operands[0] = std::move(expr);
            MUST_PARSE( op->Operands[1] = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
            MUST_PARSE( TryParseSymbol(Symbol::Colon), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON );
            MUST_PARSE( op->Operands[2] = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
            return op;
        }
        // Assignment: Expr15 = Expr16
        if(TryParseSymbol(Symbol::Equal))
        {
            unique_ptr<AST::BinaryOperator> op = make_unique<AST::BinaryOperator>(GetCurrentTokenPlace(), AST::BinaryOperatorType::Assignment);
            op->Operands[0] = std::move(expr);
            MUST_PARSE( op->Operands[1] = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
            return op;
        }
        // Just Expr15
        return expr;
    }
    return unique_ptr<AST::Expression>{};
}

bool Parser::TryParseSymbol(Symbol symbol)
{
    if(m_Tokens[m_TokenIndex].Type == TokenType_::Symbol &&
        m_Tokens[m_TokenIndex].Symbol == symbol)
    {
        ++m_TokenIndex;
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
// class EnvironmentPimpl implementation

EnvironmentPimpl::EnvironmentPimpl() :
    m_Script{PlaceInCode{0, 1, 1}}
{
}

EnvironmentPimpl::~EnvironmentPimpl()
{
}

void EnvironmentPimpl::Execute(const char* code, size_t codeLen)
{
    {
        Tokenizer tokenizer{code, codeLen};
        Parser parser{tokenizer};
        parser.ParseScript(m_Script);
    }

    AST::ExecuteContext executeContext{*this, m_GlobalContext};
    m_Script.Execute(executeContext);
}

////////////////////////////////////////////////////////////////////////////////
// class Environment

Environment::Environment() : pimpl{new EnvironmentPimpl{}} { }
Environment::~Environment() { delete pimpl; }
void Environment::Execute(const char* code, size_t codeLen) { pimpl->Execute(code, codeLen); }
const std::string& Environment::GetOutput() const { return pimpl->GetOutput(); }

} // namespace MinScriptLang

#endif // MIN_SCRIPT_LANG_IMPLEMENTATION
