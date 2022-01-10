
#pragma once
/*
MinScriptLang - minimalistic scripting language

Version: 0.0.1-development, 2021-11
Homepage: https://github.com/sawickiap/MinScriptLang
Author: Adam Sawicki, adam__REMOVE_THIS__@asawicki.info, https://asawicki.info

================================================================================
MIT License

Copyright (c) 2019-2021 Adam Sawicki

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <iostream>
#include <string>
#include <string_view>
#include <exception>
#include <memory>
#include <vector>
#include <unordered_map>
#include <variant>


    #include <map>
    #include <algorithm>
    #include <initializer_list>


#include <cstdarg>
#include <cstdint>
#include <cassert>
    #include <cstdlib>
    #include <cstring>
    #include <cmath>
    #include <ctype.h>


    // F***k Windows.h macros!
    #undef min
    #undef max


#define MINSL_EXECUTION_CHECK(condition, place, errorMessage) \
    do                                                        \
    {                                                         \
        if(!(condition))                                      \
            throw ExecutionError((place), (errorMessage));    \
    } while(false)
#define MINSL_EXECUTION_FAIL(place, errorMessage)      \
    do                                                 \
    {                                                  \
        throw ExecutionError((place), (errorMessage)); \
    } while(false)

// Convenience macros for loading function arguments
// They require to have following available: env, place, args.
#define MINSL_LOAD_ARG_BEGIN(functionNameStr)           \
    const char* minsl_functionName = (functionNameStr); \
    size_t minsl_argIndex = 0;                          \
    size_t minsl_argCount = (args).size();              \
    Value::Type minsl_argType;
#define MINSL_LOAD_ARG_NUMBER(dstVarName)                                                                          \
    MINSL_EXECUTION_CHECK(minsl_argIndex < minsl_argCount, place,                                                  \
                          Format("Function %s received too few arguments. Number expected as argument %zu.",       \
                                 minsl_functionName, minsl_argIndex));                                             \
    minsl_argType = args[minsl_argIndex].GetType();                                                                \
    MINSL_EXECUTION_CHECK(minsl_argType == Value::Type::Number, place,                                               \
                          Format("Function %s received incorrect argument %zu. Expected: Number, actual: %.*s.",   \
                                 minsl_functionName, minsl_argIndex, (int)env.GetTypeName(minsl_argType).length(), \
                                 env.GetTypeName(minsl_argType).data()));                                          \
    double dstVarName = args[minsl_argIndex++].GetNumber();
#define MINSL_LOAD_ARG_STRING(dstVarName)                                                                          \
    MINSL_EXECUTION_CHECK(minsl_argIndex < minsl_argCount, place,                                                  \
                          Format("Function %s received too few arguments. String expected as argument %zu.",       \
                                 minsl_functionName, minsl_argIndex));                                             \
    minsl_argType = args[minsl_argIndex].GetType();                                                                \
    MINSL_EXECUTION_CHECK(minsl_argType == Value::Type::String, place,                                               \
                          Format("Function %s received incorrect argument %zu. Expected: String, actual: %.*s.",   \
                                 minsl_functionName, minsl_argIndex, (int)env.GetTypeName(minsl_argType).length(), \
                                 env.GetTypeName(minsl_argType).data()));                                          \
    std::string dstVarName = std::move(args[minsl_argIndex++].GetString());
#define MINSL_LOAD_ARG_END()                 \
    MINSL_EXECUTION_CHECK(                   \
    minsl_argIndex == minsl_argCount, place, \
    Format("Function %s requires %zu arguments, %zu provided.", minsl_functionName, minsl_argIndex, minsl_argCount));

#define MINSL_LOAD_ARGS_0(functionNameStr) \
    MINSL_LOAD_ARG_BEGIN(functionNameStr); \
    MINSL_LOAD_ARG_END();
#define MINSL_LOAD_ARGS_1_NUMBER(functionNameStr, dstVarName) \
    MINSL_LOAD_ARG_BEGIN(functionNameStr);                    \
    MINSL_LOAD_ARG_NUMBER(dstVarName);                        \
    MINSL_LOAD_ARG_END();
#define MINSL_LOAD_ARGS_1_STRING(functionNameStr, dstVarName) \
    MINSL_LOAD_ARG_BEGIN(functionNameStr);                    \
    MINSL_LOAD_ARG_STRING(dstVarName);                        \
    MINSL_LOAD_ARG_END();
#define MINSL_LOAD_ARGS_2_NUMBERS(functionNameStr, dstVarName1, dstVarName2) \
    MINSL_LOAD_ARG_BEGIN(functionNameStr);                                   \
    MINSL_LOAD_ARG_NUMBER(dstVarName1);                                      \
    MINSL_LOAD_ARG_NUMBER(dstVarName2);                                      \
    MINSL_LOAD_ARG_END();

namespace MSL
{
    struct PlaceInCode
    {
        uint32_t textindex;
        uint32_t textrow;
        uint32_t textcolumn;
    };

    class Error : public std::exception
    {
        private:
            const PlaceInCode m_place;
            mutable std::string m_what;

        public:
            Error(const PlaceInCode& place) : m_place{ place }
            {
            }
            const PlaceInCode& GetPlace() const
            {
                return m_place;
            }
            virtual const char* podMessage() const;
            virtual std::string_view getMessage() const = 0;
    };

    class ParsingError : public Error
    {
        private:
            const std::string_view m_message;// Externally owned


        public:
            ParsingError(const PlaceInCode& place, const std::string_view& message) : Error{ place }, m_message{ message }
            {
            }
            virtual std::string_view getMessage() const override
            {
                return m_message;
            }

    };

    class ExecutionError : public Error
    {
        private:
            const std::string m_message;

        public:
            ExecutionError(const PlaceInCode& place, const std::string_view& message) : Error{ place }, m_message{ message }
            {
            }
            virtual std::string_view getMessage() const override
            {
                return m_message;
            }
    };

    namespace AST
    {
        class /**/FunctionDefinition;
    }
    class /**/Value;
    class /**/Object;
    class /**/Array;
    class /**/Environment;
    enum class /**/SystemFunction;

    using HostFunction = Value(Environment& env, const PlaceInCode& place, std::vector<Value>&& args);


    class Value
    {
        public:
            enum class Type
            {
                Null,
                Number,
                String,
                Function,
                SystemFunction,
                HostFunction,
                Object,
                Array,
                Type,
                Count
            };

        private:
            using VariantType = std::variant<
                // Value::Type::Null
                std::monostate,
                // Value::Type::Number
                double,
                // Value::Type::String
                std::string,
                // Value::Type::Function
                const AST::FunctionDefinition*,
                // Value::Type::SystemFunction
                SystemFunction,
                // Value::Type::HostFunction
                HostFunction*,
                // Value::Type::Object
                std::shared_ptr<Object>,
                // Value::Type::Array
                std::shared_ptr<Array>,
                // Value::Type::Type
                Type
            >;

        private:
            Type m_type = Type::Null;
            VariantType m_Variant;

        public:
            Value()
            {
            }
            explicit Value(double number) : m_type(Type::Number), m_Variant(number)
            {
            }
            explicit Value(std::string&& str) : m_type(Type::String), m_Variant(std::move(str))
            {
            }
            explicit Value(const AST::FunctionDefinition* func) : m_type{ Type::Function }, m_Variant{ func }
            {
            }
            explicit Value(SystemFunction func) : m_type{ Type::SystemFunction }, m_Variant{ func }
            {
            }
            explicit Value(HostFunction func) : m_type{ Type::HostFunction }, m_Variant{ func }
            {
                assert(func);
            }
            explicit Value(std::shared_ptr<Object>&& obj) : m_type{ Type::Object }, m_Variant(obj)
            {
            }
            explicit Value(std::shared_ptr<Array>&& arr) : m_type{ Type::Array }, m_Variant(arr)
            {
            }
            explicit Value(Type typeVal) : m_type{ Type::Type }, m_Variant(typeVal)
            {
            }

            Type GetType() const
            {
                return m_type;
            }
            double GetNumber() const
            {
                assert(m_type == Type::Number);
                return std::get<double>(m_Variant);
            }
            std::string& GetString()
            {
                assert(m_type == Type::String);
                return std::get<std::string>(m_Variant);
            }
            const std::string& GetString() const
            {
                assert(m_type == Type::String);
                return std::get<std::string>(m_Variant);
            }
            const AST::FunctionDefinition* GetFunction() const
            {
                assert(m_type == Type::Function && std::get<const AST::FunctionDefinition*>(m_Variant));
                return std::get<const AST::FunctionDefinition*>(m_Variant);
            }
            SystemFunction GetSystemFunction() const
            {
                assert(m_type == Type::SystemFunction);
                return std::get<SystemFunction>(m_Variant);
            }
            HostFunction* GetHostFunction() const
            {
                assert(m_type == Type::HostFunction);
                return std::get<HostFunction*>(m_Variant);
            }
            Object* GetObject_() const// Using underscore because the %^#& WinAPI defines GetObject as a macro.
            {
                assert(m_type == Type::Object && std::get<std::shared_ptr<Object>>(m_Variant));
                return std::get<std::shared_ptr<Object>>(m_Variant).get();
            }
            std::shared_ptr<Object> GetObjectPtr() const
            {
                assert(m_type == Type::Object && std::get<std::shared_ptr<Object>>(m_Variant));
                return std::get<std::shared_ptr<Object>>(m_Variant);
            }
            Array* GetArray() const
            {
                assert(m_type == Type::Array && std::get<std::shared_ptr<Array>>(m_Variant));
                return std::get<std::shared_ptr<Array>>(m_Variant).get();
            }
            std::shared_ptr<Array> GetArrayPtr() const
            {
                assert(m_type == Type::Array && std::get<std::shared_ptr<Array>>(m_Variant));
                return std::get<std::shared_ptr<Array>>(m_Variant);
            }
            Type GetTypeValue() const
            {
                assert(m_type == Type::Type);
                return std::get<Type>(m_Variant);
            }

            bool IsEqual(const Value& rhs) const;
            bool IsTrue() const;

            void ChangeNumber(double number)
            {
                assert(m_type == Type::Number);
                std::get<double>(m_Variant) = number;
            }


    };

    class Object
    {
        public:
            using MapType = std::unordered_map<std::string, Value>;

        public:
            MapType m_items;

        public:
            size_t GetCount() const
            {
                return m_items.size();
            }
            bool HasKey(const std::string& key) const
            {
                return m_items.find(key) != m_items.end();
            }
            Value& GetOrCreateValue(const std::string& key)
            {
                return m_items[key];
            };// Creates new null value if doesn't exist.

            Value* TryGetValue(const std::string& key);// Returns null if doesn't exist.

            const Value* TryGetValue(const std::string& key) const;// Returns null if doesn't exist.

            bool Remove(const std::string& key);// Returns true if has been found and removed.
    };

    class Array
    {
        public:
            std::vector<Value> m_items;
    };


    std::string VFormat(const char* format, va_list argList);
    std::string Format(const char* format, ...);

    class EnvironmentPimpl;
    class Environment
    {
        private:
            EnvironmentPimpl* m_implenv;

        public:
            Object GlobalScope;
            void* UserData = nullptr;

        public:
            Environment();

            ~Environment();

            Value Execute(const std::string_view& code);

            std::string_view GetTypeName(Value::Type type) const;

            void Print(const std::string_view& s);
    };

    /**
    * ======================================
    * ========= implementation =============
    * ======================================
    */

    ////////////////////////////////////////////////////////////////////////////////
    // Basic facilities

    // I would like it to be higher, but above that, even at 128, it crashes with
    // native "stack overflow" in Debug configuration.
    static const size_t LOCAL_SCOPE_STACK_MAX_SIZE = 100;

    static constexpr std::string_view ERROR_MESSAGE_PARSING_ERROR = "Parsing error.";
    static constexpr std::string_view ERROR_MESSAGE_INVALID_NUMBER = "Invalid number.";
    static constexpr std::string_view ERROR_MESSAGE_INVALID_STRING = "Invalid string.";
    static constexpr std::string_view ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE = "Invalid escape sequence in a string.";
    static constexpr std::string_view ERROR_MESSAGE_INVALID_TYPE = "Invalid type.";
    static constexpr std::string_view ERROR_MESSAGE_INVALID_MEMBER = "Invalid member.";
    static constexpr std::string_view ERROR_MESSAGE_INVALID_INDEX = "Invalid index.";
    static constexpr std::string_view ERROR_MESSAGE_INVALID_LVALUE = "Invalid l-value.";
    static constexpr std::string_view ERROR_MESSAGE_INVALID_FUNCTION = "Invalid function.";
    static constexpr std::string_view ERROR_MESSAGE_INVALID_NUMBER_OF_ARGUMENTS = "Invalid number of arguments.";
    static constexpr std::string_view ERROR_MESSAGE_UNRECOGNIZED_TOKEN = "Unrecognized token.";
    static constexpr std::string_view ERROR_MESSAGE_UNEXPECTED_END_OF_FILE_IN_MULTILINE_COMMENT
    = "Unexpected end of file inside multiline comment.";
    static constexpr std::string_view ERROR_MESSAGE_UNEXPECTED_END_OF_FILE_IN_STRING = "Unexpected end of file inside string.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_EXPRESSION = "Expected expression.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_STATEMENT = "Expected statement.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_CONSTANT_VALUE = "Expected constant value.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_IDENTIFIER = "Expected identifier.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_LVALUE = "Expected l-value.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_NUMBER = "Expected number.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_STRING = "Expected string.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_OBJECT = "Expected object.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_ARRAY = "Expected array.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_OBJECT_MEMBER = "Expected object member.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_SINGLE_CHARACTER_STRING = "Expected single character string.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_SYMBOL = "Expected symbol.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_SYMBOL_COLON = "Expected symbol ':'.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON = "Expected symbol ';'.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN = "Expected symbol '('.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE = "Expected symbol ')'.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_OPEN = "Expected symbol '{'.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE = "Expected symbol '}'.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_SYMBOL_SQUARE_BRACKET_CLOSE = "Expected symbol ']'.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_SYMBOL_DOT = "Expected symbol '.'.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_SYMBOL_WHILE = "Expected 'while'.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_UNIQUE_CONSTANT = "Expected unique constant.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_1_ARGUMENT = "Expected 1 argument.";
    static constexpr std::string_view ERROR_MESSAGE_EXPECTED_2_ARGUMENTS = "Expected 2 arguments.";
    static constexpr std::string_view ERROR_MESSAGE_VARIABLE_DOESNT_EXIST = "Variable doesn't exist.";
    static constexpr std::string_view ERROR_MESSAGE_OBJECT_MEMBER_DOESNT_EXIST = "Object member doesn't exist.";
    static constexpr std::string_view ERROR_MESSAGE_NOT_IMPLEMENTED = "Not implemented.";
    static constexpr std::string_view ERROR_MESSAGE_BREAK_WITHOUT_LOOP = "Break without a loop.";
    static constexpr std::string_view ERROR_MESSAGE_CONTINUE_WITHOUT_LOOP = "Continue without a loop.";
    static constexpr std::string_view ERROR_MESSAGE_INCOMPATIBLE_TYPES = "Incompatible types.";
    static constexpr std::string_view ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS = "Index out of bounds.";
    static constexpr std::string_view ERROR_MESSAGE_PARAMETER_NAMES_MUST_BE_UNIQUE = "Parameter naems must be unique.";
    static constexpr std::string_view ERROR_MESSAGE_NO_LOCAL_SCOPE = "There is no local scope here.";
    static constexpr std::string_view ERROR_MESSAGE_NO_THIS = "There is no 'this' here.";
    static constexpr std::string_view ERROR_MESSAGE_REPEATING_KEY_IN_OBJECT = "Repeating key in object.";
    static constexpr std::string_view ERROR_MESSAGE_STACK_OVERFLOW = "Stack overflow.";
    static constexpr std::string_view ERROR_MESSAGE_BASE_MUST_BE_OBJECT = "Base must be object.";

    static constexpr std::string_view VALUE_TYPE_NAMES[]
    = { "Null", "Number", "String", "Function", "Function", "Function", "Object", "Array", "Type" };

    static constexpr std::string_view SYMBOL_STR[] = {
        // Token types
        "", "", "", "", "",
        // Symbols
        ",", "?", ":", ";", "(", ")", "[", "]", "{", "}", "*", "/", "%", "+", "-", "=", "!", "~", "<", ">", "&", "^", "|", ".",
        // Multiple character symbols
        "++", "--", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
        // Keywords
        "null", "false", "true", "if", "else", "while", "do", "for", "break", "continue", "switch", "case", "default",
        "function", "return", "local", "this", "global", "class", "throw", "try", "catch", "finally"
    };

    struct Token
    {
        enum class Type
        {
            // Token types
            None,
            Identifier,
            Number,
            String,
            End,
            // Symbols
            Comma,// ,
            QuestionMark,// ?
            Colon,// :
            Semicolon,// ;
            RoundBracketOpen,// (
            RoundBracketClose,// )
            SquareBracketOpen,// [
            SquareBracketClose,// ]
            CurlyBracketOpen,// {
            CurlyBracketClose,// }
            Asterisk,// *
            Slash,// /
            Percent,// %
            Plus,// +
            Dash,// -
            Equals,// =
            ExclamationMark,// !
            Tilde,// ~
            Less,// <
            Greater,// >
            Amperstand,// &
            Caret,// ^
            Pipe,// |
            Dot,// .
            // Multiple character symbols
            DoublePlus,// ++
            DoubleDash,// --
            PlusEquals,// +=
            DashEquals,// -=
            AsteriskEquals,// *=
            SlashEquals,// /=
            PercentEquals,// %=
            DoubleLessEquals,// <<=
            DoubleGreaterEquals,// >>=
            AmperstandEquals,// &=
            CaretEquals,// ^=
            PipeEquals,// |=
            DoubleLess,// <<
            DoubleGreater,// >>
            LessEquals,// <=
            GreaterEquals,// >=
            DoubleEquals,// ==
            ExclamationEquals,// !=
            DoubleAmperstand,// &&
            DoublePipe,// ||
            // Keywords
            Null,
            False,
            True,
            If,
            Else,
            While,
            Do,
            For,
            Break,
            Continue,
            Switch,
            Case,
            Default,
            Function,
            Return,
            Local,
            This,
            Global,
            Class,
            Throw,
            Try,
            Catch,
            Finally,
            Count
        };

        PlaceInCode m_place;
        Type m_symtype;
        // Only when m_symtype == Type::Number
        double m_numval;
        // Only when m_symtype == Type::Identifier or String
        std::string m_stringval;
    };

    static inline bool IsDecimalNumber(char ch)
    {
        return ch >= '0' && ch <= '9';
    }
    static inline bool IsHexadecimalNumber(char ch)
    {
        return ch >= '0' && ch <= '9' || ch >= 'A' && ch <= 'F' || ch >= 'a' && ch <= 'f';
    }
    static inline bool IsAlpha(char ch)
    {
        return ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch == '_';
    }
    static inline bool IsAlphaNumeric(char ch)
    {
        return ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9' || ch == '_';
    }

    static const char* GetDebugPrintIndent(uint32_t indentLevel)
    {
        static const char* silly = "                                                                                                                                                                                                                                                                ";
        return silly + (256 - std::min<uint32_t>(indentLevel, 128) * 2);
    }

    std::string VFormat(const char* format, va_list argList)
    {
        size_t dstlen;
        size_t rtlen;
        dstlen = 1024*1024;
        std::vector<char> buf(dstlen + 1);
        rtlen = vsnprintf(&buf[0], dstlen + 1, format, argList);
        return std::string{ &buf[0], rtlen };
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
        if(!std::isfinite(number) || number < 0.f)
            return false;
        outIndex = (size_t)number;
        return (double)outIndex == number;
    }

    struct BreakException
    {
    };

    struct ContinueException
    {
    };

    ////////////////////////////////////////////////////////////////////////////////
    // class Value definition

    bool Value::IsEqual(const Value& rhs) const
    {
        if(m_type != rhs.m_type)
            return false;
        switch(m_type)
        {
            case Value::Type::Null:
                return true;
            case Value::Type::Number:
                return std::get<double>(m_Variant) == std::get<double>(rhs.m_Variant);
            case Value::Type::String:
                return std::get<std::string>(m_Variant) == std::get<std::string>(rhs.m_Variant);
            case Value::Type::Function:
                return std::get<const AST::FunctionDefinition*>(m_Variant)
                       == std::get<const AST::FunctionDefinition*>(rhs.m_Variant);
            case Value::Type::SystemFunction:
                return std::get<SystemFunction>(m_Variant) == std::get<SystemFunction>(rhs.m_Variant);
            case Value::Type::HostFunction:
                return std::get<HostFunction*>(m_Variant) == std::get<HostFunction*>(rhs.m_Variant);
            case Value::Type::Object:
                return std::get<std::shared_ptr<Object>>(m_Variant).get()
                       == std::get<std::shared_ptr<Object>>(rhs.m_Variant).get();
            case Value::Type::Array:
                return std::get<std::shared_ptr<Array>>(m_Variant).get()
                       == std::get<std::shared_ptr<Array>>(rhs.m_Variant).get();
            case Value::Type::Type:
                return std::get<Value::Type>(m_Variant) == std::get<Value::Type>(rhs.m_Variant);
            default:
                assert(0);
                return false;
        }
    }

    bool Value::IsTrue() const
    {
        switch(m_type)
        {
            case Value::Type::Null:
                return false;
            case Value::Type::Number:
                return std::get<double>(m_Variant) != 0.f;
            case Value::Type::String:
                return !std::get<std::string>(m_Variant).empty();
            case Value::Type::Function:
                return true;
            case Value::Type::SystemFunction:
                return true;
            case Value::Type::HostFunction:
                return true;
            case Value::Type::Object:
                return true;
            case Value::Type::Array:
                return true;
            case Value::Type::Type:
                return std::get<Value::Type>(m_Variant) != Value::Type::Null;
            default:
                assert(0);
                return false;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // CodeReader definition

    class CodeReader
    {
        private:
            const std::string_view m_code;
            PlaceInCode m_place;

        public:
            CodeReader(const std::string_view& code) : m_code{ code }, m_place{ 0, 1, 1 }
            {
            }

            bool IsEnd() const
            {
                return m_place.textindex >= m_code.length();
            }
            const PlaceInCode& GetCurrentPlace() const
            {
                return m_place;
            }
            const char* GetCurrentCode() const
            {
                return m_code.data() + m_place.textindex;
            }
            size_t GetCurrentLen() const
            {
                return m_code.length() - m_place.textindex;
            }
            char GetCurrentChar() const
            {
                return m_code[m_place.textindex];
            }
            bool Peek(char ch) const
            {
                return m_place.textindex < m_code.length() && m_code[m_place.textindex] == ch;
            }
            bool Peek(const char* s, size_t sLen) const
            {
                return m_place.textindex + sLen <= m_code.length() && memcmp(m_code.data() + m_place.textindex, s, sLen) == 0;
            }

            void MoveOneChar()
            {
                if(m_code[m_place.textindex++] == '\n')
                    ++m_place.textrow, m_place.textcolumn = 1;
                else
                    ++m_place.textcolumn;
            }
            void MoveChars(size_t n)
            {
                for(size_t i = 0; i < n; ++i)
                    MoveOneChar();
            }
    };

    ////////////////////////////////////////////////////////////////////////////////
    // Tokenizer definition

    class Tokenizer
    {
        public:
            static bool ParseCharHex(uint8_t& out, char ch);
            static bool ParseCharsHex(uint32_t& out, const std::string_view& chars);
            static bool AppendUtf8Char(std::string& inout, uint32_t charVal);

        private:
            CodeReader m_code;

        private:
            void SkipSpacesAndComments();
            bool ParseNumber(Token& out);
            bool ParseString(Token& out);

        public:
            Tokenizer(const std::string_view& code) : m_code{ code }
            {
            }
            void GetNextToken(Token& out);
    };

    ////////////////////////////////////////////////////////////////////////////////
    // Value definition

    enum class SystemFunction
    {
        TypeOfFunc,
        MinFunc,
        MaxFunc,
        StringResizeFunc,
        ArrayAddFunc,
        ArrayInsertFunc,
        ArrayRemoveFunc,
        CountFunc
    };
    static constexpr std::string_view SYSTEM_FUNCTION_NAMES[] =
    {
        "typeOf", "min", "max", "resize", "add", "insert", "remove",
    };
    

    struct ObjectMemberLValue
    {
        Object* objectval;
        std::string keyval;
    };

    struct StringCharacterLValue
    {
        std::string* stringval;
        size_t indexval;
    };

    struct ArrayItemLValue
    {
        Array* arrayval;
        size_t indexval;
    };

    struct LValue : public std::variant<ObjectMemberLValue, StringCharacterLValue, ArrayItemLValue>
    {
        Value* GetValueRef(const PlaceInCode& place) const;// Always returns non-null or throws exception.
        Value GetValue(const PlaceInCode& place) const;
    };

    struct ReturnException
    {
        const PlaceInCode place;
        Value thrownvalue;
    };

    ////////////////////////////////////////////////////////////////////////////////
    // namespace AST

    namespace AST
    {
        struct ThisType : public std::variant<std::monostate, std::shared_ptr<Object>, std::shared_ptr<Array>>
        {
            bool IsEmpty() const
            {
                return std::get_if<std::monostate>(this) != nullptr;
            }
            Object* GetObject_() const
            {
                const std::shared_ptr<Object>* objectPtr = std::get_if<std::shared_ptr<Object>>(this);
                return objectPtr ? objectPtr->get() : nullptr;
            }
            Array* GetArray() const
            {
                const std::shared_ptr<Array>* arrayPtr = std::get_if<std::shared_ptr<Array>>(this);
                return arrayPtr ? arrayPtr->get() : nullptr;
            }
            void Clear()
            {
                *this = ThisType{};
            }
        };

        class ExecuteContext
        {
            public:
                class LocalScopePush
                {
                    private:
                        ExecuteContext& m_context;

                    public:
                        LocalScopePush(ExecuteContext& ctx, Object* localScope, ThisType&& thisObj, const PlaceInCode& place)
                        : m_context{ ctx }
                        {
                            if(ctx.m_localscopes.size() == LOCAL_SCOPE_STACK_MAX_SIZE)
                                throw ExecutionError{ place, ERROR_MESSAGE_STACK_OVERFLOW };
                            ctx.m_localscopes.push_back(localScope);
                            ctx.m_thislist.push_back(std::move(thisObj));
                        }
                        ~LocalScopePush()
                        {
                            m_context.m_thislist.pop_back();
                            m_context.m_localscopes.pop_back();
                        }

                };

            private:
                std::vector<Object*> m_localscopes;
                std::vector<ThisType> m_thislist;

            public:
                EnvironmentPimpl& m_env;
                Object& GlobalScope;


                ExecuteContext(EnvironmentPimpl& env, Object& globalScope) : m_env{ env }, GlobalScope{ globalScope }
                {
                }
                bool IsLocal() const
                {
                    return !m_localscopes.empty();
                }
                Object* GetCurrentLocalScope()
                {
                    assert(IsLocal());
                    return m_localscopes.back();
                }
                const ThisType& GetThis()
                {
                    assert(IsLocal());
                    return m_thislist.back();
                }
                Object& GetInnermostScope() const
                {
                    return IsLocal() ? *m_localscopes.back() : GlobalScope;
                }
        };

        class Statement
        {
            protected:
                void Assign(const LValue& lhs, Value&& rhs) const;

            private:
                const PlaceInCode m_place;

            public:
                explicit Statement(const PlaceInCode& place) : m_place{ place }
                {
                }
                virtual ~Statement()
                {
                }
                const PlaceInCode& GetPlace() const
                {
                    return m_place;
                }
                virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const = 0;
                virtual void Execute(ExecuteContext& ctx) const = 0;


        };

        struct EmptyStatement : public Statement
        {
            explicit EmptyStatement(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void Execute(ExecuteContext& ctx) const
            {
            }
        };

        struct Expression;

        struct Condition : public Statement
        {
            std::unique_ptr<Expression> m_condexpr;
            std::unique_ptr<Statement> m_statements[2];// [0] executed if true, [1] executed if false, optional.
            explicit Condition(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void Execute(ExecuteContext& ctx) const;
        };



        struct WhileLoop : public Statement
        {
            enum Type { While, DoWhile };
        
            Type m_type;
            std::unique_ptr<Expression> m_condexpr;
            std::unique_ptr<Statement> Body;
            explicit WhileLoop(const PlaceInCode& place, Type type) : Statement{ place }, m_type{ type }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void Execute(ExecuteContext& ctx) const;
        };

        struct ForLoop : public Statement
        {
            std::unique_ptr<Expression> InitExpression;// Optional
            std::unique_ptr<Expression> m_condexpr;// Optional
            std::unique_ptr<Expression> IterationExpression;// Optional
            std::unique_ptr<Statement> Body;
            explicit ForLoop(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void Execute(ExecuteContext& ctx) const;
        };

        struct RangeBasedForLoop : public Statement
        {
            std::string KeyVarName;// Can be empty.
            std::string ValueVarName;// Cannot be empty.
            std::unique_ptr<Expression> RangeExpression;
            std::unique_ptr<Statement> Body;
            explicit RangeBasedForLoop(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void Execute(ExecuteContext& ctx) const;
        };

        enum class LoopBreakType
        {
            Break,
            Continue,
            Count
        };
        struct LoopBreakStatement : public Statement
        {
            LoopBreakType m_type;
            explicit LoopBreakStatement(const PlaceInCode& place, LoopBreakType type) : Statement{ place }, m_type{ type }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void Execute(ExecuteContext& ctx) const;
        };

        struct ReturnStatement : public Statement
        {
            std::unique_ptr<Expression> ReturnedValue;// Can be null.
            explicit ReturnStatement(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void Execute(ExecuteContext& ctx) const;
        };

        struct Block : public Statement
        {
            explicit Block(const PlaceInCode& place) : Statement{ place }
            {
            }
            std::vector<std::unique_ptr<Statement>> m_statements;
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void Execute(ExecuteContext& ctx) const;
        };

        struct ConstantValue;

        struct SwitchStatement : public Statement
        {
            std::unique_ptr<Expression> Condition;
            std::vector<std::unique_ptr<AST::ConstantValue>> ItemValues;// null means default block.
            std::vector<std::unique_ptr<AST::Block>> ItemBlocks;// Can be null if empty.
            explicit SwitchStatement(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void Execute(ExecuteContext& ctx) const;
        };

        struct ThrowStatement : public Statement
        {
            std::unique_ptr<Expression> ThrownExpression;
            explicit ThrowStatement(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void Execute(ExecuteContext& ctx) const;
        };

        struct TryStatement : public Statement
        {
            std::unique_ptr<Statement> TryBlock;
            std::unique_ptr<Statement> CatchBlock;// Optional
            std::unique_ptr<Statement> FinallyBlock;// Optional
            std::string ExceptionVarName;
            explicit TryStatement(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void Execute(ExecuteContext& ctx) const;
        };

        struct Script : Block
        {
            explicit Script(const PlaceInCode& place) : Block{ place }
            {
            }
            virtual void Execute(ExecuteContext& ctx) const;
        };

        struct Expression : Statement
        {
            explicit Expression(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const
            {
                return GetLValue(ctx).GetValue(GetPlace());
            }
            virtual LValue GetLValue(ExecuteContext& ctx) const
            {
                MINSL_EXECUTION_CHECK(false, GetPlace(), ERROR_MESSAGE_EXPECTED_LVALUE);
            }
            virtual void Execute(ExecuteContext& ctx) const
            {
                Evaluate(ctx, nullptr);
            }
        };

        struct ConstantExpression : Expression
        {
            explicit ConstantExpression(const PlaceInCode& place) : Expression{ place }
            {
            }
            virtual void Execute(ExecuteContext& ctx) const
            { /* Nothing - just ignore its value. */
            }
        };

        struct ConstantValue : ConstantExpression
        {
            Value Val;
            ConstantValue(const PlaceInCode& place, Value&& val) : ConstantExpression{ place }, Val{ std::move(val) }
            {
                assert(Val.GetType() == Value::Type::Null || Val.GetType() == Value::Type::Number || Val.GetType() == Value::Type::String);
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const
            {
                return Value{ Val };
            }
        };

        enum class IdentifierScope
        {
            None,
            Local,
            Global,
            Count
        };
        struct Identifier : ConstantExpression
        {
            IdentifierScope Scope = IdentifierScope::Count;
            std::string m_ident;
            Identifier(const PlaceInCode& place, IdentifierScope scope, std::string&& s)
            : ConstantExpression{ place }, Scope(scope), m_ident(std::move(s))
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
            virtual LValue GetLValue(ExecuteContext& ctx) const;
        };

        struct ThisExpression : ConstantExpression
        {
            ThisExpression(const PlaceInCode& place) : ConstantExpression{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
        };

        struct Operator : Expression
        {
            explicit Operator(const PlaceInCode& place) : Expression{ place }
            {
            }
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
            Count,
        };
        struct UnaryOperator : Operator
        {
            UnaryOperatorType m_type;
            std::unique_ptr<Expression> Operand;
            UnaryOperator(const PlaceInCode& place, UnaryOperatorType type) : Operator{ place }, m_type(type)
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
            virtual LValue GetLValue(ExecuteContext& ctx) const;

        private:
            Value BitwiseNot(Value&& operand) const;
        };

        struct MemberAccessOperator : Operator
        {
            std::unique_ptr<Expression> Operand;
            std::string MemberName;
            MemberAccessOperator(const PlaceInCode& place) : Operator{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
            virtual LValue GetLValue(ExecuteContext& ctx) const;
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
            AssignmentAdd,
            AssignmentSub,
            AssignmentMul,
            AssignmentDiv,
            AssignmentMod,
            AssignmentShiftLeft,
            AssignmentShiftRight,
            AssignmentBitwiseAnd,
            AssignmentBitwiseXor,
            AssignmentBitwiseOr,
            Less,
            Greater,
            LessEqual,
            GreaterEqual,
            Equal,
            NotEqual,
            BitwiseAnd,
            BitwiseXor,
            BitwiseOr,
            LogicalAnd,
            LogicalOr,
            Comma,
            Indexing,
            Count
        };
        struct BinaryOperator : Operator
        {
            BinaryOperatorType m_type;
            std::unique_ptr<Expression> Operands[2];
            BinaryOperator(const PlaceInCode& place, BinaryOperatorType type) : Operator{ place }, m_type(type)
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
            virtual LValue GetLValue(ExecuteContext& ctx) const;

        private:
            Value ShiftLeft(const Value& lhs, const Value& rhs) const;
            Value ShiftRight(const Value& lhs, const Value& rhs) const;
            Value Assignment(LValue&& lhs, Value&& rhs) const;
        };

        struct TernaryOperator : Operator
        {
            std::unique_ptr<Expression> Operands[3];
            explicit TernaryOperator(const PlaceInCode& place) : Operator{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
        };

        struct CallOperator : Operator
        {
            std::vector<std::unique_ptr<Expression>> Operands;
            CallOperator(const PlaceInCode& place) : Operator{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
        };

        struct FunctionDefinition : public Expression
        {
            std::vector<std::string> Parameters;
            Block Body;
            FunctionDefinition(const PlaceInCode& place) : Expression{ place }, Body{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const
            {
                return Value{ this };
            }
            bool AreParameterNamesUnique() const;
        };

        struct ObjectExpression : public Expression
        {
            std::unique_ptr<Expression> BaseExpression;
            using ItemMap = std::map<std::string, std::unique_ptr<Expression>>;
            ItemMap m_items;
            ObjectExpression(const PlaceInCode& place) : Expression{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
        };

        struct ArrayExpression : public Expression
        {
            std::vector<std::unique_ptr<Expression>> m_items;
            ArrayExpression(const PlaceInCode& place) : Expression{ place }
            {
            }
            virtual void DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value Evaluate(ExecuteContext& ctx, ThisType* outThis) const;
        };

    }// namespace AST

    static inline void CheckNumberOperand(const AST::Expression* operand, const Value& value)
    {
        MINSL_EXECUTION_CHECK(value.GetType() == Value::Type::Number, operand->GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Parser definition

    class Parser
    {
        private:
            Tokenizer& m_tokenizer;
            std::vector<Token> m_toklist;
            size_t m_tokidx = 0;

        private:
            void ParseBlock(AST::Block& outBlock);
            bool TryParseSwitchItem(AST::SwitchStatement& switchStatement);
            void ParseFunctionDefinition(AST::FunctionDefinition& funcDef);
            bool PeekSymbols(std::initializer_list<Token::Type> arr);
            std::unique_ptr<AST::Statement> TryParseStatement();
            std::unique_ptr<AST::ConstantValue> TryParseConstantValue();
            std::unique_ptr<AST::Identifier> TryParseIdentifierValue();
            std::unique_ptr<AST::ConstantExpression> TryParseConstantExpr();
            std::pair<std::string, std::unique_ptr<AST::FunctionDefinition>> TryParseFunctionSyntacticSugar();
            std::unique_ptr<AST::Expression> TryParseClassSyntacticSugar();
            std::unique_ptr<AST::Expression> TryParseObjectMember(std::string& outMemberName);
            std::unique_ptr<AST::ObjectExpression> TryParseObject();
            std::unique_ptr<AST::ArrayExpression> TryParseArray();
            std::unique_ptr<AST::Expression> TryParseExpr0();
            std::unique_ptr<AST::Expression> TryParseExpr2();
            std::unique_ptr<AST::Expression> TryParseExpr3();
            std::unique_ptr<AST::Expression> TryParseExpr5();
            std::unique_ptr<AST::Expression> TryParseExpr6();
            std::unique_ptr<AST::Expression> TryParseExpr7();
            std::unique_ptr<AST::Expression> TryParseExpr9();
            std::unique_ptr<AST::Expression> TryParseExpr10();
            std::unique_ptr<AST::Expression> TryParseExpr11();
            std::unique_ptr<AST::Expression> TryParseExpr12();
            std::unique_ptr<AST::Expression> TryParseExpr13();
            std::unique_ptr<AST::Expression> TryParseExpr14();
            std::unique_ptr<AST::Expression> TryParseExpr15();
            std::unique_ptr<AST::Expression> TryParseExpr16();
            std::unique_ptr<AST::Expression> TryParseExpr17();
            bool TryParseSymbol(Token::Type symbol);
            std::string TryParseIdentifier();// If failed, returns empty string.
            const PlaceInCode& GetCurrentTokenPlace() const
            {
                return m_toklist[m_tokidx].m_place;
            }

        public:
            Parser(Tokenizer& tokenizer) : m_tokenizer(tokenizer)
            {
            }
            void ParseScript(AST::Script& outScript);

    };

    ////////////////////////////////////////////////////////////////////////////////
    // EnvironmentPimpl definition

    class EnvironmentPimpl
    {
        private:
            Environment& m_owner;
            Object& m_globalscope;

        public:
            EnvironmentPimpl(Environment& owner, Object& globalScope) : m_owner(owner), m_globalscope{ globalScope }
            {
            }

            ~EnvironmentPimpl() = default;

            Environment& GetOwner()
            {
                return m_owner;
            }

            Value Execute(const std::string_view& code);

            std::string_view GetTypeName(Value::Type type) const;

            void Print(const std::string_view& s)
            {
                //m_Output.append(s);
                std::cout << s;
            }
    };

    ////////////////////////////////////////////////////////////////////////////////
    // Error implementation

    const char* Error::podMessage() const
    {
        if(m_what.empty())
            m_what = Format("(%u,%u): %.*s", m_place.textrow, m_place.textcolumn, (int)getMessage().length(), getMessage().data());
        return m_what.c_str();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Tokenizer implementation

    void Tokenizer::GetNextToken(Token& out)
    {
        SkipSpacesAndComments();

        out.m_place = m_code.GetCurrentPlace();

        // End of input
        if(m_code.IsEnd())
        {
            out.m_symtype = Token::Type::End;
            return;
        }

        constexpr Token::Type firstSingleCharSymbol = Token::Type::Comma;
        constexpr Token::Type firstMultiCharSymbol = Token::Type::DoublePlus;
        constexpr Token::Type firstKeywordSymbol = Token::Type::Null;

        const char* const currentCode = m_code.GetCurrentCode();
        const size_t currentCodeLen = m_code.GetCurrentLen();

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
                out.m_symtype = (Token::Type)i;
                m_code.MoveChars(symbolLen);
                return;
            }
        }
        // m_symtype
        for(size_t i = (size_t)firstSingleCharSymbol; i < (size_t)firstMultiCharSymbol; ++i)
        {
            if(currentCode[0] == SYMBOL_STR[i][0])
            {
                out.m_symtype = (Token::Type)i;
                m_code.MoveOneChar();
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
            for(size_t i = (size_t)firstKeywordSymbol; i < (size_t)Token::Type::Count; ++i)
            {
                const size_t keywordLen = SYMBOL_STR[i].length();
                if(keywordLen == tokenLen && memcmp(SYMBOL_STR[i].data(), currentCode, tokenLen) == 0)
                {
                    out.m_symtype = (Token::Type)i;
                    m_code.MoveChars(keywordLen);
                    return;
                }
            }
            // Identifier
            out.m_symtype = Token::Type::Identifier;
            out.m_stringval = std::string{ currentCode, currentCode + tokenLen };
            m_code.MoveChars(tokenLen);
            return;
        }
        throw ParsingError(out.m_place, ERROR_MESSAGE_UNRECOGNIZED_TOKEN);
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

    bool Tokenizer::ParseCharsHex(uint32_t& out, const std::string_view& chars)
    {
        size_t i;
        size_t count;
        out = 0;
        for(i = 0, count = chars.length(); i < count; ++i)
        {
            uint8_t charVal = 0;
            if(!ParseCharHex(charVal, chars[i]))
                return false;
            out = (out << 4) | charVal;
        }
        return true;
    }

    bool Tokenizer::AppendUtf8Char(std::string& inout, uint32_t charVal)
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
            inout += (char)(uint8_t)(0b10000000 | (charVal & 0b111111));
        }
        else if(charVal <= 0x10FFFF)
        {
            inout += (char)(uint8_t)(0b11110000 | (charVal >> 18));
            inout += (char)(uint8_t)(0b10000000 | ((charVal >> 12) & 0b111111));
            inout += (char)(uint8_t)(0b10000000 | ((charVal >> 6) & 0b111111));
            inout += (char)(uint8_t)(0b10000000 | (charVal & 0b111111));
        }
        else
            return false;
        return true;
    }

    void Tokenizer::SkipSpacesAndComments()
    {
        while(!m_code.IsEnd())
        {
            // Whitespace
            if(isspace(m_code.GetCurrentChar()))
                m_code.MoveOneChar();
            // Single line comment
            else if(m_code.Peek("//", 2))
            {
                m_code.MoveChars(2);
                while(!m_code.IsEnd() && m_code.GetCurrentChar() != '\n')
                    m_code.MoveOneChar();
            }
            // Multi line comment
            else if(m_code.Peek("/*", 2))
            {
                for(m_code.MoveChars(2);; m_code.MoveOneChar())
                {
                    if(m_code.IsEnd())
                        throw ParsingError(m_code.GetCurrentPlace(), ERROR_MESSAGE_UNEXPECTED_END_OF_FILE_IN_MULTILINE_COMMENT);
                    else if(m_code.Peek("*/", 2))
                    {
                        m_code.MoveChars(2);
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
        enum { kBufSize = 128 };
        uint64_t n;
        uint64_t i;
        uint8_t val;
        size_t tokenLen;
        size_t currentCodeLen;
        const char* currentCode;
        char sz[kBufSize + 1];

        currentCode = m_code.GetCurrentCode();
        currentCodeLen = m_code.GetCurrentLen();
        if(!IsDecimalNumber(currentCode[0]) && currentCode[0] != '.')
        {
            return false;
        }
        tokenLen = 0;
        // Hexadecimal: 0xHHHH...
        if(currentCode[0] == '0' && currentCodeLen >= 2 && (currentCode[1] == 'x' || currentCode[1] == 'X'))
        {
            tokenLen = 2;
            while(tokenLen < currentCodeLen && IsHexadecimalNumber(currentCode[tokenLen]))
                ++tokenLen;
            if(tokenLen < 3)
                throw ParsingError(out.m_place, ERROR_MESSAGE_INVALID_NUMBER);
            n = 0;
            for(i = 2; i < tokenLen; ++i)
            {
                val = 0;
                ParseCharHex(val, currentCode[i]);
                n = (n << 4) | val;
            }
            out.m_numval = (double)n;
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
                    throw ParsingError(out.m_place, ERROR_MESSAGE_INVALID_NUMBER);
            }
            if(tokenLen >= kBufSize)
            {
                throw ParsingError(out.m_place, ERROR_MESSAGE_INVALID_NUMBER);
            }
            memcpy(sz, currentCode, tokenLen);
            sz[tokenLen] = 0;
            out.m_numval = atof(sz);
        }
        // Letters straight after number are invalid.
        if(tokenLen < currentCodeLen && IsAlpha(currentCode[tokenLen]))
            throw ParsingError(out.m_place, ERROR_MESSAGE_INVALID_NUMBER);
        out.m_symtype = Token::Type::Number;
        m_code.MoveChars(tokenLen);
        return true;
    }

    bool Tokenizer::ParseString(Token& out)
    {
        size_t tokenLen;
        size_t currCodeLen;
        char delimiterCh;
        const char* currCode;
        currCode = m_code.GetCurrentCode();
        currCodeLen = m_code.GetCurrentLen();
        delimiterCh = currCode[0];
        if(delimiterCh != '"' && delimiterCh != '\'')
        {
            return false;
        }
        out.m_symtype = Token::Type::String;
        out.m_stringval.clear();
        tokenLen = 1;
        for(;;)
        {
            if(tokenLen == currCodeLen)
                throw ParsingError(out.m_place, ERROR_MESSAGE_UNEXPECTED_END_OF_FILE_IN_STRING);
            if(currCode[tokenLen] == delimiterCh)
                break;
            if(currCode[tokenLen] == '\\')
            {
                ++tokenLen;
                if(tokenLen == currCodeLen)
                    throw ParsingError(out.m_place, ERROR_MESSAGE_UNEXPECTED_END_OF_FILE_IN_STRING);
                switch(currCode[tokenLen])
                {
                    case '\\':
                        out.m_stringval += '\\';
                        ++tokenLen;
                        break;
                    case '/':
                        out.m_stringval += '/';
                        ++tokenLen;
                        break;
                    case '"':
                        out.m_stringval += '"';
                        ++tokenLen;
                        break;
                    case '\'':
                        out.m_stringval += '\'';
                        ++tokenLen;
                        break;
                    case '?':
                        out.m_stringval += '?';
                        ++tokenLen;
                        break;
                    case 'a':
                        out.m_stringval += '\a';
                        ++tokenLen;
                        break;
                    case 'b':
                        out.m_stringval += '\b';
                        ++tokenLen;
                        break;
                    case 'f':
                        out.m_stringval += '\f';
                        ++tokenLen;
                        break;
                    case 'n':
                        out.m_stringval += '\n';
                        ++tokenLen;
                        break;
                    case 'r':
                        out.m_stringval += '\r';
                        ++tokenLen;
                        break;
                    case 't':
                        out.m_stringval += '\t';
                        ++tokenLen;
                        break;
                    case 'v':
                        out.m_stringval += '\v';
                        ++tokenLen;
                        break;
                    case '0':
                        out.m_stringval += '\0';
                        ++tokenLen;
                        break;
                    case 'x':
                    {
                        uint32_t val = 0;
                        if(tokenLen + 2 >= currCodeLen || !ParseCharsHex(val, std::string_view{ currCode + tokenLen + 1, 2 }))
                            throw ParsingError(out.m_place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                        out.m_stringval += (char)(uint8_t)val;
                        tokenLen += 3;
                        break;
                    }
                    case 'u':
                    {
                        uint32_t val = 0;
                        if(tokenLen + 4 >= currCodeLen || !ParseCharsHex(val, std::string_view{ currCode + tokenLen + 1, 4 }))
                            throw ParsingError(out.m_place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                        if(!AppendUtf8Char(out.m_stringval, val))
                            throw ParsingError(out.m_place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                        tokenLen += 5;
                        break;
                    }
                    case 'U':
                    {
                        uint32_t val = 0;
                        if(tokenLen + 8 >= currCodeLen || !ParseCharsHex(val, std::string_view{ currCode + tokenLen + 1, 8 }))
                            throw ParsingError(out.m_place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                        if(!AppendUtf8Char(out.m_stringval, val))
                            throw ParsingError(out.m_place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                        tokenLen += 9;
                        break;
                    }
                    default:
                        throw ParsingError(out.m_place, ERROR_MESSAGE_INVALID_ESCAPE_SEQUENCE);
                }
            }
            else
                out.m_stringval += currCode[tokenLen++];
        }
        ++tokenLen;
        // Letters straight after string are invalid.
        if(tokenLen < currCodeLen && IsAlpha(currCode[tokenLen]))
            throw ParsingError(out.m_place, ERROR_MESSAGE_INVALID_STRING);
        m_code.MoveChars(tokenLen);
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Object implementation

    Value* Object::TryGetValue(const std::string& key)
    {
        auto it = m_items.find(key);
        if(it != m_items.end())
            return &it->second;
        return nullptr;
    }
    const Value* Object::TryGetValue(const std::string& key) const
    {
        auto it = m_items.find(key);
        if(it != m_items.end())
            return &it->second;
        return nullptr;
    }

    bool Object::Remove(const std::string& key)
    {
        auto it = m_items.find(key);
        if(it != m_items.end())
        {
            m_items.erase(it);
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
            if(Value* val = objMemberLval->objectval->TryGetValue(objMemberLval->keyval))
                return val;
            MINSL_EXECUTION_FAIL(place, ERROR_MESSAGE_OBJECT_MEMBER_DOESNT_EXIST);
        }
        if(const ArrayItemLValue* arrItemLval = std::get_if<ArrayItemLValue>(this))
        {
            MINSL_EXECUTION_CHECK(arrItemLval->indexval < arrItemLval->arrayval->m_items.size(), place, ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
            return &arrItemLval->arrayval->m_items[arrItemLval->indexval];
        }
        MINSL_EXECUTION_FAIL(place, ERROR_MESSAGE_INVALID_LVALUE);
    }

    Value LValue::GetValue(const PlaceInCode& place) const
    {
        if(const ObjectMemberLValue* objMemberLval = std::get_if<ObjectMemberLValue>(this))
        {
            if(const Value* val = objMemberLval->objectval->TryGetValue(objMemberLval->keyval))
                return *val;
            MINSL_EXECUTION_FAIL(place, ERROR_MESSAGE_OBJECT_MEMBER_DOESNT_EXIST);
        }
        if(const StringCharacterLValue* strCharLval = std::get_if<StringCharacterLValue>(this))
        {
            MINSL_EXECUTION_CHECK(strCharLval->indexval < strCharLval->stringval->length(), place, ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
            const char ch = (*strCharLval->stringval)[strCharLval->indexval];
            return Value{ std::string{ &ch, &ch + 1 } };
        }
        if(const ArrayItemLValue* arrItemLval = std::get_if<ArrayItemLValue>(this))
        {
            MINSL_EXECUTION_CHECK(arrItemLval->indexval < arrItemLval->arrayval->m_items.size(), place, ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
            return Value{ arrItemLval->arrayval->m_items[arrItemLval->indexval] };
        }
        assert(0);
        return {};
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Built-in functions

    static std::shared_ptr<Object> CopyObject(const Object& src)
    {
        auto dst = std::make_shared<Object>();
        for(const auto& item : src.m_items)
            dst->m_items.insert(item);
        return dst;
    }

    static std::shared_ptr<Array> CopyArray(const Array& src)
    {
        auto dst = std::make_shared<Array>();
        const size_t count = src.m_items.size();
        dst->m_items.resize(count);
        for(size_t i = 0; i < count; ++i)
            dst->m_items[i] = Value{ src.m_items[i] };
        return dst;
    }

    static std::shared_ptr<Object> ConvertExecutionErrorToObject(const ExecutionError& err)
    {
        auto obj = std::make_shared<Object>();
        obj->GetOrCreateValue("type") = Value{ "ExecutionError" };
        obj->GetOrCreateValue("index") = Value{ (double)err.GetPlace().textindex };
        obj->GetOrCreateValue("row") = Value{ (double)err.GetPlace().textrow };
        obj->GetOrCreateValue("column") = Value{ (double)err.GetPlace().textcolumn };
        obj->GetOrCreateValue("message") = Value{ std::string{ err.getMessage() } };
        return obj;
    }

    static Value BuiltInTypeCtor_Null(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
    {
        MINSL_EXECUTION_CHECK(args.empty() || args.size() == 1 && args[0].GetType() == Value::Type::Null, place,
                              "Null can be constructed only from no arguments or from another null value.");
        return {};
    }

    static Value BuiltInTypeCtor_Number(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
    {
        Environment& env = ctx.m_env.GetOwner();
        MINSL_LOAD_ARGS_1_NUMBER("Number", val);
        return Value{ val };
    }

    static Value BuiltInTypeCtor_String(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
    {
        if(args.empty())
            return Value{ std::string{} };
        Environment& env = ctx.m_env.GetOwner();
        MINSL_LOAD_ARGS_1_STRING("String", str);
        return Value{ std::move(str) };
    }

    static Value BuiltInTypeCtor_Object(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
    {
        if(args.empty())
            return Value{ std::make_shared<Object>() };
        MINSL_EXECUTION_CHECK(args.size() == 1 && args[0].GetType() == Value::Type::Object, place,
                              "Object can be constructed only from no arguments or from another object value.");
        return Value{ CopyObject(*args[0].GetObject_()) };
    }

    static Value BuiltInTypeCtor_Array(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
    {
        if(args.empty())
            return Value{ std::make_shared<Array>() };
        MINSL_EXECUTION_CHECK(args.size() == 1 && args[0].GetType() == Value::Type::Array, place,
                              "Array can be constructed only from no arguments or from another array value.");
        return Value{ CopyArray(*args[0].GetArray()) };
    }

    static Value BuiltInTypeCtor_Function(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
    {
        MINSL_EXECUTION_CHECK(args.size() == 1
                              && (args[0].GetType() == Value::Type::Function || args[0].GetType() == Value::Type::SystemFunction),
                              place, "Function can be constructed only from another function value.");
        return Value{ args[0] };
    }

    static Value BuiltInTypeCtor_Type(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
    {
        MINSL_EXECUTION_CHECK(args.size() == 1 && args[0].GetType() == Value::Type::Type, place,
                              "Type can be constructed only from another type value.");
        return Value{ args[0] };
    }

    static Value BuiltInFunction_typeOf(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
    {
        MINSL_EXECUTION_CHECK(args.size() == 1, place, ERROR_MESSAGE_EXPECTED_1_ARGUMENT);
        return Value{ args[0].GetType() };
    }

    static Value BuiltInFunction_print(Environment& env, const PlaceInCode& place, std::vector<Value>&& args)
    {
        std::string s;
        for(const auto& val : args)
        {
            switch(val.GetType())
            {
                case Value::Type::Null:
                    env.Print("null\n");
                    break;
                case Value::Type::Number:
                    s = Format("%g\n", val.GetNumber());
                    env.Print(s);
                    break;
                case Value::Type::String:
                    if(!val.GetString().empty())
                        env.Print(val.GetString());
                    env.Print("\n");
                    break;
                case Value::Type::Function:
                case Value::Type::SystemFunction:
                case Value::Type::HostFunction:
                    env.Print("function\n");
                    break;
                case Value::Type::Object:
                    env.Print("object\n");
                    break;
                case Value::Type::Array:
                    env.Print("array\n");
                    break;
                case Value::Type::Type:
                {
                    const size_t typeIndex = (size_t)val.GetTypeValue();
                    const std::string_view& typeName = VALUE_TYPE_NAMES[typeIndex];
                    s = Format("%.*s\n", (int)typeName.length(), typeName.data());
                    env.Print(s);
                }
                break;
                default:
                    assert(0);
            }
        }
        return {};
    }
    static Value BuiltInFunction_min(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
    {
        const size_t argCount = args.size();
        MINSL_EXECUTION_CHECK(argCount > 0, place, "Built-in function min requires at least 1 argument.");
        double result = 0.0;
        for(size_t i = 0; i < argCount; ++i)
        {
            MINSL_EXECUTION_CHECK(args[i].GetType() == Value::Type::Number, place, "Built-in function min requires number arguments.");
            const double argNum = args[i].GetNumber();
            if(i == 0 || argNum < result)
                result = argNum;
        }
        return Value{ result };
    }
    static Value BuiltInFunction_max(AST::ExecuteContext& ctx, const PlaceInCode& place, std::vector<Value>&& args)
    {
        const size_t argCount = args.size();
        MINSL_EXECUTION_CHECK(argCount > 0, place, "Built-in function min requires at least 1 argument.");
        double result = 0.0;
        for(size_t i = 0; i < argCount; ++i)
        {
            MINSL_EXECUTION_CHECK(args[i].GetType() == Value::Type::Number, place, "Built-in function min requires number arguments.");
            const double argNum = args[i].GetNumber();
            if(i == 0 || argNum > result)
                result = argNum;
        }
        return Value{ result };
    }

    static Value BuiltInMember_Object_Count(AST::ExecuteContext& ctx, const PlaceInCode& place, Value&& objVal)
    {
        MINSL_EXECUTION_CHECK(objVal.GetType() == Value::Type::Object && objVal.GetObject_(), place, ERROR_MESSAGE_EXPECTED_OBJECT);
        return Value{ (double)objVal.GetObject_()->GetCount() };
    }
    static Value BuiltInMember_Array_Count(AST::ExecuteContext& ctx, const PlaceInCode& place, Value&& objVal)
    {
        MINSL_EXECUTION_CHECK(objVal.GetType() == Value::Type::Array && objVal.GetArray(), place, ERROR_MESSAGE_EXPECTED_ARRAY);
        return Value{ (double)objVal.GetArray()->m_items.size() };
    }
    static Value BuiltInMember_String_Count(AST::ExecuteContext& ctx, const PlaceInCode& place, Value&& objVal)
    {
        MINSL_EXECUTION_CHECK(objVal.GetType() == Value::Type::String, place, ERROR_MESSAGE_EXPECTED_STRING);
        return Value{ (double)objVal.GetString().length() };
    }
    static Value
    BuiltInFunction_String_resize(AST::ExecuteContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args)
    {
        // TODO
        return {};
    }
    static Value
    BuiltInFunction_Array_add(AST::ExecuteContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args)
    {
        Array* arr = th.GetArray();
        MINSL_EXECUTION_CHECK(arr, place, ERROR_MESSAGE_EXPECTED_ARRAY);
        MINSL_EXECUTION_CHECK(args.size() == 1, place, ERROR_MESSAGE_EXPECTED_1_ARGUMENT);
        arr->m_items.push_back(std::move(args[0]));
        return {};
    }
    static Value
    BuiltInFunction_Array_insert(AST::ExecuteContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args)
    {
        Array* arr = th.GetArray();
        MINSL_EXECUTION_CHECK(arr, place, ERROR_MESSAGE_EXPECTED_ARRAY);
        MINSL_EXECUTION_CHECK(args.size() == 2, place, ERROR_MESSAGE_EXPECTED_2_ARGUMENTS);
        size_t index = 0;
        MINSL_EXECUTION_CHECK(args[0].GetType() == Value::Type::Number && NumberToIndex(index, args[0].GetNumber()),
                              place, ERROR_MESSAGE_INVALID_INDEX);
        arr->m_items.insert(arr->m_items.begin() + index, std::move(args[1]));
        return {};
    }
    static Value
    BuiltInFunction_Array_remove(AST::ExecuteContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args)
    {
        Array* arr = th.GetArray();
        MINSL_EXECUTION_CHECK(arr, place, ERROR_MESSAGE_EXPECTED_ARRAY);
        MINSL_EXECUTION_CHECK(args.size() == 1, place, ERROR_MESSAGE_EXPECTED_1_ARGUMENT);
        size_t index = 0;
        MINSL_EXECUTION_CHECK(args[0].GetType() == Value::Type::Number && NumberToIndex(index, args[0].GetNumber()),
                              place, ERROR_MESSAGE_INVALID_INDEX);
        arr->m_items.erase(arr->m_items.begin() + index);
        return {};
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Abstract Syntax Tree implementation

    namespace AST
    {
    #define DEBUG_PRINT_FORMAT_STR_BEG "(%u,%u) %s%.*s"
    #define DEBUG_PRINT_ARGS_BEG \
        GetPlace().textrow, GetPlace().textcolumn, GetDebugPrintIndent(indentLevel), (int)prefix.length(), prefix.data()

        void Statement::Assign(const LValue& lhs, Value&& rhs) const
        {
            if(const ObjectMemberLValue* objMemberLhs = std::get_if<ObjectMemberLValue>(&lhs))
            {
                if(rhs.GetType() == Value::Type::Null)
                    objMemberLhs->objectval->Remove(objMemberLhs->keyval);
                else
                    objMemberLhs->objectval->GetOrCreateValue(objMemberLhs->keyval) = std::move(rhs);
            }
            else if(const ArrayItemLValue* arrItemLhs = std::get_if<ArrayItemLValue>(&lhs))
            {
                MINSL_EXECUTION_CHECK(arrItemLhs->indexval < arrItemLhs->arrayval->m_items.size(), GetPlace(), ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
                arrItemLhs->arrayval->m_items[arrItemLhs->indexval] = std::move(rhs);
            }
            else if(const StringCharacterLValue* strCharLhs = std::get_if<StringCharacterLValue>(&lhs))
            {
                MINSL_EXECUTION_CHECK(strCharLhs->indexval < strCharLhs->stringval->length(), GetPlace(), ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
                MINSL_EXECUTION_CHECK(rhs.GetType() == Value::Type::String, GetPlace(), ERROR_MESSAGE_EXPECTED_STRING);
                MINSL_EXECUTION_CHECK(rhs.GetString().length() == 1, GetPlace(), ERROR_MESSAGE_EXPECTED_SINGLE_CHARACTER_STRING);
                (*strCharLhs->stringval)[strCharLhs->indexval] = rhs.GetString()[0];
            }
            else
                assert(0);
        }

        void EmptyStatement::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Empty\n", DEBUG_PRINT_ARGS_BEG);
        }

        void Condition::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "If\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            m_condexpr->DebugPrint(indentLevel, "ConditionExpression: ");
            m_statements[0]->DebugPrint(indentLevel, "TrueStatement: ");
            if(m_statements[1])
                m_statements[1]->DebugPrint(indentLevel, "FalseStatement: ");
        }

        void Condition::Execute(ExecuteContext& ctx) const
        {
            if(m_condexpr->Evaluate(ctx, nullptr).IsTrue())
                m_statements[0]->Execute(ctx);
            else if(m_statements[1])
                m_statements[1]->Execute(ctx);
        }

        void WhileLoop::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            const char* name = nullptr;
            switch(m_type)
            {
                case WhileLoop::Type::While:
                    name = "While";
                    break;
                case WhileLoop::Type::DoWhile:
                    name = "DoWhile";
                    break;
                default:
                    assert(0);
            }
            printf(DEBUG_PRINT_FORMAT_STR_BEG "%s\n", DEBUG_PRINT_ARGS_BEG, name);
            ++indentLevel;
            m_condexpr->DebugPrint(indentLevel, "ConditionExpression: ");
            Body->DebugPrint(indentLevel, "Body: ");
        }

        void WhileLoop::Execute(ExecuteContext& ctx) const
        {
            switch(m_type)
            {
                case WhileLoop::Type::While:
                    while(m_condexpr->Evaluate(ctx, nullptr).IsTrue())
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
                case WhileLoop::Type::DoWhile:
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
                    } while(m_condexpr->Evaluate(ctx, nullptr).IsTrue());
                    break;
                default:
                    assert(0);
            }
        }

        void ForLoop::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "For\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            if(InitExpression)
                InitExpression->DebugPrint(indentLevel, "InitExpression: ");
            else
                printf(DEBUG_PRINT_FORMAT_STR_BEG "(Init expression empty)\n", DEBUG_PRINT_ARGS_BEG);
            if(m_condexpr)
                m_condexpr->DebugPrint(indentLevel, "ConditionExpression: ");
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
            while(m_condexpr ? m_condexpr->Evaluate(ctx, nullptr).IsTrue() : true)
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

        void RangeBasedForLoop::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            if(!KeyVarName.empty())
                printf(DEBUG_PRINT_FORMAT_STR_BEG "Range-based for: %s, %s\n", DEBUG_PRINT_ARGS_BEG, KeyVarName.c_str(),
                       ValueVarName.c_str());
            else
                printf(DEBUG_PRINT_FORMAT_STR_BEG "Range-based for: %s\n", DEBUG_PRINT_ARGS_BEG, ValueVarName.c_str());
            ++indentLevel;
            RangeExpression->DebugPrint(indentLevel, "RangeExpression: ");
            Body->DebugPrint(indentLevel, "Body: ");
        }

        void RangeBasedForLoop::Execute(ExecuteContext& ctx) const
        {
            size_t i;
            size_t count;
            const Value rangeVal = RangeExpression->Evaluate(ctx, nullptr);
            Object& innermostCtxObj = ctx.GetInnermostScope();
            const bool useKey = !KeyVarName.empty();

            if(rangeVal.GetType() == Value::Type::String)
            {
                const std::string& rangeStr = rangeVal.GetString();
                size_t count = rangeStr.length();
                for(i = 0; i < count; ++i)
                {
                    if(useKey)
                        Assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, KeyVarName } }, Value{ (double)i });
                    const char ch = rangeStr[i];
                    Assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, ValueVarName } }, Value{ std::string{ &ch, &ch + 1 } });
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
            else if(rangeVal.GetType() == Value::Type::Object)
            {
                for(const auto& [key, value] : rangeVal.GetObject_()->m_items)
                {
                    if(useKey)
                        Assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, KeyVarName } }, Value{ std::string{ key } });
                    Assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, ValueVarName } }, Value{ value });
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
            else if(rangeVal.GetType() == Value::Type::Array)
            {
                const Array* const arr = rangeVal.GetArray();
                for(i = 0, count = arr->m_items.size(); i < count; ++i)
                {
                    if(useKey)
                        Assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, KeyVarName } }, Value{ (double)i });
                    Assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, ValueVarName } }, Value{ arr->m_items[i] });
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
                MINSL_EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_TYPE);

            if(useKey)
                Assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, KeyVarName } }, Value{});
            Assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, ValueVarName } }, Value{});
        }

        void LoopBreakStatement::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            static const char* LOOP_BREAK_TYPE_NAMES[] = { "Break", "Continue" };
            printf(DEBUG_PRINT_FORMAT_STR_BEG "%s\n", DEBUG_PRINT_ARGS_BEG, LOOP_BREAK_TYPE_NAMES[(size_t)m_type]);
        }

        void LoopBreakStatement::Execute(ExecuteContext& ctx) const
        {
            switch(m_type)
            {
                case LoopBreakType::Break:
                    throw BreakException{};
                case LoopBreakType::Continue:
                    throw ContinueException{};
                default:
                    assert(0);
            }
        }

        void ReturnStatement::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "return\n", DEBUG_PRINT_ARGS_BEG);
            if(ReturnedValue)
                ReturnedValue->DebugPrint(indentLevel + 1, "ReturnedValue: ");
        }

        void ReturnStatement::Execute(ExecuteContext& ctx) const
        {
            if(ReturnedValue)
                throw ReturnException{ GetPlace(), ReturnedValue->Evaluate(ctx, nullptr) };
            else
                throw ReturnException{ GetPlace(), Value{} };
        }

        void Block::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Block\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            for(const auto& stmtPtr : m_statements)
                stmtPtr->DebugPrint(indentLevel, std::string_view{});
        }

        void Block::Execute(ExecuteContext& ctx) const
        {
            for(const auto& stmtPtr : m_statements)
                stmtPtr->Execute(ctx);
        }

        void SwitchStatement::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
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

        void ThrowStatement::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "throw\n", DEBUG_PRINT_ARGS_BEG);
            ThrownExpression->DebugPrint(indentLevel + 1, "ThrownExpression: ");
        }

        void ThrowStatement::Execute(ExecuteContext& ctx) const
        {
            throw ThrownExpression->Evaluate(ctx, nullptr);
        }

        void TryStatement::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
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
            {
                TryBlock->Execute(ctx);
            }
            catch(Value& val)
            {
                if(CatchBlock)
                {
                    Object& innermostCtxObj = ctx.GetInnermostScope();
                    Assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, ExceptionVarName } }, std::move(val));
                    CatchBlock->Execute(ctx);
                    Assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, ExceptionVarName } }, Value{});
                    if(FinallyBlock)
                        FinallyBlock->Execute(ctx);
                }
                else
                {
                    assert(FinallyBlock);
                    // One exception is on the fly - new one is ignored, old one is thrown again.
                    try
                    {
                        FinallyBlock->Execute(ctx);
                    }
                    catch(const Value&)
                    {
                        throw val;
                    }
                    catch(const ExecutionError&)
                    {
                        throw val;
                    }
                    throw val;
                }
                return;
            }
            catch(const ExecutionError& err)
            {
                if(CatchBlock)
                {
                    Object& innermostCtxObj = ctx.GetInnermostScope();
                    Assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, ExceptionVarName } },
                           Value{ ConvertExecutionErrorToObject(err) });
                    CatchBlock->Execute(ctx);
                    Assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, ExceptionVarName } }, Value{});
                    if(FinallyBlock)
                        FinallyBlock->Execute(ctx);
                }
                else
                {
                    assert(FinallyBlock);
                    // One exception is on the fly - new one is ignored, old one is thrown again.
                    try
                    {
                        FinallyBlock->Execute(ctx);
                    }
                    catch(const Value&)
                    {
                        throw err;
                    }
                    catch(const ExecutionError&)
                    {
                        throw err;
                    }
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
            {
                assert(0 && "ParsingError not expected during execution.");
            }
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
                throw ExecutionError{ GetPlace(), ERROR_MESSAGE_BREAK_WITHOUT_LOOP };
            }
            catch(ContinueException)
            {
                throw ExecutionError{ GetPlace(), ERROR_MESSAGE_CONTINUE_WITHOUT_LOOP };
            }
        }

        void ConstantValue::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            switch(Val.GetType())
            {
                case Value::Type::Null:
                    printf(DEBUG_PRINT_FORMAT_STR_BEG "Constant null\n", DEBUG_PRINT_ARGS_BEG);
                    break;
                case Value::Type::Number:
                    printf(DEBUG_PRINT_FORMAT_STR_BEG "Constant number: %g\n", DEBUG_PRINT_ARGS_BEG, Val.GetNumber());
                    break;
                case Value::Type::String:
                    printf(DEBUG_PRINT_FORMAT_STR_BEG "Constant string: %s\n", DEBUG_PRINT_ARGS_BEG, Val.GetString().c_str());
                    break;
                default:
                    assert(0 && "ConstantValue should not be used with this type.");
            }
        }

        void Identifier::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            static const char* PREFIX[] = { "", "local.", "global." };
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Identifier: %s%s\n", DEBUG_PRINT_ARGS_BEG, PREFIX[(size_t)Scope], m_ident.c_str());
        }

        Value Identifier::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
        {
            size_t i;
            size_t count;
            MINSL_EXECUTION_CHECK(Scope != IdentifierScope::Local || ctx.IsLocal(), GetPlace(), ERROR_MESSAGE_NO_LOCAL_SCOPE);

            if(ctx.IsLocal())
            {
                // Local variable
                if((Scope == IdentifierScope::None || Scope == IdentifierScope::Local))
                    if(Value* val = ctx.GetCurrentLocalScope()->TryGetValue(m_ident); val)
                        return *val;
                // This
                if(Scope == IdentifierScope::None)
                {
                    if(const std::shared_ptr<Object>* thisObj = std::get_if<std::shared_ptr<Object>>(&ctx.GetThis()); thisObj)
                    {
                        if(Value* val = (*thisObj)->TryGetValue(m_ident); val)
                        {
                            if(outThis)
                                *outThis = ThisType{ *thisObj };
                            return *val;
                        }
                    }
                }
            }

            if(Scope == IdentifierScope::None || Scope == IdentifierScope::Global)
            {
                // Global variable
                Value* val = ctx.GlobalScope.TryGetValue(m_ident);
                if(val)
                    return *val;
                // Type
                for(i = 0, count = (size_t)Value::Type::Count; i < count; ++i)
                {
                    if(m_ident == VALUE_TYPE_NAMES[i])
                        return Value{ (Value::Type)i };
                }
                // System function
                for(i = 0, count = (size_t)SystemFunction::CountFunc; i < count; ++i)
                {
                    if(m_ident == SYSTEM_FUNCTION_NAMES[i])
                    {
                        return Value{ (SystemFunction)i };
                    }
                }
            }

            // Not found - null
            return {};
        }

        LValue Identifier::GetLValue(ExecuteContext& ctx) const
        {
            const bool isLocal = ctx.IsLocal();
            MINSL_EXECUTION_CHECK(Scope != IdentifierScope::Local || isLocal, GetPlace(), ERROR_MESSAGE_NO_LOCAL_SCOPE);

            if(isLocal)
            {
                // Local variable
                if((Scope == IdentifierScope::None || Scope == IdentifierScope::Local) && ctx.GetCurrentLocalScope()->HasKey(m_ident))
                    return LValue{ ObjectMemberLValue{ &*ctx.GetCurrentLocalScope(), m_ident } };
                // This
                if(Scope == IdentifierScope::None)
                {
                    if(const std::shared_ptr<Object>* thisObj = std::get_if<std::shared_ptr<Object>>(&ctx.GetThis());
                       thisObj && (*thisObj)->HasKey(m_ident))
                        return LValue{ ObjectMemberLValue{ (*thisObj).get(), m_ident } };
                }
            }

            // Global variable
            if((Scope == IdentifierScope::None || Scope == IdentifierScope::Global) && ctx.GlobalScope.HasKey(m_ident))
                return LValue{ ObjectMemberLValue{ &ctx.GlobalScope, m_ident } };

            // Not found: return reference to smallest scope.
            if((Scope == IdentifierScope::None || Scope == IdentifierScope::Local) && isLocal)
                return LValue{ ObjectMemberLValue{ ctx.GetCurrentLocalScope(), m_ident } };
            return LValue{ ObjectMemberLValue{ &ctx.GlobalScope, m_ident } };
        }

        void ThisExpression::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "This\n", DEBUG_PRINT_ARGS_BEG);
        }

        Value ThisExpression::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
        {
            MINSL_EXECUTION_CHECK(ctx.IsLocal() && std::get_if<std::shared_ptr<Object>>(&ctx.GetThis()), GetPlace(), ERROR_MESSAGE_NO_THIS);
            return Value{ std::shared_ptr<Object>{ *std::get_if<std::shared_ptr<Object>>(&ctx.GetThis()) } };
        }

        void UnaryOperator::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            static const char* UNARY_OPERATOR_TYPE_NAMES[]
            = { "Preincrementation", "Predecrementation", "Postincrementation", "Postdecrementation", "Plus", "Minus",
                "Logical NOT",       "Bitwise NOT" };

            printf(DEBUG_PRINT_FORMAT_STR_BEG "UnaryOperator %s\n", DEBUG_PRINT_ARGS_BEG,
                   UNARY_OPERATOR_TYPE_NAMES[(uint32_t)m_type]);
            ++indentLevel;
            Operand->DebugPrint(indentLevel, "Operand: ");
        }

        Value UnaryOperator::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
        {
            // Those require l-value.
            if(m_type == UnaryOperatorType::Preincrementation || m_type == UnaryOperatorType::Predecrementation
               || m_type == UnaryOperatorType::Postincrementation || m_type == UnaryOperatorType::Postdecrementation)
            {
                Value* val = Operand->GetLValue(ctx).GetValueRef(GetPlace());
                MINSL_EXECUTION_CHECK(val->GetType() == Value::Type::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                switch(m_type)
                {
                    case UnaryOperatorType::Preincrementation:
                        val->ChangeNumber(val->GetNumber() + 1.0);
                        return *val;
                    case UnaryOperatorType::Predecrementation:
                        val->ChangeNumber(val->GetNumber() - 1.0);
                        return *val;
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
                    default:
                        assert(0);
                        return {};
                }
            }
            // Those use r-value.
            else if(m_type == UnaryOperatorType::Plus || m_type == UnaryOperatorType::Minus
                    || m_type == UnaryOperatorType::LogicalNot || m_type == UnaryOperatorType::BitwiseNot)
            {
                Value val = Operand->Evaluate(ctx, nullptr);
                MINSL_EXECUTION_CHECK(val.GetType() == Value::Type::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                switch(m_type)
                {
                    case UnaryOperatorType::Plus:
                        return val;
                    case UnaryOperatorType::Minus:
                        return Value{ -val.GetNumber() };
                    case UnaryOperatorType::LogicalNot:
                        return Value{ val.IsTrue() ? 0.0 : 1.0 };
                    case UnaryOperatorType::BitwiseNot:
                        return BitwiseNot(std::move(val));
                    default:
                        assert(0);
                        return {};
                }
            }
            assert(0);
            return {};
        }

        LValue UnaryOperator::GetLValue(ExecuteContext& ctx) const
        {
            if(m_type == UnaryOperatorType::Preincrementation || m_type == UnaryOperatorType::Predecrementation)
            {
                LValue lval = Operand->GetLValue(ctx);
                const ObjectMemberLValue* objMemberLval = std::get_if<ObjectMemberLValue>(&lval);
                MINSL_EXECUTION_CHECK(objMemberLval, GetPlace(), ERROR_MESSAGE_INVALID_LVALUE);
                Value* val = objMemberLval->objectval->TryGetValue(objMemberLval->keyval);
                MINSL_EXECUTION_CHECK(val != nullptr, GetPlace(), ERROR_MESSAGE_VARIABLE_DOESNT_EXIST);
                MINSL_EXECUTION_CHECK(val->GetType() == Value::Type::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                switch(m_type)
                {
                    case UnaryOperatorType::Preincrementation:
                        val->ChangeNumber(val->GetNumber() + 1.0);
                        return lval;
                    case UnaryOperatorType::Predecrementation:
                        val->ChangeNumber(val->GetNumber() - 1.0);
                        return lval;
                    default:
                        assert(0);
                }
            }
            MINSL_EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_LVALUE);
        }

        void MemberAccessOperator::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "MemberAccessOperator Member=%s\n", DEBUG_PRINT_ARGS_BEG, MemberName.c_str());
            Operand->DebugPrint(indentLevel + 1, "Operand: ");
        }

        Value MemberAccessOperator::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
        {
            Value objVal = Operand->Evaluate(ctx, nullptr);
            if(objVal.GetType() == Value::Type::Object)
            {
                const Value* memberVal = objVal.GetObject_()->TryGetValue(MemberName);
                if(memberVal)
                {
                    if(outThis)
                        *outThis = ThisType{ objVal.GetObjectPtr() };
                    return *memberVal;
                }
                if(MemberName == "count")
                    return BuiltInMember_Object_Count(ctx, GetPlace(), std::move(objVal));
                return {};
            }
            if(objVal.GetType() == Value::Type::String)
            {
                if(MemberName == "count")
                    return BuiltInMember_String_Count(ctx, GetPlace(), std::move(objVal));
                else if(MemberName == "resize")
                    return Value{ SystemFunction::StringResizeFunc };
                MINSL_EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_MEMBER);
            }
            if(objVal.GetType() == Value::Type::Array)
            {
                if(outThis)
                    *outThis = ThisType{ objVal.GetArrayPtr() };
                if(MemberName == "count")
                    return BuiltInMember_Array_Count(ctx, GetPlace(), std::move(objVal));
                else if(MemberName == "add")
                    return Value{ SystemFunction::ArrayAddFunc };
                else if(MemberName == "insert")
                    return Value{ SystemFunction::ArrayInsertFunc };
                else if(MemberName == "remove")
                    return Value{ SystemFunction::ArrayRemoveFunc };
                MINSL_EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_MEMBER);
            }
            MINSL_EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_TYPE);
        }

        LValue MemberAccessOperator::GetLValue(ExecuteContext& ctx) const
        {
            Value objVal = Operand->Evaluate(ctx, nullptr);
            MINSL_EXECUTION_CHECK(objVal.GetType() == Value::Type::Object, GetPlace(), ERROR_MESSAGE_EXPECTED_OBJECT);
            return LValue{ ObjectMemberLValue{ objVal.GetObject_(), MemberName } };
        }

        Value UnaryOperator::BitwiseNot(Value&& operand) const
        {
            const int64_t operandInt = (int64_t)operand.GetNumber();
            const int64_t resultInt = ~operandInt;
            return Value{ (double)resultInt };
        }

        void BinaryOperator::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            static const char* BINARY_OPERATOR_TYPE_NAMES[] = {
                "Mul",
                "Div",
                "Mod",
                "Add",
                "Sub",
                "Shift left",
                "Shift right",
                "Assignment",
                "AssignmentAdd",
                "AssignmentSub",
                "AssignmentMul",
                "AssignmentDiv",
                "AssignmentMod",
                "AssignmentShiftLeft",
                "AssignmentShiftRight",
                "AssignmentBitwiseAnd",
                "AssignmentBitwiseXor",
                "AssignmentBitwiseOr",
                "Less",
                "Greater",
                "LessEqual",
                "GreaterEqual",
                "Equal",
                "NotEqual",
                "BitwiseAnd",
                "BitwiseXor",
                "BitwiseOr",
                "LogicalAnd",
                "LogicalOr",
                "Comma",
                "Indexing",
            };

            printf(DEBUG_PRINT_FORMAT_STR_BEG "BinaryOperator %s\n", DEBUG_PRINT_ARGS_BEG,
                   BINARY_OPERATOR_TYPE_NAMES[(uint32_t)m_type]);
            ++indentLevel;
            Operands[0]->DebugPrint(indentLevel, "LeftOperand: ");
            Operands[1]->DebugPrint(indentLevel, "RightOperand: ");
        }

        Value BinaryOperator::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
        {
            // This operator is special, discards result of left operand.
            if(m_type == BinaryOperatorType::Comma)
            {
                Operands[0]->Execute(ctx);
                return Operands[1]->Evaluate(ctx, outThis);
            }

            // Operators that require l-value.
            switch(m_type)
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
                    // Getting these explicitly so the order of thier evaluation is defined, unlike in C++ function call arguments.
                    Value rhsVal = Operands[1]->Evaluate(ctx, nullptr);
                    LValue lhsLval = Operands[0]->GetLValue(ctx);
                    return Assignment(std::move(lhsLval), std::move(rhsVal));
                }
            }

            // Remaining operators use r-values.
            Value lhs = Operands[0]->Evaluate(ctx, nullptr);

            // Logical operators with short circuit for right hand side operand.
            if(m_type == BinaryOperatorType::LogicalAnd)
            {
                if(!lhs.IsTrue())
                    return lhs;
                return Operands[1]->Evaluate(ctx, nullptr);
            }
            if(m_type == BinaryOperatorType::LogicalOr)
            {
                if(lhs.IsTrue())
                    return lhs;
                return Operands[1]->Evaluate(ctx, nullptr);
            }

            // Remaining operators use both operands as r-values.
            Value rhs = Operands[1]->Evaluate(ctx, nullptr);

            const Value::Type lhsType = lhs.GetType();
            const Value::Type rhsType = rhs.GetType();

            // These ones support various types.
            if(m_type == BinaryOperatorType::Add)
            {
                if(lhsType == Value::Type::Number && rhsType == Value::Type::Number)
                    return Value{ lhs.GetNumber() + rhs.GetNumber() };
                if(lhsType == Value::Type::String && rhsType == Value::Type::String)
                    return Value{ lhs.GetString() + rhs.GetString() };
                MINSL_EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INCOMPATIBLE_TYPES);
            }
            if(m_type == BinaryOperatorType::Equal)
            {
                return Value{ lhs.IsEqual(rhs) ? 1.0 : 0.0 };
            }
            if(m_type == BinaryOperatorType::NotEqual)
            {
                return Value{ !lhs.IsEqual(rhs) ? 1.0 : 0.0 };
            }
            if(m_type == BinaryOperatorType::Less || m_type == BinaryOperatorType::LessEqual
               || m_type == BinaryOperatorType::Greater || m_type == BinaryOperatorType::GreaterEqual)
            {
                bool result = false;
                MINSL_EXECUTION_CHECK(lhsType == rhsType, GetPlace(), ERROR_MESSAGE_INCOMPATIBLE_TYPES);
                if(lhsType == Value::Type::Number)
                {
                    switch(m_type)
                    {
                        case BinaryOperatorType::Less:
                            result = lhs.GetNumber() < rhs.GetNumber();
                            break;
                        case BinaryOperatorType::LessEqual:
                            result = lhs.GetNumber() <= rhs.GetNumber();
                            break;
                        case BinaryOperatorType::Greater:
                            result = lhs.GetNumber() > rhs.GetNumber();
                            break;
                        case BinaryOperatorType::GreaterEqual:
                            result = lhs.GetNumber() >= rhs.GetNumber();
                            break;
                        default:
                            assert(0);
                    }
                }
                else if(lhsType == Value::Type::String)
                {
                    switch(m_type)
                    {
                        case BinaryOperatorType::Less:
                            result = lhs.GetString() < rhs.GetString();
                            break;
                        case BinaryOperatorType::LessEqual:
                            result = lhs.GetString() <= rhs.GetString();
                            break;
                        case BinaryOperatorType::Greater:
                            result = lhs.GetString() > rhs.GetString();
                            break;
                        case BinaryOperatorType::GreaterEqual:
                            result = lhs.GetString() >= rhs.GetString();
                            break;
                        default:
                            assert(0);
                    }
                }
                else
                    MINSL_EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_TYPE);
                return Value{ result ? 1.0 : 0.0 };
            }
            if(m_type == BinaryOperatorType::Indexing)
            {
                if(lhsType == Value::Type::String)
                {
                    MINSL_EXECUTION_CHECK(rhsType == Value::Type::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                    size_t index = 0;
                    MINSL_EXECUTION_CHECK(NumberToIndex(index, rhs.GetNumber()), GetPlace(), ERROR_MESSAGE_INVALID_INDEX);
                    MINSL_EXECUTION_CHECK(index < lhs.GetString().length(), GetPlace(), ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
                    return Value{ std::string(1, lhs.GetString()[index]) };
                }
                if(lhsType == Value::Type::Object)
                {
                    MINSL_EXECUTION_CHECK(rhsType == Value::Type::String, GetPlace(), ERROR_MESSAGE_EXPECTED_STRING);
                    if(Value* val = lhs.GetObject_()->TryGetValue(rhs.GetString()))
                    {
                        if(outThis)
                            *outThis = ThisType{ lhs.GetObjectPtr() };
                        return *val;
                    }
                    return {};
                }
                if(lhsType == Value::Type::Array)
                {
                    MINSL_EXECUTION_CHECK(rhsType == Value::Type::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                    size_t index;
                    MINSL_EXECUTION_CHECK(NumberToIndex(index, rhs.GetNumber()) && index < lhs.GetArray()->m_items.size(),
                                          GetPlace(), ERROR_MESSAGE_INVALID_INDEX);
                    return lhs.GetArray()->m_items[index];
                }
                MINSL_EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_TYPE);
            }

            // Remaining operators require numbers.
            CheckNumberOperand(Operands[0].get(), lhs);
            CheckNumberOperand(Operands[1].get(), rhs);

            switch(m_type)
            {
                case BinaryOperatorType::Mul:
                    return Value{ lhs.GetNumber() * rhs.GetNumber() };
                case BinaryOperatorType::Div:
                    return Value{ lhs.GetNumber() / rhs.GetNumber() };
                case BinaryOperatorType::Mod:
                    return Value{ fmod(lhs.GetNumber(), rhs.GetNumber()) };
                case BinaryOperatorType::Sub:
                    return Value{ lhs.GetNumber() - rhs.GetNumber() };
                case BinaryOperatorType::ShiftLeft:
                    return ShiftLeft(std::move(lhs), std::move(rhs));
                case BinaryOperatorType::ShiftRight:
                    return ShiftRight(std::move(lhs), std::move(rhs));
                case BinaryOperatorType::BitwiseAnd:
                    return Value{ (double)((int64_t)lhs.GetNumber() & (int64_t)rhs.GetNumber()) };
                case BinaryOperatorType::BitwiseXor:
                    return Value{ (double)((int64_t)lhs.GetNumber() ^ (int64_t)rhs.GetNumber()) };
                case BinaryOperatorType::BitwiseOr:
                    return Value{ (double)((int64_t)lhs.GetNumber() | (int64_t)rhs.GetNumber()) };
            }

            assert(0);
            return {};
        }

        LValue BinaryOperator::GetLValue(ExecuteContext& ctx) const
        {
            if(m_type == BinaryOperatorType::Indexing)
            {
                Value* leftValRef = Operands[0]->GetLValue(ctx).GetValueRef(GetPlace());
                const Value indexVal = Operands[1]->Evaluate(ctx, nullptr);
                if(leftValRef->GetType() == Value::Type::String)
                {
                    MINSL_EXECUTION_CHECK(indexVal.GetType() == Value::Type::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                    size_t charIndex;
                    MINSL_EXECUTION_CHECK(NumberToIndex(charIndex, indexVal.GetNumber()), GetPlace(), ERROR_MESSAGE_INVALID_INDEX);
                    return LValue{ StringCharacterLValue{ &leftValRef->GetString(), charIndex } };
                }
                if(leftValRef->GetType() == Value::Type::Object)
                {
                    MINSL_EXECUTION_CHECK(indexVal.GetType() == Value::Type::String, GetPlace(), ERROR_MESSAGE_EXPECTED_STRING);
                    return LValue{ ObjectMemberLValue{ leftValRef->GetObject_(), indexVal.GetString() } };
                }
                if(leftValRef->GetType() == Value::Type::Array)
                {
                    MINSL_EXECUTION_CHECK(indexVal.GetType() == Value::Type::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                    size_t itemIndex;
                    MINSL_EXECUTION_CHECK(NumberToIndex(itemIndex, indexVal.GetNumber()), GetPlace(), ERROR_MESSAGE_INVALID_INDEX);
                    return LValue{ ArrayItemLValue{ leftValRef->GetArray(), itemIndex } };
                }
            }
            return Operator::GetLValue(ctx);
        }

        Value BinaryOperator::ShiftLeft(const Value& lhs, const Value& rhs) const
        {
            const int64_t lhsInt = (int64_t)lhs.GetNumber();
            const int64_t rhsInt = (int64_t)rhs.GetNumber();
            const int64_t resultInt = lhsInt << rhsInt;
            return Value{ (double)resultInt };
        }

        Value BinaryOperator::ShiftRight(const Value& lhs, const Value& rhs) const
        {
            const int64_t lhsInt = (int64_t)lhs.GetNumber();
            const int64_t rhsInt = (int64_t)rhs.GetNumber();
            const int64_t resultInt = lhsInt >> rhsInt;
            return Value{ (double)resultInt };
        }

        Value BinaryOperator::Assignment(LValue&& lhs, Value&& rhs) const
        {
            // This one is able to create new value.
            if(m_type == BinaryOperatorType::Assignment)
            {
                Assign(lhs, Value{ rhs });
                return rhs;
            }

            // Others require existing value.
            Value* const lhsValPtr = lhs.GetValueRef(GetPlace());

            if(m_type == BinaryOperatorType::AssignmentAdd)
            {
                if(lhsValPtr->GetType() == Value::Type::Number && rhs.GetType() == Value::Type::Number)
                    lhsValPtr->ChangeNumber(lhsValPtr->GetNumber() + rhs.GetNumber());
                else if(lhsValPtr->GetType() == Value::Type::String && rhs.GetType() == Value::Type::String)
                    lhsValPtr->GetString() += rhs.GetString();
                else
                    MINSL_EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INCOMPATIBLE_TYPES);
                return *lhsValPtr;
            }

            // Remaining ones work on numbers only.
            MINSL_EXECUTION_CHECK(lhsValPtr->GetType() == Value::Type::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
            MINSL_EXECUTION_CHECK(rhs.GetType() == Value::Type::Number, GetPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
            switch(m_type)
            {
                case BinaryOperatorType::AssignmentSub:
                    lhsValPtr->ChangeNumber(lhsValPtr->GetNumber() - rhs.GetNumber());
                    break;
                case BinaryOperatorType::AssignmentMul:
                    lhsValPtr->ChangeNumber(lhsValPtr->GetNumber() * rhs.GetNumber());
                    break;
                case BinaryOperatorType::AssignmentDiv:
                    lhsValPtr->ChangeNumber(lhsValPtr->GetNumber() / rhs.GetNumber());
                    break;
                case BinaryOperatorType::AssignmentMod:
                    lhsValPtr->ChangeNumber(fmod(lhsValPtr->GetNumber(), rhs.GetNumber()));
                    break;
                case BinaryOperatorType::AssignmentShiftLeft:
                    *lhsValPtr = ShiftLeft(*lhsValPtr, rhs);
                    break;
                case BinaryOperatorType::AssignmentShiftRight:
                    *lhsValPtr = ShiftRight(*lhsValPtr, rhs);
                    break;
                case BinaryOperatorType::AssignmentBitwiseAnd:
                    lhsValPtr->ChangeNumber((double)((int64_t)lhsValPtr->GetNumber() & (int64_t)rhs.GetNumber()));
                    break;
                case BinaryOperatorType::AssignmentBitwiseXor:
                    lhsValPtr->ChangeNumber((double)((int64_t)lhsValPtr->GetNumber() ^ (int64_t)rhs.GetNumber()));
                    break;
                case BinaryOperatorType::AssignmentBitwiseOr:
                    lhsValPtr->ChangeNumber((double)((int64_t)lhsValPtr->GetNumber() | (int64_t)rhs.GetNumber()));
                    break;
                default:
                    assert(0);
            }

            return *lhsValPtr;
        }

        void TernaryOperator::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "TernaryOperator\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            Operands[0]->DebugPrint(indentLevel, "ConditionExpression: ");
            Operands[1]->DebugPrint(indentLevel, "TrueExpression: ");
            Operands[2]->DebugPrint(indentLevel, "FalseExpression: ");
        }

        Value TernaryOperator::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
        {
            return Operands[0]->Evaluate(ctx, nullptr).IsTrue() ? Operands[1]->Evaluate(ctx, outThis) :
                                                                  Operands[2]->Evaluate(ctx, outThis);
        }

        void CallOperator::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
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
            std::vector<Value> arguments(argCount);
            for(size_t i = 0; i < argCount; ++i)
                arguments[i] = Operands[i + 1]->Evaluate(ctx, nullptr);

            // Calling an object: Call its function under '' key.
            if(callee.GetType() == Value::Type::Object)
            {
                std::shared_ptr<Object> calleeObj = callee.GetObjectPtr();
                if(Value* defaultVal = calleeObj->TryGetValue(std::string{}); defaultVal && defaultVal->GetType() == Value::Type::Function)
                {
                    callee = *defaultVal;
                    th = ThisType{ std::move(calleeObj) };
                }
            }

            if(callee.GetType() == Value::Type::Function)
            {
                const AST::FunctionDefinition* const funcDef = callee.GetFunction();
                MINSL_EXECUTION_CHECK(argCount == funcDef->Parameters.size(), GetPlace(), ERROR_MESSAGE_INVALID_NUMBER_OF_ARGUMENTS);
                Object localScope;
                // Setup parameters
                for(size_t argIndex = 0; argIndex != argCount; ++argIndex)
                    localScope.GetOrCreateValue(funcDef->Parameters[argIndex]) = std::move(arguments[argIndex]);
                ExecuteContext::LocalScopePush localContextPush{ ctx, &localScope, std::move(th), GetPlace() };
                try
                {
                    callee.GetFunction()->Body.Execute(ctx);
                }
                catch(ReturnException& returnEx)
                {
                    return std::move(returnEx.thrownvalue);
                }
                catch(BreakException)
                {
                    MINSL_EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_BREAK_WITHOUT_LOOP);
                }
                catch(ContinueException)
                {
                    MINSL_EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_CONTINUE_WITHOUT_LOOP);
                }
                return {};
            }
            if(callee.GetType() == Value::Type::HostFunction)
                return callee.GetHostFunction()(ctx.m_env.GetOwner(), GetPlace(), std::move(arguments));
            if(callee.GetType() == Value::Type::SystemFunction)
            {
                switch(callee.GetSystemFunction())
                {
                    case SystemFunction::TypeOfFunc:
                        return BuiltInFunction_typeOf(ctx, GetPlace(), std::move(arguments));
                    case SystemFunction::MinFunc:
                        return BuiltInFunction_min(ctx, GetPlace(), std::move(arguments));
                    case SystemFunction::MaxFunc:
                        return BuiltInFunction_max(ctx, GetPlace(), std::move(arguments));
                    case SystemFunction::StringResizeFunc:
                        return BuiltInFunction_String_resize(ctx, GetPlace(), th, std::move(arguments));
                    case SystemFunction::ArrayAddFunc:
                        return BuiltInFunction_Array_add(ctx, GetPlace(), th, std::move(arguments));
                    case SystemFunction::ArrayInsertFunc:
                        return BuiltInFunction_Array_insert(ctx, GetPlace(), th, std::move(arguments));
                    case SystemFunction::ArrayRemoveFunc:
                        return BuiltInFunction_Array_remove(ctx, GetPlace(), th, std::move(arguments));
                    default:
                        assert(0);
                        return {};
                }
            }
            if(callee.GetType() == Value::Type::Type)
            {
                switch(callee.GetTypeValue())
                {
                    case Value::Type::Null:
                        return BuiltInTypeCtor_Null(ctx, GetPlace(), std::move(arguments));
                    case Value::Type::Number:
                        return BuiltInTypeCtor_Number(ctx, GetPlace(), std::move(arguments));
                    case Value::Type::String:
                        return BuiltInTypeCtor_String(ctx, GetPlace(), std::move(arguments));
                    case Value::Type::Object:
                        return BuiltInTypeCtor_Object(ctx, GetPlace(), std::move(arguments));
                    case Value::Type::Array:
                        return BuiltInTypeCtor_Array(ctx, GetPlace(), std::move(arguments));
                    case Value::Type::Type:
                        return BuiltInTypeCtor_Type(ctx, GetPlace(), std::move(arguments));
                    case Value::Type::Function:
                    case Value::Type::SystemFunction:
                        return BuiltInTypeCtor_Function(ctx, GetPlace(), std::move(arguments));
                    default:
                        assert(0);
                        return {};
                }
            }

            MINSL_EXECUTION_FAIL(GetPlace(), ERROR_MESSAGE_INVALID_FUNCTION);
        }

        void FunctionDefinition::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
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

        void ObjectExpression::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Object\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            for(const auto& [name, value] : m_items)
                value->DebugPrint(indentLevel, name);
        }

        Value ObjectExpression::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
        {
            std::shared_ptr<Object> obj;
            if(BaseExpression)
            {
                Value baseObj = BaseExpression->Evaluate(ctx, nullptr);
                if(baseObj.GetType() != Value::Type::Object)
                    throw ExecutionError{ GetPlace(), ERROR_MESSAGE_BASE_MUST_BE_OBJECT };
                obj = CopyObject(*baseObj.GetObject_());
            }
            else
                obj = std::make_shared<Object>();
            for(const auto& [name, valueExpr] : m_items)
            {
                Value val = valueExpr->Evaluate(ctx, nullptr);
                if(val.GetType() != Value::Type::Null)
                    obj->GetOrCreateValue(name) = std::move(val);
                else if(BaseExpression)
                    obj->Remove(name);
            }
            return Value{ std::move(obj) };
        }

        void ArrayExpression::DebugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Array\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            std::string itemPrefix;
            for(size_t i = 0, count = m_items.size(); i < count; ++i)
            {
                itemPrefix = Format("%zu: ", i);
                m_items[i]->DebugPrint(indentLevel, itemPrefix);
            }
        }

        Value ArrayExpression::Evaluate(ExecuteContext& ctx, ThisType* outThis) const
        {
            auto result = std::make_shared<Array>();
            for(const auto& item : m_items)
                result->m_items.push_back(item->Evaluate(ctx, nullptr));
            return Value{ std::move(result) };
        }

    }// namespace AST

    ////////////////////////////////////////////////////////////////////////////////
    // Parser implementation

    #define MUST_PARSE(result, errorMessage)                                \
        do                                                                  \
        {                                                                   \
            if(!(result))                                                   \
                throw ParsingError(GetCurrentTokenPlace(), (errorMessage)); \
        } while(false)

    void Parser::ParseScript(AST::Script& outScript)
    {
        for(;;)
        {
            Token token;
            m_tokenizer.GetNextToken(token);
            const bool isEnd = token.m_symtype == Token::Type::End;
            if(token.m_symtype == Token::Type::String && !m_toklist.empty() && m_toklist.back().m_symtype == Token::Type::String)
                m_toklist.back().m_stringval += token.m_stringval;
            else
                m_toklist.push_back(std::move(token));
            if(isEnd)
                break;
        }

        ParseBlock(outScript);
        if(m_toklist[m_tokidx].m_symtype != Token::Type::End)
            throw ParsingError(GetCurrentTokenPlace(), ERROR_MESSAGE_PARSING_ERROR);

        //outScript.DebugPrint(0, "Script: "); // #DELME
    }

    void Parser::ParseBlock(AST::Block& outBlock)
    {
        while(m_toklist[m_tokidx].m_symtype != Token::Type::End)
        {
            std::unique_ptr<AST::Statement> stmt = TryParseStatement();
            if(!stmt)
                break;
            outBlock.m_statements.push_back(std::move(stmt));
        }
    }

    bool Parser::TryParseSwitchItem(AST::SwitchStatement& switchStatement)
    {
        const PlaceInCode place = GetCurrentTokenPlace();
        // 'default' ':' Block
        if(TryParseSymbol(Token::Type::Default))
        {
            MUST_PARSE(TryParseSymbol(Token::Type::Colon), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON);
            switchStatement.ItemValues.push_back(std::unique_ptr<AST::ConstantValue>{});
            switchStatement.ItemBlocks.push_back(std::make_unique<AST::Block>(place));
            ParseBlock(*switchStatement.ItemBlocks.back());
            return true;
        }
        // 'case' ConstantExpr ':' Block
        if(TryParseSymbol(Token::Type::Case))
        {
            std::unique_ptr<AST::ConstantValue> constVal;
            MUST_PARSE(constVal = TryParseConstantValue(), ERROR_MESSAGE_EXPECTED_CONSTANT_VALUE);
            switchStatement.ItemValues.push_back(std::move(constVal));
            MUST_PARSE(TryParseSymbol(Token::Type::Colon), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON);
            switchStatement.ItemBlocks.push_back(std::make_unique<AST::Block>(place));
            ParseBlock(*switchStatement.ItemBlocks.back());
            return true;
        }
        return false;
    }

    void Parser::ParseFunctionDefinition(AST::FunctionDefinition& funcDef)
    {
        MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
        if(m_toklist[m_tokidx].m_symtype == Token::Type::Identifier)
        {
            funcDef.Parameters.push_back(m_toklist[m_tokidx++].m_stringval);
            while(TryParseSymbol(Token::Type::Comma))
            {
                MUST_PARSE(m_toklist[m_tokidx].m_symtype == Token::Type::Identifier, ERROR_MESSAGE_EXPECTED_IDENTIFIER);
                funcDef.Parameters.push_back(m_toklist[m_tokidx++].m_stringval);
            }
        }
        MUST_PARSE(funcDef.AreParameterNamesUnique(), ERROR_MESSAGE_PARAMETER_NAMES_MUST_BE_UNIQUE);
        MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
        MUST_PARSE(TryParseSymbol(Token::Type::CurlyBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_OPEN);
        ParseBlock(funcDef.Body);
        MUST_PARSE(TryParseSymbol(Token::Type::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE);
    }

    bool Parser::PeekSymbols(std::initializer_list<Token::Type> symbols)
    {
        if(m_tokidx + symbols.size() >= m_toklist.size())
            return false;
        for(size_t i = 0; i < symbols.size(); ++i)
            if(m_toklist[m_tokidx + i].m_symtype != symbols.begin()[i])
                return false;
        return true;
    }

    std::unique_ptr<AST::Statement> Parser::TryParseStatement()
    {
        const PlaceInCode place = GetCurrentTokenPlace();

        // Empty statement: ';'
        if(TryParseSymbol(Token::Type::Semicolon))
            return std::make_unique<AST::EmptyStatement>(place);

        // Block: '{' Block '}'
        if(!PeekSymbols({ Token::Type::CurlyBracketOpen, Token::Type::String, Token::Type::Colon }) && TryParseSymbol(Token::Type::CurlyBracketOpen))
        {
            auto block = std::make_unique<AST::Block>(GetCurrentTokenPlace());
            ParseBlock(*block);
            MUST_PARSE(TryParseSymbol(Token::Type::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE);
            return block;
        }

        // Condition: 'if' '(' Expr17 ')' Statement [ 'else' Statement ]
        if(TryParseSymbol(Token::Type::If))
        {
            MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
            auto condition = std::make_unique<AST::Condition>(place);
            MUST_PARSE(condition->m_condexpr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
            MUST_PARSE(condition->m_statements[0] = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            if(TryParseSymbol(Token::Type::Else))
                MUST_PARSE(condition->m_statements[1] = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            return condition;
        }

        // Loop: 'while' '(' Expr17 ')' Statement
        if(TryParseSymbol(Token::Type::While))
        {
            MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
            auto loop = std::make_unique<AST::WhileLoop>(place, AST::WhileLoop::Type::While);
            MUST_PARSE(loop->m_condexpr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
            MUST_PARSE(loop->Body = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            return loop;
        }

        // Loop: 'do' Statement 'while' '(' Expr17 ')' ';'    - loop
        if(TryParseSymbol(Token::Type::Do))
        {
            auto loop = std::make_unique<AST::WhileLoop>(place, AST::WhileLoop::Type::DoWhile);
            MUST_PARSE(loop->Body = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            MUST_PARSE(TryParseSymbol(Token::Type::While), ERROR_MESSAGE_EXPECTED_SYMBOL_WHILE);
            MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
            MUST_PARSE(loop->m_condexpr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
            MUST_PARSE(TryParseSymbol(Token::Type::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON);
            return loop;
        }

        if(TryParseSymbol(Token::Type::For))
        {
            MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
            // Range-based loop: 'for' '(' TOKEN_IDENTIFIER [ ',' TOKEN_IDENTIFIER ] ':' Expr17 ')' Statement
            if(PeekSymbols({ Token::Type::Identifier, Token::Type::Colon })
               || PeekSymbols({ Token::Type::Identifier, Token::Type::Comma, Token::Type::Identifier, Token::Type::Colon }))
            {
                auto loop = std::make_unique<AST::RangeBasedForLoop>(place);
                loop->ValueVarName = TryParseIdentifier();
                MUST_PARSE(!loop->ValueVarName.empty(), ERROR_MESSAGE_EXPECTED_IDENTIFIER);
                if(TryParseSymbol(Token::Type::Comma))
                {
                    loop->KeyVarName = std::move(loop->ValueVarName);
                    loop->ValueVarName = TryParseIdentifier();
                    MUST_PARSE(!loop->ValueVarName.empty(), ERROR_MESSAGE_EXPECTED_IDENTIFIER);
                }
                MUST_PARSE(TryParseSymbol(Token::Type::Colon), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON);
                MUST_PARSE(loop->RangeExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON);
                MUST_PARSE(loop->Body = TryParseStatement(), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON);
                return loop;
            }
            // Loop: 'for' '(' Expr17? ';' Expr17? ';' Expr17? ')' Statement
            else
            {
                auto loop = std::make_unique<AST::ForLoop>(place);
                if(!TryParseSymbol(Token::Type::Semicolon))
                {
                    MUST_PARSE(loop->InitExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                    MUST_PARSE(TryParseSymbol(Token::Type::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON);
                }
                if(!TryParseSymbol(Token::Type::Semicolon))
                {
                    MUST_PARSE(loop->m_condexpr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                    MUST_PARSE(TryParseSymbol(Token::Type::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON);
                }
                if(!TryParseSymbol(Token::Type::RoundBracketClose))
                {
                    MUST_PARSE(loop->IterationExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                    MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
                }
                MUST_PARSE(loop->Body = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
                return loop;
            }
        }

        // 'break' ';'
        if(TryParseSymbol(Token::Type::Break))
        {
            MUST_PARSE(TryParseSymbol(Token::Type::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON);
            return std::make_unique<AST::LoopBreakStatement>(place, AST::LoopBreakType::Break);
        }

        // 'continue' ';'
        if(TryParseSymbol(Token::Type::Continue))
        {
            MUST_PARSE(TryParseSymbol(Token::Type::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON);
            return std::make_unique<AST::LoopBreakStatement>(place, AST::LoopBreakType::Continue);
        }

        // 'return' [ Expr17 ] ';'
        if(TryParseSymbol(Token::Type::Return))
        {
            auto stmt = std::make_unique<AST::ReturnStatement>(place);
            stmt->ReturnedValue = TryParseExpr17();
            MUST_PARSE(TryParseSymbol(Token::Type::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON);
            return stmt;
        }

        // 'switch' '(' Expr17 ')' '{' SwitchItem+ '}'
        if(TryParseSymbol(Token::Type::Switch))
        {
            auto stmt = std::make_unique<AST::SwitchStatement>(place);
            MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
            MUST_PARSE(stmt->Condition = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
            MUST_PARSE(TryParseSymbol(Token::Type::CurlyBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_OPEN);
            while(TryParseSwitchItem(*stmt))
            {
            }
            MUST_PARSE(TryParseSymbol(Token::Type::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE);
            // Check uniqueness. Warning: O(n^2) complexity!
            for(size_t i = 0, count = stmt->ItemValues.size(); i < count; ++i)
            {
                for(size_t j = i + 1; j < count; ++j)
                {
                    if(j != i)
                    {
                        if(!stmt->ItemValues[i] && !stmt->ItemValues[j])// 2x default
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
        if(TryParseSymbol(Token::Type::Throw))
        {
            auto stmt = std::make_unique<AST::ThrowStatement>(place);
            MUST_PARSE(stmt->ThrownExpression = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(TryParseSymbol(Token::Type::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON);
            return stmt;
        }

        // 'try' Statement ( ( 'catch' '(' TOKEN_IDENTIFIER ')' Statement [ 'finally' Statement ] ) | ( 'finally' Statement ) )
        if(TryParseSymbol(Token::Type::Try))
        {
            auto stmt = std::make_unique<AST::TryStatement>(place);
            MUST_PARSE(stmt->TryBlock = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            if(TryParseSymbol(Token::Type::Finally))
                MUST_PARSE(stmt->FinallyBlock = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            else
            {
                MUST_PARSE(TryParseSymbol(Token::Type::Catch), "Expected 'catch' or 'finally'.");
                MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
                MUST_PARSE(!(stmt->ExceptionVarName = TryParseIdentifier()).empty(), ERROR_MESSAGE_EXPECTED_IDENTIFIER);
                MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
                MUST_PARSE(stmt->CatchBlock = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
                if(TryParseSymbol(Token::Type::Finally))
                    MUST_PARSE(stmt->FinallyBlock = TryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            }
            return stmt;
        }

        // Expression as statement: Expr17 ';'
        std::unique_ptr<AST::Expression> expr = TryParseExpr17();
        if(expr)
        {
            MUST_PARSE(TryParseSymbol(Token::Type::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON);
            return expr;
        }

        // Syntactic sugar for functions:
        // 'function' IdentifierValue '(' [ TOKEN_IDENTIFIER ( ',' TOKE_IDENTIFIER )* ] ')' '{' Block '}'
        if(auto fnSyntacticSugar = TryParseFunctionSyntacticSugar(); fnSyntacticSugar.second)
        {
            auto identifierExpr
            = std::make_unique<AST::Identifier>(place, AST::IdentifierScope::None, std::move(fnSyntacticSugar.first));
            auto assignmentOp = std::make_unique<AST::BinaryOperator>(place, AST::BinaryOperatorType::Assignment);
            assignmentOp->Operands[0] = std::move(identifierExpr);
            assignmentOp->Operands[1] = std::move(fnSyntacticSugar.second);
            return assignmentOp;
        }

        if(auto cls = TryParseClassSyntacticSugar(); cls)
            return cls;

        return {};
    }

    std::unique_ptr<AST::ConstantValue> Parser::TryParseConstantValue()
    {
        const Token& t = m_toklist[m_tokidx];
        switch(t.m_symtype)
        {
            case Token::Type::Number:
                ++m_tokidx;
                return std::make_unique<AST::ConstantValue>(t.m_place, Value{ t.m_numval });
            case Token::Type::String:
                ++m_tokidx;
                return std::make_unique<AST::ConstantValue>(t.m_place, Value{ std::string(t.m_stringval) });
            case Token::Type::Null:
                ++m_tokidx;
                return std::make_unique<AST::ConstantValue>(t.m_place, Value{});
            case Token::Type::False:
                ++m_tokidx;
                return std::make_unique<AST::ConstantValue>(t.m_place, Value{ 0.0 });
            case Token::Type::True:
                ++m_tokidx;
                return std::make_unique<AST::ConstantValue>(t.m_place, Value{ 1.0 });
        }
        return {};
    }

    std::unique_ptr<AST::Identifier> Parser::TryParseIdentifierValue()
    {
        const Token& t = m_toklist[m_tokidx];
        if(t.m_symtype == Token::Type::Local || t.m_symtype == Token::Type::Global)
        {
            ++m_tokidx;
            MUST_PARSE(TryParseSymbol(Token::Type::Dot), ERROR_MESSAGE_EXPECTED_SYMBOL_DOT);
            MUST_PARSE(m_tokidx < m_toklist.size(), ERROR_MESSAGE_EXPECTED_IDENTIFIER);
            const Token& tIdentifier = m_toklist[m_tokidx++];
            MUST_PARSE(tIdentifier.m_symtype == Token::Type::Identifier, ERROR_MESSAGE_EXPECTED_IDENTIFIER);
            AST::IdentifierScope identifierScope = AST::IdentifierScope::Count;
            switch(t.m_symtype)
            {
                case Token::Type::Local:
                    identifierScope = AST::IdentifierScope::Local;
                    break;
                case Token::Type::Global:
                    identifierScope = AST::IdentifierScope::Global;
                    break;
            }
            return std::make_unique<AST::Identifier>(t.m_place, identifierScope, std::string(tIdentifier.m_stringval));
        }
        if(t.m_symtype == Token::Type::Identifier)
        {
            ++m_tokidx;
            return std::make_unique<AST::Identifier>(t.m_place, AST::IdentifierScope::None, std::string(t.m_stringval));
        }
        return {};
    }

    std::unique_ptr<AST::ConstantExpression> Parser::TryParseConstantExpr()
    {
        if(auto r = TryParseConstantValue())
            return r;
        if(auto r = TryParseIdentifierValue())
            return r;
        const PlaceInCode place = m_toklist[m_tokidx].m_place;
        if(TryParseSymbol(Token::Type::This))
            return std::make_unique<AST::ThisExpression>(place);
        return {};
    }

    std::pair<std::string, std::unique_ptr<AST::FunctionDefinition>> Parser::TryParseFunctionSyntacticSugar()
    {
        if(PeekSymbols({ Token::Type::Function, Token::Type::Identifier }))
        {
            ++m_tokidx;
            std::pair<std::string, std::unique_ptr<AST::FunctionDefinition>> result;
            result.first = m_toklist[m_tokidx++].m_stringval;
            result.second = std::make_unique<AST::FunctionDefinition>(GetCurrentTokenPlace());
            ParseFunctionDefinition(*result.second);
            return result;
        }
        return std::make_pair(std::string{}, std::unique_ptr<AST::FunctionDefinition>{});
    }

    std::unique_ptr<AST::Expression> Parser::TryParseObjectMember(std::string& outMemberName)
    {
        if(PeekSymbols({ Token::Type::String, Token::Type::Colon }) || PeekSymbols({ Token::Type::Identifier, Token::Type::Colon }))
        {
            outMemberName = m_toklist[m_tokidx].m_stringval;
            m_tokidx += 2;
            return TryParseExpr16();
        }
        return {};
    }

    std::unique_ptr<AST::Expression> Parser::TryParseClassSyntacticSugar()
    {
        const PlaceInCode beginPlace = GetCurrentTokenPlace();
        if(TryParseSymbol(Token::Type::Class))
        {
            std::string className = TryParseIdentifier();
            MUST_PARSE(!className.empty(), ERROR_MESSAGE_EXPECTED_IDENTIFIER);
            auto assignmentOp = std::make_unique<AST::BinaryOperator>(beginPlace, AST::BinaryOperatorType::Assignment);
            assignmentOp->Operands[0]
            = std::make_unique<AST::Identifier>(beginPlace, AST::IdentifierScope::None, std::move(className));
            std::unique_ptr<AST::Expression> baseExpr;
            if(TryParseSymbol(Token::Type::Colon))
            {
                baseExpr = TryParseExpr16();
                MUST_PARSE(baseExpr, ERROR_MESSAGE_EXPECTED_EXPRESSION);
            }
            auto objExpr = TryParseObject();
            objExpr->BaseExpression = std::move(baseExpr);
            assignmentOp->Operands[1] = std::move(objExpr);
            MUST_PARSE(assignmentOp->Operands[1], ERROR_MESSAGE_EXPECTED_OBJECT);
            return assignmentOp;
        }
        return std::unique_ptr<AST::ObjectExpression>();
    }

    std::unique_ptr<AST::ObjectExpression> Parser::TryParseObject()
    {
        if(PeekSymbols({ Token::Type::CurlyBracketOpen, Token::Type::CurlyBracketClose }) ||// { }
           PeekSymbols({ Token::Type::CurlyBracketOpen, Token::Type::String, Token::Type::Colon }) ||// { 'key' :
           PeekSymbols({ Token::Type::CurlyBracketOpen, Token::Type::Identifier, Token::Type::Colon }))// { key :
        {
            auto objExpr = std::make_unique<AST::ObjectExpression>(GetCurrentTokenPlace());
            TryParseSymbol(Token::Type::CurlyBracketOpen);
            if(!TryParseSymbol(Token::Type::CurlyBracketClose))
            {
                std::string memberName;
                std::unique_ptr<AST::Expression> memberValue;
                MUST_PARSE(memberValue = TryParseObjectMember(memberName), ERROR_MESSAGE_EXPECTED_OBJECT_MEMBER);
                MUST_PARSE(objExpr->m_items.insert(std::make_pair(std::move(memberName), std::move(memberValue))).second,
                           ERROR_MESSAGE_REPEATING_KEY_IN_OBJECT);
                if(!TryParseSymbol(Token::Type::CurlyBracketClose))
                {
                    while(TryParseSymbol(Token::Type::Comma))
                    {
                        if(TryParseSymbol(Token::Type::CurlyBracketClose))
                            return objExpr;
                        MUST_PARSE(memberValue = TryParseObjectMember(memberName), ERROR_MESSAGE_EXPECTED_OBJECT_MEMBER);
                        MUST_PARSE(objExpr->m_items.insert(std::make_pair(std::move(memberName), std::move(memberValue))).second,
                                   ERROR_MESSAGE_REPEATING_KEY_IN_OBJECT);
                    }
                    MUST_PARSE(TryParseSymbol(Token::Type::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE);
                }
            }
            return objExpr;
        }
        return {};
    }

    std::unique_ptr<AST::ArrayExpression> Parser::TryParseArray()
    {
        if(TryParseSymbol(Token::Type::SquareBracketOpen))
        {
            auto arrExpr = std::make_unique<AST::ArrayExpression>(GetCurrentTokenPlace());
            if(!TryParseSymbol(Token::Type::SquareBracketClose))
            {
                std::unique_ptr<AST::Expression> itemValue;
                MUST_PARSE(itemValue = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                arrExpr->m_items.push_back((std::move(itemValue)));
                if(!TryParseSymbol(Token::Type::SquareBracketClose))
                {
                    while(TryParseSymbol(Token::Type::Comma))
                    {
                        if(TryParseSymbol(Token::Type::SquareBracketClose))
                            return arrExpr;
                        MUST_PARSE(itemValue = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                        arrExpr->m_items.push_back((std::move(itemValue)));
                    }
                    MUST_PARSE(TryParseSymbol(Token::Type::SquareBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_SQUARE_BRACKET_CLOSE);
                }
            }
            return arrExpr;
        }
        return {};
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr0()
    {
        const PlaceInCode place = GetCurrentTokenPlace();

        // '(' Expr17 ')'
        if(TryParseSymbol(Token::Type::RoundBracketOpen))
        {
            std::unique_ptr<AST::Expression> expr;
            MUST_PARSE(expr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
            return expr;
        }

        if(auto objExpr = TryParseObject())
            return objExpr;
        if(auto arrExpr = TryParseArray())
            return arrExpr;

        // 'function' '(' [ TOKEN_IDENTIFIER ( ',' TOKE_IDENTIFIER )* ] ')' '{' Block '}'
        if(m_toklist[m_tokidx].m_symtype == Token::Type::Function && m_toklist[m_tokidx + 1].m_symtype == Token::Type::RoundBracketOpen)
        {
            ++m_tokidx;
            auto func = std::make_unique<AST::FunctionDefinition>(place);
            ParseFunctionDefinition(*func);
            return func;
        }

        // Constant
        std::unique_ptr<AST::ConstantExpression> constant = TryParseConstantExpr();
        if(constant)
            return constant;

        return {};
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr2()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr0();
        if(!expr)
            return {};
        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            // Postincrementation: Expr0 '++'
            if(TryParseSymbol(Token::Type::DoublePlus))
            {
                auto op = std::make_unique<AST::UnaryOperator>(place, AST::UnaryOperatorType::Postincrementation);
                op->Operand = std::move(expr);
                expr = std::move(op);
            }
            // Postdecrementation: Expr0 '--'
            else if(TryParseSymbol(Token::Type::DoubleDash))
            {
                auto op = std::make_unique<AST::UnaryOperator>(place, AST::UnaryOperatorType::Postdecrementation);
                op->Operand = std::move(expr);
                expr = std::move(op);
            }
            // Call: Expr0 '(' [ Expr16 ( ',' Expr16 )* ')'
            else if(TryParseSymbol(Token::Type::RoundBracketOpen))
            {
                auto op = std::make_unique<AST::CallOperator>(place);
                // Callee
                op->Operands.push_back(std::move(expr));
                // First argument
                expr = TryParseExpr16();
                if(expr)
                {
                    op->Operands.push_back(std::move(expr));
                    // Further arguments
                    while(TryParseSymbol(Token::Type::Comma))
                    {
                        MUST_PARSE(expr = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                        op->Operands.push_back(std::move(expr));
                    }
                }
                MUST_PARSE(TryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
                expr = std::move(op);
            }
            // Indexing: Expr0 '[' Expr17 ']'
            else if(TryParseSymbol(Token::Type::SquareBracketOpen))
            {
                auto op = std::make_unique<AST::BinaryOperator>(place, AST::BinaryOperatorType::Indexing);
                op->Operands[0] = std::move(expr);
                MUST_PARSE(op->Operands[1] = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                MUST_PARSE(TryParseSymbol(Token::Type::SquareBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_SQUARE_BRACKET_CLOSE);
                expr = std::move(op);
            }
            // Member access: Expr2 '.' TOKEN_IDENTIFIER
            else if(TryParseSymbol(Token::Type::Dot))
            {
                auto op = std::make_unique<AST::MemberAccessOperator>(place);
                op->Operand = std::move(expr);
                auto identifier = TryParseIdentifierValue();
                MUST_PARSE(identifier && identifier->Scope == AST::IdentifierScope::None, ERROR_MESSAGE_EXPECTED_IDENTIFIER);
                op->MemberName = std::move(identifier->m_ident);
                expr = std::move(op);
            }
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr3()
    {
        const PlaceInCode place = GetCurrentTokenPlace();
    #define PARSE_UNARY_OPERATOR(symbol, unaryOperatorType)                               \
        if(TryParseSymbol(symbol))                                                        \
        {                                                                                 \
            auto op = std::make_unique<AST::UnaryOperator>(place, (unaryOperatorType));        \
            MUST_PARSE(op->Operand = TryParseExpr3(), ERROR_MESSAGE_EXPECTED_EXPRESSION); \
            return op;                                                                    \
        }
        PARSE_UNARY_OPERATOR(Token::Type::DoublePlus, AST::UnaryOperatorType::Preincrementation)
        PARSE_UNARY_OPERATOR(Token::Type::DoubleDash, AST::UnaryOperatorType::Predecrementation)
        PARSE_UNARY_OPERATOR(Token::Type::Plus, AST::UnaryOperatorType::Plus)
        PARSE_UNARY_OPERATOR(Token::Type::Dash, AST::UnaryOperatorType::Minus)
        PARSE_UNARY_OPERATOR(Token::Type::ExclamationMark, AST::UnaryOperatorType::LogicalNot)
        PARSE_UNARY_OPERATOR(Token::Type::Tilde, AST::UnaryOperatorType::BitwiseNot)
    #undef PARSE_UNARY_OPERATOR
        // Just Expr2 or null if failed.
        return TryParseExpr2();
    }

    #define PARSE_BINARY_OPERATOR(binaryOperatorType, exprParseFunc)                            \
        {                                                                                       \
            auto op = std::make_unique<AST::BinaryOperator>(place, (binaryOperatorType));            \
            op->Operands[0] = std::move(expr);                                                  \
            MUST_PARSE(op->Operands[1] = (exprParseFunc)(), ERROR_MESSAGE_EXPECTED_EXPRESSION); \
            expr = std::move(op);                                                               \
        }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr5()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr3();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Token::Type::Asterisk))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Mul, TryParseExpr3)
            else if(TryParseSymbol(Token::Type::Slash))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Div, TryParseExpr3)
            else if(TryParseSymbol(Token::Type::Percent))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Mod, TryParseExpr3)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr6()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr5();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Token::Type::Plus))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Add, TryParseExpr5)
            else if(TryParseSymbol(Token::Type::Dash))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Sub, TryParseExpr5)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr7()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr6();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Token::Type::DoubleLess))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::ShiftLeft, TryParseExpr6)
            else if(TryParseSymbol(Token::Type::DoubleGreater))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::ShiftRight, TryParseExpr6)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr9()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr7();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Token::Type::Less))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Less, TryParseExpr7)
            else if(TryParseSymbol(Token::Type::LessEquals))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::LessEqual, TryParseExpr7)
            else if(TryParseSymbol(Token::Type::Greater))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Greater, TryParseExpr7)
            else if(TryParseSymbol(Token::Type::GreaterEquals))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::GreaterEqual, TryParseExpr7)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr10()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr9();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Token::Type::DoubleEquals))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Equal, TryParseExpr9)
            else if(TryParseSymbol(Token::Type::ExclamationEquals))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::NotEqual, TryParseExpr9)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr11()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr10();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Token::Type::Amperstand))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::BitwiseAnd, TryParseExpr10)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr12()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr11();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Token::Type::Caret))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::BitwiseXor, TryParseExpr11)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr13()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr12();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Token::Type::Pipe))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::BitwiseOr, TryParseExpr12)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr14()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr13();
        if(!expr)
            return {};
        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Token::Type::DoubleAmperstand))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::LogicalAnd, TryParseExpr13)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr15()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr14();
        if(!expr)
            return {};
        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Token::Type::DoublePipe))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::LogicalOr, TryParseExpr14)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr16()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr15();
        if(!expr)
            return {};

        // Ternary operator: Expr15 '?' Expr16 ':' Expr16
        if(TryParseSymbol(Token::Type::QuestionMark))
        {
            auto op = std::make_unique<AST::TernaryOperator>(GetCurrentTokenPlace());
            op->Operands[0] = std::move(expr);
            MUST_PARSE(op->Operands[1] = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(TryParseSymbol(Token::Type::Colon), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON);
            MUST_PARSE(op->Operands[2] = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            return op;
        }
        // Assignment: Expr15 = Expr16, and variants like += -=
    #define TRY_PARSE_ASSIGNMENT(symbol, binaryOperatorType)                                          \
        if(TryParseSymbol(symbol))                                                                    \
        {                                                                                             \
            auto op = std::make_unique<AST::BinaryOperator>(GetCurrentTokenPlace(), (binaryOperatorType)); \
            op->Operands[0] = std::move(expr);                                                        \
            MUST_PARSE(op->Operands[1] = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION);        \
            return op;                                                                                \
        }
        TRY_PARSE_ASSIGNMENT(Token::Type::Equals, AST::BinaryOperatorType::Assignment)
        TRY_PARSE_ASSIGNMENT(Token::Type::PlusEquals, AST::BinaryOperatorType::AssignmentAdd)
        TRY_PARSE_ASSIGNMENT(Token::Type::DashEquals, AST::BinaryOperatorType::AssignmentSub)
        TRY_PARSE_ASSIGNMENT(Token::Type::AsteriskEquals, AST::BinaryOperatorType::AssignmentMul)
        TRY_PARSE_ASSIGNMENT(Token::Type::SlashEquals, AST::BinaryOperatorType::AssignmentDiv)
        TRY_PARSE_ASSIGNMENT(Token::Type::PercentEquals, AST::BinaryOperatorType::AssignmentMod)
        TRY_PARSE_ASSIGNMENT(Token::Type::DoubleLessEquals, AST::BinaryOperatorType::AssignmentShiftLeft)
        TRY_PARSE_ASSIGNMENT(Token::Type::DoubleGreaterEquals, AST::BinaryOperatorType::AssignmentShiftRight)
        TRY_PARSE_ASSIGNMENT(Token::Type::AmperstandEquals, AST::BinaryOperatorType::AssignmentBitwiseAnd)
        TRY_PARSE_ASSIGNMENT(Token::Type::CaretEquals, AST::BinaryOperatorType::AssignmentBitwiseXor)
        TRY_PARSE_ASSIGNMENT(Token::Type::PipeEquals, AST::BinaryOperatorType::AssignmentBitwiseOr)
    #undef TRY_PARSE_ASSIGNMENT
        // Just Expr15
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr17()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr16();
        if(!expr)
            return {};
        for(;;)
        {
            const PlaceInCode place = GetCurrentTokenPlace();
            if(TryParseSymbol(Token::Type::Comma))
                PARSE_BINARY_OPERATOR(AST::BinaryOperatorType::Comma, TryParseExpr16)
            else
                break;
        }
        return expr;
    }

    bool Parser::TryParseSymbol(Token::Type symbol)
    {
        if(m_tokidx < m_toklist.size() && m_toklist[m_tokidx].m_symtype == symbol)
        {
            ++m_tokidx;
            return true;
        }
        return false;
    }

    std::string Parser::TryParseIdentifier()
    {
        if(m_tokidx < m_toklist.size() && m_toklist[m_tokidx].m_symtype == Token::Type::Identifier)
            return m_toklist[m_tokidx++].m_stringval;
        return {};
    }

    ////////////////////////////////////////////////////////////////////////////////
    // EnvironmentPimpl implementation

    Value EnvironmentPimpl::Execute(const std::string_view& code)
    {
        AST::Script script{ PlaceInCode{ 0, 1, 1 } };
        {
            Tokenizer tokenizer{ code };
            Parser parser{ tokenizer };
            parser.ParseScript(script);
        }

        try
        {
            AST::ExecuteContext executeContext{ *this, m_globalscope };
            script.Execute(executeContext);
        }
        catch(ReturnException& returnEx)
        {
            return std::move(returnEx.thrownvalue);
        }
        return {};
    }

    std::string_view EnvironmentPimpl::GetTypeName(Value::Type type) const
    {
        return VALUE_TYPE_NAMES[(size_t)type];
        // TODO support custom types
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Environment

    Environment::Environment() : m_implenv{ new EnvironmentPimpl{ *this, GlobalScope } }
    {
            GlobalScope. GetOrCreateValue("print") = Value{BuiltInFunction_print};
    }

    Environment::~Environment()
    {
        delete m_implenv;
    }
    Value Environment::Execute(const std::string_view& code)
    {
        return m_implenv->Execute(code);
    }

    std::string_view Environment::GetTypeName(Value::Type type) const
    {
        return m_implenv->GetTypeName(type);
    }

    void Environment::Print(const std::string_view& s)
    {
        m_implenv->Print(s);
    }
}
