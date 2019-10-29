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

#define EXECUTION_CHECK(condition, errorMessage) \
    do { if(!(condition)) throw ExecutionError(GetPlace(), (errorMessage)); } while(false)
#define EXECUTION_CHECK_PLACE(condition, place, errorMessage) \
    do { if(!(condition)) throw ExecutionError((place), (errorMessage)); } while(false)

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
static const char* const ERROR_MESSAGE_INVALID_STRING = "Invalid string.";
static const char* const ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE = "Invalid escape sequence in a string.";
static const char* const ERROR_MESSAGE_INVALID_TYPE = "Invalid type.";
static const char* const ERROR_MESSAGE_INVALID_INDEX = "Invalid index.";
static const char* const ERROR_MESSAGE_INVALID_LVALUE = "Invalid l-value.";
static const char* const ERROR_MESSAGE_INVALID_FUNCTION = "Invalid function.";
static const char* const ERROR_MESSAGE_INVALID_NUMBER_OF_ARGUMENTS = "Invalid number of arguments.";
static const char* const ERROR_MESSAGE_UNRECOGNIZED_TOKEN = "Unrecognized token.";
static const char* const ERROR_MESSAGE_UNEXPECTED_END_OF_FILE_IN_MULTILINE_COMMENT = "Unexpected end of file inside multiline comment.";
static const char* const ERROR_MESSAGE_UNEXPECTED_END_OF_FILE_IN_STRING = "Unexpected end of file inside string.";
static const char* const ERROR_MESSAGE_EXPECTED_EXPRESSION = "Expected expression.";
static const char* const ERROR_MESSAGE_EXPECTED_STATEMENT = "Expected statement.";
static const char* const ERROR_MESSAGE_EXPECTED_CONSTANT_VALUE = "Expected constant value.";
static const char* const ERROR_MESSAGE_EXPECTED_IDENTIFIER = "Expected identifier.";
static const char* const ERROR_MESSAGE_EXPECTED_LVALUE = "Expected l-value.";
static const char* const ERROR_MESSAGE_EXPECTED_NUMBER = "Expected number.";
static const char* const ERROR_MESSAGE_EXPECTED_STRING = "Expected string.";
static const char* const ERROR_MESSAGE_EXPECTED_SINGLE_CHARACTER_STRING = "Expected single character string.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL                     = "Expected symbol.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_COLON               = "Expected symbol ':'.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON           = "Expected symbol ';'.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN  = "Expected symbol '('.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE = "Expected symbol ')'.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_OPEN  = "Expected symbol '{'.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE = "Expected symbol '}'.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_SQUARE_BRACKET_CLOSE = "Expected symbol ']'.";
static const char* const ERROR_MESSAGE_EXPECTED_SYMBOL_WHILE = "Expected 'while'.";
static const char* const ERROR_MESSAGE_EXPECTED_UNIQUE_CONSTANT = "Expected unique constant.";
static const char* const ERROR_MESSAGE_VARIABLE_DOESNT_EXIST = "Variable doesn't exist.";
static const char* const ERROR_MESSAGE_NOT_IMPLEMENTED = "Not implemented.";
static const char* const ERROR_MESSAGE_BREAK_WITHOUT_LOOP = "Break without a loop.";
static const char* const ERROR_MESSAGE_CONTINUE_WITHOUT_LOOP = "Continue without a loop.";
static const char* const ERROR_MESSAGE_RETURN_WITHOUT_FUNCTION = "Return without a function.";
static const char* const ERROR_MESSAGE_INCOMPATIBLE_TYPES = "Incompatible types.";
static const char* const ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS = "Index out of bounds.";

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

enum class Symbol
{
    // Token types
    None,
    Identifier,
    Number,
    String,
    End,
    // Symbols
    Comma,             // ,
    QuestionMark,      // ?
    Colon,             // :
    Semicolon,         // ;
    RoundBracketOpen,  // (
    RoundBracketClose, // )
    SquareBracketOpen, // [
    SquareBracketClose, // ]
    CurlyBracketOpen,  // {
    CurlyBracketClose, // }
    Asterisk,          // *
    Slash,             // /
    Percent,           // %
    Plus,              // +
    Dash,              // -
    Equals,            // =
    ExclamationMark,   // !
    Tilde,             // ~
    Less,              // <
    Greater,           // >
    Amperstand,        // &
    Caret,             // ^
    Pipe,              // |
    // Multiple character symbols
    DoublePlus,        // ++
    DoubleDash,        // --
    PlusEquals,        // +=
    DashEquals,        // -=
    AsteriskEquals,    // *=
    SlashEquals,       // /=
    PercentEquals,     // %=
    DoubleLessEquals,  // <<=
    DoubleGreaterEquals, // >>=
    AmperstandEquals,  // &=
    CaretEquals,       // ^=
    PipeEquals,        // |=
    DoubleLess,        // <<
    DoubleGreater,     // >>
    LessEquals,        // <=
    GreaterEquals,     // >=
    DoubleEquals,      // ==
    ExclamationEquals, // !=
    DoubleAmperstand,  // &&
    DoublePipe,        // ||
    // Keywords
    Null, False, True, If, Else, While, Do, For, Break, Continue,
    Switch, Case, Default, Function, Return, Count
};
static const char* SYMBOL_STR[] = {
    // Token types
    "", "", "", "", "",
    // Symbols
    ",", "?", ":", ";", "(", ")", "[", "]", "{", "}", "*", "/", "%", "+", "-", "=", "!", "~", "<", ">", "&", "^", "|",
    // Multiple character symbols
    "++", "--", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
    // Keywords
    "null", "false", "true", "if", "else", "while", "do", "for", "break", "continue",
    "switch", "case", "default", "function", "return",
};

struct Token
{
    PlaceInCode Place;
    Symbol Symbol;
    double Number; // Only when Symbol == Symbol::Number
    string String; // Only when Symbol == Symbol::Identifier or String
};

static inline bool IsDecimalNumber(char ch) { return ch >= '0' && ch <= '9'; }
static inline bool IsHexadecimalNumber(char ch) { return ch >= '0' && ch <= '9' || ch >= 'A' && ch <= 'F' || ch >= 'a' && ch <= 'f'; }
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

static bool NumberToIndex(size_t& outIndex, double number)
{
    if(!isfinite(number) || number < 0.f)
        return false;
    outIndex = (size_t)number;
    return (double)outIndex == number;
}

struct BreakException { };
struct ContinueException { };

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
    static bool ParseCharHex(uint8_t& out, char ch);
    static bool ParseCharsHex(uint32_t& out, const char* chars, uint8_t charCount);
    static bool AppendUtf8Char(string& inout, uint32_t charVal);

    CodeReader m_Code;

    void SkipSpacesAndComments();
    bool ParseNumber(Token& out);
    bool ParseString(Token& out);
};

////////////////////////////////////////////////////////////////////////////////
// class Value definition

namespace AST { struct FunctionDefinition; }

enum class SystemFunction {
    Print, Count
};
static const char* SYSTEM_FUNCTION_NAMES[] = {
    "print",
};

class Value
{
public:
    enum class Type { Number, String, Function, SystemFunction };

    Value() : m_Number(0.0) { }
    Value(double number) : m_Type(Type::Number), m_Number(number) { }
    Value(string&& str) : m_Type(Type::String), m_String(std::move(str)) { }
    Value(const AST::FunctionDefinition* func) : m_Type{Type::Function}, m_Function{func} { }
    Value(SystemFunction func) : m_Type{Type::SystemFunction}, m_SystemFunction{func} { }

    Type GetType() const { return m_Type; }
    double GetNumber() const { assert(m_Type == Type::Number); return m_Number; }
    string& GetString() { assert(m_Type == Type::String); return m_String; }
    const string& GetString() const { assert(m_Type == Type::String); return m_String; }
    const AST::FunctionDefinition* GetFunction() const { assert(m_Type == Type::Function && m_Function); return m_Function; }
    SystemFunction GetSystemFunction() const { assert(m_Type == Type::SystemFunction); return m_SystemFunction; }

