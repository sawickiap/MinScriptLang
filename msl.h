
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
#include <sstream>
#include <string>
#include <string_view>
#include <exception>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include <variant>
#include <set>
#include <map>
#include <algorithm>
#include <type_traits>
#include <initializer_list>
#include <cstdarg>
#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctype.h>


#define MINSL_EXECUTION_CHECK(condition, loc, errorMessage) \
    if(!(condition)) \
    { \
        throw Error::RuntimeError((loc), (errorMessage)); \
    }

#define MINSL_EXECUTION_FAIL(loc, errorMessage) \
    throw Error::RuntimeError((loc), (errorMessage));


namespace MSL
{
    namespace Runtime
    {
        class /**/FunctionDefinition;
    }
    class /**/Value;
    class /**/Number;
    class /**/String;
    class /**/Object;
    class /**/Array;
    class /**/Environment;
    class /**/Context;
    class /**/ThisObject;

    namespace Util
    {
        std::shared_ptr<Object> CopyObject(const Object& src);
        std::shared_ptr<Array> CopyArray(const Array& src);

        std::optional<std::string> readFile(const std::string& file, std::optional<unsigned long> maxrd={});
        void reprChar(std::ostream& os, int c);
        void reprString(std::ostream& os, std::string_view str);
        std::string vformatString(const char* format, va_list argList);
        std::string formatString(const char* format, ...);

        /*
        // strip from start (in place)
        */
        void stripInplaceLeft(std::string& str);

        /*
        // strip from end (in place)
        */
        void stripInplaceRight(std::string& str);

        /*
        // strip from both ends (in place)
        */
        void stripInplace(std::string& str);

        std::string stripRight(std::string_view s);
        std::string stripLeft(std::string_view s);
        std::string strip(std::string_view in);
        void splitString(std::string_view str, const std::string& delim, std::function<bool(const std::string&)> cb);


        template<typename CharT, typename Type>
        void joinArgsNext(std::basic_ostream<CharT>& os, std::basic_string_view<CharT>, Type&& thing)
        {
            os << thing;
        }

        template<typename CharT, typename Type, typename... ArgsT>
        void joinArgsNext(std::basic_ostream<CharT>& os, std::basic_string_view<CharT> sep, Type&& thing, ArgsT&&... args)
        {
            joinArgsNext(os, sep, thing);
            os << sep;
            joinArgsNext(os, sep, args...);
        }

        template<typename... ArgsT>
        std::string joinArgsWith(std::string_view sep, ArgsT&&... args)
        {
            std::stringstream buf;
            joinArgsNext(buf, sep, args...);
            return buf.str();
        }

        template<typename... ArgsT>
        std::string joinArgs(ArgsT&&... args)
        {
            return joinArgsWith("", std::string(), args...);
        }
    }

    class Location
    {
        public:
            uint32_t textindex = 0;
            uint32_t textline = 1;
            uint32_t textcolumn = 0;
            std::string_view filename = "none";

        private:
            inline void copyFrom(const Location& other)
            {
                textindex = other.textindex;
                textline = other.textline;
                textcolumn = other.textcolumn;
                filename = other.filename;
            }

        public:
            inline Location()
            {
            }

            inline Location(uint32_t ti, uint32_t tl, uint32_t tc): textindex(ti), textline(tl), textcolumn(tc)
            {
            }

            inline Location(uint32_t ti, uint32_t tl, uint32_t tc, std::string_view f): Location(ti, tl, tc)
            {
                filename = f;
            }

            inline Location(const Location& other)
            {
                copyFrom(other);
            }

            inline Location& operator=(const Location& other)
            {
                copyFrom(other);
                return *this;
            }
    };

    namespace Error
    {
        class Exception : public std::exception
        {
            private:
                const Location m_place;
                mutable std::string m_what;

            public:
                inline Exception(const Location& loc) : m_place{ loc }
                {
                }

                inline const Location& location() const
                {
                    return m_place;
                }

                inline virtual std::string_view name() const
                {
                    return "Exception";
                }

                virtual std::string prettyMessage() const;
                virtual std::string_view message() const = 0;
        };

        class ParsingError : public Exception
        {
            private:
                // Externally owned
                const std::string_view m_message;

            public:
                inline ParsingError(const Location& loc, std::string_view message) : Exception{ loc }, m_message{ message }
                {
                }

                inline virtual std::string_view name() const override
                {
                    return "ParsingError";
                }                    

                inline virtual std::string_view message() const override
                {
                    return m_message;
                }
        };


        class RuntimeError : public Exception
        {
            private:
                const std::string m_message;

            public:
                inline RuntimeError(const Location& loc, std::string_view message) : Exception{ loc }, m_message{ message }
                {
                }

                inline virtual std::string_view name() const override
                {
                    return "RuntimeError";
                }

                inline virtual std::string_view message() const override
                {
                    return m_message;
                }
        };

        class TypeError: public RuntimeError
        {
            public:
                using RuntimeError::RuntimeError;

