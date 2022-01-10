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
#include "ModuleMath.hpp"
#include "../MinScriptLang.hpp"
#include <cmath>

namespace ModuleMath
{

using namespace MinScriptLang;

static Value Func_abs(Environment& env, const PlaceInCode& place, std::vector<Value>&& args)
{
    MINSL_LOAD_ARGS_1_NUMBER("math.abs", arg);
    return Value{abs(arg)};
}

void Setup(MinScriptLang::Environment& targetEnv)
{
    auto mathObj = std::make_shared<Object>();
    
    // # Constants
    mathObj->GetOrCreateValue("PI") = Value{PI};
    mathObj->GetOrCreateValue("E") = Value{E};

    // # Functions
    mathObj->GetOrCreateValue("abs") = Value{Func_abs};

    targetEnv.GlobalScope.GetOrCreateValue("math") = Value{std::move(mathObj)};
}

} // namespace ModuleMath
