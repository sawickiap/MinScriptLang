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
#include "ModuleMath.hpp"
#include "../MinScriptLang.hpp"
#include <cmath>

namespace ModuleMath
{

using namespace MinScriptLang;

static Value Func_abs(Environment&, const PlaceInCode& place, std::vector<Value>&& args)
{
    MINSL_EXECUTION_CHECK(args.size() == 1 && args[0].GetType() == ValueType::Number,
        place, "Function math.abs requires 1 number argument.");
    return Value{abs(args[0].GetNumber())};
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
