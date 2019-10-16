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
    Error(const PlaceInCode& place, std::string&& message) : m_Place{place}, m_Message{std::move(message)} { }
    virtual const char* what() const;
    const PlaceInCode& GetPlace() const { return m_Place; }
    const std::string& GetMessage() const { return m_Message; }
private:
    PlaceInCode m_Place;
    std::string m_Message;
    mutable std::string m_What;
};

class ParsingError : public Error
{
public:
    ParsingError(const PlaceInCode& place, std::string&& message) : Error{place, std::move(message)} { }
};

class ExecutionError : public Error
{
public:
    ExecutionError(const PlaceInCode& place, std::string&& message) : Error{place, std::move(message)} { }
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
#include <string>
#include <memory>
#include <algorithm>

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cmath>
#include <ctype.h>

using std::unique_ptr;
using std::string;
using std::vector;

// F***k Windows.h macros!
#undef min
#undef max

namespace MinScriptLang {

////////////////////////////////////////////////////////////////////////////////
// Basic facilities

// Name with underscore because f***g Windows.h defines TokenType as macro!
enum class TokenType_
{
    None,
    Symbol,
    Identifier,
    Number,
    End,
};

enum class Symbol
{
    Comma,             // ,
    Semicolon,         // ;
    RoundBrackerOpen,  // (
    RoundBracketClose, // )
    Mul,               // *
    Div,               // /
    Mod,               // %
    Add,               // +
    Sub,               // -
};
static const char* SYMBOL_STR[] = { ",", ";", "(", ")", "*", "/", "%", "+", "-", };

struct Token
{
    PlaceInCode Place;
    TokenType_ Type;
    Symbol Symbol; // Only when Type == TokenType::Symbol
    string String; // Only when Type == TokenType::Identifier
    double Number; // Only when Type == TokenType::Number
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
    enum class Type { Null, Number, String };

    Value() { }
    Value(Type type) : m_Type(type) { }
    Value(double number) : m_Type(Type::Number), m_Number(number) { }
    Value(string&& str) : m_Type(Type::String), m_String(std::move(str)) { }

    Type GetType() const { return m_Type; }
    double GetNumber() const { assert(m_Type == Type::Number); return m_Number; }
    const string& GetString() const { assert(m_Type == Type::String); return m_String; }