                inline virtual std::string_view name() const override
                {
                    return "TypeError";
                }
        };

        class ArgumentError: public RuntimeError
        {
            public:
                using RuntimeError::RuntimeError;

                inline virtual std::string_view name() const override
                {
                    return "ArgumentError";
                }
        };

        class IndexError: public RuntimeError
        {
            public:
                using RuntimeError::RuntimeError;

                inline virtual std::string_view name() const override
                {
                    return "IndexError";
                }
        };

        class IOError: public RuntimeError
        {
            public:
                using RuntimeError::RuntimeError;

                inline virtual std::string_view name() const override
                {
                    return "IOError";
                }
        };

        class OSError: public IOError
        {
            public:
                using IOError::IOError;

                inline virtual std::string_view name() const override
                {
                    return "OSError";
                }
        };

        class EOFError: public RuntimeError
        {
            public:
                using RuntimeError::RuntimeError;

                inline virtual std::string_view name() const override
                {
                    return "EOFError";
                }
        };
    }

    namespace GC
    {
        // Base class for all objects that are tracked by
        // the garbage collector.
        class Collectable
        {
            public:
                // For mark and sweep algorithm. When a GC occurs
                // all live objects are traversed and m_marked is
                // set to true. This is followed by the sweep phase
                // where all unmarked objects are deleted.
                bool m_marked;

            public:
                Collectable();
                Collectable(Collectable const&);
                virtual ~Collectable();

                // Mark the object and all its children as live
                void mark();

                // Overridden by derived classes to call mark()
                // on objects referenced by this object. The default
                // implemention does nothing.
                virtual void markChildren();
        };

        // Wrapper for an array of bytes managed by the garbage
        // collector.
        class Memory : public Collectable
        {
            public:
                unsigned char* m_memory;
                int m_size;

            public:
                Memory(int size);
                virtual ~Memory();

                unsigned char* get();
                int size();
        };

        // Garbage Collector. Implements mark and sweep GC algorithm.
        class Collector
        {
            public:
                using ObjectSet = std::set<Collectable*>;
                using PinnedSet = std::map<Collectable*, unsigned int>;

            public:
                // A collection of all active heap objects.
                ObjectSet m_heap;

                // Collection of objects that are scanned for garbage.
                ObjectSet m_roots;

                // Pinned objects
                PinnedSet m_pinned;

                // Global garbage collector object
                static Collector GC;

            public:
                // Perform garbage collection. If 'verbose' is true then
                // GC stats will be printed to stdout.
                void collect(bool verbose = false);

                // Add a root object to the collector.
                void addRoot(Collectable* root);

                // Remove a root object from the collector.
                void removeRoot(Collectable* root);

                // Pin an object so it temporarily won't be collected.
                // Pinned objects are reference counted. Pinning it
                // increments the count. Unpinning it decrements it. When
                // the count is zero then the object can be collected.
                void pin(Collectable* o);
                void unpin(Collectable* o);

                // Add an heap allocated object to the collector.
                void addObject(Collectable* o);

                // Remove a heap allocated object from the collector.
                void removeObject(Collectable* o);

                // Go through all objects in the heap, unmarking the live
                // objects and destroying the unreferenced ones.
                void sweep(bool verbose);

                // Number of live objects in heap
                int live();
        };
    }

    using HostFunction           = std::function<Value(Environment&, const Location&, std::vector<Value>&&)>;
    using MemberMethodFunction   = std::function<Value(Context&, const Location&, ThisObject&, std::vector<Value>&&)>;
    using MemberPropertyFunction = std::function<Value(Context&, const Location&, Value&&)>;

    class String
    {
        private:
            std::string m_string;

        public:
            String()
            {
            }

            String(const String& other): m_string(other.m_string)
            {
            }

            String(String&& other): m_string(std::move(other.m_string))
            {
            }

            String(std::string_view str): m_string(str)
            {
            }

            String(int ch, size_t cnt): m_string(std::string(ch, cnt))
            {
            }

            String& operator=(const String& other)
            {
                m_string = other.m_string;
                return *this;
            }

            String& operator=(String&& other)
            {
                m_string = std::move(other.m_string);
                return *this;
            }

            size_t size() const
            {
                return m_string.size();
            }

            size_t length() const
            {
                return m_string.length();
            }

            void push_back(int ch)
            {
                m_string.push_back(ch);
            }

            auto begin()
            {
                return m_string.begin();
            }

            auto begin() const
            {
                return m_string.begin();
            }

            auto end()
            {
                return m_string.end();
            }

            auto end() const
            {
                return m_string.end();
            }

            char& operator[](size_t i)
            {
                return m_string[i];
            }

            char operator[](size_t i) const
            {
                return m_string[i];
            }

            String& operator+=(const String& other)
            {
                m_string += other.m_string;
                return *this;
            }

            String& operator+=(String&& other)
            {
                m_string += std::move(other.m_string);
                return *this;
            }

            String& operator+=(int c)
            {
                m_string.push_back(c);
                return *this;
            }

