/*
MinScriptLang - minimalistic scripting language

Version: 0.0.1-development, 2021-11
Homepage: https://github.com/sawickiap/MinScriptLang
Author: Adam Sawicki, adam__REMOVE_THIS__@asawicki.info, https://asawicki.info

================================================================================
Modified MIT License

Copyright (c) 2019-2021 Adam Sawicki

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software. A notice about using the
Software shall be included in both textual documentation and in About/Credits
window or screen (if one exists) within the software that uses it.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
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
#include <string_view>
#include <exception>
#include <memory>
#include <vector>
#include <unordered_map>
#include <variant>
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
    virtual std::string_view GetMessage_() const = 0;
private:
    const PlaceInCode m_Place;
    mutable std::string m_What;
};

class ParsingError : public Error
{
public:
    ParsingError(const PlaceInCode& place, const std::string_view& message) : Error{place}, m_Message{message} { }
    virtual std::string_view GetMessage_() const override { return m_Message; }
private:
    const std::string_view m_Message; // Externally owned
};

class ExecutionError : public Error
{
public:
    ExecutionError(const PlaceInCode& place, std::string&& message) : Error{place}, m_Message{std::move(message)} { }
    ExecutionError(const PlaceInCode& place, const std::string_view& message) : Error{place}, m_Message{message} { }
    virtual std::string_view GetMessage_() const override { return m_Message; }
private:
    const std::string m_Message;
};

namespace AST { struct FunctionDefinition; }
class Object;
class Array;
enum class SystemFunction;
    
enum class ValueType { Null, Number, String, Function, SystemFunction, Object, Array, Type, Count };
class Value
{
public:
    Value() { }
    Value(double number) : m_Type(ValueType::Number), m_Variant(number) { }
    Value(std::string&& str) : m_Type(ValueType::String), m_Variant(std::move(str)) { }
    Value(const AST::FunctionDefinition* func) : m_Type{ValueType::Function}, m_Variant{func} { }
    Value(SystemFunction func) : m_Type{ValueType::SystemFunction}, m_Variant{func} { }
    Value(std::shared_ptr<Object> &&obj) : m_Type{ValueType::Object}, m_Variant(obj) { }
    Value(std::shared_ptr<Array> &&arr) : m_Type{ValueType::Array}, m_Variant(arr) { }
    Value(ValueType typeVal) : m_Type{ValueType::Type}, m_Variant(typeVal) { }

    ValueType GetType() const { return m_Type; }
    double GetNumber() const
    {
        assert(m_Type == ValueType::Number);
        return std::get<double>(m_Variant);
    }
    std::string& GetString()
    {
        assert(m_Type == ValueType::String);
        return std::get<std::string>(m_Variant);
    }
    const std::string& GetString() const
    {
        assert(m_Type == ValueType::String);
        return std::get<std::string>(m_Variant);
    }
    const AST::FunctionDefinition* GetFunction() const
    {
        assert(m_Type == ValueType::Function && std::get<const AST::FunctionDefinition*>(m_Variant));
        return std::get<const AST::FunctionDefinition*>(m_Variant);
    }
    SystemFunction GetSystemFunction() const
    {
        assert(m_Type == ValueType::SystemFunction);
        return std::get<SystemFunction>(m_Variant);
    }
    Object* GetObject_() const // Using underscore because the %^#& WinAPI defines GetObject as a macro.
    {
        assert(m_Type == ValueType::Object && std::get<std::shared_ptr<Object>>(m_Variant));
        return std::get<std::shared_ptr<Object>>(m_Variant).get();
    }
    std::shared_ptr<Object> GetObjectPtr() const
    {
        assert(m_Type == ValueType::Object && std::get<std::shared_ptr<Object>>(m_Variant));
        return std::get<std::shared_ptr<Object>>(m_Variant);
    }
    Array* GetArray() const
    {
        assert(m_Type == ValueType::Array && std::get<std::shared_ptr<Array>>(m_Variant));
        return std::get<std::shared_ptr<Array>>(m_Variant).get();
    }
    std::shared_ptr<Array> GetArrayPtr() const
    {
        assert(m_Type == ValueType::Array && std::get<std::shared_ptr<Array>>(m_Variant));
        return std::get<std::shared_ptr<Array>>(m_Variant);
    }
    ValueType GetTypeValue() const
    {
        assert(m_Type == ValueType::Type);
        return std::get<ValueType>(m_Variant);
    }

    bool IsEqual(const Value& rhs) const;
    bool IsTrue() const;

    void ChangeNumber(double number) { assert(m_Type == ValueType::Number); std::get<double>(m_Variant) = number; }

private:
    ValueType m_Type = ValueType::Null;
    using VariantType = std::variant<
        std::monostate, // ValueType::Null
        double, // ValueType::Number
        std::string, // ValueType::String
        const AST::FunctionDefinition*, // ValueType::Function
        SystemFunction, // ValueType::SystemFunction
        std::shared_ptr<Object>, // ValueType::Object
        std::shared_ptr<Array>, // ValueType::Array
        ValueType>; // ValueType::Type
    VariantType m_Variant;
};

class Object
{
public:
    using MapType = std::unordered_map<std::string, Value>;
    MapType m_Items;

    size_t GetCount() const { return m_Items.size(); }
    bool HasKey(const std::string& key) const { return m_Items.find(key) != m_Items.end(); }
    Value& GetOrCreateValue(const std::string& key) { return m_Items[key]; }; // Creates new null value if doesn't exist.
    Value* TryGetValue(const std::string& key); // Returns null if doesn't exist.
    const Value* TryGetValue(const std::string& key) const; // Returns null if doesn't exist.
    bool Remove(const std::string& key); // Returns true if has been found and removed.
};

class Array
{
public:
    std::vector<Value> Items;
};

#define EXECUTION_CHECK(condition, place, errorMessage) \
    do { if(!(condition)) throw ExecutionError((place), (errorMessage)); } while(false)
#define EXECUTION_FAIL(place, errorMessage) \
    do { throw ExecutionError((place), (errorMessage)); } while(false)

std::string VFormat(const char* format, va_list argList);
std::string Format(const char* format, ...);

class EnvironmentPimpl;
class Environment
{
public:
    Environment();
    ~Environment();
    Value Execute(const std::string_view& code);
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

#include <map>
#include <algorithm>
#include <initializer_list>

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
using std::string_view;
using std::vector;

// F***k Windows.h macros!
#undef min
#undef max

namespace MinScriptLang {

////////////////////////////////////////////////////////////////////////////////
// Basic facilities

// I would like it to be higher, but above that, even at 128, it crashes with
// native "stack overflow" in Debug configuration.
static const size_t LOCAL_SCOPE_STACK_MAX_SIZE = 100;

static constexpr string_view ERROR_MESSAGE_PARSING_ERROR = "Parsing error.";
static constexpr string_view ERROR_MESSAGE_INVALID_NUMBER = "Invalid number.";
static constexpr string_view ERROR_MESSAGE_INVALID_STRING = "Invalid string.";
static constexpr string_view ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE = "Invalid escape sequence in a string.";
static constexpr string_view ERROR_MESSAGE_INVALID_TYPE = "Invalid type.";
static constexpr string_view ERROR_MESSAGE_INVALID_MEMBER = "Invalid member.";
static constexpr string_view ERROR_MESSAGE_INVALID_INDEX = "Invalid index.";
static constexpr string_view ERROR_MESSAGE_INVALID_LVALUE = "Invalid l-value.";
static constexpr string_view ERROR_MESSAGE_INVALID_FUNCTION = "Invalid function.";
static constexpr string_view ERROR_MESSAGE_INVALID_NUMBER_OF_ARGUMENTS = "Invalid number of arguments.";
static constexpr string_view ERROR_MESSAGE_UNRECOGNIZED_TOKEN = "Unrecognized token.";
static constexpr string_view ERROR_MESSAGE_UNEXPECTED_END_OF_FILE_IN_MULTILINE_COMMENT = "Unexpected end of file inside multiline comment.";
static constexpr string_view ERROR_MESSAGE_UNEXPECTED_END_OF_FILE_IN_STRING = "Unexpected end of file inside string.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_EXPRESSION = "Expected expression.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_STATEMENT = "Expected statement.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_CONSTANT_VALUE = "Expected constant value.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_IDENTIFIER = "Expected identifier.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_LVALUE = "Expected l-value.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_NUMBER = "Expected number.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_STRING = "Expected string.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_OBJECT = "Expected object.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_ARRAY = "Expected array.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_OBJECT_MEMBER = "Expected object member.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_SINGLE_CHARACTER_STRING = "Expected single character string.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_SYMBOL                     = "Expected symbol.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_SYMBOL_COLON               = "Expected symbol ':'.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON           = "Expected symbol ';'.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN  = "Expected symbol '('.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE = "Expected symbol ')'.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_OPEN  = "Expected symbol '{'.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE = "Expected symbol '}'.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_SYMBOL_SQUARE_BRACKET_CLOSE = "Expected symbol ']'.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_SYMBOL_DOT                 = "Expected symbol '.'.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_SYMBOL_WHILE = "Expected 'while'.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_UNIQUE_CONSTANT = "Expected unique constant.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_1_ARGUMENT = "Expected 1 argument.";
static constexpr string_view ERROR_MESSAGE_EXPECTED_2_ARGUMENTS = "Expected 2 arguments.";
static constexpr string_view ERROR_MESSAGE_VARIABLE_DOESNT_EXIST = "Variable doesn't exist.";
static constexpr string_view ERROR_MESSAGE_OBJECT_MEMBER_DOESNT_EXIST = "Object member doesn't exist.";
static constexpr string_view ERROR_MESSAGE_NOT_IMPLEMENTED = "Not implemented.";
static constexpr string_view ERROR_MESSAGE_BREAK_WITHOUT_LOOP = "Break without a loop.";
static constexpr string_view ERROR_MESSAGE_CONTINUE_WITHOUT_LOOP = "Continue without a loop.";
static constexpr string_view ERROR_MESSAGE_INCOMPATIBLE_TYPES = "Incompatible types.";
static constexpr string_view ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS = "Index out of bounds.";
static constexpr string_view ERROR_MESSAGE_PARAMETER_NAMES_MUST_BE_UNIQUE = "Parameter naems must be unique.";
static constexpr string_view ERROR_MESSAGE_NO_LOCAL_SCOPE = "There is no local scope here.";
static constexpr string_view ERROR_MESSAGE_NO_THIS = "There is no 'this' here.";
static constexpr string_view ERROR_MESSAGE_REPEATING_KEY_IN_OBJECT = "Repeating key in object.";
static constexpr string_view ERROR_MESSAGE_STACK_OVERFLOW = "Stack overflow.";
static constexpr string_view ERROR_MESSAGE_BASE_MUST_BE_OBJECT = "Base must be object.";

static constexpr string_view VALUE_TYPE_NAMES[] = { "Null", "Number", "String", "Function", "Function", "Object", "Array", "Type" };
static_assert(_countof(VALUE_TYPE_NAMES) == (size_t)ValueType::Count);

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
    Dot,               // .
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
    Switch, Case, Default, Function, Return,
    Local, This, Global, Class, Throw, Try, Catch, Finally, Count
};
static constexpr string_view SYMBOL_STR[] = {
    // Token types
    "", "", "", "", "",
    // Symbols
    ",", "?", ":", ";", "(", ")", "[", "]", "{", "}", "*", "/", "%", "+", "-", "=", "!", "~", "<", ">", "&", "^", "|", ".",
    // Multiple character symbols
    "++", "--", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
    // Keywords
    "null", "false", "true", "if", "else", "while", "do", "for", "break", "continue",
    "switch", "case", "default", "function", "return",
    "local", "this", "global", "class", "throw", "try", "catch", "finally"
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

std::string VFormat(const char* format, va_list argList)
{
    size_t dstLen = (size_t)_vscprintf(format, argList);
    if(dstLen)
    {
        std::vector<char> buf(dstLen + 1);
        vsprintf_s(&buf[0], dstLen + 1, format, argList);
        return string{&buf[0], &buf[dstLen]};
    }
    else
        return {};
}

std::string Format(const char* format, ...)
{
    va_list argList;
    va_start(argList, format);
    auto result = VFormat(format, argList);
    va_end(argList);
    return result;
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
// class Value definition

bool Value::IsEqual(const Value& rhs) const
{
    if(m_Type != rhs.m_Type)
        return false;
    switch(m_Type)
    {
    case ValueType::Null:           return true;
    case ValueType::Number:         return std::get<double>(m_Variant) == std::get<double>(rhs.m_Variant);
    case ValueType::String:         return std::get<std::string>(m_Variant) == std::get<std::string>(rhs.m_Variant);
    case ValueType::Function:       return std::get<const AST::FunctionDefinition*>(m_Variant) == std::get<const AST::FunctionDefinition*>(rhs.m_Variant);
    case ValueType::SystemFunction: return std::get<SystemFunction>(m_Variant) == std::get<SystemFunction>(rhs.m_Variant);
    case ValueType::Object:         return std::get<std::shared_ptr<Object>>(m_Variant).get() == std::get<std::shared_ptr<Object>>(rhs.m_Variant).get();
    case ValueType::Array:          return std::get<std::shared_ptr<Array>>(m_Variant).get() == std::get<std::shared_ptr<Array>>(rhs.m_Variant).get();
    case ValueType::Type:           return std::get<ValueType>(m_Variant) == std::get<ValueType>(rhs.m_Variant);
    default: assert(0); return false;
    }
}

bool Value::IsTrue() const
{
    switch(m_Type)
    {
    case ValueType::Null:           return false;
    case ValueType::Number:         return std::get<double>(m_Variant) != 0.f;
    case ValueType::String:         return !std::get<std::string>(m_Variant).empty();
    case ValueType::Function:       return true;
    case ValueType::SystemFunction: return true;
    case ValueType::Object:         return true;
    case ValueType::Array:          return true;
    case ValueType::Type:           return std::get<ValueType>(m_Variant) != ValueType::Null;
    default: assert(0); return false;
    }
}

////////////////////////////////////////////////////////////////////////////////
// class CodeReader definition

class CodeReader
{
public:
    CodeReader(const string_view& code) :
        m_Code{code},
        m_Place{0, 1, 1}
    {
    }

    bool IsEnd() const { return m_Place.Index >= m_Code.length(); }
    const PlaceInCode& GetCurrentPlace() const { return m_Place; }
    const char* GetCurrentCode() const { return m_Code.data() + m_Place.Index; }
    size_t GetCurrentLen() const { return m_Code.length() - m_Place.Index; }
    char GetCurrentChar() const { return m_Code[m_Place.Index]; }
    bool Peek(char ch) const { return m_Place.Index < m_Code.length() && m_Code[m_Place.Index] == ch; }
    bool Peek(const char* s, size_t sLen) const { return m_Place.Index + sLen <= m_Code.length() && memcmp(m_Code.data() + m_Place.Index, s, sLen) == 0; }

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
    const string_view m_Code;
    PlaceInCode m_Place;
};

////////////////////////////////////////////////////////////////////////////////
// class Tokenizer definition

class Tokenizer
{
public:
    Tokenizer(const string_view& code) : m_Code{code} { }
    void GetNextToken(Token& out);

private:
    static bool ParseCharHex(uint8_t& out, char ch);
    static bool ParseCharsHex(uint32_t& out, const string_view& chars);
    static bool AppendUtf8Char(string& inout, uint32_t charVal);

    CodeReader m_Code;

    void SkipSpacesAndComments();
    bool ParseNumber(Token& out);
    bool ParseString(Token& out);
};

////////////////////////////////////////////////////////////////////////////////
// class Value definition

enum class SystemFunction {
    TypeOf, Print,
    Array_Add, Array_Insert, Array_Remove,
    Count
};
static constexpr string_view SYSTEM_FUNCTION_NAMES[] = {
    "typeOf", "print",
    "add", "insert", "remove",
};
static_assert(_countof(SYSTEM_FUNCTION_NAMES) == (size_t)SystemFunction::Count);

struct ObjectMemberLValue
{
    Object* Obj;
    string Key;
};
struct StringCharacterLValue
{
    string* Str;
    size_t Index;
};
struct ArrayItemLValue
{
    Array* Arr;
    size_t Index;
};
struct LValue : public std::variant<ObjectMemberLValue, StringCharacterLValue, ArrayItemLValue>
{
    Value* GetValueRef(const PlaceInCode& place) const; // Always returns non-null or throws exception.
    Value GetValue(const PlaceInCode& place) const;
};

struct ReturnException
{
    const PlaceInCode Place;
    Value ThrownValue;
};

////////////////////////////////////////////////////////////////////////////////
// namespace AST

namespace AST
{

struct ThisType : public std::variant<
    std::monostate,
    shared_ptr<Object>,
    shared_ptr<Array>>
{
    bool IsEmpty() const { return std::get_if<std::monostate>(this) != nullptr; }
    Object* GetObject_() const
    {
        const shared_ptr<Object>* objectPtr = std::get_if<shared_ptr<Object>>(this);
        return objectPtr ? objectPtr->get() : nullptr;
    }
    Array* GetArray() const
    {
        const shared_ptr<Array>* arrayPtr = std::get_if<shared_ptr<Array>>(this);
        return arrayPtr ? arrayPtr->get() : nullptr;
    }
    void Clear() { *this = ThisType{}; }
};

struct ExecuteContext
{
public:
    EnvironmentPimpl& Env;
    Object& GlobalScope;

    struct LocalScopePush
    {
        LocalScopePush(ExecuteContext& ctx, Object* localScope, ThisType&& thisObj, const PlaceInCode& place) :
            m_Ctx{ctx}
        {
            if(ctx.LocalScopes.size() == LOCAL_SCOPE_STACK_MAX_SIZE)
                throw ExecutionError{place, ERROR_MESSAGE_STACK_OVERFLOW};
            ctx.LocalScopes.push_back(localScope);
            ctx.Thises.push_back(std::move(thisObj));
        }
        ~LocalScopePush()
        {
            m_Ctx.Thises.pop_back();
            m_Ctx.LocalScopes.pop_back();
        }
    private:
        ExecuteContext& m_Ctx;
    };

    ExecuteContext(EnvironmentPimpl& env, Object& globalScope) : Env{env}, GlobalScope{globalScope} { }
    bool IsLocal() const { return !LocalScopes.empty(); }
    Object* GetCurrentLocalScope() { assert(IsLocal()); return LocalScopes.back(); }
    const ThisType& GetThis() { assert(IsLocal()); return Thises.back(); }
    Object& GetInnermostScope() const { return IsLocal() ? *LocalScopes.back() : GlobalScope; }

private:
    vector<Object*> LocalScopes;
    vector<ThisType> Thises;
};

struct Statement
{
    explicit Statement(const PlaceInCode& place) : m_Place{place} { }
    virtual ~Statement() { }
    const PlaceInCode& GetPlace() const { return m_Place; }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const = 0;
    virtual void Execute(ExecuteContext& ctx) const = 0;
protected:
    void Assign(const LValue& lhs, Value&& rhs) const;
private:
    const PlaceInCode m_Place;
};

struct EmptyStatement : public Statement
{
    explicit EmptyStatement(const PlaceInCode& place) : Statement{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual void Execute(ExecuteContext& ctx) const { }
};

struct Expression;

struct Condition : public Statement
{
    unique_ptr<Expression> ConditionExpression;
    unique_ptr<Statement> Statements[2]; // [0] executed if true, [1] executed if false, optional.
    explicit Condition(const PlaceInCode& place) : Statement{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

const enum WhileLoopType { While, DoWhile };

struct WhileLoop : public Statement
{
    WhileLoopType Type;
    unique_ptr<Expression> ConditionExpression;
    unique_ptr<Statement> Body;
    explicit WhileLoop(const PlaceInCode& place, WhileLoopType type) : Statement{place}, Type{type} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

struct ForLoop : public Statement
{
    unique_ptr<Expression> InitExpression; // Optional
    unique_ptr<Expression> ConditionExpression; // Optional
    unique_ptr<Expression> IterationExpression; // Optional
    unique_ptr<Statement> Body;
    explicit ForLoop(const PlaceInCode& place) : Statement{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

struct RangeBasedForLoop : public Statement
{
    string KeyVarName; // Can be empty.
    string ValueVarName; // Cannot be empty.
    unique_ptr<Expression> RangeExpression;
    unique_ptr<Statement> Body;
    explicit RangeBasedForLoop(const PlaceInCode& place) : Statement{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

enum class LoopBreakType { Break, Continue, Count };
struct LoopBreakStatement : public Statement
{
    LoopBreakType Type;
    explicit LoopBreakStatement(const PlaceInCode& place, LoopBreakType type) : Statement{place}, Type{type} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

struct ReturnStatement : public Statement
{
    unique_ptr<Expression> ReturnedValue; // Can be null.
    explicit ReturnStatement(const PlaceInCode& place) : Statement{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

struct Block : public Statement
{
    explicit Block(const PlaceInCode& place) : Statement{place} { }
    vector<unique_ptr<Statement>> Statements;
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

struct ConstantValue;

struct SwitchStatement : public Statement
{
    unique_ptr<Expression> Condition;
    vector<unique_ptr<AST::ConstantValue>> ItemValues; // null means default block.
    vector<unique_ptr<AST::Block>> ItemBlocks; // Can be null if empty.
    explicit SwitchStatement(const PlaceInCode& place) : Statement{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

struct ThrowStatement : public Statement
{
    unique_ptr<Expression> ThrownExpression;
    explicit ThrowStatement(const PlaceInCode& place) : Statement{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual void Execute(ExecuteContext& ctx) const;
};

struct TryStatement : public Statement
{
    unique_ptr<Statement> TryBlock;
    unique_ptr<Statement> CatchBlock; // Optional
    unique_ptr<Statement> FinallyBlock; // Optional
    string ExceptionVarName;
    explicit TryStatement(const PlaceInCode& place) : Statement{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
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
    virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const { return GetLValue(ctx).GetValue(GetPlace()); }
    virtual LValue GetLValue(ExecuteContext& ctx) const { EXECUTION_CHECK( false, GetPlace(), ERROR_MESSAGE_EXPECTED_LVALUE ); }
    virtual void Execute(ExecuteContext& ctx) const { Evaluate(ctx, nullptr); }
};

struct ConstantExpression : Expression
{
    explicit ConstantExpression(const PlaceInCode& place) : Expression{place} { }
    virtual void Execute(ExecuteContext& ctx) const { /* Nothing - just ignore its value. */ }
};