    bool operator==(const Value& rhs) const
    {
        if(m_Type == rhs.m_Type)
        {
            switch(m_Type)
            {
            case Type::Number: return m_Number == rhs.m_Number;
            case Type::String: return m_String == rhs.m_String;
            default: assert(0);
            }
        }
        return false;
    }
    bool operator!=(const Value& rhs) const
    {
        if(m_Type == rhs.m_Type)
        {
            switch(m_Type)
            {
            case Type::Number: return m_Number != rhs.m_Number;
            case Type::String: return m_String != rhs.m_String;
            default: assert(0);
            }
        }
        return true;
    }

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
    union
    {
        double m_Number;
        const AST::FunctionDefinition* m_Function;
        SystemFunction m_SystemFunction;
    };
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
    size_t Index; // SIZE_MAX if not indexing single character.
    
    bool HasIndex() const { return Index != SIZE_MAX; }
};

////////////////////////////////////////////////////////////////////////////////
// struct ReturnException definition

struct ReturnException
{
    const PlaceInCode Place;
    Value ThrownValue;
};

////////////////////////////////////////////////////////////////////////////////
// Abstract Syntax Tree definitions

namespace AST
{

struct ExecuteContext
{
    EnvironmentPimpl& Env;
    Object& GlobalContext;
    vector<Object*> LocalContexts;