            String& operator+=(const std::string& str)
            {
                m_string += str;
                return *this;
            }

            String& operator+=(std::string&& str)
            {
                m_string += std::move(str);
                return *this;
            }

            bool operator==(const String& other) const
            {
                return (m_string == other.m_string);
            }

            bool operator==(const std::string& other) const
            {
                return (m_string == other);
            }
    };

    class Value/*: public GC::Collectable*/
    {
        public:
            enum class Type
            {
                Null,
                Number,
                String,
                Function,
                HostFunction,
                Object,
                Array,
                Type,
                MemberMethod,
                MemberProperty,
                Count
            };
            using List = std::vector<Value>;
            using NumberValType = double;
            using StringValType = std::string;
            using AstFuncValType = Runtime::FunctionDefinition*;
            using HostFuncValType = HostFunction;
            using MemberFuncValType = MemberMethodFunction;
            using MemberPropValType = MemberPropertyFunction;
            using ObjectValType = std::shared_ptr<Object>;
            using ArrayValType = std::shared_ptr<Array>;
            // redundant use of redundant names cause redundancy, claim redundancy students of a study about redundancy
            using TypeValType = Type;

        public:
            inline static std::string_view getTypename(Type t)
            {
                switch(t)
                {
                    case Type::Null:
                        return "Null";
                    case Type::Number:
                        return "Number";
                    case Type::String:
                        return "String";
                    case Type::Function:
                    case Type::HostFunction:
                    case Type::MemberMethod:
                        return "Function";
                    case Type::Object:
                        return "Object";
                    case Type::Array:
                        return "Array";
                    case Type::Type:
                        return "Type";
                    default:
                        break;
                }
                return "Null";
            }

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
                // Value::Type::HostFunction
                HostFuncValType,
                // Value::Type::Object
                ObjectValType,
                // Value::Type::Array
                ArrayValType,
                // Value::Type::Type
                TypeValType,
                // Value::Type::MemberMethod
                MemberFuncValType,
                // Value::Type::MemberProperty
                MemberPropValType
            >;

        private:
            Type m_type = Type::Null;
            Location m_location;
            VariantType m_variant;

        private:
            void actualToStream(std::ostream& os, bool repr) const;
            //virtual void markChildren() override;

        public:
            /**
            * constructors.
            */
            Value();
            inline Value(const Value& other)
            {
                m_type = other.m_type;
                m_location = other.m_location;
                m_variant = other.m_variant;
            }

            explicit Value(NumberValType number);
            explicit Value(StringValType&& str);
            explicit Value(AstFuncValType func);
            explicit Value(HostFunction func);
            explicit Value(MemberMethodFunction func);
            explicit Value(MemberPropertyFunction func);
            explicit Value(ObjectValType&& obj);
            explicit Value(ArrayValType&& arr);
            explicit Value(Type typeVal);

            inline Value& operator=(const Value& other)
            {
                m_type = other.m_type;
                m_variant = other.m_variant;
                m_location = other.m_location;
                return *this;
            }

            /**
            * returns current type.
            * default type is always Type::Null.
            */
            Type type() const;

            /**
            * read/modify currently stored location information.
            * by default, it is empty.
            */
            const Location& location() const;
            Location& location();

            /**
            * value accessor functions
            */
            NumberValType number() const;
            const StringValType& string() const;
            StringValType& string();
            AstFuncValType scriptFunction();
            AstFuncValType scriptFunction() const;
            HostFuncValType hostFunction() const;
            MemberFuncValType memberFunction() const;
            MemberPropValType propertyFunction() const;
            Object* object() const;
            ObjectValType objectRef() const;
            Array* array() const;
            ArrayValType arrayRef() const;
            Type getTypeValue() const;

            /**
            * explicitly sets a number value, for both the
            * internal variant, as well as the type.
            */
            void setNumberValue(NumberValType number);

            /**
            * comparison functions.
            */
            bool isEqual(const Value& rhs) const;
            bool isTrue() const;
            bool isNull() const;
            bool isObject() const;
            bool isNumber() const;
            bool isString() const;
            bool isArray() const;

            template<typename CharT, typename TraitsT>
            std::basic_ostream<CharT, TraitsT>& toStream(std::basic_ostream<CharT,TraitsT>& os, bool repr) const
            {
                actualToStream(os, repr);
                return os;
            }

            template<typename CharT, typename TraitsT>
            std::basic_ostream<CharT, TraitsT>& toStream(std::basic_ostream<CharT,TraitsT>& os) const
            {
                return toStream(os, false);
            }

            template<typename CharT, typename TraitsT>
            std::basic_ostream<CharT, TraitsT>& reprToStream(std::basic_ostream<CharT,TraitsT>& os) const
            {
                return toStream(os, true);
            }

            std::string toString() const;
            std::string toRepr() const;

            /**
            * value operator abstraction methods - just to avoid
            * having to write stuff akin to `Value::operator<<()` ... you get the idea.
            */

