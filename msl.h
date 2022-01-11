
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
            inline Error(const PlaceInCode& place) : m_place{ place }
            {
            }

            inline const PlaceInCode& getPlace() const
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
            inline ParsingError(const PlaceInCode& place, const std::string_view& message) : Error{ place }, m_message{ message }
            {
            }

            inline virtual std::string_view getMessage() const override
            {
                return m_message;
            }
    };

    class ExecutionError : public Error
    {
        private:
            const std::string m_message;

        public:
            inline ExecutionError(const PlaceInCode& place, const std::string_view& message) : Error{ place }, m_message{ message }
            {
            }

            inline virtual std::string_view getMessage() const override
            {
                return m_message;
            }
    };

    namespace AST
    {
        class /**/FunctionDefinition;
        class /**/ExecutionContext;
        class /**/ThisType;
    }
    class /**/Value;
    class /**/Object;
    class /**/Array;
    class /**/Environment;
    enum class /**/SystemFunction;

    using HostFunction = Value(Environment&, const PlaceInCode&, std::vector<Value>&&);
    using MemberMethodFunction = Value(AST::ExecutionContext&, const PlaceInCode&, const AST::ThisType&, std::vector<Value>&&);
    using MemberPropertyFunction = Value(AST::ExecutionContext&, const PlaceInCode&, Value&&);
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
                MemberMethod,
                MemberProperty,
                Object,
                Array,
                Type,
                Count
            };

            using NumberValType = double;
            using StringValType = std::string;
            using AstFuncValType = const AST::FunctionDefinition*;
            using HostFuncValType = HostFunction*;
            using SystemFuncValType = SystemFunction;
            using MemberFuncValType = MemberMethodFunction*;
            using MemberPropValType = MemberPropertyFunction*;
            using ObjectValType = std::shared_ptr<Object>;
            using ArrayValType = std::shared_ptr<Array>;
            // redundant use of redundant names cause redundancy, claim redundancy students of a study about redundancy
            using TypeValType = Type;

        private:
            using VariantType = std::variant<
                // Value::Type::Null
                std::monostate,
                // Value::Type::Number
                NumberValType,
                // Value::Type::String
                StringValType,
                // Value::Type::Function
                AstFuncValType,
                // Value::Type::SystemFunction
                SystemFuncValType,
                // Value::Type::HostFunction
                HostFuncValType,
                // Value::Type::MemberMethod
                MemberFuncValType,
                // Value::Type::MemberProperty
                MemberPropValType,
                // Value::Type::Object
                ObjectValType,
                // Value::Type::Array
                ArrayValType,
                // Value::Type::Type
                TypeValType
            >;

        private:
            Type m_type = Type::Null;
            VariantType m_variant;

        public:
            inline Value()
            {
            }

            inline explicit Value(double number) : m_type(Type::Number), m_variant(number)
            {
            }

            inline explicit Value(std::string&& str) : m_type(Type::String), m_variant(std::move(str))
            {
            }

            inline explicit Value(const AST::FunctionDefinition* func) : m_type{ Type::Function }, m_variant{ func }
            {
            }

            inline explicit Value(SystemFunction func) : m_type{ Type::SystemFunction }, m_variant{ func }
            {
            }

            inline explicit Value(HostFunction func) : m_type{ Type::HostFunction }, m_variant{ func }
            {
                assert(func);
            }

            inline explicit Value(MemberMethodFunction func): m_type{Type::MemberMethod}, m_variant{func}
            {
            }

            inline explicit Value(MemberPropertyFunction func): m_type{Type::MemberProperty}, m_variant{func}
            {
            }        

            inline explicit Value(std::shared_ptr<Object>&& obj) : m_type{ Type::Object }, m_variant(obj)
            {
            }

            inline explicit Value(std::shared_ptr<Array>&& arr) : m_type{ Type::Array }, m_variant(arr)
            {
            }

            inline explicit Value(Type typeVal) : m_type{ Type::Type }, m_variant(typeVal)
            {
            }

            inline Type type() const
            {
                return m_type;
            }

            inline NumberValType getNumber() const
            {
                assert(m_type == Type::Number);
                return std::get<NumberValType>(m_variant);
            }

            inline StringValType& getString()
            {
                assert(m_type == Type::String);
                return std::get<StringValType>(m_variant);
            }

            inline const StringValType& getString() const
            {
                assert(m_type == Type::String);
                return std::get<StringValType>(m_variant);
            }

            inline AstFuncValType getFunction() const
            {
                assert(m_type == Type::Function && std::get<AstFuncValType>(m_variant));
                return std::get<AstFuncValType>(m_variant);
            }

            inline SystemFuncValType getSysFunction() const
            {
                assert(m_type == Type::SystemFunction);
                return std::get<SystemFuncValType>(m_variant);
            }

            inline HostFuncValType getHostFunction() const
            {
                assert(m_type == Type::HostFunction);
                return std::get<HostFuncValType>(m_variant);
            }

            inline MemberFuncValType getMemberFunction() const
            {
                assert(m_type == Type::MemberMethod);
                return std::get<MemberFuncValType>(m_variant);
            }

            inline MemberPropValType getPropertyFunction() const
            {
                assert(m_type == Type::MemberProperty);
                return std::get<MemberPropValType>(m_variant);
            }

            inline Object* getObject() const
            {
                assert(m_type == Type::Object && std::get<ObjectValType>(m_variant));
                return std::get<ObjectValType>(m_variant).get();
            }

            inline ObjectValType getObjectRef() const
            {
                assert(m_type == Type::Object && std::get<ObjectValType>(m_variant));
                return std::get<ObjectValType>(m_variant);
            }

            inline Array* getArray() const
            {
                assert(m_type == Type::Array && std::get<ArrayValType>(m_variant));
                return std::get<ArrayValType>(m_variant).get();
            }

            inline ArrayValType getArrayRef() const
            {
                assert(m_type == Type::Array && std::get<ArrayValType>(m_variant));
                return std::get<ArrayValType>(m_variant);
            }

            inline Type getTypeValue() const
            {
                assert(m_type == Type::Type);
                return std::get<TypeValType>(m_variant);
            }

            inline void setNumberValue(double number)
            {
                assert(m_type == Type::Number);
                std::get<double>(m_variant) = number;
            }

            bool isEqual(const Value& rhs) const;

            bool isTrue() const;

            inline bool isNull() const
            {
                return (m_type == Type::Null);
            }

            inline bool isObject() const
            {
                return (m_type == Type::Object);
            }

            inline bool isNumber() const
            {
                return (m_type == Type::Number);
            }

            inline bool isString() const
            {
                return (m_type == Type::String);
            }

            inline bool isArray() const
            {
                return (m_type == Type::Array);
            }
    };

    class Object
    {
        public:
            using MapType = std::unordered_map<std::string, Value>;

        public:
            MapType m_items;

        public:
            size_t size() const
            {
                return m_items.size();
            }

            bool hasKey(const std::string& key) const
            {
                return m_items.find(key) != m_items.end();
            }

            // Creates new null value if doesn't exist.
            Value& entry(const std::string& key)
            {
                return m_items[key];
            }

            Value* tryGet(const std::string& key);// Returns null if doesn't exist.

            const Value* tryGet(const std::string& key) const;// Returns null if doesn't exist.

            bool removeEntry(const std::string& key);// Returns true if has been found and removed.
    };

    class Array
    {
        public:
            std::vector<Value> m_items;
    };

    std::string VFormat(const char* format, va_list argList);
    std::string Format(const char* format, ...);

    class /**/EnvironmentPimpl;
    class Environment
    {
        private:
            EnvironmentPimpl* m_implenv;

        public:
            Object m_globalscope;
            void* m_userdata = nullptr;

        public:
            Environment();

            ~Environment();

            Value execute(const std::string_view& code);

            std::string_view getTypeName(Value::Type type) const;

            void Print(const std::string_view& s);

            inline Object& global()
            {
                return m_globalscope;
            }

            inline Value& global(const std::string& key)
            {
                return m_globalscope.entry(key);
            }
    };


    // I would like it to be higher, but above that, even at 128, it crashes with
    // native "stack overflow" in Debug configuration.
    static const size_t LOCAL_SCOPE_STACK_MAX_SIZE = 100;

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
        Type symtype;
        // Only when symtype == Type::Number
        double numberval;
        // Only when symtype == Type::Identifier or String
        std::string stringval;
    };

    struct BreakException
    {
    };

    struct ContinueException
    {
    };

    class CodeReader
    {
        private:
            const std::string_view m_code;
            PlaceInCode m_place;

        public:
            CodeReader(const std::string_view& code);
            bool isAtEnd() const;
            const PlaceInCode& getCurrentPlace() const;
            const char* getCurrentCode() const;
            size_t getCurrentLength() const;
            char getCurrentChar() const;
            bool peekNext(char ch) const;
            bool peekNext(const char* s, size_t sLen) const;

            void moveOneChar();
            void MoveChars(size_t n);
    };

    ////////////////////////////////////////////////////////////////////////////////
    // Tokenizer definition

    class Tokenizer
    {
        public:
            static bool parseHexChar(uint8_t& out, char ch);
            static bool parseHexLiteral(uint32_t& out, const std::string_view& chars);
            static bool appendUTF8Char(std::string& inout, uint32_t charVal);

        private:
            CodeReader m_code;

        private:
            void skipSpacesAndComments();
            bool parseNumber(Token& out);
            bool parseString(Token& out);

        public:
            Tokenizer(const std::string_view& code) : m_code{ code }
            {
            }
            void getNextToken(Token& out);
    };

    ////////////////////////////////////////////////////////////////////////////////
    // Value definition

    enum class SystemFunction
    {
        StringResizeFunc,
        ArrayAddFunc,
        ArrayInsertFunc,
        ArrayRemoveFunc,
        FuncCount
    };
    static constexpr std::string_view SYSTEM_FUNCTION_NAMES[] =
    {
        "resize", "add", "insert", "remove",
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
        Value* getValueRef(const PlaceInCode& place) const;// Always returns non-null or throws exception.
        Value getValue(const PlaceInCode& place) const;
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
            inline bool isEmpty() const
            {
                return std::get_if<std::monostate>(this) != nullptr;
            }

            inline Object* getObject() const
            {
                const std::shared_ptr<Object>* objectPtr = std::get_if<std::shared_ptr<Object>>(this);
                return objectPtr ? objectPtr->get() : nullptr;
            }

            inline Array* getArray() const
            {
                const std::shared_ptr<Array>* arrayPtr = std::get_if<std::shared_ptr<Array>>(this);
                return arrayPtr ? arrayPtr->get() : nullptr;
            }

            inline void clear()
            {
                *this = ThisType{};
            }
        };

        class ExecutionContext
        {
            public:
                class LocalScopePush
                {
                    private:
                        ExecutionContext& m_context;

                    public:
                        LocalScopePush(ExecutionContext& ctx, Object* localScope, ThisType&& thisObj, const PlaceInCode& place)
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
                Object& m_globalscope;

                ExecutionContext(EnvironmentPimpl& env, Object& globalScope) : m_env{ env }, m_globalscope{ globalScope }
                {
                }

                bool isLocal() const
                {
                    return !m_localscopes.empty();
                }

                Object* getCurrentLocalScope()
                {
                    assert(isLocal());
                    return m_localscopes.back();
                }

                const ThisType& getThis()
                {
                    assert(isLocal());
                    return m_thislist.back();
                }

                Object& getInnermostScope() const
                {
                    return isLocal() ? *m_localscopes.back() : m_globalscope;
                }
        };

        class Statement
        {
            private:
                const PlaceInCode m_place;

            protected:
                void assign(const LValue& lhs, Value&& rhs) const;

            public:
                explicit Statement(const PlaceInCode& place) : m_place{ place }
                {
                }

                virtual ~Statement()
                {
                }

                const PlaceInCode& getPlace() const
                {
                    return m_place;
                }

                virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const = 0;

                virtual void execute(ExecutionContext& ctx) const = 0;
        };

        struct EmptyStatement : public Statement
        {
            explicit EmptyStatement(const PlaceInCode& place) : Statement{ place }
            {
            }

            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;

            virtual void execute(ExecutionContext&) const
            {
            }
        };

        struct /**/Expression;

        struct Condition : public Statement
        {
            // [0] executed if true, [1] executed if false, optional.
            std::unique_ptr<Expression> m_condexpr;
            std::unique_ptr<Statement> m_statements[2];

            explicit Condition(const PlaceInCode& place) : Statement{ place }
            {
            }

            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;

            virtual void execute(ExecutionContext& ctx) const;
        };

        struct WhileLoop : public Statement
        {
            enum Type { While, DoWhile };
        
            Type m_type;
            std::unique_ptr<Expression> m_condexpr;
            std::unique_ptr<Statement> m_body;

            explicit WhileLoop(const PlaceInCode& place, Type type) : Statement{ place }, m_type{ type }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void execute(ExecutionContext& ctx) const;
        };

        struct ForLoop : public Statement
        {
            // Optional
            std::unique_ptr<Expression> m_initexpr;
            // Optional
            std::unique_ptr<Expression> m_condexpr;
            // Optional
            std::unique_ptr<Expression> m_iterexpr;
            std::unique_ptr<Statement> m_body;
            explicit ForLoop(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void execute(ExecutionContext& ctx) const;
        };

        struct RangeBasedForLoop : public Statement
        {
            // Can be empty.
            std::string m_keyvar;
            // Cannot be empty.
            std::string m_valuevar;
            std::unique_ptr<Expression> m_rangeexpr;
            std::unique_ptr<Statement> m_body;
            explicit RangeBasedForLoop(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void execute(ExecutionContext& ctx) const;
        };


        struct LoopBreakStatement : public Statement
        {
            enum class Type
            {
                Break,
                Continue,
                Count
            };
            Type m_type;

            explicit LoopBreakStatement(const PlaceInCode& place, Type type) : Statement{ place }, m_type{ type }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void execute(ExecutionContext& ctx) const;
        };

        struct ReturnStatement : public Statement
        {
            // Can be null.
            std::unique_ptr<Expression> m_retvalue;

            explicit ReturnStatement(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void execute(ExecutionContext& ctx) const;
        };

        struct Block : public Statement
        {
            std::vector<std::unique_ptr<Statement>> m_statements;

            explicit Block(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void execute(ExecutionContext& ctx) const;
        };

        struct /**/ConstantValue;

        struct SwitchStatement : public Statement
        {
            std::unique_ptr<Expression> m_cond;
            // null means default block.
            std::vector<std::unique_ptr<AST::ConstantValue>> m_itemvals;
            // Can be null if empty.
            std::vector<std::unique_ptr<AST::Block>> m_itemblocks;

            explicit SwitchStatement(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void execute(ExecutionContext& ctx) const;
        };

        struct ThrowStatement : public Statement
        {
            std::unique_ptr<Expression> m_thrownexpr;
            explicit ThrowStatement(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void execute(ExecutionContext& ctx) const;
        };

        struct TryStatement : public Statement
        {
            std::unique_ptr<Statement> m_tryblock;
            std::unique_ptr<Statement> m_catchblock;// Optional
            std::unique_ptr<Statement> m_finallyblock;// Optional
            std::string m_exvarname;
            explicit TryStatement(const PlaceInCode& place) : Statement{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual void execute(ExecutionContext& ctx) const;
        };

        struct Script : Block
        {
            explicit Script(const PlaceInCode& place) : Block{ place }
            {
            }
            virtual void execute(ExecutionContext& ctx) const;
        };

        struct Expression : Statement
        {
            explicit Expression(const PlaceInCode& place) : Statement{ place }
            {
            }

            virtual Value evaluate(ExecutionContext& ctx, ThisType* outThis) const
            {
                (void)outThis;
                return getLeftValue(ctx).getValue(getPlace());
            }

            virtual LValue getLeftValue(ExecutionContext& ctx) const
            {
                (void)ctx;
                MINSL_EXECUTION_CHECK(false, getPlace(), ERROR_MESSAGE_EXPECTED_LVALUE);
            }

            virtual void execute(ExecutionContext& ctx) const
            {
                evaluate(ctx, nullptr);
            }
        };

        struct ConstantExpression : Expression
        {
            explicit ConstantExpression(const PlaceInCode& place) : Expression{ place }
            {
            }
            virtual void execute(ExecutionContext&) const
            { /* Nothing - just ignore its value. */
            }
        };

        struct ConstantValue : ConstantExpression
        {
            Value m_val;
            ConstantValue(const PlaceInCode& place, Value&& val) : ConstantExpression{ place }, m_val{ std::move(val) }
            {
                assert(m_val.type() == Value::Type::Null || m_val.isNumber() || m_val.isString());
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value evaluate(ExecutionContext&, ThisType*) const
            {
                return Value{ m_val };
            }
        };

        struct Identifier : ConstantExpression
        {
            enum class Scope
            {
                None,
                Local,
                Global,
                Count
            };

            Scope m_scope = Scope::Count;
            std::string m_ident;

            Identifier(const PlaceInCode& place, Scope scope, std::string&& s): ConstantExpression{ place }, m_scope(scope), m_ident(std::move(s))
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value evaluate(ExecutionContext& ctx, ThisType* outThis) const;
            virtual LValue getLeftValue(ExecutionContext& ctx) const;
        };

        struct ThisExpression : ConstantExpression
        {
            ThisExpression(const PlaceInCode& place) : ConstantExpression{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value evaluate(ExecutionContext& ctx, ThisType* outThis) const;
        };

        struct Operator : Expression
        {
            explicit Operator(const PlaceInCode& place) : Expression{ place }
            {
            }
        };

        class UnaryOperator: public Operator
        {
            public:
                enum class Type
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

            private:
                Value BitwiseNot(Value&& operand) const;

            public:
                Type m_type;
                std::unique_ptr<Expression> m_operand;

            public:
                UnaryOperator(const PlaceInCode& place, Type type) : Operator{ place }, m_type(type)
                {
                }
                virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
                virtual Value evaluate(ExecutionContext& ctx, ThisType* outThis) const;
                virtual LValue getLeftValue(ExecutionContext& ctx) const;
        };

        struct MemberAccessOperator: Operator
        {
            std::unique_ptr<Expression> m_operand;
            std::string m_membername;

            MemberAccessOperator(const PlaceInCode& place) : Operator{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value evaluate(ExecutionContext& ctx, ThisType* outThis) const;
            virtual LValue getLeftValue(ExecutionContext& ctx) const;
        };

        class BinaryOperator: public Operator
        {
            public:
                enum class Type
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

            public:
                Type m_type;
                std::unique_ptr<Expression> m_oplist[2];

            private:
                Value ShiftLeft(const Value& lhs, const Value& rhs) const;
                Value ShiftRight(const Value& lhs, const Value& rhs) const;
                Value Assignment(LValue&& lhs, Value&& rhs) const;

            public:
                BinaryOperator(const PlaceInCode& place, Type type) : Operator{ place }, m_type(type)
                {
                }
                virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
                virtual Value evaluate(ExecutionContext& ctx, ThisType* outThis) const;
                virtual LValue getLeftValue(ExecutionContext& ctx) const;
        };

        struct TernaryOperator : Operator
        {
            std::unique_ptr<Expression> m_oplist[3];
            explicit TernaryOperator(const PlaceInCode& place) : Operator{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value evaluate(ExecutionContext& ctx, ThisType* outThis) const;
        };

        struct CallOperator : Operator
        {
            std::vector<std::unique_ptr<Expression>> m_oplist;
            CallOperator(const PlaceInCode& place) : Operator{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value evaluate(ExecutionContext& ctx, ThisType* outThis) const;
        };

        struct FunctionDefinition : public Expression
        {
            std::vector<std::string> m_paramlist;
            Block m_body;
            FunctionDefinition(const PlaceInCode& place) : Expression{ place }, m_body{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value evaluate(ExecutionContext&, ThisType*) const
            {
                return Value{ this };
            }
            bool areParamsUnique() const;
        };

        struct ObjectExpression : public Expression
        {
            using ItemMap = std::map<std::string, std::unique_ptr<Expression>>;

            std::unique_ptr<Expression> m_baseexpr;
            ItemMap m_items;
            ObjectExpression(const PlaceInCode& place) : Expression{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value evaluate(ExecutionContext& ctx, ThisType* outThis) const;
        };

        struct ArrayExpression : public Expression
        {
            std::vector<std::unique_ptr<Expression>> m_items;
            ArrayExpression(const PlaceInCode& place) : Expression{ place }
            {
            }
            virtual void debugPrint(uint32_t indentLevel, const std::string_view& prefix) const;
            virtual Value evaluate(ExecutionContext& ctx, ThisType* outThis) const;
        };

    }// namespace AST

    static inline void CheckNumberOperand(const AST::Expression* operand, const Value& value)
    {
        MINSL_EXECUTION_CHECK(value.isNumber(), operand->getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
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
            void parseBlock(AST::Block& outBlock);
            bool tryParseSwitchItem(AST::SwitchStatement& switchStatement);
            void parseFuncDef(AST::FunctionDefinition& funcDef);
            bool peekSymbols(std::initializer_list<Token::Type> arr);
            std::unique_ptr<AST::Statement> tryParseStatement();
            std::unique_ptr<AST::ConstantValue> tryParseConstVal();
            std::unique_ptr<AST::Identifier> tryParseIdentVal();
            std::unique_ptr<AST::ConstantExpression> tryParseConstExpr();
            std::pair<std::string, std::unique_ptr<AST::FunctionDefinition>> tryParseFuncSynSugar();
            std::unique_ptr<AST::Expression> tryParseClassSynSugar();
            std::unique_ptr<AST::Expression> tryParseObjMember(std::string& outMemberName);
            std::unique_ptr<AST::ObjectExpression> tryParseObject();
            std::unique_ptr<AST::ArrayExpression> tryParseArray();
            std::unique_ptr<AST::Expression> tryParseParentheses();
            std::unique_ptr<AST::Expression> tryParseUnary();
            std::unique_ptr<AST::Expression> tryParseOperator();
            std::unique_ptr<AST::Expression> tryParseBinary();
            std::unique_ptr<AST::Expression> tryParseAddSub();
            std::unique_ptr<AST::Expression> tryParseAngleSign();
            std::unique_ptr<AST::Expression> tryParseAngleCompare();
            std::unique_ptr<AST::Expression> tryParseEquals();
            std::unique_ptr<AST::Expression> TryParseExpr11();
            std::unique_ptr<AST::Expression> TryParseExpr12();
            std::unique_ptr<AST::Expression> TryParseExpr13();
            std::unique_ptr<AST::Expression> TryParseExpr14();
            std::unique_ptr<AST::Expression> TryParseExpr15();
            std::unique_ptr<AST::Expression> TryParseExpr16();
            std::unique_ptr<AST::Expression> TryParseExpr17();
            bool tryParseSymbol(Token::Type symbol);
            std::string tryParseIdentifier();// If failed, returns empty string.
            const PlaceInCode& getCurrentTokenPlace() const
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

            Environment& getOwner()
            {
                return m_owner;
            }

            Value execute(const std::string_view& code);

            std::string_view getTypeName(Value::Type type) const;

            void Print(const std::string_view& s)
            {
                //m_Output.append(s);
                std::cout << s;
            }
    };

    std::shared_ptr<Object> CopyObject(const Object& src);
    bool NumberToIndex(size_t& outIndex, double number);

    namespace Builtins
    {

        Value func_typeof(Environment& env, const PlaceInCode& place, std::vector<Value>&& args);
        Value func_print(Environment& env, const PlaceInCode& place, std::vector<Value>&& args);
        Value func_min(Environment& ctx, const PlaceInCode& place, std::vector<Value>&& args);
        Value func_max(Environment& ctx, const PlaceInCode& place, std::vector<Value>&& args);

        Value ctor_null(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args);
        Value ctor_number(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args);
        Value ctor_string(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args);
        Value ctor_object(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args);
        Value ctor_array(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args);
        Value ctor_function(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args);
        Value ctor_type(AST::ExecutionContext& ctx, const PlaceInCode& place, std::vector<Value>&& args);


        Value memberfn_object_count(AST::ExecutionContext& ctx, const PlaceInCode& place, Value&& objVal);

        Value memberfn_string_count(AST::ExecutionContext& ctx, const PlaceInCode& place, Value&& objVal);
        Value memberfn_string_resize(AST::ExecutionContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args);

        Value memberfn_array_count(AST::ExecutionContext& ctx, const PlaceInCode& place, Value&& objVal);
        Value memberfn_array_add(AST::ExecutionContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args);
        Value memberfn_array_insert(AST::ExecutionContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args);
        Value memberfn_array_remove(AST::ExecutionContext& ctx, const PlaceInCode& place, const AST::ThisType& th, std::vector<Value>&& args);
    }
}