    struct LocalContextPush
    {
        LocalContextPush(ExecuteContext& ctx, Object* localObj) : m_Ctx{ctx} { ctx.LocalContexts.push_back(localObj); }
        ~LocalContextPush() { m_Ctx.LocalContexts.pop_back(); }
    private:
        ExecuteContext& m_Ctx;
    };
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

enum class LoopBreakType { Break, Continue };
struct LoopBreakStatement : public Statement
{
    LoopBreakType Type;
    explicit LoopBreakStatement(const PlaceInCode& place, LoopBreakType type) : Statement{place}, Type{type} { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

struct ReturnStatement : public Statement
{
    unique_ptr<Expression> ReturnedValue; // Can be null.
    explicit ReturnStatement(const PlaceInCode& place) : Statement{place} { }
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

struct ConstantValue;

struct SwitchStatement : public Statement
{
    unique_ptr<Expression> Condition;
    vector<unique_ptr<AST::ConstantValue>> ItemValues; // null means default block.
    vector<unique_ptr<AST::Block>> ItemBlocks; // Can be null if empty.
    explicit SwitchStatement(const PlaceInCode& place) : Statement{place} { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

struct Script : Block
{
    explicit Script(const PlaceInCode& place) : Block{place} { }
    virtual void Execute(ExecuteContext& ctx) const;
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
    Postincrementation,
    Postdecrementation,
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
    Mul, Div, Mod, Add, Sub, ShiftLeft, ShiftRight,
    Assignment, AssignmentAdd, AssignmentSub, AssignmentMul, AssignmentDiv, AssignmentMod, AssignmentShiftLeft, AssignmentShiftRight,
    AssignmentBitwiseAnd, AssignmentBitwiseXor, AssignmentBitwiseOr,
    Less, Greater, LessEqual, GreaterEqual, Equal, NotEqual,
    BitwiseAnd, BitwiseXor, BitwiseOr, LogicalAnd, LogicalOr,
    Comma, Indexing,
};

struct BinaryOperator : Operator
{
    BinaryOperatorType Type;
    unique_ptr<Expression> Operands[2];
    BinaryOperator(const PlaceInCode& place, BinaryOperatorType type) : Operator{place}, Type(type) { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual Value Evaluate(ExecuteContext& ctx) const;
    virtual LValue GetLValue(ExecuteContext& ctx) const;

private:
    Value ShiftLeft(const Value& lhs, const Value& rhs) const;
    Value ShiftRight(const Value& lhs, const Value& rhs) const;
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

struct FunctionDefinition : public Expression
{
    vector<string> Parameters;
    Block Body;
    FunctionDefinition(const PlaceInCode& place) : Expression{place}, Body{place} { }
    virtual void DebugPrint(uint32_t indentLevel) const;
    virtual Value Evaluate(ExecuteContext& ctx) const;
};

} // namespace AST

static inline void CheckNumberOperand(const AST::Expression* operand, const Value& value)
{
    EXECUTION_CHECK_PLACE( value.GetType() == Value::Type::Number, operand->GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER );
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
    bool TryParseSwitchItem(AST::SwitchStatement& switchStatement);
    unique_ptr<AST::Statement> TryParseStatement();
    unique_ptr<AST::ConstantValue> TryParseConstantValue();
    unique_ptr<AST::ConstantExpression> TryParseConstantExpr();
    unique_ptr<AST::Expression> TryParseExpr0();
    unique_ptr<AST::Expression> TryParseExpr2();
    unique_ptr<AST::Expression> TryParseExpr3();
    unique_ptr<AST::Expression> TryParseExpr5();
    unique_ptr<AST::Expression> TryParseExpr6();
    unique_ptr<AST::Expression> TryParseExpr7();
    unique_ptr<AST::Expression> TryParseExpr9();
    unique_ptr<AST::Expression> TryParseExpr10();
    unique_ptr<AST::Expression> TryParseExpr11();
    unique_ptr<AST::Expression> TryParseExpr12();
    unique_ptr<AST::Expression> TryParseExpr13();
    unique_ptr<AST::Expression> TryParseExpr14();
    unique_ptr<AST::Expression> TryParseExpr15();
    unique_ptr<AST::Expression> TryParseExpr16();
    unique_ptr<AST::Expression> TryParseExpr17();
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
        out.Symbol = Symbol::End;
        return;
    }

    constexpr Symbol firstSingleCharSymbol = Symbol::Comma;
    constexpr Symbol firstMultiCharSymbol = Symbol::DoublePlus;
    constexpr Symbol firstKeywordSymbol = Symbol::Null;

    const char* const currentCode = m_Code.GetCurrentCode();
    const size_t currentCodeLen = m_Code.GetCurrentLen();

    if(ParseString(out))
        return;
    // Multi char symbol
    for(size_t i = (size_t)firstMultiCharSymbol; i < (size_t)firstKeywordSymbol; ++i)
    {
        const size_t symbolLen = strlen(SYMBOL_STR[i]);
        if(currentCodeLen >= symbolLen && memcmp(SYMBOL_STR[i], currentCode, symbolLen) == 0)
        {
            out.Symbol = (Symbol)i;
            m_Code.MoveChars(symbolLen);
            return;
        }
    }
    // Symbol
    for(size_t i = (size_t)firstSingleCharSymbol; i < (size_t)firstMultiCharSymbol; ++i)
    {
        if(currentCode[0] == SYMBOL_STR[i][0])
        {
            out.Symbol = (Symbol)i;
            m_Code.MoveOneChar();
            return;
        }
    }
    if(ParseNumber(out))
        return;
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
                out.Symbol = (Symbol)i;
                m_Code.MoveChars(keywordLen);
                return;
            }
        }
        // Identifier
        out.Symbol = Symbol::Identifier;
        out.String = string{currentCode, currentCode + tokenLen};
        m_Code.MoveChars(tokenLen);
        return;
    }
    throw ParsingError(out.Place, ERROR_MESSAGE_UNRECOGNIZED_TOKEN);
}

bool Tokenizer::ParseCharHex(uint8_t& out, char ch)
{
    if(ch >= '0' && ch <= '9')
        out = (uint8_t)(ch - '0');
    else if(ch >= 'a' && ch <= 'f')
        out = (uint8_t)(ch - 'a' + 10);
    else if(ch >= 'A' && ch <= 'F')
        out = (uint8_t)(ch - 'A' + 10);
    else
        return false;
    return true;
}

bool Tokenizer::ParseCharsHex(uint32_t& out, const char* chars, uint8_t charCount)
{
    out = 0;
    for(uint8_t i = 0; i < charCount; ++i)
    {
        uint8_t charVal = 0;
        if(!ParseCharHex(charVal, chars[i]))
            return false;
        out = (out << 4) | charVal;
    }
    return true;
}

bool Tokenizer::AppendUtf8Char(string& inout, uint32_t charVal)
{
    if(charVal <= 0x7F)
        inout += (char)(uint8_t)charVal;
    else if(charVal <= 0x7FF)
    {
        inout += (char)(uint8_t)(0b11000000 | (charVal >> 6));
        inout += (char)(uint8_t)(0b10000000 | (charVal & 0b111111));
    }
    else if(charVal <= 0xFFFF)
    {
        inout += (char)(uint8_t)(0b11100000 | (charVal >> 12));
        inout += (char)(uint8_t)(0b10000000 | ((charVal >> 6) & 0b111111));
        inout += (char)(uint8_t)(0b10000000 | ( charVal       & 0b111111));
    }
    else if(charVal <= 0x10FFFF)
    {
        inout += (char)(uint8_t)(0b11110000 | (charVal >> 18));
        inout += (char)(uint8_t)(0b10000000 | ((charVal >> 12) & 0b111111));
        inout += (char)(uint8_t)(0b10000000 | ((charVal >>  6) & 0b111111));
        inout += (char)(uint8_t)(0b10000000 | ( charVal        & 0b111111));
    }
    else
        return false;
    return true;
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

bool Tokenizer::ParseNumber(Token& out)
{
    const char* const currentCode = m_Code.GetCurrentCode();
    const size_t currentCodeLen = m_Code.GetCurrentLen();
    if(!IsDecimalNumber(currentCode[0]) && currentCode[0] != '.')
        return false;
    size_t tokenLen = 0;
    // Hexadecimal: 0xHHHH...
    if(currentCode[0] == '0' && currentCodeLen >= 2 && (currentCode[1] == 'x' || currentCode[1] == 'X'))
    {
        tokenLen = 2;
        while(tokenLen < currentCodeLen && IsHexadecimalNumber(currentCode[tokenLen]))
            ++tokenLen;
        if(tokenLen < 3)
            throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_NUMBER);
        uint64_t n = 0;
        for(size_t i = 2; i < tokenLen; ++i)
        {
            uint8_t val = 0;
            ParseCharHex(val, currentCode[i]);
            n = (n << 4) | val;
        }
        out.Number = (double)n;
    }
    else
    {
        while(tokenLen < currentCodeLen && IsDecimalNumber(currentCode[tokenLen]))
            ++tokenLen;
        const size_t digitsBeforeDecimalPoint = tokenLen;
        size_t digitsAfterDecimalPoint = 0;
        if(tokenLen < currentCodeLen && currentCode[tokenLen] == '.')
        {
            ++tokenLen;
            while(tokenLen < currentCodeLen && IsDecimalNumber(currentCode[tokenLen]))
                ++tokenLen;
            digitsAfterDecimalPoint = tokenLen - digitsBeforeDecimalPoint - 1;
        }
        // Only dot '.' with no digits around: not a number token.
        if(digitsBeforeDecimalPoint + digitsAfterDecimalPoint == 0)
            return false;
        if(tokenLen < currentCodeLen && (currentCode[tokenLen] == 'e' || currentCode[tokenLen] == 'E'))
        {
            ++tokenLen;
            if(tokenLen < currentCodeLen && (currentCode[tokenLen] == '+' || currentCode[tokenLen] == '-'))
                ++tokenLen;
            const size_t tokenLenBeforeExponent = tokenLen;
            while(tokenLen < currentCodeLen && IsDecimalNumber(currentCode[tokenLen]))
                ++tokenLen;
            if(tokenLen - tokenLenBeforeExponent == 0)
                throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_NUMBER);
        }
        char sz[128];
        if(tokenLen >= _countof(sz))
            throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_NUMBER);
        memcpy(sz, currentCode, tokenLen);
        sz[tokenLen] = 0;
        out.Number = atof(sz);
    }
    // Letters straight after number are invalid.
    if(tokenLen < currentCodeLen && IsAlpha(currentCode[tokenLen]))
        throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_NUMBER);
    out.Symbol = Symbol::Number;
    m_Code.MoveChars(tokenLen);
    return true;
}

bool Tokenizer::ParseString(Token& out)
{
    const char* const currCode = m_Code.GetCurrentCode();
    const size_t currCodeLen = m_Code.GetCurrentLen();
    const char delimiterCh = currCode[0];
    if(delimiterCh != '"' && delimiterCh != '\'')
        return false;
    out.Symbol = Symbol::String;
    out.String.clear();
    size_t tokenLen = 1;
    for(;;)
    {
        if(tokenLen == currCodeLen)
            throw ParsingError(out.Place, ERROR_MESSAGE_UNEXPECTED_END_OF_FILE_IN_STRING);
        if(currCode[tokenLen] == delimiterCh)
            break;
        if(currCode[tokenLen] == '\\')
        {
            ++tokenLen;
            if(tokenLen == currCodeLen)
                throw ParsingError(out.Place, ERROR_MESSAGE_UNEXPECTED_END_OF_FILE_IN_STRING);
            switch(currCode[tokenLen])
            {
            case '\\': out.String += '\\'; ++tokenLen; break;
            case '/': out.String += '/'; ++tokenLen; break;
            case '"': out.String += '"'; ++tokenLen; break;
            case '\'': out.String += '\''; ++tokenLen; break;
            case '?': out.String += '?'; ++tokenLen; break;
            case 'a': out.String += '\a'; ++tokenLen; break;
            case 'b': out.String += '\b'; ++tokenLen; break;
            case 'f': out.String += '\f'; ++tokenLen; break;
            case 'n': out.String += '\n'; ++tokenLen; break;
            case 'r': out.String += '\r'; ++tokenLen; break;
            case 't': out.String += '\t'; ++tokenLen; break;
            case 'v': out.String += '\v'; ++tokenLen; break;
            case '0': out.String += '\0'; ++tokenLen; break;
            case 'x':
            {
                uint32_t val = 0;
                if(tokenLen + 2 >= currCodeLen || !ParseCharsHex(val, currCode + tokenLen + 1, 2))
                    throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                out.String += (char)(uint8_t)val;
                tokenLen += 3;
                break;
            }
            case 'u':
            {
                uint32_t val = 0;
                if(tokenLen + 4 >= currCodeLen || !ParseCharsHex(val, currCode + tokenLen + 1, 4))
                    throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                if(!AppendUtf8Char(out.String, val))
                    throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                tokenLen += 5;
                break;
            }
            case 'U':
            {
                uint32_t val = 0;
                if(tokenLen + 8 >= currCodeLen || !ParseCharsHex(val, currCode + tokenLen + 1, 8))
                    throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                if(!AppendUtf8Char(out.String, val))
                    throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                tokenLen += 9;
                break;
            }
            default:
                throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
            }
        }
        else
            out.String += currCode[tokenLen++];
    }
    ++tokenLen;
    // Letters straight after string are invalid.
    if(tokenLen < currCodeLen && IsAlpha(currCode[tokenLen]))
        throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_STRING);
    m_Code.MoveChars(tokenLen);
    return true;
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
        {
            try
            {
                Body->Execute(ctx);
            }
            catch(BreakException)
            {
                break;
            }
            catch(ContinueException)
            {
                continue;
            }
        }
        break;
    case WhileLoopType::DoWhile:
        do
        {
            try
            {
                Body->Execute(ctx);
            }
            catch(BreakException)
            {
                break;
            }
            catch(ContinueException)
            {
                continue;
            }
        }
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
        try
        {
            Body->Execute(ctx);
        }
        catch(BreakException)
        {
            break;
        }
        catch(ContinueException)
        {
        }
        if(IterationExpression)
            IterationExpression->Execute(ctx);
    }
}

void LoopBreakStatement::DebugPrint(uint32_t indentLevel) const
{
    static const char* TYPE_NAMES[] = { "Break", "Continue" };
    printf(DEBUG_PRINT_FORMAT_STR_BEG "%s\n", DEBUG_PRINT_ARGS_BEG, TYPE_NAMES[(size_t)Type]);
}