            // comparison ops
            bool opCompareLessThan(const Value& rhs);
            bool opCompareLessEqual(const Value& rhs);
            bool opCompareGreaterThan(const Value& rhs);
            bool opCompareGreaterEqual(const Value& rhs);

            // operations that create a new value
            Value opPlus(const Value& rhs);
            Value opMinus(const Value& rhs);
            Value opMul(const Value& rhs);
            Value opDiv(const Value& rhs);
            Value opMod(const Value& rhs);
            Value opBinaryOr(const Value& rhs);
            Value opBinaryAnd(const Value& rhs);
            Value opShiftRight(const Value& rhs);
            Value opShiftLeft(const Value& rhs);
            Value opBitwiseAnd(const Value& rhs);
            Value opBitwiseXor(const Value& rhs);
            Value opBitwiseOr(const Value& rhs);

            // operations that modify this value
            Value& opPlusAssign(const Value& rhs);
            Value& opMinusAssign(const Value& rhs);
            Value& opMulAssign(const Value& rhs);
            Value& opDivAssign(const Value& rhs);
            Value& opModAssign(const Value& rhs);
            Value& opShiftLeftAssign(const Value& rhs);
            Value& opShiftRightAssign(const Value& rhs);
            Value& opBitwiseAndAssign(const Value& rhs);
            Value& opBitwiseOrAssign(const Value& rhs);
            Value& opBitwiseXorAssign(const Value& rhs);

            // specialty operations, like indexing, etc
            Value&& opIndex(const Value& rhs, ThisObject* othis);
    };

    using ThisVariant = std::variant<
        std::monostate,
        std::shared_ptr<Object>,
        std::shared_ptr<Array>,
        Value::StringValType
    >;

    class ThisObject : public ThisVariant
    {
        public:
            inline bool isEmpty() const
            {
                return std::get_if<std::monostate>(this) != nullptr;
            }

            inline Object* object() const
            {
                auto objectPtr = std::get_if<std::shared_ptr<Object>>(this);
                return objectPtr ? objectPtr->get() : nullptr;
            }

            inline Array* array() const
            {
                auto arrayPtr = std::get_if<std::shared_ptr<Array>>(this);
                return arrayPtr ? arrayPtr->get() : nullptr;
            }

            inline Value::StringValType& string()
            {
                return std::get<Value::StringValType>(*this);
            }

            inline const Value::StringValType& string() const
            {
                return std::get<Value::StringValType>(*this);
            }

            inline void clear()
            {
                *this = ThisObject{};
            }
    };

    namespace Util
    {
        bool NumberToIndex(size_t& outIndex, Value::NumberValType number);

        class ArgumentCheck
        {
            private:
                const Location& m_location;
                Value::List& m_args;
                std::string_view m_fname;

            public:
                inline ArgumentCheck(const Location& loc, Value::List& va, std::string_view fname):
                    m_location(loc), m_args(va), m_fname(fname)
                {
                }

                bool checkCount(size_t expect, bool alsothrow=true);
                std::optional<Value> checkOptional(size_t idx, std::initializer_list<Value::Type> types);
                Value checkArgument(size_t idx, std::initializer_list<Value::Type> types);
        };
    }

    class Object/*: public GC::Collectable*/
    {
        public:
            using MapType = std::unordered_map<Value::StringValType, Value>;

        public:
            MapType m_entrymap;

        private:
            //virtual void markChildren() override;

        public:
            Object();

            virtual ~Object();

            size_t size() const
            {
                return m_entrymap.size();
            }

            bool hasKey(const std::string& key) const
            {
                return m_entrymap.find(key) != m_entrymap.end();
            }

            // Creates new null value if doesn't exist.
            void put(const std::string& key, Value&& val)
            {
                m_entrymap[key] = val;
            }

            // Returns null if doesn't exist.
            Value* tryGet(const std::string& key);

            // Returns null if doesn't exist.
            const Value* tryGet(const std::string& key) const;

            // Returns true if has been found and removed.
            bool removeEntry(const std::string& key);
    };

    class Array/*: public GC::Collectable*/
    {
        private:
            Value::List m_arrayitems;

        private:
            //virtual void markChildren() override;
            
        public:
            Array();

            virtual ~Array();

            inline Array(size_t cnt): m_arrayitems(Value::List(cnt))
            {
            }

            inline size_t size() const
            {
                return m_arrayitems.size();
            }

            size_t length() const
            {
                return m_arrayitems.size();
            }

            void resize(size_t cnt)
            {
                m_arrayitems.resize(cnt);
            }

            void push_back(Value&& v)
            {
                m_arrayitems.push_back(v);
            }

            void push_back(const Value& v)
            {
                m_arrayitems.push_back(v);
            }

            Value& at(size_t idx)
            {
                return m_arrayitems[idx];
            }

            const Value& at(size_t idx) const
            {
                return m_arrayitems[idx];
            }

            const Value& back()
            {
                return m_arrayitems.back();
            }

