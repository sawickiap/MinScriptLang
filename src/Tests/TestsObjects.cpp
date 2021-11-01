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

TEST_CASE("Objects")
{
    Environment env;
    SECTION("Initialization and member access")
    {
        const char* code = 
            "a={}; print(a); print(a.x);"
            "a={'x':2}; print(a); print(a.x);"
            "a={'x':2,}; print(a); print(a.x);"
            "a={'x':2,'y':3}; print(a); print(a.y);"
            "a={'x':2,'y':3,}; print(a); print(a.y);"
            "a.z=5; a.w=a.z; a.z=4; print(a.z); print(a.w);";
        env.Execute(code);
        REQUIRE(env.GetOutput() ==
            "object\nnull\n"
            "object\n2\n"
            "object\n2\n"
            "object\n3\n"
            "object\n3\n"
            "4\n5\n");
    }
    SECTION("Object definition using identifiers")
    {
        const char* code = "obj={a:1, b:2, c:3}; print(obj.a, obj.b, obj.c);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n2\n3\n");
    }
    SECTION("Compound string in object definition")
    {
        const char* code = "a={ 'AAA' /*comment*/ \"BBB\": 5 }; print(a.AAABBB);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "5\n");
    }
    SECTION("Object definition versus block")
    {
        const char* code = "{ print(1); { print(2); } { } print(3); } { }; {'a':1, 'b':false}; { print(4); } print(5);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n2\n3\n4\n5\n");
    }
    SECTION("Nested object")
    {
        const char* code =
            "o1 = { 'x': 1 }; \n"
            "o2 = { 'y': o1 }; \n"
            "print(o2.y.x); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n");
    }
    SECTION("Nested object definition")
    {
        const char* code =
            "o1 = { 'x': { 'y': 5 } }; \n"
            "o1.x.y += 5; \n"
            "print(o1.x.y); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "10\n");
    }
    SECTION("Passing object around by reference")
    {
        const char* code =
            "o1 = { 'x': 1 }; \n"
            "o2 = o1; \n"
            "o2.x += 1; \n"
            "function f(arg) { arg.x += 2; return arg; } \n"
            "print(f(o2).x); \n"
            "print(o1.x); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "4\n4\n");
    }
    SECTION("Operator sequence Expr2")
    {
        const char* code =
            "o1 = { 'f1': function() { return o2; }, 'val1': 1 }; \n"
            "o2 = { 'f2': function() { return o1; }, 'val2': 2 }; \n"
            "print(o1.f1().f2().val1); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n");
    }
    SECTION("Object indexing")
    {
        const char* code =
            "o1 = { 'x': 1, 'y': 2, 'a b c': 3 }; \n"
            "index = 'y'; \n"
            "print(o1['x'], o1[index], o1['a b' + ' c']); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n2\n3\n");
    }
    SECTION("Object indexing nested")
    {
        const char* code =
            "o1 = { 'x': { 'y': { 'z': 5 } } }; \n"
            "print(o1['x'].y['z']); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "5\n");
    }
    SECTION("Object indexing l-value")
    {
        const char* code =
            "o={}; o['A A'] = 1; o['A A'] += 5; \n"
            "print(o['A A']); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "6\n");
    }
    SECTION("Object indexing nested")
    {
        const char* code =
            "o={}; o['A'] = {}; o['A']['B'] = {}; o['A']['B'].c = 10; \n"
            "print(o.A.B['c']); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "10\n");
    }
    SECTION("Equality of objects")
    {
        const char* code =
            "o1={'a':1, 'b':2}; o2=o1; o3={'a':1, 'b':2}; \n"
            "print(o1==o1, o1==o2, o1==o3, o1!=o1, o1!=o2, o1!=o3); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n1\n0\n0\n0\n1\n");
    }
    SECTION("Count and setting unsetting members")
    {
        const char* code =
            "obj={'a':1, 'b':2, 'd':null}; obj.c=3; obj.b=null; \n"
            "print(obj.Count); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "2\n");
    }
    SECTION("Object with repeating keys")
    {
        const char* code = "obj={'a':1, 'b':2, 'a':3};";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
    }
    SECTION("Range-based for loop for objects")
    {
        const char* code =
            "obj={'a':1, 'bbbb':4, 'cc':10}; \n"
            "sum = 0; for(val: obj) sum += val; print(sum);\n"
            "sumKeyLen = 0; sumVal = 0; for(key, val: obj) { sumKeyLen += key.Count; sumVal += val; } print(sumKeyLen, sumVal); \n"
            "print(key, val); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "15\n7\n15\n"
            "null\nnull\n");
    }
    SECTION("Implicit this")
    {
        const char* code =
            "obj={ 'x': 2, 'f1': function() { x += 1; print(x); } }; \n"
            "obj.f1(); obj['f1'](); (101, '102', obj.f1)(); \n"
            "obj2={'subObj':obj}; obj2.subObj.f1();";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "3\n4\n5\n6\n");
    }
    SECTION("Explicit this")
    {
        const char* code =
            "obj={ 'x': 2, 'f1': function() { this.y = 1; this.x += y; print(this.x); } }; \n"
            "obj.f1(); obj['f1'](); (101, '102', obj.f1)(); \n"
            "obj2={'subObj':obj}; obj2.subObj.f1();";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "3\n4\n5\n6\n");
    }
    SECTION("Returning this")
    {
        const char* code =
            "obj={ 'x': 2, 'f': function() { this.x += 1; return this; } }; \n"
            "obj2 = obj.f(); print(obj2.x); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "3\n");
    }
    // It used to pass this together with value before the rules was changed in the language. Now it should fail.
    SECTION("Passing function together with this")
    {
        const char* code =
            "obj={ 'x': 2, 'f': function() { this.x += 1; print(this.x); } }; \n"
            "function call(a) { a(); } \n"
            "call(obj.f); call(obj.f); \n";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("This in member access and indexing operator")
    {
        const char* code =
            "obj={ 'x': 2, function f() { print(++this.x); } }; \n"
            "obj.f(); obj['f'](); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "3\n4\n");
    }
    SECTION("This in grouping and comma operator")
    {
        const char* code =
            "obj={ 'x': 2, function f() { print(++this.x); } }; \n"
            "((obj)).f(); (111, 'AAA', obj.f)(); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "3\n4\n");
    }
    SECTION("This in ternary operator")
    {
        const char* code =
            "obj1={ x: 2, f: function() { print(++this.x); } }; \n"
            "obj2={ f: function() { print('obj2f'); } }; \n"
            "function call_f(cond) { (cond ? obj1 : obj2).f(); } \n"
            "call_f(true); call_f(false); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "3\nobj2f\n");
    }
    SECTION("This not preserved when assigned")
    {
        const char* code =
            "x=100; \n"
            "obj={ x: 2, f: function() { print(++x); } }; \n"
            "fn = obj.f; \n"
            "fn(); obj.f(); fn(); obj.f(); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "101\n3\n102\n4\n");
    }
    SECTION("Nested this")
    {
        const char* code =
            "objInner = { x: 2, f: function() { print(++x); } }; \n"
            "objOuter = { o: objInner }; \n"
            "objOuter.o.f(); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "3\n");
    }
    SECTION("Calling a method from another method")
    {
        const char* code =
            "obj={ 'x': 2, 'f1': function() { f2(); this.f2(); }, 'f2': function() { print(x); print(this.x); } }; \n"
            "obj.f1(); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "2\n2\n2\n2\n");
    }
    SECTION("Invalid use of this in global scope")
    {
        const char* code = "print(this.x);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Invalid use of this in local scope")
    {
        const char* code = "function f() { print(this.x); } \n"
            "f();";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Function syntactic sugar in object definition")
    {
        const char* code = "obj={ var: 2, function fn() { print(var); } }; \n"
            "obj.fn(); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "2\n");
    }
    SECTION("Calling object default function")
    {
        const char* code = "obj={ var: 2, function fn() { print('fn'); }, '':function(a) { print('Default', a); } }; \n"
            "obj(3); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "Default\n3\n");
    }
    SECTION("Class syntactic sugar")
    {
        const char* code = "class C { \n"
            "   var: 1, \n"
            "   '': function(x) { var=x; }, \n"
            "   function print() { global.print(this.var); } \n"
            "} \n"
            "C(2); C.print(); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "2\n");
    }
    SECTION("Class inheritance syntax")
    {
        const char* code = "class A { \n"
            "   var: 1, \n"
            "   '': function(x) { var=x; }, \n"
            "   function print() { global.print(this.var); } \n"
            "} \n"
            "class B : A { \n"
            "   '': function(x) { var=x+1; } \n"
            "} \n"
            " \n"
            "B(2); B.print(); A.print(); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "3\n1\n");
    }
    SECTION("Null member in derived class")
    {
        const char* code = "class Base { a: 121, b: 122, c: 123 }; \n"
            "class Derived : Base { b: 124, c: null }; \n"
            "print(Derived.a, Derived.b, Derived.c);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "121\n124\nnull\n");
    }
    SECTION("Object conversion to bool")
    {
        const char* code = "o1={a:123}; o2={}; \n"
            "print(o1?'T':'F', o2?'T':'F');";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "T\nT\n");
    }
    SECTION("Object definitions with trailing comma")
    {
        const char* code = "o1={a:123, b:124,}; o2={'a':o1.a, 'b':o1.b,}; \n"
            "print(o1['a'], o1['b'], o2.a, o2.b);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "123\n124\n123\n124\n");
    }
}