void LoopBreakStatement::Execute(ExecuteContext& ctx) const
{
    switch(Type)
    {
    case LoopBreakType::Break:
        throw BreakException{};
    case LoopBreakType::Continue:
        throw ContinueException{};
    default:
        assert(0);
    }
}

void ReturnStatement::DebugPrint(uint32_t indentLevel) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "return\n", DEBUG_PRINT_ARGS_BEG);
    if(ReturnedValue)
        ReturnedValue->DebugPrint(indentLevel + 1);
}

void ReturnStatement::Execute(ExecuteContext& ctx) const
{
    if(ReturnedValue)
        throw ReturnException{GetPlace(), ReturnedValue->Evaluate(ctx)};
    else
        throw ReturnException{GetPlace(), Value{}};
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

void SwitchStatement::DebugPrint(uint32_t indentLevel) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "switch\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    Condition->DebugPrint(indentLevel);
    for(size_t i = 0; i < ItemValues.size(); ++i)
    {
        if(ItemValues[i])
            ItemValues[i]->DebugPrint(indentLevel);
        else
            printf(DEBUG_PRINT_FORMAT_STR_BEG "default\n", DEBUG_PRINT_ARGS_BEG);
        if(ItemBlocks[i])
            ItemBlocks[i]->DebugPrint(indentLevel);
        else
            printf(DEBUG_PRINT_FORMAT_STR_BEG "(empty block)\n", DEBUG_PRINT_ARGS_BEG);
    }
}

void SwitchStatement::Execute(ExecuteContext& ctx) const
{
    const Value condVal = Condition->Evaluate(ctx);
    size_t itemIndex, defaultItemIndex = SIZE_MAX;
    const size_t itemCount = ItemValues.size();
    for(itemIndex = 0; itemIndex < itemCount; ++itemIndex)
    {
        if(ItemValues[itemIndex])
        {
            if(ItemValues[itemIndex]->Val == condVal)
                break;
        }
        else
            defaultItemIndex = itemIndex;
    }
    if(itemIndex == itemCount && defaultItemIndex != SIZE_MAX)
        itemIndex = defaultItemIndex;
    if(itemIndex != itemCount)
    {
        for(; itemIndex < itemCount; ++itemIndex)
        {
            try
            {
                ItemBlocks[itemIndex]->Execute(ctx);
            }
            catch(BreakException)
            {
                break;
            }
        }
    }
}

void Script::Execute(ExecuteContext& ctx) const
{
    try
    {
        Block::Execute(ctx);
    }
    catch(BreakException)
    {
        throw ExecutionError{GetPlace(), ERROR_MESSAGE_BREAK_WITHOUT_LOOP};
    }
    catch(ContinueException)
    {
        throw ExecutionError{GetPlace(), ERROR_MESSAGE_CONTINUE_WITHOUT_LOOP};
    }
}

LValue Expression::GetLValue(ExecuteContext& ctx) const
{
    EXECUTION_CHECK( false, ERROR_MESSAGE_EXPECTED_LVALUE );
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
    Object* const localObj = ctx.LocalContexts.empty() ? nullptr : ctx.LocalContexts.back();

    // Local variable
    if(localObj)
    {
        Value* val = localObj->TryGetValue(S);
        if(val)
            return *val;
    }
    
    // #TODO this
    
    // Global variable
    Value* val = ctx.GlobalContext.TryGetValue(S);
    if(val)
        return *val;

    // System function
    for(size_t i = 0, count = (size_t)SystemFunction::Count; i < count; ++i)
        if(S == SYSTEM_FUNCTION_NAMES[i])
            return Value{(SystemFunction)i};

    // Not found
    EXECUTION_CHECK( false, string("Variable \"") + S + "\" doesn't exist." );
}

LValue Identifier::GetLValue(ExecuteContext& ctx) const
{
    Object* const localObj = ctx.LocalContexts.empty() ? nullptr : ctx.LocalContexts.back();

    // Local variable
    if(localObj && localObj->HasKey(S))
        return LValue{*localObj, Constant{string(S)}, SIZE_MAX};
    
    // #TODO this
    
    // Global variable
    if(ctx.GlobalContext.HasKey(S))
        return LValue{ctx.GlobalContext, Constant{string(S)}, SIZE_MAX};

    // Not found: return reference to smallest scope.
    if(localObj)
        return LValue{*localObj, Constant{string(S)}, SIZE_MAX};
    else
        return LValue{ctx.GlobalContext, Constant{string(S)}, SIZE_MAX};
}

void UnaryOperator::DebugPrint(uint32_t indentLevel) const
{
    static const char* UNARY_OPERATOR_TYPE_NAMES[] = {
        "Preincrementation", "Predecrementation", "Postincrementation", "Postdecrementation",
        "Plus", "Minus", "Logical NOT", "Bitwise NOT" };
    printf(DEBUG_PRINT_FORMAT_STR_BEG "UnaryOperator %s\n", DEBUG_PRINT_ARGS_BEG, UNARY_OPERATOR_TYPE_NAMES[(uint32_t)Type]);
    ++indentLevel;
    Operand->DebugPrint(indentLevel);
}

Value UnaryOperator::Evaluate(ExecuteContext& ctx) const
{
    // Those require l-value.
    if(Type == UnaryOperatorType::Preincrementation ||
        Type == UnaryOperatorType::Predecrementation ||
        Type == UnaryOperatorType::Postincrementation ||
        Type == UnaryOperatorType::Postdecrementation)
    {
        LValue lval = Operand->GetLValue(ctx);
        EXECUTION_CHECK( !lval.HasIndex(), ERROR_MESSAGE_INVALID_LVALUE );
        Value* val = lval.Obj.TryGetValue(lval.Key);
        EXECUTION_CHECK( val != nullptr, ERROR_MESSAGE_VARIABLE_DOESNT_EXIST );
        EXECUTION_CHECK( val->GetType() == Value::Type::Number, ERROR_MESSAGE_EXPECTED_NUMBER );
        switch(Type)
        {
        case UnaryOperatorType::Preincrementation: val->SetNumber(val->GetNumber() + 1.0); return *val;
        case UnaryOperatorType::Predecrementation: val->SetNumber(val->GetNumber() - 1.0); return *val;
        case UnaryOperatorType::Postincrementation:
        {
            Value result = *val;
            val->SetNumber(val->GetNumber() + 1.0);
            return std::move(result);
        }
        case UnaryOperatorType::Postdecrementation:
        {
            Value result = *val;
            val->SetNumber(val->GetNumber() - 1.0);
            return std::move(result);
        }
        default: assert(0); return Value{};
        }
    }
    // Those use r-value.
    if(Type == UnaryOperatorType::Plus ||
        Type == UnaryOperatorType::Minus ||
        Type == UnaryOperatorType::LogicalNot ||
        Type == UnaryOperatorType::BitwiseNot)
    {
        Value val = Operand->Evaluate(ctx);
        EXECUTION_CHECK( val.GetType() == Value::Type::Number, ERROR_MESSAGE_EXPECTED_NUMBER );
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
        "Mul", "Div", "Mod", "Add", "Sub", "Shift left", "Shift right", "Assignment",
        "Less", "Greater", "LessEqual", "GreaterEqual", "Equal", "NotEqual",
        "BitwiseAnd", "BitwiseXor", "BitwiseOr", "LogicalAnd", "LogicalOr",
        "Comma", "Indexing", };
    printf(DEBUG_PRINT_FORMAT_STR_BEG "BinaryOperator %s\n", DEBUG_PRINT_ARGS_BEG, BINARY_OPERATOR_TYPE_NAMES[(uint32_t)Type]);
    ++indentLevel;
    Operands[0]->DebugPrint(indentLevel);
    Operands[1]->DebugPrint(indentLevel);
}

