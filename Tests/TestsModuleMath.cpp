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
#include "PCH.hpp"
#include "../Modules/ModuleMath.hpp"
#include "../MinScriptLang.hpp"
#include "../3rdParty/Catch2/catch.hpp"

using namespace MinScriptLang;

TEST_CASE("Module math")
{
    Environment env;
    ModuleMath::Setup(env);
    SECTION("abs")
    {
        const char* code = "a=-10; print(a, math.abs(a)); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "-10\n10\n");
    }
    SECTION("abs called incorrectly string")
    {
        const char* code = "print(math.abs('A')); \n";
        REQUIRE_THROWS_AS(env.Execute(code), ExecutionError);
    }
    SECTION("abs called incorrectly no arguments")
    {
        const char* code = "print(math.abs()); \n";
        REQUIRE_THROWS_AS(env.Execute(code), ExecutionError);
    }
    SECTION("Constants")
    {
        const char* code = "print(math.abs(math.E - 2.71828182) < 1e-6, math.abs(math.PI - 3.14159265) < 1e-6); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n1\n");
    }
    SECTION("abs called incorrectly no arguments inspect error")
    {
        const char* code = "try print(math.abs()); catch(ex) print(ex.message);\n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "Function math.abs received too few arguments. Number expected as argument 0.\n");
    }
    SECTION("abs called incorrectly too many arguments inspect error")
    {
        const char* code = "try print(math.abs(1, 2, 3)); catch(ex) print(ex.message);\n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "Function math.abs requires 1 arguments, 3 provided.\n");
    }
    SECTION("abs called incorrectly array inspect error")
    {
        const char* code = "try print(math.abs([1, 2, 3])); catch(ex) print(ex.message);\n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "Function math.abs received incorrect argument 0. Expected: Number, actual: Array.\n");
    }
}
