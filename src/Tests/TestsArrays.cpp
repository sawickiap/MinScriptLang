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

TEST_CASE("Arrays")
{
    Environment env;
    SECTION("Empty array definition")
    {
        const char* code = "a1=[]; print(a1); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "array\n");
    }
    SECTION("Array definition and indexing as rvalue")
    {
        const char* code = "a1=[1, 2, 3]; \n"
            "print(a1[0], a1[1], a1[2]); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n2\n3\n");
    }
    SECTION("Array definition and indexing as lvalue")
    {
        const char* code = "a1=[1, 2, 3, 4, 5]; \n"
            "a1[0]=10; a1[1]+=10; a1[2]*=10; a1[3]++; ++a1[4];\n"
            "print(a1[0], a1[1], a1[2], a1[3], a1[4]); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "10\n12\n30\n5\n6\n");
    }
    SECTION("Invalid array indexing with other value type")
    {
        const char* code = "a=[1, 2, 3]; print(a[Number]); \n";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Invalid array indexing out of bounds")
    {
        const char* code = "a=[1, 2, 3]; print(a[100]); \n";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Passing and returning array by reference")
    {
        const char* code = "a1=[1, 2, 3]; a2=a1; \n"
            "function f(arg) { arg[1]++; return arg; } \n"
            "a3=f(a2); \n"
            "print(a1[0], a1[1], a1[2], a3[1], a1==a3, a1!=a3); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n3\n3\n3\n1\n0\n");
    }
    SECTION("Array constructor to copy")
    {
        const char* code = "a1=[1, 2, 3]; a2=Array(a1); a0=Array(); \n"
            "print(a2[0], a2[1], a2[2], a1==a2, a1==a0); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n2\n3\n0\n0\n");
    }
    SECTION("Array definition with trailing comma")
    {
        const char* code = "a=[1, 2, 3,]; print(a[0], a[1], a[2]);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n2\n3\n");
    }
    SECTION("Null as array item")
    {
        const char* code = "a=[1, 2, 3]; a[1]=null; a[2]=null; \n"
            "print(a[0], a[1], a[2]);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\nnull\nnull\n");
    }
    SECTION("Range-based for loop for array")
    {
        const char* code = "a=[1, 2, 3]; \n"
            "for(val : a) print(val); \n"
            "print(val); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n2\n3\n"
            "null\n");
    }
    SECTION("Range-based for loop for array with iterator")
    {
        const char* code = "a=[1, 2, 3]; \n"
            "for(i, val : a) { { print(i, val); } ;;; } \n"
            "print(i, val); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n1\n1\n2\n2\n3\n"
            "null\nnull\n");
    }
    SECTION("For loop always sets local variable")
    {
        const char* code = "function f() { \n"
            "  a=[1, 2, 3]; \n"
            "  for(i, v : a) { print(i, v); } \n"
            "} \n"
            "i='A'; v='B'; \n"
            "f(); \n"
            "print(i, v); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n1\n1\n2\n2\n3\n"
            "A\nB\n");
    }
    SECTION("Array conversion to bool")
    {
        const char* code = "a0=[]; a3=[1, 2, 3]; n=null;\n"
            "print(a0?'Y':'N', a3?'Y':'N', n?'Y':'N'); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "Y\nY\nN\n");
    }
    SECTION("Array Count property")
    {
        const char* code = "a0=[]; a3=[1, 2, 3]; \n"
            "print(a0.Count, a3.Count); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n3\n");
    }
    SECTION("Array methods Add Insert Remove")
    {
        const char* code = "a=[1, 2, 3]; \n"
            "a.Add(4); a.Add(5); a.Add(6); /* 1, 2, 3, 4, 5, 6 */ \n"
            "a.Remove(0); a.Remove(a.Count-1); a.Remove(1); /* 2, 4, 5 */ \n"
            "a.Insert(3, 100); a.Insert(2, 101); a.Insert(0, 102); /* 102, 2, 4, 101, 5, 100 */ \n"
            "print(a.Count, a[0], a[1], a[2], a[3], a[4], a[5]); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "6\n102\n2\n4\n101\n5\n100\n");
    }
    SECTION("Incorrect calling array method for object")
    {
        const char* code = "a=[1, 2, 3]; obj={fn: a.Add}; obj.fn(1);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
}