Value BinaryOperator::Evaluate(ExecuteContext& ctx) const
{
    // This operator is special, discards result of left operand.
    if(Type == BinaryOperatorType::Comma)
    {
        Operands[0]->Execute(ctx);
        return Operands[1]->Evaluate(ctx);
    }
    
    // Operators that require l-value.
    switch(Type)
    {
    case BinaryOperatorType::Assignment:
    case BinaryOperatorType::AssignmentAdd:
    case BinaryOperatorType::AssignmentSub:
    case BinaryOperatorType::AssignmentMul:
    case BinaryOperatorType::AssignmentDiv:
    case BinaryOperatorType::AssignmentMod:
    case BinaryOperatorType::AssignmentShiftLeft:
    case BinaryOperatorType::AssignmentShiftRight:
    case BinaryOperatorType::AssignmentBitwiseAnd:
    case BinaryOperatorType::AssignmentBitwiseXor:
    case BinaryOperatorType::AssignmentBitwiseOr:
    {
        LValue lhs = Operands[0]->GetLValue(ctx);
        Value rhs = Operands[1]->Evaluate(ctx);
        return Assignment(lhs, std::move(rhs));
    }
    }
    
    // Remaining operators use r-values.
    Value lhs = Operands[0]->Evaluate(ctx);

    // Logical operators with short circuit for right hand side operand.
    if(Type == BinaryOperatorType::LogicalAnd || Type == BinaryOperatorType::LogicalOr)
    {
        bool result = false;
        switch(Type)
        {
        case BinaryOperatorType::LogicalAnd: result = lhs.IsTrue() ? Operands[1]->Evaluate(ctx).IsTrue() : false; break;
        case BinaryOperatorType::LogicalOr:  result = lhs.IsTrue() ? true : Operands[1]->Evaluate(ctx).IsTrue(); break;
        }
        return Value{result ? 1.0 : 0.0};
    }

    // Remaining operators use both operands as r-values.
    Value rhs = Operands[1]->Evaluate(ctx);

    const Value::Type lhsType = lhs.GetType();
    const Value::Type rhsType = rhs.GetType();

    // These ones support various types.
    if(Type == BinaryOperatorType::Add)
    {
        if(lhsType == Value::Type::Number && rhsType == Value::Type::Number)
            return Value{lhs.GetNumber() + rhs.GetNumber()};
        else if(lhsType == Value::Type::String && rhsType == Value::Type::String)
            return Value{lhs.GetString() + rhs.GetString()};
        else
            EXECUTION_CHECK( false, ERROR_MESSAGE_INCOMPATIBLE_TYPES );
    }
    if(Type == BinaryOperatorType::Equal)
    {
        return Value{lhs == rhs ? 1.0 : 0.0};
    }
    if(Type == BinaryOperatorType::NotEqual)
    {
        return Value{lhs != rhs ? 1.0 : 0.0};
    }
    if(Type == BinaryOperatorType::Less || Type == BinaryOperatorType::LessEqual ||
        Type == BinaryOperatorType::Greater || Type == BinaryOperatorType::GreaterEqual)
    {
        bool result = false;
        EXECUTION_CHECK( lhsType == rhsType, ERROR_MESSAGE_INCOMPATIBLE_TYPES );
        if(lhsType == Value::Type::Number)
        {
            switch(Type)
            {
            case BinaryOperatorType::Less:         result = lhs.GetNumber() <  rhs.GetNumber(); break;
            case BinaryOperatorType::LessEqual:    result = lhs.GetNumber() <= rhs.GetNumber(); break;
            case BinaryOperatorType::Greater:      result = lhs.GetNumber() >  rhs.GetNumber(); break;
            case BinaryOperatorType::GreaterEqual: result = lhs.GetNumber() >= rhs.GetNumber(); break;
            default: assert(0);
            }
        }
        else if(lhsType == Value::Type::String)
        {
            switch(Type)
            {
            case BinaryOperatorType::Less:         result = lhs.GetString() <  rhs.GetString(); break;
            case BinaryOperatorType::LessEqual:    result = lhs.GetString() <= rhs.GetString(); break;
            case BinaryOperatorType::Greater:      result = lhs.GetString() >  rhs.GetString(); break;
            case BinaryOperatorType::GreaterEqual: result = lhs.GetString() >= rhs.GetString(); break;
            default: assert(0);
            }
        }
        else
            EXECUTION_CHECK( false, ERROR_MESSAGE_INVALID_TYPE );
        return Value{result ? 1.0 : 0.0};
    }
    if(Type == BinaryOperatorType::Indexing)
    {
        EXECUTION_CHECK( lhsType == Value::Type::String && rhsType == Value::Type::Number, ERROR_MESSAGE_INVALID_TYPE );
        size_t index = 0;
        EXECUTION_CHECK( NumberToIndex(index, rhs.GetNumber()), ERROR_MESSAGE_INVALID_INDEX );
        EXECUTION_CHECK( index < lhs.GetString().length(), ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS );
        return Value{string(1, lhs.GetString()[index])};
    }

    // Remaining operators require numbers.
    CheckNumberOperand(Operands[0].get(), lhs);
    CheckNumberOperand(Operands[1].get(), rhs);

    switch(Type)
    {
    case BinaryOperatorType::Mul:          return Value{lhs.GetNumber() * rhs.GetNumber()};
    case BinaryOperatorType::Div:          return Value{lhs.GetNumber() / rhs.GetNumber()};
    case BinaryOperatorType::Mod:          return Value{fmod(lhs.GetNumber(), rhs.GetNumber())};
    case BinaryOperatorType::Sub:          return Value{lhs.GetNumber() - rhs.GetNumber()};
    case BinaryOperatorType::ShiftLeft:    return ShiftLeft(std::move(lhs), std::move(rhs));
    case BinaryOperatorType::ShiftRight:   return ShiftRight(std::move(lhs), std::move(rhs));
    case BinaryOperatorType::BitwiseAnd:   return Value{ (double)( (int64_t)lhs.GetNumber() & (int64_t)rhs.GetNumber() ) };
    case BinaryOperatorType::BitwiseXor:   return Value{ (double)( (int64_t)lhs.GetNumber() ^ (int64_t)rhs.GetNumber() ) };
    case BinaryOperatorType::BitwiseOr:    return Value{ (double)( (int64_t)lhs.GetNumber() | (int64_t)rhs.GetNumber() ) };
    }

    assert(0); return Value{};
}

LValue BinaryOperator::GetLValue(ExecuteContext& ctx) const
{
    if(Type == BinaryOperatorType::Indexing)
    {
        LValue lval = Operands[0]->GetLValue(ctx);
        const Value indexVal = Operands[1]->Evaluate(ctx);
        EXECUTION_CHECK( indexVal.GetType() == Value::Type::Number, ERROR_MESSAGE_EXPECTED_NUMBER );
        EXECUTION_CHECK( NumberToIndex(lval.Index, indexVal.GetNumber()), ERROR_MESSAGE_INVALID_INDEX );
        return lval;
    }
    return __super::GetLValue(ctx);
}

Value BinaryOperator::ShiftLeft(const Value& lhs, const Value& rhs) const
{
    const int64_t lhsInt = (int64_t)lhs.GetNumber();
    const int64_t rhsInt = (int64_t)rhs.GetNumber();
    const int64_t resultInt = lhsInt << rhsInt;
    return Value{(double)resultInt};
}

Value BinaryOperator::ShiftRight(const Value& lhs, const Value& rhs) const
{
    const int64_t lhsInt = (int64_t)lhs.GetNumber();
    const int64_t rhsInt = (int64_t)rhs.GetNumber();
    const int64_t resultInt = lhsInt >> rhsInt;
    return Value{(double)resultInt};
}

