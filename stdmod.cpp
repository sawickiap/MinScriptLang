
#include "priv.h"

namespace MSL
{
    namespace Builtins
    {
        Value ctor_null(AST::ExecutionContext& ctx, const Location& place, Value::List&& args)
        {
            (void)ctx;
            (void)place;
            (void)args;
            return {};
        }

        Value ctor_number(AST::ExecutionContext& ctx, const Location& place, Value::List&& args)
        {
            (void)ctx;
            auto val = Util::checkArgument(place, "Number", args, 0, Value::Type::Number);
            return Value{ val };
        }

        Value ctor_string(AST::ExecutionContext& ctx, const Location& place, Value::List&& args)
        {
            (void)ctx;
            if(args.empty())
            {
                return Value{ std::string{} };
            }
            auto strv = Util::checkArgument(place, "String", args, 0, Value::Type::String);
            return Value{ std::move(strv.string()) };
        }

        Value ctor_object(AST::ExecutionContext& ctx, const Location& place, Value::List&& args)
        {
            (void)ctx;
            if(args.empty())
            {
                return Value{ std::make_shared<Object>() };
            }
            if((args.size() == 0) || !args[0].isObject())
            {
                throw Error::ArgumentError(place, "Object can be constructed only from no arguments or from another object value.");
            }
            return Value{ Util::CopyObject(*args[0].object()) };
        }

        Value ctor_array(AST::ExecutionContext& ctx, const Location& place, Value::List&& args)
        {
            (void)ctx;
            if(args.empty())
            {
                return Value{ std::make_shared<Array>() };
            }
            if((args.size() == 0) || !args[0].isArray())
            {
                throw Error::ArgumentError(place, "Array can be constructed only from no arguments or from another array value.");
            }
            return Value{ Util::CopyArray(*args[0].array()) };
        }

        Value ctor_function(AST::ExecutionContext& ctx, const Location& place, Value::List&& args)
        {
            (void)ctx;
            if((args.size() == 0) || ((args[0].type() != Value::Type::Function) && (args[0].type() != Value::Type::MemberMethod)))
            {
                throw Error::ArgumentError(place, "Function can be constructed only from another function value.");
            }
            return Value{ args[0] };
        }

        Value ctor_type(AST::ExecutionContext& ctx, const Location& place, Value::List&& args)
        {
            (void)ctx;
            if((args.size() == 0) || (args[0].type() != Value::Type::Type))
            {
                
                throw Error::ArgumentError(place, "Type can be constructed only from another type value.");
            }
            return Value{ args[0] };
        }

        Value protofn_object_count(AST::ExecutionContext& ctx, const Location& place, Value&& objVal)
        {
            (void)ctx;
            Object* obj;
            if(!objVal.isObject() || ((obj = objVal.object()) == nullptr))
            {
                throw Error::TypeError(place, "Object() requires an object");
            }
            return Value{ (double)obj->size() };
        }

        Value protofn_array_length(AST::ExecutionContext& ctx, const Location& place, Value&& objVal)
        {
            (void)ctx;
            Array* arr;
            if(!objVal.isArray() || ((arr = objVal.array()) == nullptr))
            {
                throw Error::TypeError(place, "Array.length called on something not an Array");
            }
            return Value{ (double)objVal.array()->size() };
        }


        Value memberfn_array_push(AST::ExecutionContext& ctx, const Location& place, AST::ThisType& th, Value::List&& args)
        {
            size_t i;
            Array* arr;
            (void)ctx;
            (void)th;
            arr = th.array();
            if(!arr)
            {
                throw Error::TypeError(place, "Array.add() called on something not an Array");
            }
            if(args.size() == 0)
            {
                throw Error::ArgumentError(place, "Array.add() requires at least 1 argument");
            }
            for(i=0; i<args.size(); i++)
            {
                arr->push_back(std::move(args[i]));
            }
            return {};
        }

        Value memberfn_array_pop(AST::ExecutionContext& ctx, const Location& place, AST::ThisType& th, Value::List&& args)
        {
            Array* arr;
            (void)ctx;
            (void)th;
            (void)args;
            arr = th.array();
            if(!arr)
            {
                throw Error::TypeError(place, "Array.pop() called on something not an Array");
            }
            auto popped =arr->back();
            arr->pop_back();
            return popped;
        }

