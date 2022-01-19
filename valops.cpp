
#include "priv.h"

namespace MSL
{
    
    static Value binaryBadTypes(const Location& loc, std::string_view opstr, Value::Type tleft, Value::Type tright)
    {
        throw Error::TypeError(loc,
            Util::joinArgs("incompatible types ", Value::getTypename(tleft), ", ", Value::getTypename(tright), " for '", opstr, "'")
        );
        return Value{};
    }

    template<typename InT>
    static Number::IntegerValueType shiftLeft(const InT& lhs, const InT& rhs)
    {
        const Number::IntegerValueType leftnum = (Number::IntegerValueType)lhs;
        const Number::IntegerValueType rightnum = (Number::IntegerValueType)rhs;
        const Number::IntegerValueType resval = leftnum << rightnum;
        return ((Number::IntegerValueType)resval);
    }

    template<typename InT>
    static Number::IntegerValueType shiftRight(const InT& lhs, const InT& rhs)
    {
        const Number::IntegerValueType leftnum = (Number::IntegerValueType)lhs;
        const Number::IntegerValueType rightnum = (Number::IntegerValueType)rhs;
        const Number::IntegerValueType resval = leftnum >> rightnum;
        return ((Number::IntegerValueType)resval);
    }

    bool Value::opCompareLessThan(const Value& rhs)
    {
        if(isNumber() && rhs.isNumber())
        {
            return (number() < rhs.number());
        }
        else if(isString() && rhs.isString())
        {
            return (string() < rhs.string());
        }
        binaryBadTypes(location(), "<", type(), rhs.type());
        return false;
    }

    bool Value::opCompareLessEqual(const Value& rhs)
    {
        if(isNumber() && rhs.isNumber())
        {
            return (number() <= rhs.number());
        }
        else if(isString() && rhs.isString())
        {
            return (string() <= rhs.string());
        }
        binaryBadTypes(location(), "<=", type(), rhs.type());
        return false;
    }

    bool Value::opCompareGreaterThan(const Value& rhs)
    {
        if(isNumber() && rhs.isNumber())
        {
            return (number() > rhs.number());
        }
        else if(isString() && rhs.isString())
        {
            return (string() > rhs.string());
        }
        binaryBadTypes(location(), ">", type(), rhs.type());
        return false;
    }

    bool Value::opCompareGreaterEqual(const Value& rhs)
    {
        if(isNumber() && rhs.isNumber())
        {
            return (number() >= rhs.number());
        }
        else if(isString() && rhs.isString())
        {
            return (string() >= rhs.string());
        }
        binaryBadTypes(location(), ">=", type(), rhs.type());
        return false;
    }

    Value Value::opPlus(const Value& right)
    {
        auto typleft = type();
        auto typright = right.type();
        if(typleft == Value::Type::Number && typright == Value::Type::Number)
        {
            return Value{ number() + right.number() };
        }
        else if(typleft == Value::Type::String)
        {
            if(typright == Value::Type::String)
            {
                return Value{ string() + right.string() };
            }
            else if(typright == Value::Type::Number)
            {
                string().push_back(right.number().toInteger());
                return Value{ std::move(string()) };
            }
            else
            {
                string() += right.toString();
                return Value{ std::move(string()) };
            }
        }
        else if(typleft == Value::Type::Array)
        {
            auto nary = Util::CopyArray(*array());
            nary->push_back(right);
            return Value{std::move(nary)};
        }
        return binaryBadTypes(location(), "+", typleft, typright);
    }