            void pop_back()
            {
                m_arrayitems.pop_back();
            }

            template<typename... ArgsT>
            auto insert(ArgsT&&... args)
            {
                return m_arrayitems.insert(std::forward<ArgsT>(args)...);
            }

            template<typename... ArgsT>
            auto erase(ArgsT&&... args)
            {
                return m_arrayitems.erase(std::forward<ArgsT>(args)...);
            }

            auto begin()
            {
                return m_arrayitems.begin();
            }

            auto begin() const
            {
                return m_arrayitems.begin();
            }

            auto end()
            {
                return m_arrayitems.end();
            }

            auto end() const
            {
                return m_arrayitems.end();
            }
    };

    class Environment
    {
        public:
            class Impl
            {
                private:
                    Environment& m_owner;
                    Object& m_globalscope;

                public:
                    Impl(Environment& owner, Object& globalScope) : m_owner(owner), m_globalscope{ globalScope }
                    {
                    }

                    ~Impl() = default;

                    Environment& getOwner()
                    {
                        return m_owner;
                    }

                    Value execute(std::string_view code, std::string_view file);
                    Value execute(std::string_view code);

                    std::string_view getTypename(Value::Type type) const;
            };

        private:
            Impl* m_implenv;
        public:
            Object m_globalscope;

        private:
            void makeStdHandle(const Location& upplace, const std::string& name, FILE* strm);

        public:
            Environment();

            ~Environment();

            Value execute(std::string_view code, std::string_view filename);
            Value execute(std::string_view code);

            std::string_view getTypename(Value::Type type) const;

            inline void setGlobal(const std::string& key, Value&& val)
            {
                return m_globalscope.put(key, std::move(val));
            }

    };

    class Context
    {
        public:
            class PushLocalScope
            {
                private:
                    Context& m_context;

                public:
                    PushLocalScope(Context& ctx, Object* localScope, ThisObject&& thisObj, const Location& loc);
                    ~PushLocalScope();
            };

        private:
            std::vector<Object*> m_localscopes;
            std::vector<ThisObject> m_thislist;

        public:
            Environment::Impl& m_env;
            Object& m_globalscope;

        public:
            Context(Environment::Impl& env, Object& globalScope);

            Environment::Impl& env();

            Environment::Impl& env() const;

            bool isLocal() const;

            Object* getCurrentLocalScope();

            const ThisObject& getThis();

            Object& getInnermostScope() const;
    };


    // I would like it to be higher, but above that, even at 128, it crashes with
    // native "stack overflow" in Debug configuration.
    static const size_t LOCAL_SCOPE_STACK_MAX_SIZE = 100;

    class Token
    {
        public:
            enum class Type
            {
                /* Token types -- */
                None,
                Identifier,
                Number,
                String,
                End,

                /* -- Symbols -- */
                // ,
                Comma,
                // ?
                QuestionMark,
                // :
                Colon,
                // ;
                Semicolon,
                // (
                RoundBracketOpen,
                // )
                RoundBracketClose,
                // [
                SquareBracketOpen,
                // ]
                SquareBracketClose,
                // {
                CurlyBracketOpen,
                // }
                CurlyBracketClose,
                // *
                Asterisk,
                // /
                Slash,
                // %
                Percent,
                // +
                Plus,
                // -
                Dash,
                // =
                Equals,
                // !
                ExclamationMark,
                // ~
                Tilde,
                // <
                Less,
                // >
                Greater,
                // &
                Ampersand,
                // ^
                Caret,
                // |
                Pipe,
                // .
                Dot,

                /* --  Multiple character symbols -- */
                // ++
                DoublePlus,
                // --
                DoubleDash,
                // +=
                PlusAssign,
                // -=
                MinusAssign,
                // *=
                MulAssign,
                // /=
                DivAssign,
                // %=
                ModAssign,
                // <<=
                ShiftLeftAssign,
                // >>=
                ShiftRightAssign,
                // &=
                BitwiseAndAssign,
                // ^=
                BitwiseXorAssign,
                // |=
                BitwiseOrAssign,
                // <<
                ShiftLeftOper,
                // >>
                ShiftRightOper,
                // <=
                LessThanAssign,
                // >=
                GreaterThanAssign,
                // ==
                CompareEqual,
                // !=
                CompareNotEqual,
                // &&
                LogicalAndOper,
                // ||
                LogicalOrOper,

                /* -- Keywords -- */
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

        public:
            Location m_place;
            Type m_symtype = Type::None;
            // Only when symtype == Type::Number
            Value::NumberValType m_numberval = 0;
            // Only when symtype == Type::Identifier or String
            std::string m_stringval = {};

        private:
            inline void copyFrom(const Token& other)
            {
                m_place = other.m_place;
                m_symtype = other.m_symtype;
                m_numberval = other.m_numberval;
                m_stringval = other.m_stringval;
            }

        public:
            inline Token()
            {
            }

            inline Token(const Token& other)
            {
                copyFrom(other);
            }