    void SetNull() { m_Type = Type::Null; m_String.clear(); }
    void SetNumber(double number) { m_Type = Type::Number; m_Number = number; m_String.clear(); }
    void SetString(string&& str) { m_Type = Type::String; m_String = std::move(str); }

private:
    Type m_Type = Type::Null;
    double m_Number = 0.0;
    string m_String;
};

////////////////////////////////////////////////////////////////////////////////
// Abstract Syntax Tree definitions

namespace AST
{

struct ExecuteContext
{
    EnvironmentPimpl& env;
};

struct Statement
{
    Statement(const PlaceInCode& place) : m_Place{place} { }
    virtual ~Statement() { }
    const PlaceInCode& GetPlace() const { return m_Place; }
    virtual void DebugPrint(uint32_t indentLevel) const = 0;
    virtual void Execute(ExecuteContext& ctx) const = 0;
private:
    const PlaceInCode m_Place;
};

struct EmptyStatement : public Statement
{
    EmptyStatement(const PlaceInCode& place) : Statement{place} { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual void Execute(ExecuteContext& ctx) const { }
};

struct Block : public Statement
{
    Block(const PlaceInCode& place) : Statement{place} { }
    vector<unique_ptr<Statement>> Statements;
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

struct Script : Block
{
    Script(const PlaceInCode& place) : Block{place} { }
};

struct Expression : Statement
{
    Expression(const PlaceInCode& place) : Statement{place} { }
    virtual Value Evaluate(ExecuteContext& ctx) const = 0;
    virtual void Execute(ExecuteContext& ctx) const { Evaluate(ctx); }
};

struct Constant : Expression
{
    Constant(const PlaceInCode& place) : Expression{place} { }
    virtual void Execute(ExecuteContext& ctx) const { /* Nothing - just ignore its value. */ }
};

struct NumberConstant : Constant
{
    double Number;
    NumberConstant(const PlaceInCode& place, double number) : Constant{place}, Number(number) { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual Value Evaluate(ExecuteContext& ctx) const { return Value{Number}; }
};

struct IdentifierConstant : Constant
{
    string Identifier;
    IdentifierConstant(const PlaceInCode& place, string&& identifier) : Constant{place}, Identifier(std::move(identifier)) { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual Value Evaluate(ExecuteContext& ctx) const { return Value{string(Identifier)}; }
};

struct Operator : Expression
{
    Operator(const PlaceInCode& place) : Expression{place} { }
};

enum class UnaryOperatorType
{
    None,
};

struct UnaryOperator : Operator
{
    UnaryOperatorType Type;
    unique_ptr<Expression> Operand;
    UnaryOperator(const PlaceInCode& place, UnaryOperatorType type) : Operator{place}, Type(type) { }
    virtual void DebugPrint(uint32_t indentLevel) const { }
    virtual Value Evaluate(ExecuteContext& ctx) const { return Value{}; }
};

enum class BinaryOperatorType
{
    Mul,
    Div,
    Mod,
    Add,
    Sub,
};

struct BinaryOperator : Operator
{
    BinaryOperatorType Type;
    unique_ptr<Expression> Operands[2];
    BinaryOperator(const PlaceInCode& place, BinaryOperatorType type) : Operator{place}, Type(type) { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual Value Evaluate(ExecuteContext& ctx) const;
    
private:
    Value Mul(Value&& lhs, Value&& rhs) const;
    Value Div(Value&& lhs, Value&& rhs) const;
    Value Mod(Value&& lhs, Value&& rhs) const;
    Value Add(Value&& lhs, Value&& rhs) const;
    Value Sub(Value&& lhs, Value&& rhs) const;
};

struct TernaryOperator : Operator
{
    unique_ptr<Expression> Operands[3];
    TernaryOperator(const PlaceInCode& place) : Operator{place} { }
    virtual void DebugPrint(uint32_t indentLevel) const { }
    virtual Value Evaluate(ExecuteContext& ctx) const { return Value{}; }
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
        throw ExecutionError(operand->GetPlace(), "Operand requires number operand.");
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

