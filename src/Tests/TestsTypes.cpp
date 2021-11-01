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
#include "PCH.h"
#include "../MinScriptLang.h"
#include "../3rdParty/Catch2/catch.hpp"

using namespace MinScriptLang;

TEST_CASE("Types")
{
    Environment env;
    SECTION("Type identifier")
    {
        const char* code = "t1=Null; t2=Number; t3=String; t4=Object; \n"
            "print(t1, t2, t3, t4);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "Null\nNumber\nString\nObject\n");
    }
    SECTION("typeOf and comparisons")
    {
        const char* code = "tn1=Number; n2=123; tn2=typeOf(n2); tnull=typeOf(nonExistent); \n"
            "print(tn1, tn2, tnull); \n"
            "print(tn1==tn2, tn1!=tn2, tn2==tnull, tn2!=tnull); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "Number\nNumber\nNull\n"
            "1\n0\n0\n1\n");
    }
    SECTION("Type conversion to bool")
    {
        const char* code = "tobj=typeOf({a:1, b:2}); tnull=typeOf(nonExistent); \n"
            "print(tobj?1:0, tnull?1:0); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n0\n");
    }
    SECTION("Null construction")
    {
        const char* code = "v1=Null(); v2=Null(nonExistent); \n"
            "print(v1, v2); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "null\nnull\n");
    }
    SECTION("Null construction invalid")
    {
        const char* code = "v1=Null(); v2=Null(123); \n"
            "print(v1, v2); \n";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Number construction")
    {
        const char* code = "v1=2; v2=Number(v1); \n"
            "print(v1, v2); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "2\n2\n");
    }
    SECTION("Number construction invalid")
    {
        const char* code = "v2=Number('A'); \n"
            "print(v2); \n";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("String construction")
    {
        const char* code = "v1='A'; v2=String(v1); \n"
            "print(v1, v2); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "A\nA\n");
    }
    SECTION("String construction invalid")
    {
        const char* code = "v2=String('A', 'B', 123); \n";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Object construction")
    {
        const char* code = "v1={a:1, b:2}; v2=Object(v1); \n"
            "print(v1.a, v2.a, v1==v2); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n1\n0\n");
    }
    SECTION("Object construction invalid")
    {
        const char* code = "v2=Object(123, 'A'); \n";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Function construction")
    {
        const char* code = "function f(){return 123;} v2=Function(f); v3=print;\n"
            "print(f==v2, v2==v3, v2()); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n0\n123\n");
    }
    SECTION("Function construction invalid")
    {
        const char* code = "v2=Function(); \n";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Type construction")
    {
        const char* code = "v1=typeOf(125); v2=Type(v1); v3=v2(123);\n"
            "print(v1==v2, v3); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n123\n");
    }
    SECTION("Type construction invalid")
    {
        const char* code = "v2=Type(123); \n";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
}