    Value Value::opMinus(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            return binaryBadTypes(location(), "-", type(), rhs.type());
        }
        return Value{ number() - rhs.number() };
    }

    Value Value::opMul(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            return binaryBadTypes(location(), "*", type(), rhs.type());
        }
        return Value{ number() * rhs.number() };
    }

    Value Value::opDiv(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            return binaryBadTypes(location(), "/", type(), rhs.type());
        }
        return Value{ number() / rhs.number() };
    }

    Value Value::opMod(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            return binaryBadTypes(location(), "%", type(), rhs.type());
        }
        return Value{ number() % rhs.number() };
    }

    Value Value::opShiftLeft(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            return binaryBadTypes(location(), "<<", type(), rhs.type());
        }
        return Value{ Number::makeInteger(number().toInteger() << rhs.number().toInteger()) };
    }

    Value Value::opShiftRight(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            return binaryBadTypes(location(), ">>", type(), rhs.type());
        }
        return Value{ Number::makeInteger(number().toInteger() >> rhs.number().toInteger()) };
    }

    Value Value::opBitwiseAnd(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            return binaryBadTypes(location(), "&", type(), rhs.type());
        }
        return Value{ number() & rhs.number() };
    }

    Value Value::opBitwiseXor(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            return binaryBadTypes(location(), "^", type(), rhs.type());
        }
        return Value{ Number::makeInteger(number().toInteger() ^ rhs.number().toInteger()) };
    }

    Value Value::opBitwiseOr(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            return binaryBadTypes(location(), "|", type(), rhs.type());
        }
        return Value{ number() | rhs.number() };
    }

    Value& Value::opPlusAssign(const Value& rhs)
    {
        if(isNumber() && rhs.isNumber())
        {
            setNumberValue(number() + rhs.number());
        }
        else if(isString())
        {
            if(rhs.isString())
            {
                string() += rhs.string();
            }
            else if(rhs.isNumber())
            {
                string().push_back(int(rhs.number().toInteger()));
            }
            else
            {
                string() += rhs.toString();
            }
            return *this;
        }
        else if(isArray())
        {
            array()->push_back(std::move(rhs));
            return *this;
        }
        else
        {
            binaryBadTypes(location(), "+=", type(), rhs.type());
        }
        return *this;
    }

    Value& Value::opMinusAssign(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            binaryBadTypes(location(), "-=", type(), rhs.type());
        }
        //number() -= rhs.number();
        setNumberValue(number() - rhs.number());
        return *this;
    }


    Value& Value::opMulAssign(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            binaryBadTypes(location(), "*=", type(), rhs.type());
        }
        //number() *= rhs.number();
        setNumberValue(number() * rhs.number());
        return *this;
    }

    Value& Value::opDivAssign(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            binaryBadTypes(location(), "/=", type(), rhs.type());
        }
        setNumberValue(number() / rhs.number());
        return *this;
    }

    Value& Value::opModAssign(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            binaryBadTypes(location(), "%=", type(), rhs.type());
        }
        setNumberValue(number() % rhs.number());
        return *this;
    }

    Value& Value::opShiftLeftAssign(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            binaryBadTypes(location(), "<<=", type(), rhs.type());
        }
        setNumberValue(Number::makeInteger(shiftLeft(number().toInteger(), rhs.number().toInteger())));
        return *this;
    }

    Value& Value::opShiftRightAssign(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            binaryBadTypes(location(), ">>=", type(), rhs.type());
        }
        setNumberValue(Number::makeInteger(shiftRight(number().toInteger(), rhs.number().toInteger())));
        return *this;
    }

    Value& Value::opBitwiseAndAssign(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            binaryBadTypes(location(), "&=", type(), rhs.type());
        }
        setNumberValue(number() & rhs.number());
        return *this;
    }

    Value& Value::opBitwiseOrAssign(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            binaryBadTypes(location(), "|=", type(), rhs.type());
        }
        setNumberValue(number() | rhs.number());
        return *this;
    }

    Value& Value::opBitwiseXorAssign(const Value& rhs)
    {
        if(!isNumber() || !rhs.isNumber())
        {
            binaryBadTypes(location(), "^=", type(), rhs.type());
        }
        setNumberValue(Number::makeInteger(number().toInteger() ^ rhs.number().toInteger()));
        return *this;
    }

    Value&& Value::opIndex(const Value& rhs, ThisObject* othis)
    {
        size_t index;
        Value defretval;
        auto typleft = type();
        auto typright = rhs.type();
        if(typleft == Value::Type::String)
        {
            if(typright != Value::Type::Number)
            {
                //throw Error::TypeError(location(), "");
                binaryBadTypes(location(), "string index", typleft, typright);
            }
            index = 0;
            //MINSL_EXECUTION_CHECK(Util::NumberToIndex(index, rhs.number()), location(), "string index out of bounds");
            if(!Util::NumberToIndex(index, rhs.number()))
            {
                throw Error::IndexError(location(), Util::joinArgs("string index ", rhs.number(), " too large"));
            }
            //MINSL_EXECUTION_CHECK(index < string().length(), location(), "string index out of bounds");
            if(index > string().length())
            {
                throw Error::IndexError(location(), Util::joinArgs("string index ", index, " out of bounds"));
            }
            return std::move(Value{ std::string(1, string()[index]) });
        }
        if(typleft == Value::Type::Object)
        {
            //MINSL_EXECUTION_CHECK(typright == Value::Type::String, location(), "expected string value");
            if(typright != Value::Type::String)
            {
                //throw Error::IndexError(location(), Util::joinArgs("object index must be String, got ", Value::getTypename(typright), " instead"));
                binaryBadTypes(location(), "object index", typleft, typright);
            }
            auto val = object()->tryGet(rhs.string());
            if(val)
            {
                if(othis)
                {
                    *othis = ThisObject{ objectRef() };
                }
                return std::move(*val);
            }
            return std::move(Value{});
        }
        if(typleft == Value::Type::Array)
        {
            //MINSL_EXECUTION_CHECK(typright == Value::Type::Number, location(), "expected numeric value");
            if(typright != Value::Type::Number)
            {
                binaryBadTypes(location(), "array index", typleft, typright);
            }
            //MINSL_EXECUTION_CHECK(Util::NumberToIndex(index, rhs.number()) && index < array()->size(), location(), "array index out of bounds");
            if(!Util::NumberToIndex(index, rhs.number()))
            {
                throw Error::IndexError(location(), Util::joinArgs("array index ", rhs.number(), " too large"));
            }
            if(index > array()->size())
            {
                throw Error::IndexError(location(), Util::joinArgs("array index ", index, " out of bounds"));
            }
            return std::move(array()->at(index));
        }
        //throw Error::TypeError(location(), "cannot index this type");
        binaryBadTypes(location(), "[]", typleft, typright);
        return std::move(Value{});
    }
}