    void ParseBlockContent(AST::Block& outBlock);
    unique_ptr<AST::Statement> TryParseStatement();
    unique_ptr<AST::Constant> TryParseConstant();
    unique_ptr<AST::Expression> TryParseExpr0();
    unique_ptr<AST::Expression> TryParseExpr2();
    unique_ptr<AST::Expression> TryParseExpr5();
    unique_ptr<AST::Expression> TryParseExpr6();
    unique_ptr<AST::Expression> TryParseExpr16() { return TryParseExpr6(); }
    unique_ptr<AST::Expression> TryParseExpr17() { return TryParseExpr16(); }
    bool TryParseSymbol(Symbol symbol);
    void ParseSymbol(Symbol symbol);
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
    string m_Output;
};

////////////////////////////////////////////////////////////////////////////////
// class Error implementation

const char* Error::what() const
{
    if(m_What.empty())
        Format(m_What, "(%u,%u): %s", m_Place.Row, m_Place.Column, m_Message.c_str());
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
    // Symbols
    const char* const currentCode = m_Code.GetCurrentCode();
    if(currentCode[0] == ',') { m_Code.MoveOneChar(); out.Type = TokenType_::Symbol; out.Symbol = Symbol::Comma; return; }
    if(currentCode[0] == ';') { m_Code.MoveOneChar(); out.Type = TokenType_::Symbol; out.Symbol = Symbol::Semicolon; return; }
    if(currentCode[0] == '(') { m_Code.MoveOneChar(); out.Type = TokenType_::Symbol; out.Symbol = Symbol::RoundBrackerOpen; return; }
    if(currentCode[0] == ')') { m_Code.MoveOneChar(); out.Type = TokenType_::Symbol; out.Symbol = Symbol::RoundBracketClose; return; }
    if(currentCode[0] == '*') { m_Code.MoveOneChar(); out.Type = TokenType_::Symbol; out.Symbol = Symbol::Mul; return; }
    if(currentCode[0] == '/') { m_Code.MoveOneChar(); out.Type = TokenType_::Symbol; out.Symbol = Symbol::Div; return; }
    if(currentCode[0] == '%') { m_Code.MoveOneChar(); out.Type = TokenType_::Symbol; out.Symbol = Symbol::Mod; return; }
    if(currentCode[0] == '+') { m_Code.MoveOneChar(); out.Type = TokenType_::Symbol; out.Symbol = Symbol::Add; return; }
    if(currentCode[0] == '-') { m_Code.MoveOneChar(); out.Type = TokenType_::Symbol; out.Symbol = Symbol::Sub; return; }
    // Number
    const size_t currentCodeLen = m_Code.GetCurrentLen();
    if(IsDecimalNumber(currentCode[0]))
    {
        size_t tokenLen = 1;
        while(tokenLen < currentCodeLen && IsDecimalNumber(currentCode[tokenLen]))
            ++tokenLen;
        out.Type = TokenType_::Number;
        out.Number = ParseNumber(currentCode, tokenLen);
        m_Code.MoveChars(tokenLen);
        return;
    }
    // Identifier
    if(IsAlpha(currentCode[0]))
    {
        size_t tokenLen = 1;
        while(tokenLen < currentCodeLen && IsAlphaNumeric(currentCode[tokenLen]))
            ++tokenLen;
        out.Type = TokenType_::Identifier;
        out.String = string{currentCode, currentCode + tokenLen};
        m_Code.MoveChars(tokenLen);
        return;
    }
    throw ParsingError(out.Place, "Unrecognized token.");
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
                    throw ParsingError(m_Code.GetCurrentPlace(), string("Unexpected end of file inside multiline comment."));
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
// Abstract Syntax Tree implementation

namespace AST {

#define DEBUG_PRINT_FORMAT_STR_BEG "(%u,%u) %s"
#define DEBUG_PRINT_ARGS_BEG GetPlace().Row, GetPlace().Column, GetDebugPrintIndent(indentLevel)

void EmptyStatement::DebugPrint(uint32_t indentLevel) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Empty\n", DEBUG_PRINT_ARGS_BEG);
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

void NumberConstant::DebugPrint(uint32_t indentLevel) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Number %g\n", DEBUG_PRINT_ARGS_BEG, Number);
}

void IdentifierConstant::DebugPrint(uint32_t indentLevel) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Identifier %s\n", DEBUG_PRINT_ARGS_BEG, Identifier.c_str());
}

void BinaryOperator::DebugPrint(uint32_t indentLevel) const
{
    static const char* BINARY_OPERATOR_TYPE_NAMES[] = { "Mul", "Div", "Mod", "Add", "Sub" };
    printf(DEBUG_PRINT_FORMAT_STR_BEG "BinaryOperator %s\n", DEBUG_PRINT_ARGS_BEG, BINARY_OPERATOR_TYPE_NAMES[(uint32_t)Type]);
    ++indentLevel;
    Operands[0]->DebugPrint(indentLevel);
    Operands[1]->DebugPrint(indentLevel);
}

Value BinaryOperator::Evaluate(ExecuteContext& ctx) const
{
    Value lhs = Operands[0]->Evaluate(ctx);
    Value rhs = Operands[1]->Evaluate(ctx);
    switch(Type)
    {
    case BinaryOperatorType::Mul: return Mul(std::move(lhs), std::move(rhs));
    case BinaryOperatorType::Div: return Div(std::move(lhs), std::move(rhs));
    case BinaryOperatorType::Mod: return Mod(std::move(lhs), std::move(rhs));
    case BinaryOperatorType::Add: return Add(std::move(lhs), std::move(rhs));
    case BinaryOperatorType::Sub: return Sub(std::move(lhs), std::move(rhs));
    default: assert(0); return Value{};
    }
}

Value BinaryOperator::Mul(Value&& lhs, Value&& rhs) const
{
    CheckNumberOperand(Operands[0].get(), lhs);
    CheckNumberOperand(Operands[1].get(), rhs);
    return Value{lhs.GetNumber() * rhs.GetNumber()};
}

Value BinaryOperator::Div(Value&& lhs, Value&& rhs) const
{
    CheckNumberOperand(Operands[0].get(), lhs);
    CheckNumberOperand(Operands[1].get(), rhs);
    return Value{lhs.GetNumber() / rhs.GetNumber()};
}

Value BinaryOperator::Mod(Value&& lhs, Value&& rhs) const
{
    CheckNumberOperand(Operands[0].get(), lhs);
    CheckNumberOperand(Operands[1].get(), rhs);
    return Value{fmod(lhs.GetNumber(), rhs.GetNumber())};
}

Value BinaryOperator::Add(Value&& lhs, Value&& rhs) const
{
    CheckNumberOperand(Operands[0].get(), lhs);
    CheckNumberOperand(Operands[1].get(), rhs);
    return Value{lhs.GetNumber() + rhs.GetNumber()};
}

Value BinaryOperator::Sub(Value&& lhs, Value&& rhs) const
{
    CheckNumberOperand(Operands[0].get(), lhs);
    CheckNumberOperand(Operands[1].get(), rhs);
    return Value{lhs.GetNumber() - rhs.GetNumber()};
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
        case Value::Type::Null: s.clear(); break;
        case Value::Type::Number: Format(s, "%g\n", val.GetNumber()); break;
        case Value::Type::String: Format(s, "%s\n", val.GetString().c_str()); break;
        default: assert(0);
        }
        if(!s.empty())
            ctx.env.Print(s.data(), s.length());
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
    IdentifierConstant* calleeIdentifier = dynamic_cast<IdentifierConstant*>(Operands[0].get()); assert(calleeIdentifier); // #TODO make it flexible!
    const size_t argCount = Operands.size() - 1;
    vector<Value> values(argCount);
    for(size_t i = 0; i < argCount; ++i)
        values[i] = Operands[i + 1]->Evaluate(ctx);
    if(calleeIdentifier->Identifier == "print")
        return BuiltInFunction_Print(ctx, values.data(), values.size());
    else
        throw ExecutionError(GetPlace(), string("Unknown function: ") + calleeIdentifier->Identifier);
}

} // namespace AST

////////////////////////////////////////////////////////////////////////////////
// class Parser implementation

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