            inline Token& operator=(const Token& other)
            {
                copyFrom(other);
                return *this;
            }

            inline const Location& location() const
            {
                return m_place;
            }

            inline Type type() const
            {
                return m_symtype;
            }

            inline void type(Type t)
            {
                m_symtype = t;
            }

            Value::NumberValType& number()
            {
                return m_numberval;
            }

            const Value::NumberValType& number() const
            {
                return m_numberval;
            }

            std::string& string()
            {
                return m_stringval;
            }

            const std::string& string() const
            {
                return m_stringval;
            }
    };

    class BreakException
    {
    };

    class ContinueException
    {
    };

    class CodeReader
    {
        private:
            const std::string_view m_code;
            Location m_place;

        public:
            CodeReader(std::string_view code);
            bool isAtEnd() const;
            const Location& location() const;
            const char* code() const;
            size_t codeLength() const;
            char getCurrent() const;
            bool peekNext(char ch) const;
            bool peekNext(const char* s, size_t sLen) const;

            void moveForward();
            void moveForward(size_t n);
    };

    class Tokenizer
    {
        public:
            static bool parseHexChar(uint8_t& out, char ch);
            static bool parseHexLiteral(uint32_t& out, std::string_view chars);
            static bool appendUTF8Char(std::string& inout, uint32_t charVal);

        private:
            CodeReader m_code;

        private:
            void skipSpacesAndComments();
            bool parseNumber(Token& out);
            bool parseString(Token& out);

        public:
            Tokenizer(std::string_view code) : m_code{ code }
            {
            }
            void getNextToken(Token& out);
    };
    
    namespace LeftValue
    {
        class ObjectMember
        {
            public:
                Object* objectval;
                std::string keyval;
        };

        class StringCharacter
        {
            public:
                std::string* stringval;
                size_t indexval;
        };

        class ArrayItem
        {
            public:
                Array* arrayval;
                size_t indexval;
        };

        //! TODO: needs a better name
        class Getter : public std::variant<ObjectMember, StringCharacter, ArrayItem>
        {
            public:
                // Always returns non-null or throws exception.
                Value* getValueRef(const Location& loc) const;
                Value getValue(const Location& loc) const;
        };
    }

    class ReturnException
    {
        public:
            const Location place;
            Value thrownvalue;
    };

    namespace Runtime
    {
        class DebugWriter
        {
            public:
                virtual void writeValue(const Value& v) = 0;
                virtual void writeString(std::string_view str) = 0;
                virtual void writeReprString(std::string_view str) = 0;
                virtual void writeChar(int ch) = 0;
                virtual void writeNumber(Value::NumberValType n) = 0;

                inline virtual void flush()
                {
                }
        };

        class Statement
        {
            private:
                const Location m_place;

            protected:
                void assign(const LeftValue::Getter& lhs, Value&& rhs) const;

            public:
                explicit Statement(const Location& loc);

                virtual ~Statement();

                const Location& location() const;

                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const = 0;

                virtual Value execute(Context& ctx) = 0;
        };

        class EmptyStatement : public Statement
        {
            public:
                explicit EmptyStatement(const Location& loc) : Statement{ loc }
                {
                }

                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;

                virtual Value execute(Context&) override
                {
                    return {};
                }
        };

        class /**/Expression;

        class Condition : public Statement
        {
            public:
                // [0] executed if true, [1] executed if false, optional.
                std::unique_ptr<Expression> m_condexpr;
                std::unique_ptr<Statement> m_truestmt;
                std::unique_ptr<Statement> m_falsestmt;

            public:
                explicit Condition(const Location& loc) : Statement{ loc }
                {
                }

                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;

                virtual Value execute(Context& ctx) override;
        };

        class WhileLoop : public Statement
        {
            public:
                enum Type { While, DoWhile };

            public:
                Type m_type;
                std::unique_ptr<Expression> m_condexpr;
                std::unique_ptr<Statement> m_body;

            public:
                explicit WhileLoop(const Location& loc, Type type) : Statement{ loc }, m_type{ type }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value execute(Context& ctx) override;
        };

        class ForLoop : public Statement
        {
            public:
                // Optional
                std::unique_ptr<Expression> m_initexpr;
                // Optional
                std::unique_ptr<Expression> m_condexpr;
                // Optional
                std::unique_ptr<Expression> m_iterexpr;
                std::unique_ptr<Statement> m_body;

            public:
                explicit ForLoop(const Location& loc) : Statement{ loc }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value execute(Context& ctx) override;
        };

        class RangeBasedForLoop : public Statement
        {
            public:
                // Can be empty.
                std::string m_keyvar;
                // Cannot be empty.
                std::string m_valuevar;
                std::unique_ptr<Expression> m_rangeexpr;
                std::unique_ptr<Statement> m_body;

            public:
                explicit RangeBasedForLoop(const Location& loc) : Statement{ loc }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value execute(Context& ctx) override;
        };