Value BinaryOperator::Assignment(const LValue& lhs, Value&& rhs) const
{
    // Indexing: string[index] = newChar
    if(lhs.HasIndex())
    {
        EXECUTION_CHECK( Type == BinaryOperatorType::Assignment, ERROR_MESSAGE_INVALID_LVALUE );
        Value* const lhsValPtr = lhs.Obj.TryGetValue(lhs.Key);
        EXECUTION_CHECK( lhsValPtr != nullptr, ERROR_MESSAGE_VARIABLE_DOESNT_EXIST );
        EXECUTION_CHECK( lhsValPtr->GetType() == Value::Type::String && rhs.GetType() == Value::Type::String, ERROR_MESSAGE_EXPECTED_STRING );
        EXECUTION_CHECK( lhs.Index < lhsValPtr->GetString().length(), ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS );
        EXECUTION_CHECK( rhs.GetString().length() == 1, ERROR_MESSAGE_EXPECTED_SINGLE_CHARACTER_STRING );
        lhsValPtr->GetString()[lhs.Index] = rhs.GetString()[0];
        return *lhsValPtr;
    }

    // This one is able to create new value.
    if(Type == BinaryOperatorType::Assignment)
    {
        Value& lhsValRef = lhs.Obj.GetOrCreateValue(lhs.Key);
        lhsValRef = std::move(rhs);
        return lhsValRef;
    }

    // These ones require existing value.
    Value* const lhsValPtr = lhs.Obj.TryGetValue(lhs.Key);
    EXECUTION_CHECK( lhsValPtr != nullptr, ERROR_MESSAGE_VARIABLE_DOESNT_EXIST );

    if(Type == BinaryOperatorType::AssignmentAdd)
    {
        if(lhsValPtr->GetType() == Value::Type::Number && rhs.GetType() == Value::Type::Number)
            lhsValPtr->SetNumber(lhsValPtr->GetNumber() + rhs.GetNumber());
        else if(lhsValPtr->GetType() == Value::Type::String && rhs.GetType() == Value::Type::String)
            lhsValPtr->GetString() += rhs.GetString();
        else
            EXECUTION_CHECK( false, ERROR_MESSAGE_INCOMPATIBLE_TYPES );
        return *lhsValPtr;
    }

    // Remaining ones work on numbers only.
    EXECUTION_CHECK( lhsValPtr->GetType() == Value::Type::Number, ERROR_MESSAGE_EXPECTED_NUMBER );
    EXECUTION_CHECK( rhs.GetType() == Value::Type::Number, ERROR_MESSAGE_EXPECTED_NUMBER);
    switch(Type)
    {
    case BinaryOperatorType::AssignmentSub: lhsValPtr->SetNumber(lhsValPtr->GetNumber() - rhs.GetNumber()); break;
    case BinaryOperatorType::AssignmentMul: lhsValPtr->SetNumber(lhsValPtr->GetNumber() * rhs.GetNumber()); break;
    case BinaryOperatorType::AssignmentDiv: lhsValPtr->SetNumber(lhsValPtr->GetNumber() / rhs.GetNumber()); break;
    case BinaryOperatorType::AssignmentMod: lhsValPtr->SetNumber(fmod(lhsValPtr->GetNumber(), rhs.GetNumber())); break;
    case BinaryOperatorType::AssignmentShiftLeft:  *lhsValPtr = ShiftLeft (*lhsValPtr, rhs); break;
    case BinaryOperatorType::AssignmentShiftRight: *lhsValPtr = ShiftRight(*lhsValPtr, rhs); break;
    case BinaryOperatorType::AssignmentBitwiseAnd: lhsValPtr->SetNumber( (double)( (int64_t)lhsValPtr->GetNumber() & (int64_t)rhs.GetNumber() ) ); break;
    case BinaryOperatorType::AssignmentBitwiseXor: lhsValPtr->SetNumber( (double)( (int64_t)lhsValPtr->GetNumber() ^ (int64_t)rhs.GetNumber() ) ); break;
    case BinaryOperatorType::AssignmentBitwiseOr:  lhsValPtr->SetNumber( (double)( (int64_t)lhsValPtr->GetNumber() | (int64_t)rhs.GetNumber() ) ); break;
    default:
        assert(0);
    }

    return *lhsValPtr;
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
    static const char* MULTI_OPERATOR_TYPE_NAMES[] = { "Call" };
    printf(DEBUG_PRINT_FORMAT_STR_BEG "MultiOperator %s\n", DEBUG_PRINT_ARGS_BEG, MULTI_OPERATOR_TYPE_NAMES[(uint32_t)Type]);
    ++indentLevel;
    for(const auto& exprPtr : Operands)
        exprPtr->DebugPrint(indentLevel);
}

void FunctionDefinition::DebugPrint(uint32_t indentLevel) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Function(", DEBUG_PRINT_ARGS_BEG);
    if(!Parameters.empty())
    {
        printf("%s", Parameters[0].c_str());
        for(size_t i = 1, count = Parameters.size(); i < count; ++i)
            printf(", %s", Parameters[i].c_str());
    }
    printf(")\n");
    Body.DebugPrint(indentLevel + 1);
}

Value FunctionDefinition::Evaluate(ExecuteContext& ctx) const
{
    return Value{this};
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
        case Value::Type::Number:
            Format(s, "%g\n", val.GetNumber());
            ctx.Env.Print(s.data(), s.length());
            break;
        case Value::Type::String:
            if(!val.GetString().empty())
                ctx.Env.Print(val.GetString().data(), val.GetString().length());
            ctx.Env.Print("\n", 1);
            break;
        default: assert(0);
        }
            
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
    Value callee = Operands[0]->Evaluate(ctx);
    const size_t argCount = Operands.size() - 1;
    vector<Value> arguments(argCount);
    for(size_t i = 0; i < argCount; ++i)
        arguments[i] = Operands[i + 1]->Evaluate(ctx);

    if(callee.GetType() == Value::Type::Function)
    {
        const AST::FunctionDefinition* const funcDef = callee.GetFunction();
        EXECUTION_CHECK( argCount == funcDef->Parameters.size(), ERROR_MESSAGE_INVALID_NUMBER_OF_ARGUMENTS );
        Object localContext;
        // Setup parameters
        for(size_t argIndex = 0; argIndex != argCount; ++argIndex)
            localContext.GetOrCreateValue(funcDef->Parameters[argIndex]) = std::move(arguments[argIndex]);
        ExecuteContext::LocalContextPush localContextPush{ctx, &localContext};
        try
        {
            callee.GetFunction()->Body.Execute(ctx);
        }
        catch(ReturnException& returnEx)
        {
            return std::move(returnEx.ThrownValue);
        }
        return Value{};
    }

    if(callee.GetType() == Value::Type::SystemFunction)
    {
        switch(callee.GetSystemFunction())
        {
        case SystemFunction::Print:
            return BuiltInFunction_Print(ctx, arguments.data(), arguments.size());
        default:
            assert(0); return Value{};
        }
    }

    EXECUTION_CHECK( false, ERROR_MESSAGE_INVALID_FUNCTION );
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
        if(token.Symbol == Symbol::End)
            break;
    }

    ParseBlock(outScript);
    if(m_Tokens[m_TokenIndex].Symbol != Symbol::End)
        throw ParsingError(GetCurrentTokenPlace(), ERROR_MESSAGE_PARSING_ERROR);

    //outScript.DebugPrint(0); // #DELME
}

void Parser::ParseBlock(AST::Block& outBlock)
{
    while(m_Tokens[m_TokenIndex].Symbol != Symbol::End)
    {
        unique_ptr<AST::Statement> stmt = TryParseStatement();
        if(!stmt)
            break;
        outBlock.Statements.push_back(std::move(stmt));
    }
}