    ParseBlockContent(outScript);

    outScript.DebugPrint(0); // #TEMP
}

void Parser::ParseBlockContent(AST::Block& outBlock)
{
    while(m_Tokens[m_TokenIndex].Type != TokenType_::End)
    {
        unique_ptr<AST::Statement> stmt = TryParseStatement();
        if(!stmt)
            throw ParsingError(GetCurrentTokenPlace(), string("Cannot parse statement."));
        outBlock.Statements.push_back(std::move(stmt));
    }
}

unique_ptr<AST::Statement> Parser::TryParseStatement()
{
    const PlaceInCode place = GetCurrentTokenPlace();
    // Empty statement: ';'
    if(TryParseSymbol(Symbol::Semicolon))
        return std::make_unique<AST::EmptyStatement>(place);
    // Expression as statement: Expr17 ';'
    unique_ptr<AST::Expression> expr = TryParseExpr17();
    if(expr)
    {
        ParseSymbol(Symbol::Semicolon);
        return expr;
    }
    return unique_ptr<AST::Statement>{};
}

unique_ptr<AST::Constant> Parser::TryParseConstant()
{
    const Token& t = m_Tokens[m_TokenIndex];
    switch(t.Type)
    {
    case TokenType_::Number:
        ++m_TokenIndex;
        return std::make_unique<AST::NumberConstant>(t.Place, t.Number);
    case TokenType_::Identifier:
        ++m_TokenIndex;
        return std::make_unique<AST::IdentifierConstant>(t.Place, string(t.String));
    }
    return unique_ptr<AST::Constant>{};
}

unique_ptr<AST::Expression> Parser::TryParseExpr0()
{
    // '(' Constant ')'
    if(TryParseSymbol(Symbol::RoundBrackerOpen))
    {
        unique_ptr<AST::Expression> expr = TryParseExpr17();
        if(!expr)
            throw ParsingError(GetCurrentTokenPlace(), string("Expected expression."));
        ParseSymbol(Symbol::RoundBracketClose);
        return expr;
    }
    else
    {
        // Constant
        unique_ptr<AST::Constant> constant = TryParseConstant();
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
        if(TryParseSymbol(Symbol::RoundBrackerOpen))
        {
            unique_ptr<AST::MultiOperator> multiOperator = std::make_unique<AST::MultiOperator>(place, AST::MultiOperatorType::Call);
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
                    expr = TryParseExpr16();
                    if(!expr)
                        throw ParsingError(GetCurrentTokenPlace(), "Expected expression.");
                    multiOperator->Operands.push_back(std::move(expr));
                }
            }
            ParseSymbol(Symbol::RoundBracketClose);
            return multiOperator;
        }
        return expr;
    }
    return unique_ptr<AST::Expression>{};
}

unique_ptr<AST::Expression> Parser::TryParseExpr5()
{
    unique_ptr<AST::Expression> expr = TryParseExpr2();
    if(expr)
    {
        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Symbol::Mul))
            {
                unique_ptr<AST::BinaryOperator> op = std::make_unique<AST::BinaryOperator>(place, AST::BinaryOperatorType::Mul);
                op->Operands[0] = std::move(expr);
                op->Operands[1] = TryParseExpr2();
                if(!op->Operands[1])
                    throw ParsingError(GetCurrentTokenPlace(), "Expected expression.");
                expr = std::move(op);
            }
            else if(TryParseSymbol(Symbol::Div))
            {
                unique_ptr<AST::BinaryOperator> op = std::make_unique<AST::BinaryOperator>(place, AST::BinaryOperatorType::Div);
                op->Operands[0] = std::move(expr);
                op->Operands[1] = TryParseExpr2();
                if(!op->Operands[1])
                    throw ParsingError(GetCurrentTokenPlace(), "Expected expression.");
                expr = std::move(op);
            }
            else if(TryParseSymbol(Symbol::Mod))
            {
                unique_ptr<AST::BinaryOperator> op = std::make_unique<AST::BinaryOperator>(place, AST::BinaryOperatorType::Mod);
                op->Operands[0] = std::move(expr);
                op->Operands[1] = TryParseExpr2();
                if(!op->Operands[1])
                    throw ParsingError(GetCurrentTokenPlace(), "Expected expression.");
                expr = std::move(op);
            }
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
            {
                unique_ptr<AST::BinaryOperator> op = std::make_unique<AST::BinaryOperator>(place, AST::BinaryOperatorType::Add);
                op->Operands[0] = std::move(expr);
                op->Operands[1] = TryParseExpr5();
                if(!op->Operands[1])
                    throw ParsingError(GetCurrentTokenPlace(), "Expected expression.");
                expr = std::move(op);
            }
            else if(TryParseSymbol(Symbol::Sub))
            {
                unique_ptr<AST::BinaryOperator> op = std::make_unique<AST::BinaryOperator>(place, AST::BinaryOperatorType::Sub);
                op->Operands[0] = std::move(expr);
                op->Operands[1] = TryParseExpr5();
                if(!op->Operands[1])
                    throw ParsingError(GetCurrentTokenPlace(), "Expected expression.");
                expr = std::move(op);
            }
            else
                break;
        }
        return expr;
    }
    return unique_ptr<AST::Expression>{};
}

bool Parser::TryParseSymbol(Symbol symbol)
{
    if(m_TokenIndex < m_Tokens.size() &&
        m_Tokens[m_TokenIndex].Type == TokenType_::Symbol &&
        m_Tokens[m_TokenIndex].Symbol == symbol)
    {
        ++m_TokenIndex;
        return true;
    }
    else
        return false;
}

void Parser::ParseSymbol(Symbol symbol)
{
    if(!TryParseSymbol(symbol))
        throw ParsingError(GetCurrentTokenPlace(), string("Expected symbol \"") + SYMBOL_STR[(uint32_t)symbol] + "\".");
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

    AST::ExecuteContext executeContext{*this};
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
