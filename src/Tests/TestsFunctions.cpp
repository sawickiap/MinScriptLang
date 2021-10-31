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

TEST_CASE("Functions")
{
    Environment env;
    SECTION("Basic function")
    {
        const char* code = "f = function() { print('Foo'); };\n"
            "f(); f();";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "Foo\nFoo\n");
    }
    SECTION("Local variables")
    {
        const char* code = "a=1; print(a); f=function(){ b=2; print(b); print(a); a=10; print(a); };\n"
            "f(); print(a);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n2\n1\n10\n10\n");
    }
    SECTION("Local variables invalid")
    {
        const char* code = "f=function(){ a=1; print(a); }; f(); print(a);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\nnull\n");
    }
    SECTION("Parameters")
    {
        const char* code = "f=function(a, b){ a='['+a+'] ['+b+']'; print(a); }; \n"
            "f('AAA', 'BBB'); f('CCC', 'DDD');";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "[AAA] [BBB]\n[CCC] [DDD]\n");
    }
    SECTION("Return")
    {
        const char* code = "functionNotReturning = function(){ print('functionNotReturning'); }; \n"
            "functionReturningNull = function(){ print('functionReturningNull'); return; print('DUPA'); }; \n"
            "functionReturningSomething = function(){ print('functionReturningSomething'); return 123; print('DUPA'); }; \n"
            "print(functionNotReturning()); print(functionReturningNull()); print(functionReturningSomething());";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "functionNotReturning\nnull\nfunctionReturningNull\nnull\nfunctionReturningSomething\n123\n");
    }
    SECTION("Return without function")
    {
        const char* code = "return 2;";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Syntactic sugar")
    {
        const char* code = "function add(a, b){ return a+b; }\n"
            "print(add(2, 5));";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "7\n");
    }
    SECTION("Parameters not unique")
    {
        const char* code = "function f(a, a) { }";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
    }
    SECTION("Recursion")
    {
        const char* code = "function factorial(n){ if(n==0) return 1; return n*factorial(n-1); } \n"
            "print(factorial(0)); print(factorial(3)); print(factorial(4));";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n6\n24\n");
    }
    SECTION("Break without loop in function")
    {
        const char* code = "function Bad() { break; } \n"
            "for(i=0; i<10; ++i) { Bad(); }";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "function Bad() { continue; } \n"
            "for(i=0; i<10; ++i) { Bad(); }";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Wrong number of parameters expected 0 passed 1")
    {
        const char* code = "function f() { print('a'); } f(2);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Wrong number of parameters expected 1 passed 0")
    {
        const char* code = "function f(a) { print(a); } f();";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Wrong number of parameters expected 1 passed 3")
    {
        const char* code = "function f(a) { print(a); } f(1, 2, 3);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Wrong number of parameters expected 4 passed 0")
    {
        const char* code = "function f(a, b, c, d) { print(a, b, c, d); } f();";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Wrong number of parameters expected 4 passed 2")
    {
        const char* code = "function f(a, b, c, d) { print(a, b, c, d); } f(1, 2);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Wrong number of parameters expected 4 passed 5")
    {
        const char* code = "function f(a, b, c, d) { print(a, b, c, d); } f('1', '2', '3' ,'4', '5');";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
}