bool Parser::TryParseSwitchItem(AST::SwitchStatement& switchStatement)
{
    const PlaceInCode place = GetCurrentTokenPlace();
    // 'default' ':' Block
    if(TryParseSymbol(Symbol::Default))
    {
        MUST_PARSE( TryParseSymbol(Symbol::Colon), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON );
        switchStatement.ItemValues.push_back(unique_ptr<AST::ConstantValue>{});
        switchStatement.ItemBlocks.push_back(std::make_unique<AST::Block>(place));
        ParseBlock(*switchStatement.ItemBlocks.back());
        return true;
    }
    // 'case' ConstantExpr ':' Block
    if(TryParseSymbol(Symbol::Case))
    {
        unique_ptr<AST::ConstantValue> constVal;
        MUST_PARSE( constVal = TryParseConstantValue(), ERROR_MESSAGE_EXPECTED_CONSTANT_VALUE );
        switchStatement.ItemValues.push_back(std::move(constVal));
        MUST_PARSE( TryParseSymbol(Symbol::Colon), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON );
        switchStatement.ItemBlocks.push_back(std::make_unique<AST::Block>(place));
        ParseBlock(*switchStatement.ItemBlocks.back());
        return true;
    }
    return false;
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

    // 'break' ';'
    if(TryParseSymbol(Symbol::Break))
    {
        MUST_PARSE( TryParseSymbol(Symbol::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON );
        return make_unique<AST::LoopBreakStatement>(place, AST::LoopBreakType::Break);
    }
    
    // 'continue' ';'
    if(TryParseSymbol(Symbol::Continue))
    {
        MUST_PARSE( TryParseSymbol(Symbol::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON );
        return make_unique<AST::LoopBreakStatement>(place, AST::LoopBreakType::Continue);
    }

    // 'return' [ Expr17 ] ';'
    if(TryParseSymbol(Symbol::Return))
    {
        unique_ptr<AST::ReturnStatement> stmt = std::make_unique<AST::ReturnStatement>(place);
        stmt->ReturnedValue = TryParseExpr17();
        MUST_PARSE( TryParseSymbol(Symbol::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON );
        return stmt;
    }

    // 'switch' '(' Expr17 ')' '{' SwitchItem+ '}'
    if(TryParseSymbol(Symbol::Switch))
    {
        unique_ptr<AST::SwitchStatement> stmt = std::make_unique<AST::SwitchStatement>(place);
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN );
        MUST_PARSE( stmt->Condition = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE );
        MUST_PARSE( TryParseSymbol(Symbol::CurlyBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_OPEN );
        while(TryParseSwitchItem(*stmt)) { }
        MUST_PARSE( TryParseSymbol(Symbol::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE );
        // Check uniqueness. Warning: O(n^2) complexity!
        for(size_t i = 0, count = stmt->ItemValues.size(); i < count; ++i)
        {
            for(size_t j = i + 1; j < count; ++j)
            {
                if(j != i)
                {
                    if(!stmt->ItemValues[i] && !stmt->ItemValues[j]) // 2x default
                        throw ParsingError(place, ERROR_MESSAGE_EXPECTED_UNIQUE_CONSTANT);
                    if(stmt->ItemValues[i] && stmt->ItemValues[j])
                    {
                        if(stmt->ItemValues[i]->Val == stmt->ItemValues[j]->Val)
                            throw ParsingError(stmt->ItemValues[j]->GetPlace(), ERROR_MESSAGE_EXPECTED_UNIQUE_CONSTANT);
                    }
                }
            }
        }
        return stmt;
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

unique_ptr<AST::ConstantValue> Parser::TryParseConstantValue()
{
    const Token& t = m_Tokens[m_TokenIndex];
    switch(t.Symbol)
    {
    case Symbol::Number:
        ++m_TokenIndex;
        return make_unique<AST::ConstantValue>(t.Place, Value{t.Number});
    case Symbol::String:
    {
        ++m_TokenIndex;
        unique_ptr<AST::ConstantValue> expr = make_unique<AST::ConstantValue>(t.Place, string(t.String));
        while(m_Tokens[m_TokenIndex].Symbol == Symbol::String)
            expr->Val.GetString() += m_Tokens[m_TokenIndex++].String;
        return expr;
    }
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
    return unique_ptr<AST::ConstantValue>{};
}

unique_ptr<AST::ConstantExpression> Parser::TryParseConstantExpr()
{
    const Token& t = m_Tokens[m_TokenIndex];
    if(t.Symbol == Symbol::Identifier)
    {
        ++m_TokenIndex;
        return make_unique<AST::Identifier>(t.Place, string(t.String));
    }
    return TryParseConstantValue();
}

unique_ptr<AST::Expression> Parser::TryParseExpr0()
{
    const PlaceInCode place = GetCurrentTokenPlace();
    
    // '(' Expr17 ')'
    if(TryParseSymbol(Symbol::RoundBracketOpen))
    {
        unique_ptr<AST::Expression> expr;
        MUST_PARSE( expr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE );
        return expr;
    }

    // 'function' '(' [ TOKEN_IDENTIFIER ( ',' TOKE_IDENTIFIER )* ] ')' '{' Block '}'
    if(TryParseSymbol(Symbol::Function))
    {
        unique_ptr<AST::FunctionDefinition> func = std::make_unique<AST::FunctionDefinition>(place);
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN );
        if(m_Tokens[m_TokenIndex].Symbol == Symbol::Identifier)
        {
            func->Parameters.push_back(m_Tokens[m_TokenIndex++].String);
            while(TryParseSymbol(Symbol::Comma))
            {
                MUST_PARSE( m_Tokens[m_TokenIndex].Symbol == Symbol::Identifier, ERROR_MESSAGE_EXPECTED_IDENTIFIER );
                func->Parameters.push_back(m_Tokens[m_TokenIndex++].String);
            }
        }
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE );
        MUST_PARSE( TryParseSymbol(Symbol::CurlyBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_OPEN );
        ParseBlock(func->Body);
        MUST_PARSE( TryParseSymbol(Symbol::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE );
        return func;
    }

    // Constant
    unique_ptr<AST::ConstantExpression> constant = TryParseConstantExpr();
    if(constant)
        return constant;

    return unique_ptr<AST::Expression>{};
}

unique_ptr<AST::Expression> Parser::TryParseExpr2()
{
    unique_ptr<AST::Expression> expr = TryParseExpr0();
    if(!expr)
        return unique_ptr<AST::Expression>{};
    const PlaceInCode place = GetCurrentTokenPlace();
    // Postincrementation: Expr0 '++'
    if(TryParseSymbol(Symbol::DoublePlus))
    {
        unique_ptr<AST::UnaryOperator> op = std::make_unique<AST::UnaryOperator>(place, AST::UnaryOperatorType::Postincrementation);
        op->Operand = std::move(expr);
        return op;
    }
    // Postdecrementation: Expr0 '--'
    if(TryParseSymbol(Symbol::DoubleDash))
    {
        unique_ptr<AST::UnaryOperator> op = std::make_unique<AST::UnaryOperator>(place, AST::UnaryOperatorType::Postdecrementation);
        op->Operand = std::move(expr);
        return op;
    }
    // Call: Expr0 '(' [ Expr16 ( ',' Expr16 )* ')'
    if(TryParseSymbol(Symbol::RoundBracketOpen))
    {
        unique_ptr<AST::MultiOperator> op = make_unique<AST::MultiOperator>(place, AST::MultiOperatorType::Call);
        // Callee
        op->Operands.push_back(std::move(expr));
        // First argument
        expr = TryParseExpr16();
        if(expr)
        {
            op->Operands.push_back(std::move(expr));
            // Further arguments
            while(TryParseSymbol(Symbol::Comma))
            {
                MUST_PARSE( expr = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
                op->Operands.push_back(std::move(expr));
            }
        }
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE );
        return op;
    }
    // Indexing: Expr0 '[' Expr17 ']'
    if(TryParseSymbol(Symbol::SquareBracketOpen))
    {
        unique_ptr<AST::BinaryOperator> op = std::make_unique<AST::BinaryOperator>(place, AST::BinaryOperatorType::Indexing);
        op->Operands[0] = std::move(expr);
        MUST_PARSE( op->Operands[1] = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
        MUST_PARSE( TryParseSymbol(Symbol::SquareBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_SQUARE_BRACKET_CLOSE );
        return op;
    }
    // #TODO Member access: Expr0 '.' TOKEN_IDENTIFIER
    // Just Expr0
    return expr;
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
    PARSE_UNARY_OPERATOR(Symbol::DoublePlus, AST::UnaryOperatorType::Preincrementation)
    PARSE_UNARY_OPERATOR(Symbol::DoubleDash, AST::UnaryOperatorType::Predecrementation)
    PARSE_UNARY_OPERATOR(Symbol::Plus, AST::UnaryOperatorType::Plus)
    PARSE_UNARY_OPERATOR(Symbol::Dash, AST::UnaryOperatorType::Minus)
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
    if(!expr)
        return unique_ptr<AST::Expression>{};
 
    for(;;)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        if(TryParseSymbol(Symbol::Asterisk))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Mul, TryParseExpr3)
        else if(TryParseSymbol(Symbol::Slash))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Div, TryParseExpr3)
        else if(TryParseSymbol(Symbol::Percent))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Mod, TryParseExpr3)
        else
            break;
    }
    return expr;
}

unique_ptr<AST::Expression> Parser::TryParseExpr6()
{
    unique_ptr<AST::Expression> expr = TryParseExpr5();
    if(!expr)
        return unique_ptr<AST::Expression>{};

    for(;;)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        if(TryParseSymbol(Symbol::Plus))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Add, TryParseExpr5)
        else if(TryParseSymbol(Symbol::Dash))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Sub, TryParseExpr5)
        else
            break;
    }
    return expr;
}

unique_ptr<AST::Expression> Parser::TryParseExpr7()
{
    unique_ptr<AST::Expression> expr = TryParseExpr6();
    if(!expr)
        return unique_ptr<AST::Expression>{};

    for(;;)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        if(TryParseSymbol(Symbol::DoubleLess))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::ShiftLeft, TryParseExpr6)
        else if(TryParseSymbol(Symbol::DoubleGreater))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::ShiftRight, TryParseExpr6)
        else
            break;
    }
    return expr;
}