        class LoopBreakStatement : public Statement
        {
            public:
                enum class Type
                {
                    Break,
                    Continue,
                    Count
                };

            public:
                Type m_type;

            public:
                explicit LoopBreakStatement(const Location& loc, Type type) : Statement{ loc }, m_type{ type }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value execute(Context& ctx) override;
        };

        class ReturnStatement : public Statement
        {
            public:
                // Can be null.
                std::unique_ptr<Expression> m_retvalue;

            public:
                explicit ReturnStatement(const Location& loc) : Statement{ loc }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value execute(Context& ctx) override;
        };

        class Block : public Statement
        {
            public:
                std::vector<std::unique_ptr<Statement>> m_statements;

            public:
                explicit Block(const Location& loc);
                virtual ~Block();
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value execute(Context& ctx) override;
        };

        class /**/ConstantValue;

        class SwitchStatement : public Statement
        {
            public:
                std::unique_ptr<Expression> m_cond;
                // null means default block.
                std::vector<std::unique_ptr<Runtime::ConstantValue>> m_itemvals;
                // Can be null if empty.
                std::vector<std::unique_ptr<Runtime::Block>> m_itemblocks;

            public:
                explicit SwitchStatement(const Location& loc) : Statement{ loc }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value execute(Context& ctx) override;
        };

        class ThrowStatement : public Statement
        {
            public:
                std::unique_ptr<Expression> m_thrownexpr;

            public:
                explicit ThrowStatement(const Location& loc) : Statement{ loc }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value execute(Context& ctx) override;
        };

        class TryStatement : public Statement
        {
            public:
                std::unique_ptr<Statement> m_tryblock;
                std::unique_ptr<Statement> m_catchblock;// Optional
                std::unique_ptr<Statement> m_finallyblock;// Optional
                std::string m_exvarname;

            public:
                explicit TryStatement(const Location& loc) : Statement{ loc }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value execute(Context& ctx) override;
        };

        class Script: public Block
        {
            public:
                explicit Script(const Location& loc) : Block{ loc }
                {
                }
                virtual Value execute(Context& ctx) override;
        };

        class Expression: public Statement
        {
            public:
                explicit Expression(const Location& loc) : Statement{ loc }
                {
                }

                virtual Value evaluate(Context& ctx, ThisObject* outThis)
                {
                    (void)outThis;
                    return getLeftValue(ctx).getValue(location());
                }

                virtual LeftValue::Getter getLeftValue(Context& ctx) const
                {
                    (void)ctx;
                    throw Error::RuntimeError(location(), "expected lvalue to expression");
                }

                virtual Value execute(Context& ctx) override
                {
                    return evaluate(ctx, nullptr);
                }
        };

        class ConstantExpression: public Expression
        {
            public:
                explicit ConstantExpression(const Location& loc) : Expression{ loc }
                {
                }

        };

        class ConstantValue: public ConstantExpression
        {
            public:
                Value m_val;

            public:
                ConstantValue(const Location& loc, Value&& val) : ConstantExpression{ loc }, m_val{ std::move(val) }
                {
                    assert(m_val.type() == Value::Type::Null || m_val.isNumber() || m_val.isString());
                }

                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;

                virtual Value evaluate(Context&, ThisObject*) override
                {
                    return Value{ m_val };
                }
        };

        class Identifier : public ConstantExpression
        {
            public:
                enum class Scope
                {
                    None,
                    Local,
                    Global,
                    Count
                };

            public:
                Scope m_scope = Scope::Count;
                std::string m_ident;

            public:
                Identifier(const Location& loc, Scope scope, std::string&& s): ConstantExpression{ loc }, m_scope(scope), m_ident(std::move(s))
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value evaluate(Context& ctx, ThisObject* outThis) override;
                virtual LeftValue::Getter getLeftValue(Context& ctx) const override;
        };

        class ThisExpression: public ConstantExpression
        {
            public:
                ThisExpression(const Location& loc) : ConstantExpression{ loc }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value evaluate(Context& ctx, ThisObject* outThis) override;
        };

        class Operator: public Expression
        {
            public:
                explicit Operator(const Location& loc) : Expression{ loc }
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
                UnaryOperator(const Location& loc, Type type) : Operator{ loc }, m_type(type)
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value evaluate(Context& ctx, ThisObject* outThis) override;
                virtual LeftValue::Getter getLeftValue(Context& ctx) const override;
        };

        class MemberAccessOperator: public Operator
        {
            public:
                std::unique_ptr<Expression> m_operand;
                std::string m_membername;

            public:
                MemberAccessOperator(const Location& loc) : Operator{ loc }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value evaluate(Context& ctx, ThisObject* outThis) override;
                virtual LeftValue::Getter getLeftValue(Context& ctx) const override;
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
                std::unique_ptr<Expression> m_leftoper;
                std::unique_ptr<Expression> m_rightoper;

            private:
                Value ShiftLeft(const Value& lhs, const Value& rhs) const;
                Value ShiftRight(const Value& lhs, const Value& rhs) const;
                Value Assignment(LeftValue::Getter&& lhs, Value&& rhs) const;