struct ConstantValue : ConstantExpression
{
    Value Val;
    ConstantValue(const PlaceInCode& place, Value&& val) : ConstantExpression{place}, Val{std::move(val)}
    {
        assert(Val.GetType() == ValueType::Null || Val.GetType() == ValueType::Number || Val.GetType() == ValueType::String);
    }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const { return Value{Val}; }
};

enum class IdentifierScope { None, Local, Global, Count };
struct Identifier : ConstantExpression
{
    IdentifierScope Scope = IdentifierScope::Count;
    string S;
    Identifier(const PlaceInCode& place, IdentifierScope scope, string&& s) : ConstantExpression{place}, Scope(scope), S(std::move(s)) { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
    virtual LValue GetLValue(ExecuteContext& ctx) const;
};

struct ThisExpression : ConstantExpression
{
    ThisExpression(const PlaceInCode& place) : ConstantExpression{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
};

struct Operator : Expression
{
    explicit Operator(const PlaceInCode& place) : Expression{place} { }
};

enum class UnaryOperatorType
{
    Preincrementation, Predecrementation, Postincrementation, Postdecrementation,
    Plus, Minus, LogicalNot, BitwiseNot, Count,
};
struct UnaryOperator : Operator
{
    UnaryOperatorType Type;
    unique_ptr<Expression> Operand;
    UnaryOperator(const PlaceInCode& place, UnaryOperatorType type) : Operator{place}, Type(type) { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
    virtual LValue GetLValue(ExecuteContext& ctx) const;

private:
    Value BitwiseNot(Value&& operand) const;
};

struct MemberAccessOperator : Operator
{
    unique_ptr<Expression> Operand;
    string MemberName;
    MemberAccessOperator(const PlaceInCode& place) : Operator{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
    virtual LValue GetLValue(ExecuteContext& ctx) const;
};

enum class BinaryOperatorType
{
    Mul, Div, Mod, Add, Sub, ShiftLeft, ShiftRight,
    Assignment, AssignmentAdd, AssignmentSub, AssignmentMul, AssignmentDiv, AssignmentMod, AssignmentShiftLeft, AssignmentShiftRight,
    AssignmentBitwiseAnd, AssignmentBitwiseXor, AssignmentBitwiseOr,
    Less, Greater, LessEqual, GreaterEqual, Equal, NotEqual,
    BitwiseAnd, BitwiseXor, BitwiseOr, LogicalAnd, LogicalOr,
    Comma, Indexing, Count
};
struct BinaryOperator : Operator
{
    BinaryOperatorType Type;
    unique_ptr<Expression> Operands[2];
    BinaryOperator(const PlaceInCode& place, BinaryOperatorType type) : Operator{place}, Type(type) { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
    virtual LValue GetLValue(ExecuteContext& ctx) const;

private:
    Value ShiftLeft(const Value& lhs, const Value& rhs) const;
    Value ShiftRight(const Value& lhs, const Value& rhs) const;
    Value Assignment(LValue&& lhs, Value&& rhs) const;
};

struct TernaryOperator : Operator
{
    unique_ptr<Expression> Operands[3];
    explicit TernaryOperator(const PlaceInCode& place) : Operator{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
};

struct CallOperator : Operator
{
    vector<unique_ptr<Expression>> Operands;
    CallOperator(const PlaceInCode& place) : Operator{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
};

struct FunctionDefinition : public Expression
{
    vector<string> Parameters;
    Block Body;
    FunctionDefinition(const PlaceInCode& place) : Expression{place}, Body{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const { return Value{this}; }
    bool AreParameterNamesUnique() const;
};

struct ObjectExpression : public Expression
{
    unique_ptr<Expression> BaseExpression;
    using ItemMap = std::map<string, unique_ptr<Expression>>;
    ItemMap Items;
    ObjectExpression(const PlaceInCode& place) : Expression{place} { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
};

struct ArrayExpression : public Expression
{
    vector<unique_ptr<Expression>> Items;
    ArrayExpression(const PlaceInCode& place) : Expression{ place } { }
    virtual void DebugPrint(uint32_t indentLevel, const string_view& prefix) const;
    virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
};

} // namespace AST

static inline void CheckNumberOperand(const AST::Expression* operand, const Value& value)
{
    EXECUTION_CHECK( value.GetType() == ValueType::Number, operand->GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER );
}

////////////////////////////////////////////////////////////////////////////////
// class Parser definition

class Parser
{
public:
    Parser(Tokenizer& tokenizer) : m_Tokenizer(tokenizer) { }
    void ParseScript(AST::Script& outScript);

private:
    Tokenizer& m_Tokenizer;
    vector<Token> m_Tokens;
    size_t m_TokenIndex = 0;

