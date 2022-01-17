
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
                string().push_back(right.number());
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
                string().push_back(int(rhs.number()));
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
}