unique_ptr<AST::Expression> Parser::TryParseExpr9()
{
    unique_ptr<AST::Expression> expr = TryParseExpr7();
    if(!expr)
        return unique_ptr<AST::Expression>{};

    for(;;)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        if(TryParseSymbol(Symbol::Less))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Less, TryParseExpr7)
        else if(TryParseSymbol(Symbol::LessEquals))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::LessEqual, TryParseExpr7)
        else if(TryParseSymbol(Symbol::Greater))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Greater, TryParseExpr7)
        else if(TryParseSymbol(Symbol::GreaterEquals))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::GreaterEqual, TryParseExpr7)
        else
            break;
    }
    return expr;
}

unique_ptr<AST::Expression> Parser::TryParseExpr10()
{
    unique_ptr<AST::Expression> expr = TryParseExpr9();
    if(!expr)
        return unique_ptr<AST::Expression>{};

    for(;;)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        if(TryParseSymbol(Symbol::DoubleEquals))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Equal, TryParseExpr9)
        else if(TryParseSymbol(Symbol::ExclamationEquals))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::NotEqual, TryParseExpr9)
        else
            break;
    }
    return expr;
}

unique_ptr<AST::Expression> Parser::TryParseExpr11()
{
    unique_ptr<AST::Expression> expr = TryParseExpr10();
    if(!expr)
        return unique_ptr<AST::Expression>{};

    for(;;)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        if(TryParseSymbol(Symbol::Amperstand))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::BitwiseAnd, TryParseExpr10)
        else
            break;
    }
    return expr;
}

unique_ptr<AST::Expression> Parser::TryParseExpr12()
{
    unique_ptr<AST::Expression> expr = TryParseExpr11();
    if(!expr)
        return unique_ptr<AST::Expression>{};

    for(;;)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        if(TryParseSymbol(Symbol::Caret))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::BitwiseXor, TryParseExpr11)
        else
            break;
    }
    return expr;
}

unique_ptr<AST::Expression> Parser::TryParseExpr13()
{
    unique_ptr<AST::Expression> expr = TryParseExpr12();
    if(!expr)
        return unique_ptr<AST::Expression>{};

    for(;;)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        if(TryParseSymbol(Symbol::Pipe))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::BitwiseOr, TryParseExpr12)
        else
            break;
    }
    return expr;
}

unique_ptr<AST::Expression> Parser::TryParseExpr14()
{
    unique_ptr<AST::Expression> expr = TryParseExpr13();
    if(!expr)
        return unique_ptr<AST::Expression>{};
    for(;;)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        if(TryParseSymbol(Symbol::DoubleAmperstand))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::LogicalAnd, TryParseExpr13)
        else
            break;
    }
    return expr;
}

unique_ptr<AST::Expression> Parser::TryParseExpr15()
{
    unique_ptr<AST::Expression> expr = TryParseExpr14();
    if(!expr)
        return unique_ptr<AST::Expression>{};
    for(;;)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        if(TryParseSymbol(Symbol::DoublePipe))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::LogicalOr, TryParseExpr14)
        else
            break;
    }
    return expr;
}

unique_ptr<AST::Expression> Parser::TryParseExpr16()
{
    unique_ptr<AST::Expression> expr = TryParseExpr15();
    if(!expr)
        return unique_ptr<AST::Expression>{};

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
    // Assignment: Expr15 = Expr16, and variants like += -=
#define TRY_PARSE_ASSIGNMENT(symbol, binaryOperatorType) \
    if(TryParseSymbol(symbol)) \
    { \
        unique_ptr<AST::BinaryOperator> op = make_unique<AST::BinaryOperator>(GetCurrentTokenPlace(), (binaryOperatorType)); \
        op->Operands[0] = std::move(expr); \
        MUST_PARSE( op->Operands[1] = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION ); \
        return op; \
    }
    TRY_PARSE_ASSIGNMENT(Symbol::Equals, AST::BinaryOperatorType::Assignment)
    TRY_PARSE_ASSIGNMENT(Symbol::PlusEquals, AST::BinaryOperatorType::AssignmentAdd)
    TRY_PARSE_ASSIGNMENT(Symbol::DashEquals, AST::BinaryOperatorType::AssignmentSub)
    TRY_PARSE_ASSIGNMENT(Symbol::AsteriskEquals, AST::BinaryOperatorType::AssignmentMul)
    TRY_PARSE_ASSIGNMENT(Symbol::SlashEquals, AST::BinaryOperatorType::AssignmentDiv)
    TRY_PARSE_ASSIGNMENT(Symbol::PercentEquals, AST::BinaryOperatorType::AssignmentMod)
    TRY_PARSE_ASSIGNMENT(Symbol::DoubleLessEquals, AST::BinaryOperatorType::AssignmentShiftLeft)
    TRY_PARSE_ASSIGNMENT(Symbol::DoubleGreaterEquals, AST::BinaryOperatorType::AssignmentShiftRight)
    TRY_PARSE_ASSIGNMENT(Symbol::AmperstandEquals, AST::BinaryOperatorType::AssignmentBitwiseAnd)
    TRY_PARSE_ASSIGNMENT(Symbol::CaretEquals, AST::BinaryOperatorType::AssignmentBitwiseXor)
    TRY_PARSE_ASSIGNMENT(Symbol::PipeEquals, AST::BinaryOperatorType::AssignmentBitwiseOr)
#undef TRY_PARSE_ASSIGNMENT
    // Just Expr15
    return expr;
}

unique_ptr<AST::Expression> Parser::TryParseExpr17()
{
    unique_ptr<AST::Expression> expr = TryParseExpr16();
    if(!expr)
        return unique_ptr<AST::Expression>{};
    for(;;)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        if(TryParseSymbol(Symbol::Comma))
            PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Comma, TryParseExpr16)
        else
            break;
    }
    return expr;
}

bool Parser::TryParseSymbol(Symbol symbol)
{
    if(m_Tokens[m_TokenIndex].Symbol == symbol)
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
    m_Script.Statements.clear();

    {
        Tokenizer tokenizer{code, codeLen};
        Parser parser{tokenizer};
        parser.ParseScript(m_Script);
    }

    AST::ExecuteContext executeContext{*this, m_GlobalContext};
    try
    {
        m_Script.Execute(executeContext);
    }
    catch(const ReturnException& returnEx)
    {
        throw ExecutionError(returnEx.Place, ERROR_MESSAGE_RETURN_WITHOUT_FUNCTION);
    }
}

////////////////////////////////////////////////////////////////////////////////
// class Environment

Environment::Environment() : pimpl{new EnvironmentPimpl{}} { }
Environment::~Environment() { delete pimpl; }
void Environment::Execute(const char* code, size_t codeLen) { pimpl->Execute(code, codeLen); }
const std::string& Environment::GetOutput() const { return pimpl->GetOutput(); }

} // namespace MinScriptLang

#endif // MIN_SCRIPT_LANG_IMPLEMENTATION