    void ParseBlock(AST::Block& outBlock);
    bool TryParseSwitchItem(AST::SwitchStatement& switchStatement);
    void ParseFunctionDefinition(AST::FunctionDefinition& funcDef);
    bool PeekSymbols(std::initializer_list<Symbol> arr);
    unique_ptr<AST::Statement> TryParseStatement();
    unique_ptr<AST::ConstantValue> TryParseConstantValue();
    unique_ptr<AST::Identifier> TryParseIdentifierValue();
    unique_ptr<AST::ConstantExpression> TryParseConstantExpr();
    std::pair<string, unique_ptr<AST::FunctionDefinition>> TryParseFunctionSyntacticSugar();
    unique_ptr<AST::Expression> TryParseClassSyntacticSugar();
    unique_ptr<AST::Expression> TryParseObjectMember(string& outMemberName);
    unique_ptr<AST::ObjectExpression> TryParseObject();
    unique_ptr<AST::ArrayExpression> TryParseArray();
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
    string TryParseIdentifier(); // If failed, returns empty string.
    const PlaceInCode& GetCurrentTokenPlace() const { return m_Tokens[m_TokenIndex].Place; }
};
  
  ////////////////////////////////////////////////////////////////////////////////
// class EnvironmentPimpl definition

class EnvironmentPimpl
{
public:
    EnvironmentPimpl() : m_Script{PlaceInCode{0, 1, 1}} { }
    ~EnvironmentPimpl() = default;
    Value Execute(const string_view& code);
    const string& GetOutput() const { return m_Output; }
    void Print(const string_view& s) { m_Output.append(s); }

private:
    AST::Script m_Script;
    Object m_GlobalScope;
    string m_Output;
};

////////////////////////////////////////////////////////////////////////////////
// class Error implementation

const char* Error::what() const
{
    if(m_What.empty())
        m_What = Format("(%u,%u): %.*s", m_Place.Row, m_Place.Column, (int)GetMessage_().length(), GetMessage_().data());
    return m_What.c_str();
}

////////////////////////////////////////////////////////////////////////////////
// class Tokenizer implementation

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
    if(ParseNumber(out))
        return;
    // Multi char symbol
    for(size_t i = (size_t)firstMultiCharSymbol; i < (size_t)firstKeywordSymbol; ++i)
    {
        const size_t symbolLen = SYMBOL_STR[i].length();
        if(currentCodeLen >= symbolLen && memcmp(SYMBOL_STR[i].data(), currentCode, symbolLen) == 0)
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
    // Identifier or keyword
    if(IsAlpha(currentCode[0]))
    {
        size_t tokenLen = 1;
        while(tokenLen < currentCodeLen && IsAlphaNumeric(currentCode[tokenLen]))
            ++tokenLen;
        // Keyword
        for(size_t i = (size_t)firstKeywordSymbol; i < (size_t)Symbol::Count; ++i)
        {
            const size_t keywordLen = SYMBOL_STR[i].length();
            if(keywordLen == tokenLen && memcmp(SYMBOL_STR[i].data(), currentCode, tokenLen) == 0)
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

bool Tokenizer::ParseCharsHex(uint32_t& out, const string_view& chars)
{
    out = 0;
    for(size_t i = 0, count = chars.length(); i < count; ++i)
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
                if(tokenLen + 2 >= currCodeLen || !ParseCharsHex(val, string_view{currCode + tokenLen + 1, 2}))
                    throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                out.String += (char)(uint8_t)val;
                tokenLen += 3;
                break;
            }
            case 'u':
            {
                uint32_t val = 0;
                if(tokenLen + 4 >= currCodeLen || !ParseCharsHex(val, string_view{currCode + tokenLen + 1, 4}))
                    throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                if(!AppendUtf8Char(out.String, val))
                    throw ParsingError(out.Place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                tokenLen += 5;
                break;
            }
            case 'U':
            {
                uint32_t val = 0;
                if(tokenLen + 8 >= currCodeLen || !ParseCharsHex(val, string_view{currCode + tokenLen + 1, 8}))
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

Value* Object::TryGetValue(const string& key)
{
    auto it = m_Items.find(key);
    if(it != m_Items.end())
        return &it->second;
    return nullptr;
}
const Value* Object::TryGetValue(const string& key) const
{
    auto it = m_Items.find(key);
    if(it != m_Items.end())
        return &it->second;
    return nullptr;
}

bool Object::Remove(const string& key)
{
    auto it = m_Items.find(key);
    if(it != m_Items.end())
    {
        m_Items.erase(it);
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
// struct LValue implementation

Value* LValue::GetValueRef(const PlaceInCode& place) const
{
    if(const ObjectMemberLValue* objMemberLval = std::get_if<ObjectMemberLValue>(this))
    {
        if(Value* val = objMemberLval->Obj->TryGetValue(objMemberLval->Key))
            return val;
        EXECUTION_FAIL(place, ERROR_MESSAGE_OBJECT_MEMBER_DOESNT_EXIST);
    }
    if(const ArrayItemLValue* arrItemLval = std::get_if<ArrayItemLValue>(this))
    {
        EXECUTION_CHECK(arrItemLval->Index < arrItemLval->Arr->Items.size(), place, ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
        return &arrItemLval->Arr->Items[arrItemLval->Index];
    }
    EXECUTION_FAIL(place, ERROR_MESSAGE_INVALID_LVALUE);
}

Value LValue::GetValue(const PlaceInCode& place) const
{
    if(const ObjectMemberLValue* objMemberLval = std::get_if<ObjectMemberLValue>(this))
    {
        if(const Value* val = objMemberLval->Obj->TryGetValue(objMemberLval->Key))
            return *val;
        EXECUTION_FAIL(place, ERROR_MESSAGE_OBJECT_MEMBER_DOESNT_EXIST);
    }
    if(const StringCharacterLValue* strCharLval = std::get_if<StringCharacterLValue>(this))
    {
        EXECUTION_CHECK(strCharLval->Index < strCharLval->Str->length(), place, ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
        const char ch = (*strCharLval->Str)[strCharLval->Index];
        return Value{string{&ch, &ch + 1}};
    }
    if(const ArrayItemLValue* arrItemLval = std::get_if<ArrayItemLValue>(this))
    {
        EXECUTION_CHECK(arrItemLval->Index < arrItemLval->Arr->Items.size(), place, ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
        return Value{arrItemLval->Arr->Items[arrItemLval->Index]};
    }
    assert(0);
    return {};
}

////////////////////////////////////////////////////////////////////////////////
// Built-in functions

static shared_ptr<Object> CopyObject(const Object& src)
{
    auto dst = std::make_shared<Object>();
    for(const auto& item : src.m_Items)
        dst->m_Items.insert(item);
    return dst;
}
static shared_ptr<Array> CopyArray(const Array& src)
{
    auto dst = std::make_shared<Array>();
    const size_t count = src.Items.size();
    dst->Items.resize(count);
    for(size_t i = 0; i < count; ++i)
        dst->Items[i] = Value{src.Items[i]};
    return dst;
}
static shared_ptr<Object> ConvertExecutionErrorToObject(const ExecutionError& err)
{
    auto obj = std::make_shared<Object>();
    obj->GetOrCreateValue("Type") = Value{"ExecutionError"};
    obj->GetOrCreateValue("Index") = Value{(double)err.GetPlace().Index};
    obj->GetOrCreateValue("Row") = Value{(double)err.GetPlace().Row};
    obj->GetOrCreateValue("Column") = Value{(double)err.GetPlace().Column};
    obj->GetOrCreateValue("Message") = Value{string{err.GetMessage_()}};
    return obj;
}
static Value BuiltInTypeCtor_Null(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
{
    EXECUTION_CHECK(args.empty() || args.size() == 1 && args[0].GetType() == ValueType::Null, place, "Null can be constructed only from no arguments or from another null value.");
    return {};
}
static Value BuiltInTypeCtor_Number(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
{
    EXECUTION_CHECK(args.size() == 1 && args[0].GetType() == ValueType::Number, place, "Number can be constructed only from another number.");
    return Value{args[0].GetNumber()};
}
static Value BuiltInTypeCtor_String(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
{
    if(args.empty())
        return Value{string{}};
    EXECUTION_CHECK(args.size() == 1 && args[0].GetType() == ValueType::String, place, "String can be constructed only from no arguments or from another string value.");
    return Value{string{args[0].GetString()}};
}
static Value BuiltInTypeCtor_Object(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
{
    if(args.empty())
        return Value{std::make_shared<Object>()};
    EXECUTION_CHECK(args.size() == 1 && args[0].GetType() == ValueType::Object, place, "Object can be constructed only from no arguments or from another object value.");
    return Value{CopyObject(*args[0].GetObject_())};
}
static Value BuiltInTypeCtor_Array(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
{
    if(args.empty())
        return Value{std::make_shared<Array>()};
    EXECUTION_CHECK(args.size() == 1 && args[0].GetType() == ValueType::Array, place, "Array can be constructed only from no arguments or from another array value.");
    return Value{CopyArray(*args[0].GetArray())};
}
static Value BuiltInTypeCtor_Function(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
{
    EXECUTION_CHECK(args.size() == 1 && (args[0].GetType() == ValueType::Function || args[0].GetType() == ValueType::SystemFunction),
        place, "Function can be constructed only from another function value.");
    return Value{args[0]};
}
static Value BuiltInTypeCtor_Type(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
{
    EXECUTION_CHECK(args.size() == 1 && args[0].GetType() == ValueType::Type, place, "Type can be constructed only from another type value.");
    return Value{args[0]};
}

static Value BuiltInFunction_TypeOf(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
{
    EXECUTION_CHECK(args.size() == 1, place, ERROR_MESSAGE_EXPECTED_1_ARGUMENT);
    return Value{args[0].GetType()};
}

static Value BuiltInFunction_Print(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
{
    string s;
    for(const auto& val : args)
    {
        switch(val.GetType())
        {
        case ValueType::Null:
            ctx.Env.Print("null\n");
            break;
        case ValueType::Number:
            s = Format("%g\n", val.GetNumber());
            ctx.Env.Print(s);
            break;
        case ValueType::String:
            if(!val.GetString().empty())
                ctx.Env.Print(val.GetString());
            ctx.Env.Print("\n");
            break;
        case ValueType::Function:
        case ValueType::SystemFunction:
            ctx.Env.Print("function\n");
            break;
        case ValueType::Object:
            ctx.Env.Print("object\n");
            break;
        case ValueType::Array:
            ctx.Env.Print("array\n");
            break;
        case ValueType::Type:
        {
            const size_t typeIndex = (size_t)val.GetTypeValue();
            const string_view& typeName = VALUE_TYPE_NAMES[typeIndex];
            s = Format("%.*s\n", (int)typeName.length(), typeName.data());
            ctx.Env.Print(s);
        }
            break;
        default: assert(0);
        }
    }
    return {};
}

static Value BuiltInMember_Object_Count(AST::ExecuteContext& ctx, const PlaceInCode& place, Value&& objVal)
{
    EXECUTION_CHECK(objVal.GetType() == ValueType::Object && objVal.GetObject_(), place, ERROR_MESSAGE_EXPECTED_OBJECT);
    return Value{(double)objVal.GetObject_()->GetCount()};
}
static Value BuiltInMember_Array_Count(AST::ExecuteContext& ctx, const PlaceInCode& place, Value&& objVal)
{
    EXECUTION_CHECK(objVal.GetType() == ValueType::Array && objVal.GetArray(), place, ERROR_MESSAGE_EXPECTED_ARRAY);
    return Value{(double)objVal.GetArray()->Items.size()};
}
static Value BuiltInMember_String_Count(AST::ExecuteContext& ctx, const PlaceInCode& place, Value&& objVal)
{
    EXECUTION_CHECK(objVal.GetType() == ValueType::String, place, ERROR_MESSAGE_EXPECTED_STRING);
    return Value{(double)objVal.GetString().length()};
}

static Value BuiltInFunction_Array_Add(AST::ExecuteContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args)
{
    Array* arr = th.GetArray();
    EXECUTION_CHECK(arr, place, ERROR_MESSAGE_EXPECTED_ARRAY);
    EXECUTION_CHECK(args.size() == 1, place, ERROR_MESSAGE_EXPECTED_1_ARGUMENT);
    arr->Items.push_back(std::move(args[0]));
    return {};
}
static Value BuiltInFunction_Array_Insert(AST::ExecuteContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args)
{
    Array* arr = th.GetArray();
    EXECUTION_CHECK(arr, place, ERROR_MESSAGE_EXPECTED_ARRAY);
    EXECUTION_CHECK(args.size() == 2, place, ERROR_MESSAGE_EXPECTED_2_ARGUMENTS);
    size_t index = 0;
    EXECUTION_CHECK(args[0].GetType() == ValueType::Number && NumberToIndex(index, args[0].GetNumber()), place, ERROR_MESSAGE_INVALID_INDEX);
    arr->Items.insert(arr->Items.begin() + index, std::move(args[1]));
    return {};
}
static Value BuiltInFunction_Array_Remove(AST::ExecuteContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args)
{
    Array* arr = th.GetArray();
    EXECUTION_CHECK(arr, place, ERROR_MESSAGE_EXPECTED_ARRAY);
    EXECUTION_CHECK(args.size() == 1, place, ERROR_MESSAGE_EXPECTED_1_ARGUMENT);
    size_t index = 0;
    EXECUTION_CHECK(args[0].GetType() == ValueType::Number && NumberToIndex(index, args[0].GetNumber()), place, ERROR_MESSAGE_INVALID_INDEX);
    arr->Items.erase(arr->Items.begin() + index);
    return {};
}

////////////////////////////////////////////////////////////////////////////////
// Abstract Syntax Tree implementation

namespace AST {

#define DEBUG_PRINT_FORMAT_STR_BEG "(%u,%u) %s%.*s"
#define DEBUG_PRINT_ARGS_BEG GetPlace().Row, GetPlace().Column, GetDebugPrintIndent(indentLevel), (int)prefix.length(), prefix.data()

void Statement::Assign(const LValue& lhs, Value&& rhs) const
{
    if(const ObjectMemberLValue* objMemberLhs = std::get_if<ObjectMemberLValue>(&lhs))
    {
        if(rhs.GetType() == ValueType::Null)
            objMemberLhs->Obj->Remove(objMemberLhs->Key);
        else
            objMemberLhs->Obj->GetOrCreateValue(objMemberLhs->Key) = std::move(rhs);
    }
    else if(const ArrayItemLValue* arrItemLhs = std::get_if<ArrayItemLValue>(&lhs))
    {
        EXECUTION_CHECK( arrItemLhs->Index < arrItemLhs->Arr->Items.size(), GetPlace(), ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS );
        arrItemLhs->Arr->Items[arrItemLhs->Index] = std::move(rhs);
    }
    else if(const StringCharacterLValue* strCharLhs = std::get_if<StringCharacterLValue>(&lhs))
    {
        EXECUTION_CHECK( strCharLhs->Index < strCharLhs->Str->length(), GetPlace(), ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS );
        EXECUTION_CHECK( rhs.GetType() == ValueType::String, GetPlace(), ERROR_MESSAGE_EXPECTED_STRING );
        EXECUTION_CHECK( rhs.GetString().length() == 1, GetPlace(), ERROR_MESSAGE_EXPECTED_SINGLE_CHARACTER_STRING );
        (*strCharLhs->Str)[strCharLhs->Index] = rhs.GetString()[0];
    }
    else
        assert(0);
}

void EmptyStatement::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Empty\n", DEBUG_PRINT_ARGS_BEG);
}

void Condition::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "If\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    ConditionExpression->DebugPrint(indentLevel, "ConditionExpression: ");
    Statements[0]->DebugPrint(indentLevel, "TrueStatement: ");
    if(Statements[1])
        Statements[1]->DebugPrint(indentLevel, "FalseStatement: ");
}

void Condition::Execute(ExecuteContext& ctx) const
{
    if(ConditionExpression->Evaluate(ctx, nullptr).IsTrue())
        Statements[0]->Execute(ctx);
    else if(Statements[1])
        Statements[1]->Execute(ctx);
}

void WhileLoop::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
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
    ConditionExpression->DebugPrint(indentLevel, "ConditionExpression: ");
    Body->DebugPrint(indentLevel, "Body: ");
}

void WhileLoop::Execute(ExecuteContext& ctx) const
{
    switch(Type)
    {
    case WhileLoopType::While:
        while(ConditionExpression->Evaluate(ctx, nullptr).IsTrue())
        {
            try
            {
                Body->Execute(ctx);
            }
            catch(const BreakException&)
            {
                break;
            }
            catch(const ContinueException&)
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
            catch(const BreakException&)
            {
                break;
            }
            catch(const ContinueException&)
            {
                continue;
            }
        }
        while(ConditionExpression->Evaluate(ctx, nullptr).IsTrue());
        break;
    default: assert(0);
    }
}

void ForLoop::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "For\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    if(InitExpression)
        InitExpression->DebugPrint(indentLevel, "InitExpression: ");
    else
        printf(DEBUG_PRINT_FORMAT_STR_BEG "(Init expression empty)\n", DEBUG_PRINT_ARGS_BEG);
    if(ConditionExpression)
        ConditionExpression->DebugPrint(indentLevel, "ConditionExpression: ");
    else
        printf(DEBUG_PRINT_FORMAT_STR_BEG "(Condition expression empty)\n", DEBUG_PRINT_ARGS_BEG);
    if(IterationExpression)
        IterationExpression->DebugPrint(indentLevel, "IterationExpression: ");
    else
        printf(DEBUG_PRINT_FORMAT_STR_BEG "(Iteration expression empty)\n", DEBUG_PRINT_ARGS_BEG);
    Body->DebugPrint(indentLevel, "Body: ");
}

void ForLoop::Execute(ExecuteContext& ctx) const
{
    if(InitExpression)
        InitExpression->Execute(ctx);
    while(ConditionExpression ? ConditionExpression->Evaluate(ctx, nullptr).IsTrue() : true)
    {
        try
        {
            Body->Execute(ctx);
        }
        catch(const BreakException&)
        {
            break;
        }
        catch(const ContinueException&)
        {
        }
        if(IterationExpression)
            IterationExpression->Execute(ctx);
    }
}

void RangeBasedForLoop::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    if(!KeyVarName.empty())
        printf(DEBUG_PRINT_FORMAT_STR_BEG "Range-based for: %s, %s\n", DEBUG_PRINT_ARGS_BEG, KeyVarName.c_str(), ValueVarName.c_str());
    else
        printf(DEBUG_PRINT_FORMAT_STR_BEG "Range-based for: %s\n", DEBUG_PRINT_ARGS_BEG, ValueVarName.c_str());
    ++indentLevel;
    RangeExpression->DebugPrint(indentLevel, "RangeExpression: ");
    Body->DebugPrint(indentLevel, "Body: ");
}

void RangeBasedForLoop::Execute(ExecuteContext& ctx) const
{
    const Value rangeVal = RangeExpression->Evaluate(ctx, nullptr);
    Object& innermostCtxObj = ctx.GetInnermostScope();
    const bool useKey = !KeyVarName.empty();

    if(rangeVal.GetType() == ValueType::String)
    {
        const string& rangeStr = rangeVal.GetString();
        const size_t count = rangeStr.length();
        for(size_t i = 0; i < count; ++i)
        {
            if(useKey)
                Assign(LValue{ObjectMemberLValue{&innermostCtxObj, KeyVarName}}, Value{(double)i});
            const char ch = rangeStr[i];
            Assign(LValue{ObjectMemberLValue{&innermostCtxObj, ValueVarName}}, Value{string{&ch, &ch + 1}});
            try
            {
                Body->Execute(ctx);
            }
            catch(const BreakException&)
            {
                break;
            }
            catch(const ContinueException&)
            {
            }
        }
    }
    else if(rangeVal.GetType() == ValueType::Object)
    {
        for(const auto& [key, value]: rangeVal.GetObject_()->m_Items)
        {
            if(useKey)
                Assign(LValue{ObjectMemberLValue{&innermostCtxObj, KeyVarName}}, Value{string{key}});
            Assign(LValue{ObjectMemberLValue{&innermostCtxObj, ValueVarName}}, Value{value});
            try
            {
                Body->Execute(ctx);
            }
            catch(const BreakException&)
            {
                break;
            }
            catch(const ContinueException&)
            {
            }
        }
    }
    else if(rangeVal.GetType() == ValueType::Array)
    {
        const Array* const arr = rangeVal.GetArray();
        for(size_t i = 0, count = arr->Items.size(); i < count; ++i)
        {
            if(useKey)
                Assign(LValue{ObjectMemberLValue{&innermostCtxObj, KeyVarName}}, Value{(double)i});
            Assign(LValue{ObjectMemberLValue{&innermostCtxObj, ValueVarName}}, Value{arr->Items[i]});
            try
            {
                Body->Execute(ctx);
            }
            catch(const BreakException&)
            {
                break;
            }
            catch(const ContinueException&)
            {
            }
        }
    }
    else
        EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_TYPE);

    if(useKey)
        Assign(LValue{ObjectMemberLValue{&innermostCtxObj, KeyVarName}}, Value{});
    Assign(LValue{ObjectMemberLValue{&innermostCtxObj, ValueVarName}}, Value{});
}

void LoopBreakStatement::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    static const char* LOOP_BREAK_TYPE_NAMES[] = { "Break", "Continue" };
    static_assert(_countof(LOOP_BREAK_TYPE_NAMES) == (size_t)LoopBreakType::Count, "TYPE_NAMES is invalid.");
    printf(DEBUG_PRINT_FORMAT_STR_BEG "%s\n", DEBUG_PRINT_ARGS_BEG, LOOP_BREAK_TYPE_NAMES[(size_t)Type]);
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

void ReturnStatement::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "return\n", DEBUG_PRINT_ARGS_BEG);
    if(ReturnedValue)
        ReturnedValue->DebugPrint(indentLevel + 1, "ReturnedValue: ");
}

void ReturnStatement::Execute(ExecuteContext& ctx) const
{
    if(ReturnedValue)
        throw ReturnException{GetPlace(), ReturnedValue->Evaluate(ctx, nullptr)};
    else
        throw ReturnException{GetPlace(), Value{}};
}

void Block::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Block\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    for(const auto& stmtPtr : Statements)
        stmtPtr->DebugPrint(indentLevel, string_view{});
}

void Block::Execute(ExecuteContext& ctx) const
{
    for(const auto& stmtPtr : Statements)
        stmtPtr->Execute(ctx);
}

void SwitchStatement::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "switch\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    Condition->DebugPrint(indentLevel, "Condition: ");
    for(size_t i = 0; i < ItemValues.size(); ++i)
    {
        if(ItemValues[i])
            ItemValues[i]->DebugPrint(indentLevel, "ItemValue: ");
        else
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Default\n", DEBUG_PRINT_ARGS_BEG);
        if(ItemBlocks[i])
            ItemBlocks[i]->DebugPrint(indentLevel, "ItemBlock: ");
        else
            printf(DEBUG_PRINT_FORMAT_STR_BEG "(Empty block)\n", DEBUG_PRINT_ARGS_BEG);
    }
}

void SwitchStatement::Execute(ExecuteContext& ctx) const
{
    const Value condVal = Condition->Evaluate(ctx, nullptr);
    size_t itemIndex, defaultItemIndex = SIZE_MAX;
    const size_t itemCount = ItemValues.size();
    for(itemIndex = 0; itemIndex < itemCount; ++itemIndex)
    {
        if(ItemValues[itemIndex])
        {
            if(ItemValues[itemIndex]->Val.IsEqual(condVal))
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
            catch(const BreakException&)
            {
                break;
            }
        }
    }
}

void ThrowStatement::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "throw\n", DEBUG_PRINT_ARGS_BEG);
    ThrownExpression->DebugPrint(indentLevel + 1, "ThrownExpression: ");
}

void ThrowStatement::Execute(ExecuteContext& ctx) const
{
    throw ThrownExpression->Evaluate(ctx, nullptr);
}

void TryStatement::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "try\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    TryBlock->DebugPrint(indentLevel, "TryBlock: ");
    if(CatchBlock)
        CatchBlock->DebugPrint(indentLevel, "CatchBlock: ");
    if(FinallyBlock)
        FinallyBlock->DebugPrint(indentLevel, "FinallyBlock: ");
}

void TryStatement::Execute(ExecuteContext& ctx) const
{
    // Careful with this function! It contains logic that was difficult to get right.
    try
        { TryBlock->Execute(ctx); }
    catch(Value &val)
    {
        if(CatchBlock)
        {
            Object& innermostCtxObj = ctx.GetInnermostScope();
            Assign(LValue{ObjectMemberLValue{&innermostCtxObj, ExceptionVarName}}, std::move(val));
            CatchBlock->Execute(ctx);
            Assign(LValue{ObjectMemberLValue{&innermostCtxObj, ExceptionVarName}}, Value{});
            if(FinallyBlock)
                FinallyBlock->Execute(ctx);
        }
        else
        {
            assert(FinallyBlock);
            // One exception is on the fly - new one is ignored, old one is thrown again.
            try
                { FinallyBlock->Execute(ctx); }
            catch(const Value&)
                { throw val; }
            catch(const ExecutionError&)
                { throw val; }
            throw val;
        }
        return;
    }
    catch(const ExecutionError& err)
    {
        if(CatchBlock)
        {
            Object& innermostCtxObj = ctx.GetInnermostScope();
            Assign(LValue{ObjectMemberLValue{&innermostCtxObj, ExceptionVarName}}, Value{ConvertExecutionErrorToObject(err)});
            CatchBlock->Execute(ctx);
            Assign(LValue{ObjectMemberLValue{&innermostCtxObj, ExceptionVarName}}, Value{});
            if(FinallyBlock)
                FinallyBlock->Execute(ctx);
        }
        else
        {
            assert(FinallyBlock);
            // One exception is on the fly - new one is ignored, old one is thrown again.
            try { FinallyBlock->Execute(ctx); }
            catch(const Value&)
                { throw err; }
            catch(const ExecutionError&)
                { throw err; }
            throw err;
        }
        return;
    }
    catch(const BreakException&)
    {
        if(FinallyBlock)
            FinallyBlock->Execute(ctx);
        throw;
    }
    catch(const ContinueException&)
    {
        if(FinallyBlock)
            FinallyBlock->Execute(ctx);
        throw;
    }
    catch(ReturnException&)
    {
        if(FinallyBlock)
            FinallyBlock->Execute(ctx);
        throw;
    }
    catch(const ParsingError&)
        { assert(0 && "ParsingError not expected during execution."); }
    if(FinallyBlock)
        FinallyBlock->Execute(ctx);
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

void ConstantValue::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    switch(Val.GetType())
    {
    case ValueType::Null: printf(DEBUG_PRINT_FORMAT_STR_BEG "Constant null\n", DEBUG_PRINT_ARGS_BEG); break;
    case ValueType::Number: printf(DEBUG_PRINT_FORMAT_STR_BEG "Constant number: %g\n", DEBUG_PRINT_ARGS_BEG, Val.GetNumber()); break;
    case ValueType::String: printf(DEBUG_PRINT_FORMAT_STR_BEG "Constant string: %s\n", DEBUG_PRINT_ARGS_BEG, Val.GetString().c_str()); break;
    default: assert(0 && "ConstantValue should not be used with this type.");
    }
}

void Identifier::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    static const char* PREFIX[] = { "", "local.", "global." };
    static_assert(_countof(PREFIX) == (size_t)IdentifierScope::Count);
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Identifier: %s%s\n", DEBUG_PRINT_ARGS_BEG, PREFIX[(size_t)Scope], S.c_str());
}

Value Identifier::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
{
    EXECUTION_CHECK(Scope != IdentifierScope::Local || ctx.IsLocal(), GetPlace(), ERROR_MESSAGE_NO_LOCAL_SCOPE);

    if(ctx.IsLocal())
    {
        // Local variable
        if((Scope == IdentifierScope::None || Scope == IdentifierScope::Local))
            if(Value* val = ctx.GetCurrentLocalScope()->TryGetValue(S); val)
                return *val;
        // This
        if(Scope == IdentifierScope::None)
        {
            if(const shared_ptr<Object>* thisObj = std::get_if<shared_ptr<Object>>(&ctx.GetThis()); thisObj)
            {
                if(Value* val = (*thisObj)->TryGetValue(S); val)
                {
                    if(outThis)
                        *outThis = ThisType{*thisObj};
                    return *val;
                }
            }
        }
    }
    
    if(Scope == IdentifierScope::None || Scope == IdentifierScope::Global)
    {
        // Global variable
        Value* val = ctx.GlobalScope.TryGetValue(S);
        if(val)
            return *val;
        // Type
        for(size_t i = 0, count = (size_t)ValueType::Count; i < count; ++i)
            if(S == VALUE_TYPE_NAMES[i])
                return Value{(ValueType)i};
        // System function
        for(size_t i = 0, count = (size_t)SystemFunction::Count; i < count; ++i)
            if(S == SYSTEM_FUNCTION_NAMES[i])
                return Value{(SystemFunction)i};
    }

    // Not found - null
    return {};
}

LValue Identifier::GetLValue(ExecuteContext& ctx) const
{
    const bool isLocal = ctx.IsLocal();
    EXECUTION_CHECK(Scope != IdentifierScope::Local || isLocal, GetPlace(), ERROR_MESSAGE_NO_LOCAL_SCOPE);

    if(isLocal)
    {
        // Local variable
        if((Scope == IdentifierScope::None || Scope == IdentifierScope::Local) && ctx.GetCurrentLocalScope()->HasKey(S))
            return LValue{ObjectMemberLValue{&*ctx.GetCurrentLocalScope(), S}};
        // This
        if(Scope == IdentifierScope::None)
        {
            if(const shared_ptr<Object>* thisObj = std::get_if<shared_ptr<Object>>(&ctx.GetThis()); thisObj && (*thisObj)->HasKey(S))
                return LValue{ObjectMemberLValue{(*thisObj).get(), S}};
        }
    }    
    
    // Global variable
    if((Scope == IdentifierScope::None || Scope == IdentifierScope::Global) && ctx.GlobalScope.HasKey(S))
        return LValue{ObjectMemberLValue{&ctx.GlobalScope, S}};

    // Not found: return reference to smallest scope.
    if((Scope == IdentifierScope::None || Scope == IdentifierScope::Local) && isLocal)
        return LValue{ObjectMemberLValue{ctx.GetCurrentLocalScope(), S}};
    return LValue{ObjectMemberLValue{&ctx.GlobalScope, S}};
}

void ThisExpression::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "This\n", DEBUG_PRINT_ARGS_BEG);
}

Value ThisExpression::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
{
    EXECUTION_CHECK(ctx.IsLocal() && std::get_if<shared_ptr<Object>>(&ctx.GetThis()), GetPlace(), ERROR_MESSAGE_NO_THIS);
    return Value{shared_ptr<Object>{*std::get_if<shared_ptr<Object>>(&ctx.GetThis())}};
}

void UnaryOperator::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    static const char* UNARY_OPERATOR_TYPE_NAMES[] = {
        "Preincrementation", "Predecrementation", "Postincrementation", "Postdecrementation",
        "Plus", "Minus", "Logical NOT", "Bitwise NOT" };
    static_assert(_countof(UNARY_OPERATOR_TYPE_NAMES) == (size_t)UnaryOperatorType::Count, "UNARY_OPERATOR_TYPE_NAMES is invalid.");
    printf(DEBUG_PRINT_FORMAT_STR_BEG "UnaryOperator %s\n", DEBUG_PRINT_ARGS_BEG, UNARY_OPERATOR_TYPE_NAMES[(uint32_t)Type]);
    ++indentLevel;
    Operand->DebugPrint(indentLevel, "Operand: ");
}

Value UnaryOperator::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
{
    // Those require l-value.
    if(Type == UnaryOperatorType::Preincrementation ||
        Type == UnaryOperatorType::Predecrementation ||
        Type == UnaryOperatorType::Postincrementation ||
        Type == UnaryOperatorType::Postdecrementation)
    {
        Value* val = Operand->GetLValue(ctx).GetValueRef(GetPlace());
        EXECUTION_CHECK( val->GetType() == ValueType::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER );
        switch(Type)
        {
        case UnaryOperatorType::Preincrementation: val->ChangeNumber(val->GetNumber() + 1.0); return *val;
        case UnaryOperatorType::Predecrementation: val->ChangeNumber(val->GetNumber() - 1.0); return *val;
        case UnaryOperatorType::Postincrementation:
        {
            Value result = *val;
            val->ChangeNumber(val->GetNumber() + 1.0);
            return std::move(result);
        }
        case UnaryOperatorType::Postdecrementation:
        {
            Value result = *val;
            val->ChangeNumber(val->GetNumber() - 1.0);
            return std::move(result);
        }
        default: assert(0); return {};
        }
    }
    // Those use r-value.
    else if(Type == UnaryOperatorType::Plus ||
        Type == UnaryOperatorType::Minus ||
        Type == UnaryOperatorType::LogicalNot ||
        Type == UnaryOperatorType::BitwiseNot)
    {
        Value val = Operand->Evaluate(ctx, nullptr);
        EXECUTION_CHECK( val.GetType() == ValueType::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER );
        switch(Type)
        {
        case UnaryOperatorType::Plus: return val;
        case UnaryOperatorType::Minus: return Value{-val.GetNumber()};
        case UnaryOperatorType::LogicalNot: return Value{val.IsTrue() ? 0.0 : 1.0};
        case UnaryOperatorType::BitwiseNot: return BitwiseNot(std::move(val));
        default: assert(0); return {};
        }
    }
    assert(0); return {};
}

LValue UnaryOperator::GetLValue(ExecuteContext& ctx) const
{
    if(Type == UnaryOperatorType::Preincrementation || Type == UnaryOperatorType::Predecrementation)
    {
        LValue lval = Operand->GetLValue(ctx);
        const ObjectMemberLValue* objMemberLval = std::get_if<ObjectMemberLValue>(&lval);
        EXECUTION_CHECK( objMemberLval, GetPlace(), ERROR_MESSAGE_INVALID_LVALUE );
        Value* val = objMemberLval->Obj->TryGetValue(objMemberLval->Key);
        EXECUTION_CHECK( val != nullptr, GetPlace(), ERROR_MESSAGE_VARIABLE_DOESNT_EXIST );
        EXECUTION_CHECK( val->GetType() == ValueType::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER );
        switch(Type)
        {
        case UnaryOperatorType::Preincrementation: val->ChangeNumber(val->GetNumber() + 1.0); return lval;
        case UnaryOperatorType::Predecrementation: val->ChangeNumber(val->GetNumber() - 1.0); return lval;
        default: assert(0);
        }
    }
    EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_LVALUE);
}

void MemberAccessOperator::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "MemberAccessOperator Member=%s\n", DEBUG_PRINT_ARGS_BEG, MemberName.c_str());
    Operand->DebugPrint(indentLevel + 1, "Operand: ");
}

Value MemberAccessOperator::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
{
    Value objVal = Operand->Evaluate(ctx, nullptr);
    if(objVal.GetType() == ValueType::Object)
    {
        const Value* memberVal = objVal.GetObject_()->TryGetValue(MemberName);
        if(memberVal)
        {
            if(outThis)
                *outThis = ThisType{objVal.GetObjectPtr()};
            return *memberVal;
        }
        if(MemberName == "count")
            return BuiltInMember_Object_Count(ctx, GetPlace(), std::move(objVal));
        return {};
    }
    if(objVal.GetType() == ValueType::String)
    {
        if(MemberName == "count")
            return BuiltInMember_String_Count(ctx, GetPlace(), std::move(objVal));
        EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_MEMBER);
    }
    if(objVal.GetType() == ValueType::Array)
    {
        if(outThis)
            *outThis = ThisType{objVal.GetArrayPtr()};
        if(MemberName == "count") return BuiltInMember_Array_Count(ctx, GetPlace(), std::move(objVal));
        else if(MemberName == "add") return Value{SystemFunction::Array_Add};
        else if(MemberName == "insert") return Value{SystemFunction::Array_Insert};
        else if(MemberName == "remove") return Value{SystemFunction::Array_Remove};
        EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_MEMBER);
    }
    EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_TYPE);
}

LValue MemberAccessOperator::GetLValue(ExecuteContext& ctx) const
{
    Value objVal = Operand->Evaluate(ctx, nullptr);
    EXECUTION_CHECK(objVal.GetType() == ValueType::Object, GetPlace(), ERROR_MESSAGE_EXPECTED_OBJECT);
    return LValue{ObjectMemberLValue{objVal.GetObject_(), MemberName}};
}

Value UnaryOperator::BitwiseNot(Value&& operand) const
{
    const int64_t operandInt = (int64_t)operand.GetNumber();
    const int64_t resultInt = ~operandInt;
    return Value{(double)resultInt};
}

void BinaryOperator::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    static const char* BINARY_OPERATOR_TYPE_NAMES[] = {
        "Mul", "Div", "Mod", "Add", "Sub", "Shift left", "Shift right",
        "Assignment", "AssignmentAdd", "AssignmentSub", "AssignmentMul", "AssignmentDiv", "AssignmentMod", "AssignmentShiftLeft", "AssignmentShiftRight",
        "AssignmentBitwiseAnd", "AssignmentBitwiseXor", "AssignmentBitwiseOr",
        "Less", "Greater", "LessEqual", "GreaterEqual", "Equal", "NotEqual",
        "BitwiseAnd", "BitwiseXor", "BitwiseOr", "LogicalAnd", "LogicalOr",
        "Comma", "Indexing", };
    static_assert(_countof(BINARY_OPERATOR_TYPE_NAMES) == (size_t)BinaryOperatorType::Count, "BINARY_OPERATOR_TYPE_NAMES is invalid.");
    printf(DEBUG_PRINT_FORMAT_STR_BEG "BinaryOperator %s\n", DEBUG_PRINT_ARGS_BEG, BINARY_OPERATOR_TYPE_NAMES[(uint32_t)Type]);
    ++indentLevel;
    Operands[0]->DebugPrint(indentLevel, "LeftOperand: ");
    Operands[1]->DebugPrint(indentLevel, "RightOperand: ");
}

Value BinaryOperator::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
{
    // This operator is special, discards result of left operand.
    if(Type == BinaryOperatorType::Comma)
    {
        Operands[0]->Execute(ctx);
        return Operands[1]->Evaluate(ctx, outThis);
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
        return Assignment(Operands[0]->GetLValue(ctx), Operands[1]->Evaluate(ctx, nullptr));
    }
    
    // Remaining operators use r-values.
    Value lhs = Operands[0]->Evaluate(ctx, nullptr);

    // Logical operators with short circuit for right hand side operand.
    if(Type == BinaryOperatorType::LogicalAnd)
    {
        if(!lhs.IsTrue())
            return lhs;
        return Operands[1]->Evaluate(ctx, nullptr);
    }
    if(Type == BinaryOperatorType::LogicalOr)
    {
        if(lhs.IsTrue())
            return lhs;
        return Operands[1]->Evaluate(ctx, nullptr);
    }

    // Remaining operators use both operands as r-values.
    Value rhs = Operands[1]->Evaluate(ctx, nullptr);

    const ValueType lhsType = lhs.GetType();
    const ValueType rhsType = rhs.GetType();

    // These ones support various types.
    if(Type == BinaryOperatorType::Add)
    {
        if(lhsType == ValueType::Number && rhsType == ValueType::Number)
            return Value{lhs.GetNumber() + rhs.GetNumber()};
        if(lhsType == ValueType::String && rhsType == ValueType::String)
            return Value{lhs.GetString() + rhs.GetString()};
        EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INCOMPATIBLE_TYPES);
    }
    if(Type == BinaryOperatorType::Equal)
    {
        return Value{lhs.IsEqual(rhs) ? 1.0 : 0.0};
    }
    if(Type == BinaryOperatorType::NotEqual)
    {
        return Value{!lhs.IsEqual(rhs) ? 1.0 : 0.0};
    }
    if(Type == BinaryOperatorType::Less || Type == BinaryOperatorType::LessEqual ||
        Type == BinaryOperatorType::Greater || Type == BinaryOperatorType::GreaterEqual)
    {
        bool result = false;
        EXECUTION_CHECK( lhsType == rhsType, GetPlace(), ERROR_MESSAGE_INCOMPATIBLE_TYPES );
        if(lhsType == ValueType::Number)
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
        else if(lhsType == ValueType::String)
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
            EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_TYPE);
        return Value{result ? 1.0 : 0.0};
    }
    if(Type == BinaryOperatorType::Indexing)
    {
        if(lhsType == ValueType::String)
        {
            EXECUTION_CHECK( rhsType == ValueType::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER );
            size_t index = 0;
            EXECUTION_CHECK( NumberToIndex(index, rhs.GetNumber()), GetPlace(), ERROR_MESSAGE_INVALID_INDEX );
            EXECUTION_CHECK( index < lhs.GetString().length(), GetPlace(), ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS );
            return Value{string(1, lhs.GetString()[index])};
        }
        if(lhsType == ValueType::Object)
        {
            EXECUTION_CHECK( rhsType == ValueType::String, GetPlace(), ERROR_MESSAGE_EXPECTED_STRING );
            if(Value* val = lhs.GetObject_()->TryGetValue(rhs.GetString()))
            {
                if(outThis)
                    *outThis = ThisType{lhs.GetObjectPtr()};
                return *val;
            }
            return {};
        }
        if(lhsType == ValueType::Array)
        {
            EXECUTION_CHECK( rhsType == ValueType::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER );
            size_t index;
            EXECUTION_CHECK( NumberToIndex(index, rhs.GetNumber()) && index < lhs.GetArray()->Items.size(), GetPlace(), ERROR_MESSAGE_INVALID_INDEX );
            return lhs.GetArray()->Items[index];
        }
        EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_TYPE);
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