            public:
                BinaryOperator(const Location& loc, Type type) : Operator{ loc }, m_type(type)
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value evaluate(Context& ctx, ThisObject* outThis) override;
                virtual LeftValue::Getter getLeftValue(Context& ctx) const override;
        };

        class TernaryOperator: public Operator
        {
            public:
                std::unique_ptr<Expression> m_condexpr;
                std::unique_ptr<Expression> m_trueexpr;
                std::unique_ptr<Expression> m_falseexpr;

            public:
                explicit TernaryOperator(const Location& loc) : Operator{ loc }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value evaluate(Context& ctx, ThisObject* outThis) override;
        };

        class CallOperator: public Operator
        {
            public:
                std::vector<std::unique_ptr<Expression>> m_oplist;

            public:
                CallOperator(const Location& loc) : Operator{ loc }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value evaluate(Context& ctx, ThisObject* outThis) override;
        };

        class FunctionDefinition : public Expression
        {
            public:
                std::vector<std::string> m_paramlist;
                Block m_body;

            public:
                FunctionDefinition(const Location& loc) : Expression{ loc }, m_body{ loc }
                {
                }

                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;

                virtual Value evaluate(Context&, ThisObject*) override
                {
                    return Value{ this };
                }

                bool areParamsUnique() const;

                Value call(Context& ctx, Value::List&& args, ThisObject& th);
        };

        class ObjectExpression : public Expression
        {
            public:
                using ItemMap = std::map<std::string, std::unique_ptr<Expression>>;

            public:
                std::unique_ptr<Expression> m_baseexpr;
                ItemMap m_exprmap;

            public:
                ObjectExpression(const Location& loc) : Expression{ loc }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value evaluate(Context& ctx, ThisObject* outThis) override;
        };

        class ArrayExpression : public Expression
        {
            public:
                std::vector<std::unique_ptr<Expression>> m_exprlist;

            public:
                ArrayExpression(const Location& loc) : Expression{ loc }
                {
                }
                virtual void debugPrint(DebugWriter& dw, uint32_t indentLevel, std::string_view prefix) const override;
                virtual Value evaluate(Context& ctx, ThisObject* outThis) override;
        };

    }

    class Parser
    {
        private:
            Tokenizer& m_tokenizer;
            std::vector<Token> m_toklist;
            size_t m_tokidx = 0;

        private:
            void parseBlock(Runtime::Block& outBlock);
            bool tryParseSwitchItem(Runtime::SwitchStatement& switchStatement);
            void parseFuncDef(Runtime::FunctionDefinition& funcDef);
            bool peekSymbols(std::initializer_list<Token::Type> arr);
            std::unique_ptr<Runtime::Statement> tryParseStatement();
            std::unique_ptr<Runtime::ConstantValue> tryParseConstVal();
            std::unique_ptr<Runtime::Identifier> tryParseIdentVal();
            std::unique_ptr<Runtime::ConstantExpression> tryParseConstExpr();
            std::pair<std::string, std::unique_ptr<Runtime::FunctionDefinition>> tryParseFuncSynSugar();
            std::unique_ptr<Runtime::Expression> tryParseClassSynSugar();
            std::unique_ptr<Runtime::Expression> tryParseObjMember(std::string& outMemberName);
            std::unique_ptr<Runtime::ObjectExpression> tryParseObject();
            std::unique_ptr<Runtime::ArrayExpression> tryParseArray();
            std::unique_ptr<Runtime::Expression> tryParseParentheses();
            std::unique_ptr<Runtime::Expression> tryParseUnary();
            std::unique_ptr<Runtime::Expression> tryParseOperator();
            std::unique_ptr<Runtime::Expression> tryParseBinary();
            std::unique_ptr<Runtime::Expression> tryParseAddSub();
            std::unique_ptr<Runtime::Expression> tryParseAngleSign();
            std::unique_ptr<Runtime::Expression> tryParseAngleCompare();
            std::unique_ptr<Runtime::Expression> tryParseEquals();
            std::unique_ptr<Runtime::Expression> TryParseExpr11();
            std::unique_ptr<Runtime::Expression> TryParseExpr12();
            std::unique_ptr<Runtime::Expression> TryParseExpr13();
            std::unique_ptr<Runtime::Expression> TryParseExpr14();
            std::unique_ptr<Runtime::Expression> TryParseExpr15();
            std::unique_ptr<Runtime::Expression> TryParseExpr16();
            std::unique_ptr<Runtime::Expression> TryParseExpr17();
            bool tryParseSymbol(Token::Type symbol);

            // If failed, returns empty string.
            std::string tryParseIdentifier();

            const Location& currentTokenLocation() const
            {
                return m_toklist[m_tokidx].m_place;
            }

        public:
            Parser(Tokenizer& tokenizer) : m_tokenizer(tokenizer)
            {
            }
            void parseScript(Runtime::Script& outScript);
    };
}