        Value memberfn_array_insert(AST::ExecutionContext& ctx, const Location& place, AST::ThisType& th, Value::List&& args)
        {
            Array* arr;
            (void)ctx;
            arr = th.array();
            if(!arr)
            {
                throw Error::ArgumentError(place, "Array.insert() called on something not an Array");
            }
            if(args.size() != 2)
            {
                throw Error::ArgumentError(place, "Array.insert() requires at least 2 arguments");
            }
            size_t index;
            index= 0;
            if(!args[0].isNumber() || !Util::NumberToIndex(index, args[0].number()))
            {
                throw Error::RuntimeError(place, "cannot insert out-of-bounds");
            }
            arr->insert(arr->begin() + index, std::move(args[1]));
            return {};
        }

        Value memberfn_array_remove(AST::ExecutionContext& ctx, const Location& place, AST::ThisType& th, Value::List&& args)
        {
            Array* arr;
            (void)ctx;
            arr = th.array();
            if(!arr)
            {
                throw Error::TypeError(place, "expected array object");
            }
            if(args.size() == 0)
            {
                throw Error::TypeError(place, "too few arguments");
            }
            size_t index = 0;
            if(!args[0].isNumber() || !Util::NumberToIndex(index, args[0].number()))
            {
                throw Error::RuntimeError(place, "cannot remove out-of-bounds");
            }
            arr->erase(arr->begin() + index);
            return {};
        }

        Value protofn_string_length(AST::ExecutionContext& ctx, const Location& place, Value&& objVal)
        {
            (void)ctx;
            if(!objVal.isString())
            {
                throw Error::TypeError(place, "expected string object");
            }
            return Value{ (double)objVal.string().length() };
        }

        Value protofn_string_chars(AST::ExecutionContext& ctx, const Location& place, Value&& objVal)
        {
            
            (void)ctx;
            if(!objVal.isString())
            {
                throw Error::TypeError(place, "expected string object");
            }
            auto res = std::make_shared<Array>();
            for(auto ch: objVal.string())
            {
                res->push_back(Value{double(ch)});
            }
            return Value{ std::move(res) };
        }

        Value protofn_string_stripleft(AST::ExecutionContext& ctx, const Location& place, Value&& objVal)
        {
            (void)ctx;
            (void)place;
            auto self = objVal.string();
            Util::stripInplaceLeft(self);
            return Value{std::move(self)};
        }

        Value protofn_string_stripright(AST::ExecutionContext& ctx, const Location& place, Value&& objVal)
        {
            (void)ctx;
            (void)place;
            auto self = objVal.string();
            Util::stripInplaceRight(self);
            return Value{std::move(self)};
        }

        Value protofn_string_strip(AST::ExecutionContext& ctx, const Location& place, Value&& objVal)
        {
            (void)ctx;
            (void)place;
            auto self = objVal.string();
            Util::stripInplace(self);
            return Value{std::move(self)};
        }

        Value memberfn_string_resize(AST::ExecutionContext& ctx, const Location& place, AST::ThisType& th, Value::List&& args)
        {
            // TODO
            (void)ctx;
            (void)place;
            (void)th;
            (void)args;
            return Value{};
        }

        Value memberfn_string_startswith(AST::ExecutionContext& ctx, const Location& place, AST::ThisType& th, Value::List&& args)
        {
            double res;
            (void)ctx;
            if(args.size() == 0)
            {
                throw Error::ArgumentError(place, "String.startsWith() needs exactly 1 argument");
            }
            auto findme = args[0];
            res = (th.string().rfind(findme.string(), 0) == 0);
            return Value{res};
        }

        Value memberfn_string_endswith(AST::ExecutionContext& ctx, const Location& place, AST::ThisType& th, Value::List&& args)
        {
            (void)ctx;
            const auto& self = th.string();
            auto findme = Util::checkArgument(place, "Strings.endsWith", args, 0, Value::Type::String);
            auto sf = findme.string();
            if(sf.size() > self.size())
            {
                return Value{double(0)};
            }
            return Value{double(std::equal(sf.rbegin(), sf.rend(), self.rbegin()))};
        }
    }
}