    assert(0); return {};
}

LValue BinaryOperator::GetLValue(ExecuteContext& ctx) const
{
    if(Type == BinaryOperatorType::Indexing)
    {
        Value* leftValRef = Operands[0]->GetLValue(ctx).GetValueRef(GetPlace());
        const Value indexVal = Operands[1]->Evaluate(ctx, nullptr);
        if(leftValRef->GetType() == ValueType::String)
        {
            EXECUTION_CHECK( indexVal.GetType() == ValueType::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER );
            size_t charIndex;
            EXECUTION_CHECK( NumberToIndex(charIndex, indexVal.GetNumber()), GetPlace(), ERROR_MESSAGE_INVALID_INDEX );
            return LValue{StringCharacterLValue{&leftValRef->GetString(), charIndex}};
        }
        if(leftValRef->GetType() == ValueType::Object)
        {
            EXECUTION_CHECK( indexVal.GetType() == ValueType::String, GetPlace(), ERROR_MESSAGE_EXPECTED_STRING );
            return LValue{ObjectMemberLValue{leftValRef->GetObject_(), indexVal.GetString()}};
        }
        if(leftValRef->GetType() == ValueType::Array)
        {
            EXECUTION_CHECK( indexVal.GetType() == ValueType::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER );
            size_t itemIndex;
            EXECUTION_CHECK( NumberToIndex(itemIndex, indexVal.GetNumber()), GetPlace(), ERROR_MESSAGE_INVALID_INDEX );
            return LValue{ArrayItemLValue{leftValRef->GetArray(), itemIndex}};
        }
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

Value BinaryOperator::Assignment(LValue&& lhs, Value&& rhs) const
{
    // This one is able to create new value.
    if(Type == BinaryOperatorType::Assignment)
    {
        Assign(lhs, Value{rhs});
        return rhs;
    }

    // Others require existing value.
    Value* const lhsValPtr = lhs.GetValueRef(GetPlace());

    if(Type == BinaryOperatorType::AssignmentAdd)
    {
        if(lhsValPtr->GetType() == ValueType::Number && rhs.GetType() == ValueType::Number)
            lhsValPtr->ChangeNumber(lhsValPtr->GetNumber() + rhs.GetNumber());
        else if(lhsValPtr->GetType() == ValueType::String && rhs.GetType() == ValueType::String)
            lhsValPtr->GetString() += rhs.GetString();
        else
            EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INCOMPATIBLE_TYPES);
        return *lhsValPtr;
    }

    // Remaining ones work on numbers only.
    EXECUTION_CHECK( lhsValPtr->GetType() == ValueType::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER );
    EXECUTION_CHECK( rhs.GetType() == ValueType::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
    switch(Type)
    {
    case BinaryOperatorType::AssignmentSub: lhsValPtr->ChangeNumber(lhsValPtr->GetNumber() - rhs.GetNumber()); break;
    case BinaryOperatorType::AssignmentMul: lhsValPtr->ChangeNumber(lhsValPtr->GetNumber() * rhs.GetNumber()); break;
    case BinaryOperatorType::AssignmentDiv: lhsValPtr->ChangeNumber(lhsValPtr->GetNumber() / rhs.GetNumber()); break;
    case BinaryOperatorType::AssignmentMod: lhsValPtr->ChangeNumber(fmod(lhsValPtr->GetNumber(), rhs.GetNumber())); break;
    case BinaryOperatorType::AssignmentShiftLeft:  *lhsValPtr = ShiftLeft (*lhsValPtr, rhs); break;
    case BinaryOperatorType::AssignmentShiftRight: *lhsValPtr = ShiftRight(*lhsValPtr, rhs); break;
    case BinaryOperatorType::AssignmentBitwiseAnd: lhsValPtr->ChangeNumber( (double)( (int64_t)lhsValPtr->GetNumber() & (int64_t)rhs.GetNumber() ) ); break;
    case BinaryOperatorType::AssignmentBitwiseXor: lhsValPtr->ChangeNumber( (double)( (int64_t)lhsValPtr->GetNumber() ^ (int64_t)rhs.GetNumber() ) ); break;
    case BinaryOperatorType::AssignmentBitwiseOr:  lhsValPtr->ChangeNumber( (double)( (int64_t)lhsValPtr->GetNumber() | (int64_t)rhs.GetNumber() ) ); break;
    default:
        assert(0);
    }

    return *lhsValPtr;
}

void TernaryOperator::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "TernaryOperator\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    Operands[0]->DebugPrint(indentLevel, "ConditionExpression: ");
    Operands[1]->DebugPrint(indentLevel, "TrueExpression: ");
    Operands[2]->DebugPrint(indentLevel, "FalseExpression: ");
}

Value TernaryOperator::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
{
    return Operands[0]->Evaluate(ctx, nullptr).IsTrue() ? Operands[1]->Evaluate(ctx, outThis) : Operands[2]->Evaluate(ctx, outThis);
}

void CallOperator::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "CallOperator\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    Operands[0]->DebugPrint(indentLevel, "Callee: ");
    for(size_t i = 1, count = Operands.size(); i < count; ++i)
        Operands[i]->DebugPrint(indentLevel, "Argument: ");
}

Value CallOperator::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
{
    ThisType th = ThisType{};
    Value callee = Operands[0]->Evaluate(ctx, &th);
    const size_t argCount = Operands.size() - 1;
    vector<Value> arguments(argCount);
    for(size_t i = 0; i < argCount; ++i)
        arguments[i] = Operands[i + 1]->Evaluate(ctx, nullptr);

    // Calling an object: Call its function under '' key.
    if(callee.GetType() == ValueType::Object)
    {
        shared_ptr<Object> calleeObj = callee.GetObjectPtr();
        if(Value* defaultVal = calleeObj->TryGetValue(string{}); defaultVal && defaultVal->GetType() == ValueType::Function)
        {
            callee = *defaultVal;
            th = ThisType{std::move(calleeObj)};
        }
    }

    if(callee.GetType() == ValueType::Function)
    {
        const AST::FunctionDefinition* const funcDef = callee.GetFunction();
        EXECUTION_CHECK( argCount == funcDef->Parameters.size(), GetPlace(), ERROR_MESSAGE_INVALID_NUMBER_OF_ARGUMENTS );
        Object localScope;
        // Setup parameters
        for(size_t argIndex = 0; argIndex != argCount; ++argIndex)
            localScope.GetOrCreateValue(funcDef->Parameters[argIndex]) = std::move(arguments[argIndex]);
        ExecuteContext::LocalScopePush localContextPush{ctx, &localScope, std::move(th), GetPlace()};
        try
        {
            callee.GetFunction()->Body.Execute(ctx);
        }
        catch(ReturnException& returnEx)
        {
            return std::move(returnEx.ThrownValue);
        }
        catch(BreakException)
        {
            EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_BREAK_WITHOUT_LOOP);
        }
        catch(ContinueException)
        {
            EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_CONTINUE_WITHOUT_LOOP);
        }
        return {};
    }
    if(callee.GetType() == ValueType::SystemFunction)
    {
        switch(callee.GetSystemFunction())
        {
        case SystemFunction::TypeOf: return BuiltInFunction_TypeOf(ctx, GetPlace(), std::move(arguments));
        case SystemFunction::Print: return BuiltInFunction_Print(ctx, GetPlace(), std::move(arguments));
        case SystemFunction::Array_Add: return BuiltInFunction_Array_Add(ctx, GetPlace(), th, std::move(arguments));
        case SystemFunction::Array_Insert: return BuiltInFunction_Array_Insert(ctx, GetPlace(), th, std::move(arguments));
        case SystemFunction::Array_Remove: return BuiltInFunction_Array_Remove(ctx, GetPlace(), th, std::move(arguments));
        default: assert(0); return {};
        }
    }
    if(callee.GetType() == ValueType::Type)
    {
        switch(callee.GetTypeValue())
        {
        case ValueType::Null: return BuiltInTypeCtor_Null(ctx, GetPlace(), std::move(arguments));
        case ValueType::Number: return BuiltInTypeCtor_Number(ctx, GetPlace(), std::move(arguments));
        case ValueType::String: return BuiltInTypeCtor_String(ctx, GetPlace(), std::move(arguments));
        case ValueType::Object: return BuiltInTypeCtor_Object(ctx, GetPlace(), std::move(arguments));
        case ValueType::Array: return BuiltInTypeCtor_Array(ctx, GetPlace(), std::move(arguments));
        case ValueType::Type: return BuiltInTypeCtor_Type(ctx, GetPlace(), std::move(arguments));
        case ValueType::Function:
        case ValueType::SystemFunction:
            return BuiltInTypeCtor_Function(ctx, GetPlace(), std::move(arguments));
        default: assert(0); return {};
        }
    }

    EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_FUNCTION);
}

void FunctionDefinition::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Function(", DEBUG_PRINT_ARGS_BEG);
    if(!Parameters.empty())
    {
        printf("%s", Parameters[0].c_str());
        for(size_t i = 1, count = Parameters.size(); i < count; ++i)
            printf(", %s", Parameters[i].c_str());
    }
    printf(")\n");
    Body.DebugPrint(indentLevel + 1, "Body: ");
}

bool FunctionDefinition::AreParameterNamesUnique() const
{
    // Warning! O(n^2) algorithm.
    for(size_t i = 0, count = Parameters.size(); i < count; ++i)
        for(size_t j = i + 1; j < count; ++j)
            if(Parameters[i] == Parameters[j])
                return false;
    return true;
}

void ObjectExpression::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Object\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    for(const auto& [name, value] : Items)
        value->DebugPrint(indentLevel, name);
}

Value ObjectExpression::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
{
    shared_ptr<Object> obj;
    if(BaseExpression)
    {
        Value baseObj = BaseExpression->Evaluate(ctx, nullptr);
        if(baseObj.GetType() != ValueType::Object)
            throw ExecutionError{GetPlace(), ERROR_MESSAGE_BASE_MUST_BE_OBJECT};
        obj = CopyObject(*baseObj.GetObject_());
    }
    else
        obj = make_shared<Object>();
    for(const auto& [name, valueExpr] : Items)
    {
        Value val = valueExpr->Evaluate(ctx, nullptr);
        if(val.GetType() != ValueType::Null)
            obj->GetOrCreateValue(name) = std::move(val);
        else if(BaseExpression)
            obj->Remove(name);
    }
    return Value{std::move(obj)};
}

void ArrayExpression::DebugPrint(uint32_t indentLevel, const string_view& prefix) const
{
    printf(DEBUG_PRINT_FORMAT_STR_BEG "Array\n", DEBUG_PRINT_ARGS_BEG);
    ++indentLevel;
    string itemPrefix;
    for(size_t i = 0, count = Items.size(); i < count; ++i)
    {
        itemPrefix = Format("%zu: ", i);
        Items[i]->DebugPrint(indentLevel, itemPrefix);
    }
}

Value ArrayExpression::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
{
    auto result = std::make_shared<Array>();
    for (const auto& item : Items)
        result->Items.push_back(item->Evaluate(ctx, nullptr));
    return Value{std::move(result)};
}

} // namespace AST

////////////////////////////////////////////////////////////////////////////////
// class Parser implementation

#define MUST_PARSE(result, errorMessage)   do { if(!(result)) throw ParsingError(GetCurrentTokenPlace(), (errorMessage)); } while(false)

void Parser::ParseScript(AST::Script& outScript)
{
    for(;;)
    {
        Token token;
        m_Tokenizer.GetNextToken(token);
        const bool isEnd = token.Symbol == Symbol::End;
        if(token.Symbol == Symbol::String && !m_Tokens.empty() && m_Tokens.back().Symbol == Symbol::String)
            m_Tokens.back().String += token.String;
        else
            m_Tokens.push_back(std::move(token));
        if(isEnd)
            break;
    }

    ParseBlock(outScript);
    if(m_Tokens[m_TokenIndex].Symbol != Symbol::End)
        throw ParsingError(GetCurrentTokenPlace(), ERROR_MESSAGE_PARSING_ERROR);

    //outScript.DebugPrint(0, "Script: "); // #DELME
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

void Parser::ParseFunctionDefinition(AST::FunctionDefinition& funcDef)
{
    MUST_PARSE( TryParseSymbol(Symbol::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN );
    if(m_Tokens[m_TokenIndex].Symbol == Symbol::Identifier)
    {
        funcDef.Parameters.push_back(m_Tokens[m_TokenIndex++].String);
        while(TryParseSymbol(Symbol::Comma))
        {
            MUST_PARSE( m_Tokens[m_TokenIndex].Symbol == Symbol::Identifier, ERROR_MESSAGE_EXPECTED_IDENTIFIER );
            funcDef.Parameters.push_back(m_Tokens[m_TokenIndex++].String);
        }
    }
    MUST_PARSE( funcDef.AreParameterNamesUnique(), ERROR_MESSAGE_PARAMETER_NAMES_MUST_BE_UNIQUE );
    MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE );
    MUST_PARSE( TryParseSymbol(Symbol::CurlyBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_OPEN );
    ParseBlock(funcDef.Body);
    MUST_PARSE( TryParseSymbol(Symbol::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE );
}

bool Parser::PeekSymbols(std::initializer_list<Symbol> symbols)
{
    if(m_TokenIndex + symbols.size() >= m_Tokens.size())
        return false;
    for(size_t i = 0; i < symbols.size(); ++i)
        if(m_Tokens[m_TokenIndex + i].Symbol != symbols.begin()[i])
            return false;
    return true;
}

unique_ptr<AST::Statement> Parser::TryParseStatement()
{
    const PlaceInCode place = GetCurrentTokenPlace();
    
    // Empty statement: ';'
    if(TryParseSymbol(Symbol::Semicolon))
        return make_unique<AST::EmptyStatement>(place);
    
    // Block: '{' Block '}'
    if(!PeekSymbols({Symbol::CurlyBracketOpen, Symbol::String, Symbol::Colon}) && TryParseSymbol(Symbol::CurlyBracketOpen))
    {
        auto block = make_unique<AST::Block>(GetCurrentTokenPlace());
        ParseBlock(*block);
        MUST_PARSE( TryParseSymbol(Symbol::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE );
        return block;
    }

    // Condition: 'if' '(' Expr17 ')' Statement [ 'else' Statement ]
    if(TryParseSymbol(Symbol::If))
    {
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN );
        auto condition = make_unique<AST::Condition>(place);
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
        auto loop = make_unique<AST::WhileLoop>(place, AST::WhileLoopType::While);
        MUST_PARSE( loop->ConditionExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE );
        MUST_PARSE( loop->Body = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT );
        return loop;
    }

    // Loop: 'do' Statement 'while' '(' Expr17 ')' ';'    - loop
    if(TryParseSymbol(Symbol::Do))
    {
        auto loop = make_unique<AST::WhileLoop>(place, AST::WhileLoopType::DoWhile);
        MUST_PARSE( loop->Body = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT );
        MUST_PARSE( TryParseSymbol(Symbol::While), ERROR_MESSAGE_EXPECTED_SYMBOL_WHILE );
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN );
        MUST_PARSE( loop->ConditionExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE );
        MUST_PARSE( TryParseSymbol(Symbol::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON );
        return loop;
    }

    if(TryParseSymbol(Symbol::For))
    {
        MUST_PARSE( TryParseSymbol(Symbol::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN );
        // Range-based loop: 'for' '(' TOKEN_IDENTIFIER [ ',' TOKEN_IDENTIFIER ] ':' Expr17 ')' Statement
        if(PeekSymbols({Symbol::Identifier, Symbol::Colon}) ||
            PeekSymbols({Symbol::Identifier, Symbol::Comma, Symbol::Identifier, Symbol::Colon}))
        {
            auto loop = make_unique<AST::RangeBasedForLoop>(place);
            loop->ValueVarName = TryParseIdentifier();
            MUST_PARSE( !loop->ValueVarName.empty(), ERROR_MESSAGE_EXPECTED_IDENTIFIER );
            if(TryParseSymbol(Symbol::Comma))
            {
                loop->KeyVarName = std::move(loop->ValueVarName);
                loop->ValueVarName = TryParseIdentifier();
                MUST_PARSE( !loop->ValueVarName.empty(), ERROR_MESSAGE_EXPECTED_IDENTIFIER );
            }
            MUST_PARSE( TryParseSymbol(Symbol::Colon), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON );
            MUST_PARSE( loop->RangeExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
            MUST_PARSE( TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON );
            MUST_PARSE( loop->Body = TryParseStatement(), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON );
            return loop;
        }
        // Loop: 'for' '(' Expr17? ';' Expr17? ';' Expr17? ')' Statement
        else
        {
            auto loop = make_unique<AST::ForLoop>(place);
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
        auto stmt = std::make_unique<AST::ReturnStatement>(place);
        stmt->ReturnedValue = TryParseExpr17();
        MUST_PARSE( TryParseSymbol(Symbol::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON );
        return stmt;
    }

    // 'switch' '(' Expr17 ')' '{' SwitchItem+ '}'
    if(TryParseSymbol(Symbol::Switch))
    {
        auto stmt = std::make_unique<AST::SwitchStatement>(place);
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
                        if(stmt->ItemValues[i]->Val.IsEqual(stmt->ItemValues[j]->Val))
                            throw ParsingError(stmt->ItemValues[j]->GetPlace(), ERROR_MESSAGE_EXPECTED_UNIQUE_CONSTANT);
                    }
                }
            }
        }
        return stmt;
    }

    // 'throw' Expr17 ';'
    if(TryParseSymbol(Symbol::Throw))
    {
        auto stmt = std::make_unique<AST::ThrowStatement>(place);
        MUST_PARSE(stmt->ThrownExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
        MUST_PARSE(TryParseSymbol(Symbol::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON);
        return stmt;
    }

    // 'try' Statement ( ( 'catch' '(' TOKEN_IDENTIFIER ')' Statement [ 'finally' Statement ] ) | ( 'finally' Statement ) )
    if(TryParseSymbol(Symbol::Try))
    {
        auto stmt = std::make_unique<AST::TryStatement>(place);
        MUST_PARSE(stmt->TryBlock = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
        if(TryParseSymbol(Symbol::Finally))
            MUST_PARSE(stmt->FinallyBlock = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
        else
        {
            MUST_PARSE(TryParseSymbol(Symbol::Catch), "Expected 'catch' or 'finally'.");
            MUST_PARSE(TryParseSymbol(Symbol::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
            MUST_PARSE(!(stmt->ExceptionVarName = TryParseIdentifier()).empty(), ERROR_MESSAGE_EXPECTED_IDENTIFIER);
            MUST_PARSE(TryParseSymbol(Symbol::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
            MUST_PARSE(stmt->CatchBlock = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            if(TryParseSymbol(Symbol::Finally))
                MUST_PARSE(stmt->FinallyBlock = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
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

    // Syntactic sugar for functions:
    // 'function' IdentifierValue '(' [ TOKEN_IDENTIFIER ( ',' TOKE_IDENTIFIER )* ] ')' '{' Block '}'
    if(auto fnSyntacticSugar = TryParseFunctionSyntacticSugar(); fnSyntacticSugar.second)
    {
        auto identifierExpr = std::make_unique<AST::Identifier>(place, AST::IdentifierScope::None, std::move(fnSyntacticSugar.first));
        auto assignmentOp = std::make_unique<AST::BinaryOperator>(place, AST::BinaryOperatorType::Assignment);
        assignmentOp->Operands[0] = std::move(identifierExpr);
        assignmentOp->Operands[1] = std::move(fnSyntacticSugar.second);
        return assignmentOp;
    }

    if(auto cls = TryParseClassSyntacticSugar(); cls)
        return cls;
    
    return {};
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
        ++m_TokenIndex;
        return make_unique<AST::ConstantValue>(t.Place, string(t.String));
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
    return {};
}

unique_ptr<AST::Identifier> Parser::TryParseIdentifierValue()
{
    const Token& t = m_Tokens[m_TokenIndex];
    if(t.Symbol == Symbol::Local || t.Symbol == Symbol::Global)
    {
        ++m_TokenIndex;
        MUST_PARSE( TryParseSymbol(Symbol::Dot), ERROR_MESSAGE_EXPECTED_SYMBOL_DOT );
        MUST_PARSE( m_TokenIndex < m_Tokens.size(), ERROR_MESSAGE_EXPECTED_IDENTIFIER );
        const Token& tIdentifier = m_Tokens[m_TokenIndex++];
        MUST_PARSE( tIdentifier.Symbol == Symbol::Identifier, ERROR_MESSAGE_EXPECTED_IDENTIFIER );
        AST::IdentifierScope identifierScope = AST::IdentifierScope::Count;
        switch(t.Symbol)
        {
        case Symbol::Local: identifierScope = AST::IdentifierScope::Local; break;
        case Symbol::Global: identifierScope = AST::IdentifierScope::Global; break;
        }
        return make_unique<AST::Identifier>(t.Place, identifierScope, string(tIdentifier.String));
    }
    if(t.Symbol == Symbol::Identifier)
    {
        ++m_TokenIndex;
        return make_unique<AST::Identifier>(t.Place, AST::IdentifierScope::None, string(t.String));
    }
    return {};
}

unique_ptr<AST::ConstantExpression> Parser::TryParseConstantExpr()
{
    if(auto r = TryParseConstantValue())
        return r;
    if(auto r = TryParseIdentifierValue())
        return r;
    const PlaceInCode place = m_Tokens[m_TokenIndex].Place;
    if(TryParseSymbol(Symbol::This))
        return make_unique<AST::ThisExpression>(place);
    return {};
}

std::pair<string, unique_ptr<AST::FunctionDefinition>> Parser::TryParseFunctionSyntacticSugar()
{
    if(PeekSymbols({Symbol::Function, Symbol::Identifier}))
    {
        ++m_TokenIndex;
        std::pair<string, unique_ptr<AST::FunctionDefinition>> result;
        result.first = m_Tokens[m_TokenIndex++].String;
        result.second = make_unique<AST::FunctionDefinition>(GetCurrentTokenPlace());
        ParseFunctionDefinition(*result.second);
        return result;
    }
    return std::make_pair(string{}, unique_ptr<AST::FunctionDefinition>{});
}

unique_ptr<AST::Expression> Parser::TryParseObjectMember(string& outMemberName)
{
    if(PeekSymbols({Symbol::String, Symbol::Colon}) ||
        PeekSymbols({Symbol::Identifier, Symbol::Colon}))
    {
        outMemberName = m_Tokens[m_TokenIndex].String;
        m_TokenIndex += 2;
        return TryParseExpr16();
    }
    return {};
}

unique_ptr<AST::Expression> Parser::TryParseClassSyntacticSugar()
{
    const PlaceInCode beginPlace = GetCurrentTokenPlace();
    if(TryParseSymbol(Symbol::Class))
    {
        string className = TryParseIdentifier();
        MUST_PARSE( !className.empty(), ERROR_MESSAGE_EXPECTED_IDENTIFIER );
        auto assignmentOp = std::make_unique<AST::BinaryOperator>(beginPlace, AST::BinaryOperatorType::Assignment);
        assignmentOp->Operands[0] = std::make_unique<AST::Identifier>(beginPlace, AST::IdentifierScope::None, std::move(className));
        unique_ptr<AST::Expression> baseExpr;
        if(TryParseSymbol(Symbol::Colon))
        {
            baseExpr = TryParseExpr16();
            MUST_PARSE( baseExpr, ERROR_MESSAGE_EXPECTED_EXPRESSION );
        }
        auto objExpr = TryParseObject();
        objExpr->BaseExpression = std::move(baseExpr);
        assignmentOp->Operands[1] = std::move(objExpr);
        MUST_PARSE( assignmentOp->Operands[1], ERROR_MESSAGE_EXPECTED_OBJECT );
        return assignmentOp;
    }
    return unique_ptr<AST::ObjectExpression>();
}

unique_ptr<AST::ObjectExpression> Parser::TryParseObject()
{
    if(PeekSymbols({Symbol::CurlyBracketOpen, Symbol::CurlyBracketClose}) || // { }
        PeekSymbols({Symbol::CurlyBracketOpen, Symbol::String, Symbol::Colon}) || // { 'key' :
        PeekSymbols({Symbol::CurlyBracketOpen, Symbol::Identifier, Symbol::Colon})) // { key :
    {
        auto objExpr = make_unique<AST::ObjectExpression>(GetCurrentTokenPlace());
        TryParseSymbol(Symbol::CurlyBracketOpen);
        if(!TryParseSymbol(Symbol::CurlyBracketClose))
        {
            string memberName;
            unique_ptr<AST::Expression> memberValue;
            MUST_PARSE( memberValue = TryParseObjectMember(memberName), ERROR_MESSAGE_EXPECTED_OBJECT_MEMBER );
            MUST_PARSE( objExpr->Items.insert(std::make_pair(std::move(memberName), std::move(memberValue))).second, ERROR_MESSAGE_REPEATING_KEY_IN_OBJECT );
            if(!TryParseSymbol(Symbol::CurlyBracketClose))
            {
                while(TryParseSymbol(Symbol::Comma))
                {
                    if(TryParseSymbol(Symbol::CurlyBracketClose))
                        return objExpr;
                    MUST_PARSE( memberValue = TryParseObjectMember(memberName), ERROR_MESSAGE_EXPECTED_OBJECT_MEMBER );
                    MUST_PARSE( objExpr->Items.insert(std::make_pair(std::move(memberName), std::move(memberValue))).second, ERROR_MESSAGE_REPEATING_KEY_IN_OBJECT );
                }
                MUST_PARSE( TryParseSymbol(Symbol::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE );
            }
        }
        return objExpr;
    }
    return {};
}

unique_ptr<AST::ArrayExpression> Parser::TryParseArray()
{
    if(TryParseSymbol(Symbol::SquareBracketOpen))
    {
        auto arrExpr = make_unique<AST::ArrayExpression>(GetCurrentTokenPlace());
        if(!TryParseSymbol(Symbol::SquareBracketClose))
        {
            unique_ptr<AST::Expression> itemValue;
            MUST_PARSE( itemValue = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
            arrExpr->Items.push_back((std::move(itemValue)));
            if(!TryParseSymbol(Symbol::SquareBracketClose))
            {
                while(TryParseSymbol(Symbol::Comma))
                {
                    if(TryParseSymbol(Symbol::SquareBracketClose))
                        return arrExpr;
                    MUST_PARSE( itemValue = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
                    arrExpr->Items.push_back((std::move(itemValue)));
                }
                MUST_PARSE( TryParseSymbol(Symbol::SquareBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_SQUARE_BRACKET_CLOSE );
            }
        }
        return arrExpr;
    }
    return {};
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

    if(auto objExpr = TryParseObject())
        return objExpr;
    if(auto arrExpr = TryParseArray())
        return arrExpr;

    // 'function' '(' [ TOKEN_IDENTIFIER ( ',' TOKE_IDENTIFIER )* ] ')' '{' Block '}'
    if(m_Tokens[m_TokenIndex].Symbol == Symbol::Function && m_Tokens[m_TokenIndex + 1].Symbol == Symbol::RoundBracketOpen)
    {
        ++m_TokenIndex;
        auto func = std::make_unique<AST::FunctionDefinition>(place);
        ParseFunctionDefinition(*func);
        return func;
    }

    // Constant
    unique_ptr<AST::ConstantExpression> constant = TryParseConstantExpr();
    if(constant)
        return constant;

    return {};
}

unique_ptr<AST::Expression> Parser::TryParseExpr2()
{
    unique_ptr<AST::Expression> expr = TryParseExpr0();
    if(!expr)
        return {};
    for(;;)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        // Postincrementation: Expr0 '++'
        if(TryParseSymbol(Symbol::DoublePlus))
        {
            auto op = std::make_unique<AST::UnaryOperator>(place, AST::UnaryOperatorType::Postincrementation);
            op->Operand = std::move(expr);
            expr = std::move(op);
        }
        // Postdecrementation: Expr0 '--'
        else if(TryParseSymbol(Symbol::DoubleDash))
        {
            auto op = std::make_unique<AST::UnaryOperator>(place, AST::UnaryOperatorType::Postdecrementation);
            op->Operand = std::move(expr);
            expr = std::move(op);
        }
        // Call: Expr0 '(' [ Expr16 ( ',' Expr16 )* ')'
        else if(TryParseSymbol(Symbol::RoundBracketOpen))
        {
            auto op = make_unique<AST::CallOperator>(place);
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
            expr = std::move(op);
        }
        // Indexing: Expr0 '[' Expr17 ']'
        else if(TryParseSymbol(Symbol::SquareBracketOpen))
        {
            auto op = std::make_unique<AST::BinaryOperator>(place, AST::BinaryOperatorType::Indexing);
            op->Operands[0] = std::move(expr);
            MUST_PARSE( op->Operands[1] = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION );
            MUST_PARSE( TryParseSymbol(Symbol::SquareBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_SQUARE_BRACKET_CLOSE );
            expr = std::move(op);
        }
        // Member access: Expr2 '.' TOKEN_IDENTIFIER
        else if(TryParseSymbol(Symbol::Dot))
        {
            auto op = std::make_unique<AST::MemberAccessOperator>(place);
            op->Operand = std::move(expr);
            auto identifier = TryParseIdentifierValue();
            MUST_PARSE( identifier && identifier->Scope == AST::IdentifierScope::None, ERROR_MESSAGE_EXPECTED_IDENTIFIER );
            op->MemberName = std::move(identifier->S);
            expr = std::move(op);
        }
        else
            break;
    }
    return expr;
}

unique_ptr<AST::Expression> Parser::TryParseExpr3()
{
    const PlaceInCode place = GetCurrentTokenPlace();
#define PARSE_UNARY_OPERATOR(symbol, unaryOperatorType) \
    if(TryParseSymbol(symbol)) \
    { \
        auto op = make_unique<AST::UnaryOperator>(place, (unaryOperatorType)); \
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
        auto op = make_unique<AST::BinaryOperator>(place, (binaryOperatorType)); \
        op->Operands[0] = std::move(expr); \
        MUST_PARSE( op->Operands[1] = (exprParseFunc)(), ERROR_MESSAGE_EXPECTED_EXPRESSION ); \
        expr = std::move(op); \
    }

unique_ptr<AST::Expression> Parser::TryParseExpr5()
{
    unique_ptr<AST::Expression> expr = TryParseExpr3();
    if(!expr)
        return {};
 
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
        return {};

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
        return {};

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
        return {};

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
        return {};

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
        return {};

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
        return {};

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
        return {};

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
        return {};
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
        return {};
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
        return {};

    // Ternary operator: Expr15 '?' Expr16 ':' Expr16
    if(TryParseSymbol(Symbol::QuestionMark))
    {
        auto op = make_unique<AST::TernaryOperator>(GetCurrentTokenPlace());
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
        auto op = make_unique<AST::BinaryOperator>(GetCurrentTokenPlace(), (binaryOperatorType)); \
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
        return {};
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
    if(m_TokenIndex < m_Tokens.size() && m_Tokens[m_TokenIndex].Symbol == symbol)
    {
        ++m_TokenIndex;
        return true;
    }
    return false;
}

string Parser::TryParseIdentifier()
{
    if(m_TokenIndex < m_Tokens.size() && m_Tokens[m_TokenIndex].Symbol == Symbol::Identifier)
        return m_Tokens[m_TokenIndex++].String;
    return {};
}

////////////////////////////////////////////////////////////////////////////////
// class EnvironmentPimpl implementation

Value EnvironmentPimpl::Execute(const string_view& code)
{
    m_Script.Statements.clear();

    {
        Tokenizer tokenizer{code};
        Parser parser{tokenizer};
        parser.ParseScript(m_Script);
    }

    try
    {
        AST::ExecuteContext executeContext{*this, m_GlobalScope};
        m_Script.Execute(executeContext);
    }
    catch(ReturnException& returnEx)
    {
        return std::move(returnEx.ThrownValue);
    }
    return {};
}

////////////////////////////////////////////////////////////////////////////////
// class Environment

Environment::Environment() : pimpl{new EnvironmentPimpl{}} { }
Environment::~Environment() { delete pimpl; }
Value Environment::Execute(const string_view& code) { return pimpl->Execute(code); }
const std::string& Environment::GetOutput() const { return pimpl->GetOutput(); }

} // namespace MinScriptLang

#endif // MIN_SCRIPT_LANG_IMPLEMENTATION
