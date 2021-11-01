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

TEST_CASE("Basic")
{
    Environment env;
    SECTION("Comments empty statements")
    {
        const char* code = "// Single line comment\n"
            "/* multi line comment \n"
            "*/ ;;;; \n";
        env.Execute(code);
        REQUIRE(env.GetOutput().empty());
    }
    SECTION("Unexpected end of comment")
    {
        const char* code = "/* foo";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
    }
    SECTION("Number formats")
    {
        const char* code = "print(123); print(-00444); print(+0xaF250); print(-0xFF); print(0xAA00FF5544CD);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "123\n-444\n717392\n-255\n1.86921e+14\n");
    }
    SECTION("Floating point number variants")
    {
        const char* code = "print(01.00); print(10.5); print(23.); print(.25); print(1e3); print(1e+2); print(.001e-1); print(3.E+0);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n10.5\n23\n0.25\n1000\n100\n0.0001\n3\n");
    }
    SECTION("Invalid floating point number variant 1")
    {
        const char* code = "print(.);";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
    }
    SECTION("Invalid floating point number variant 2")
    {
        const char* code = "print(2.0e);";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
    }
    SECTION("Characters after number")
    {
        const char* code = "123print(1);";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
    }
    SECTION("Missing semicolon")
    {
        const char* code = "print(1)";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
    }
    SECTION("Garbage after last statement")
    {
        const char* code = "print(1); $~!@#$%^^&*()}";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
    }
    SECTION("Print Add Sub")
    {
        const char* code = "print(2 + 6 - 3);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "5\n");
    }
    SECTION("Mul Div Mod arithmetic operator precedence")
    {
        const char* code = "print(2 + 3 * 4); print(2 - 10 / 2 + 7 % 3); print(60 / 3 * 2);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "14\n-2\n40\n");
    }
    SECTION("Round brackets expression grouping")
    {
        const char* code = "print(2 + 3 * ((4))); print((2 + 3) * 4);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "14\n20\n");
    }
    SECTION("Unary operators")
    {
        const char* code = "a=4; print(a, +a, -a);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "4\n4\n-4\n");
    }
    SECTION("Unary plus on non-number") // To make sure it doesn't just pass-through any value.
    {
        const char* code = "a='AAA'; print(+a);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Curly brackets block")
    {
        const char* code = "print(1); { print(2); print(3); } print(4); { }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n2\n3\n4\n");
    }
    SECTION("Constants null false true")
    {
        const char* code = "print(null); print(false); print(true); print(true + true - false);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "null\n0\n1\n2\n");
    }
    SECTION("If conditions")
    {
        const char* code = "if(true) print(true); if(false) print(2); if(2); if(2-2) { } else { print(10); print(11); }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n10\n11\n");
    }
    SECTION("If conditions nested")
    {
        const char* code = "if(false) print(1); else if(false) print(2); else print(3); \n"
            "if(true) if(false) print(1); else print(2);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "3\n2\n");
    }
    SECTION("Ternary operator")
    {
        const char* code = "print(false ? 1 : 0); print(true ? false ? 1 : 2 : true ? 3 : 4);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n2\n");
    }
    SECTION("Variables")
    {
        const char* code = "a=1; print(a); b=a+1; print(b); c=d=b*b; print(b, c, d);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n2\n2\n4\n4\n");
    }
    SECTION("Variable name")
    {
        const char* code = "a1234_3252saczf434=1; print(a1234_3252saczf434+1);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "2\n");
    }
    SECTION("while loop")
    {
        const char* code = "a=6; while(a) { print(a); a = a - 2; }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "6\n4\n2\n");
    }
    SECTION("do while loop")
    {
        const char* code = "a=6; do { print(a); a = a - 2; } while(a);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "6\n4\n2\n");
    }
    SECTION("for loop")
    {
        const char* code = "for(a=10; a; a=a-2) { print(a); }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "10\n8\n6\n4\n2\n");
    }
    SECTION("For loop with empty statement")
    {
        const char* code = "for(a=0; a<10; ++a) ; print('a');";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "a\n");
    }
    SECTION("for loop with empty sections")
    {
        const char* code = "a=10; for(; a;) { print(a); a=a-2; }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "10\n8\n6\n4\n2\n");
    }
    SECTION("Preincrementation")
    {
        const char* code = "for(a=-4; a; ++a) { print(a); }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "-4\n-3\n-2\n-1\n");
    }
    SECTION("Logical operators")
    {
        const char* code = "print(!true); print(!!0);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n0\n");
    }
    SECTION("Bitwise shift")
    {
        const char* code = "a=2; print(a<<0); print(a<<2); print(a<<33); print(a>>1); print(a>>2); print(-a<<3); print(-256>>4);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "2\n8\n1.71799e+10\n1\n0\n-16\n-16\n");
    }
    SECTION("Bitwise not")
    {
        const char* code = "a=226; a=~a+1; print(a);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "-226\n");
    }
    SECTION("Comparisons")
    {
        const char* code = "a=-10; b=2; c=1000000000000; print(a<b); print(c<=b); print(c>a); print(b>=b); \n"
            "print(a==a); print(a==b); print(a!=b); print(b==4/2);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n0\n1\n1\n"
            "1\n0\n1\n1\n");
    }
    SECTION("Bitwise operators")
    {
        const char* code = "a=4294967295; /*0xFFFFFFFF*/ print(a&10); print(a&1|4); print(16 | a & 65535 ^ 12345);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "10\n5\n53206\n");
    }
    SECTION("Logical operators")
    {
        const char* code = "a=3; print(1==1 && a==3); print(false || true); print(false || true && false);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n1\n0\n");
    }
    SECTION("Logical operators short circuit")
    {
        const char* code = "(print(101)||true)&&print(102); (print(201)||false)&&print(202);"
            "print(301)||print(302); (print(401)||true)||print(402);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "101\n102\n201\n"
            "301\n302\n401\n");
    }
    SECTION("Logical operators returning value not bool")
    {
        const char* code = "five=5; six=6; zero=0; nth=null; \n"
            "print(five && zero); print(zero && five); print(zero && zero); print(five && six); print(zero && nth); print(nth && zero); \n"
            "print(five || zero); print(zero || five); print(zero || zero); print(five || six); print(zero || nth); print(nth || zero); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n0\n0\n6\n0\nnull\n"
            "5\n5\n0\n5\nnull\n0\n");
    }
    SECTION("Comma operator")
    {
        const char* code = "a=(1,2,4); print(a, 3, 5); print((a, 3, 5));";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "4\n3\n5\n5\n");
    }
    SECTION("Legit for loop")
    {
        const char* code = "for(i=0; i<5; ++i) print(i);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n1\n2\n3\n4\n");
    }
    SECTION("Loop break")
    {
        const char* code = "for(i=0;; ++i) { print(i); if(i>=5) break; }"
            "i=5; while(true) { print(i); break; }"
            "do { print(i); if(--i==0) break; } while(true);"
            "i=0; for(;;) { print(i); if(++i==4) { break; } }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n1\n2\n3\n4\n5\n"
            "5\n"
            "5\n4\n3\n2\n1\n"
            "0\n1\n2\n3\n");
    }
    SECTION("Loop continue")
    {
        const char* code = "for(i=0; i<10; ++i) { if(i>5) continue; print(i); }"
            "i=-10; while(true) { ++i; if(i<-5) { continue; } print(i); if(i>0) break; }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n1\n2\n3\n4\n5\n"
            "-5\n-4\n-3\n-2\n-1\n0\n1\n");
    }
    SECTION("Error break without a loop")
    {
        const char* code = "if(true) { do { print(1); }while(false); break; }";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Postincrementation postdecrementation")
    {
        const char* code = "for(i=0; i<3; i++) print(i); print(i++); print(i); print(i--); print(i);"
            "";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n1\n2\n3\n4\n4\n3\n");
    }
    SECTION("Compound assignment")
    {
        const char* code = "a=1; a+=3; print(a); a-=10; print(a); a*=-2; print(a); a/=4; print(a); a%=2; print(a); a<<=2; print(a); a>>=1; print(a);"
            "a=6; a|=8; print(a); a&=3; print(a); a^=3; print(a);"
            "a=1; b=2; b*=a+=7;  print(a);  print(b);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "4\n-6\n12\n3\n1\n4\n2\n"
            "14\n2\n1\n"
            "8\n16\n");
    }
    SECTION("Chained assignment")
    {
        const char* code = "a=b=c=2; b=3; print(a, b, c);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "2\n3\n2\n");
    }
    SECTION("Operator sequence Expr3")
    {
        const char* code = "a=1; ++----++++a; print(a);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "2\n");
    }
    SECTION("Operator sequence arithmetic")
    {
        const char* code = "a=1; b=2; c=a+b+b-2+b*3; print(c);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "9\n");
    }
    SECTION("Operator sequence logical")
    {
        const char* code = "a=false; b=true; c=a&&b&&a&&b||a||a&&a; print(c);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n");
    }
    SECTION("String")
    {
        const char* code = "a=\"aaa\"; b='bbb\n"
            "ccc'; print(a, a?1:0); empty=''; print(empty?1:0); \n"
            "print('aa' 'bb' /* comment */ \"cc\");";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "aaa\n1\n0\naabbcc\n");
    }
    SECTION("String escape sequences")
    {
        const char* code = "print('\\\\ \\\" \\' \\b \\f \\n \\r \\t \\? \\a \\v \\/ \\0a');";
        env.Execute(code);
        const char expectedOutput[] = "\\ \" ' \b \f \n \r \t \? \a \v / \0a\n";
        REQUIRE((env.GetOutput().length() == _countof(expectedOutput) - 1 &&
            memcmp(env.GetOutput().data(), expectedOutput, _countof(expectedOutput) - 1) == 0));
    }
    SECTION("String escape sequences numeric")
    {
        const char* code = "print('\\xA5 \\xa5');";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "\xa5 \xa5\n");
    }
    SECTION("String escape sequences Unicode")
    {
        const char* code = "print('\\u0104 \\U00000104');";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "\xC4\x84 \xC4\x84\n");
    }
    SECTION("String escape sequences wrong")
    {
        const char* code = " '\\256' ;";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
        code = " '\\Z' ;";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
        code = " '\\xZ' ;";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
        code = " '\\x1' ;";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
        code = " '\\u01";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
        code = " '\\U0000010G';";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
    }
    SECTION("String concatenation and appending")
    {
        const char* code = "s='aa' + 'bb'; s += 'cc'; s += s; print(s);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "aabbccaabbcc\n");
    }
    SECTION("Mixed types in conditional operator")
    {
        const char* code = "b=true; v=b?123:'aaa'; print(v);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "123\n");
    }
    SECTION("Type mismatch in operators")
    {
        const char* code = "print('aaa' + 123);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "v='aaa'; v += 1;";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "v=1 ^ 'aaa';";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "v='bbb' - 'aaa';";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "v='bbb' < null;";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Equality of numbers")
    {
        const char* code =
            "n1=1; n2=n1; n3=3; \n"
            "print(n1==n1, n1==n2, n1==n3, n1!=n1, n1!=n2, n1!=n3); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n1\n0\n0\n0\n1\n");
    }
    SECTION("Equality of strings")
    {
        const char* code =
            "s1='A'; s2=s1; s3='C'; \n"
            "print(s1==s1, s1==s2, s1==s3, s1!=s1, s1!=s2, s1!=s3); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n1\n0\n0\n0\n1\n");
    }
    SECTION("Equality of functions")
    {
        const char* code =
            "f1=function(){}; f2=f1; f3=function(){}; \n"
            "print(f1==f1, f1==f2, f1==f3, f1!=f1, f1!=f2, f1!=f3); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n1\n0\n0\n0\n1\n");
    }
    SECTION("Equality of system functions")
    {
        const char* code =
            "sf1=print; sf2=sf1; sf3=print; \n"
            "print(sf1==sf1, sf1==sf2, sf1==sf3, sf1!=sf1, sf1!=sf2, sf1!=sf3); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n1\n1\n0\n0\n0\n");
    }
    SECTION("String comparisons equality")
    {
        const char* code = "a='aa'; b='bb'; n=1; \n"
            "print(a==a); print(a==b); print(b!=b); print(b!=a); print(a==n); print(n!=b);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n0\n0\n1\n0\n1\n");
    }
    SECTION("String comparisons inequality")
    {
        const char* code = "a='aa'; b='bb'; \n"
            "print(a<b); print(a<a); print(a<=a); print(b>=a); print(b>b);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n0\n1\n1\n0\n");
    }
    SECTION("String indexing")
    {
        const char* code = "s='ABCDEF'; print(s[1]); i=3; t=s[i-1]; print(t + t);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "B\nCC\n");
    }
    SECTION("String count")
    {
        const char* code = "s='ABCD'; \n"
            "for(i=0; i<s.count; ++i) { print(i, s[i]); }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\nA\n1\nB\n2\nC\n3\nD\n");
    }
    SECTION("Invalid string indexing")
    {
        const char* code = "s='ABCDEF'; print(s[-1]);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "s='ABCDEF'; print(s[2.5]);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "s='ABCDEF'; print(s['a']);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "s='ABCDEF'; b='aaa'; print(s[b]);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "s='ABCDEF'; print(s[10]);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Invalid string member")
    {
        const char* code = "s='ABCD'; s.FOO;";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("String indexing as l_value")
    {
        const char* code = "s='ABCDEF'; print(s); s[0]='a'; print(s); s[5]='z'; i=2; s[i*i]='w'; print(s);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "ABCDEF\naBCDEF\naBCDwz\n");
    }
    SECTION("Invalid string indexing as l-value")
    {
        const char* code = "'ABCDEF'[0] = 'a';";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "s='ABCDEF'; s[-1]='a';";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "s='ABCDEF'; s[0.5]='a';";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "s='ABCDEF'; s[10]='a';";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "s='ABCDEF'; s['x']='a';";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("String copy to variable")
    {
        const char* code = "a='ABC'; b=a; b[0]='X'; print(a); print(b); b='DEF'; print(a); print(b);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "ABC\nXBC\nABC\nDEF\n");
    }
    SECTION("String passing as parameter")
    {
        const char* code =
            "function f(x){ print(x); x=x+x; print(x); x='A'; print(x); }"
            "a='ABC'; f('---'); f(a); print(a); f(a); print(a);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "---\n------\nA\nABC\nABCABC\nA\nABC\nABC\nABCABC\nA\nABC\n");
    }
    SECTION("Range-based for loop on string")
    {
        const char* code =
            "s='ABCD'; \n"
            "for(ch : s) print(ch); \n"
            "for(i, ch : s) print(i, ch); \n"
            "print(i, ch);\n";
        env.Execute(code);
        REQUIRE(env.GetOutput() ==
            "A\nB\nC\nD\n"
            "0\nA\n1\nB\n2\nC\n3\nD\n"
            "null\nnull\n");
    }
    SECTION("Switch basic")
    {
        const char* code = "switch(123) { case 1: print(1); case 'a': print('a'); case 123: print(123); default: print('Boo!'); }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "123\nBoo!\n");
    }
    SECTION("Switch with break")
    {
        const char* code = "switch(123) { case 1: print(1);break; case 'a': print('a');break; case 123: print(123);break; default: print('Boo!');break; }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "123\n");
    }
    SECTION("Switch with default active")
    {
        const char* code = "switch(124) { default: print('Boo!');break; case 1: print(1);break; case 'a': print('a');break; case 123: print(123);break; }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "Boo!\n");
    }
    SECTION("Switch with nothing active")
    {
        const char* code = "switch(124) { case 1: print(1);break; case 'a': print('a');break; case 123: print(123);break; }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "");
    }
    SECTION("Switch inside a loop")
    {
        const char* code = "for(i=0; i<5; ++i) { switch(i) { case 0:print(0);break; case 1:print('1');continue; case 2:print(2);break; case 3:default:print('Other'); } }";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n1\n2\nOther\nOther\n");
    }
    SECTION("Invalid switch")
    {
        const char* code = "switch(1) { case 2+2: }";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
        code = "i=1; switch(1) { case i: }";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
        code = "switch(1) { case 1: case 1: }";
        REQUIRE_THROWS_AS( env.Execute(code), ParsingError );
    }
    SECTION("local global")
    {
        const char* code =
            "function ChangeDefault() { print(a); a=1; print(a); }"
            "function ChangeGlobal() { print(a); global.a=2; print(a); }"
            "function ChangeLocal() { print(a); local.a=3; print(a); }"
            "        ChangeDefault(); ChangeGlobal(); ChangeLocal();"
            "a=10;   ChangeDefault(); ChangeGlobal(); ChangeLocal();"
            "a=null; ChangeDefault(); ChangeGlobal(); ChangeLocal();";
        env.Execute(code);
        REQUIRE(env.GetOutput() ==
            "null\n1\nnull\n2\n2\n3\n"
            "10\n1\n1\n2\n2\n3\n"
            "null\n1\nnull\n2\n2\n3\n");
    }
    SECTION("invalid local env")
    {
        const char* code = "local.a=1;";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "env.a=1;";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("local global in function definition")
    {
        const char* code = "function Outer() { \n"
            "   function InnerNone() { print('InnerNone'); } \n"
            "   local.InnerLocal = function() { print('InnerLocal'); }; \n"
            "   global.InnerGlobal = function() { print('InnerGlobal'); }; \n"
            "   InnerNone(); InnerLocal(); InnerGlobal(); \n"
            "} Outer(); InnerGlobal(); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "InnerNone\nInnerLocal\nInnerGlobal\nInnerGlobal\n");
    }
    SECTION("Increment number by null")
    {
        const char* code = "a=1; a+=null; print(a);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Increment string by null")
    {
        const char* code = "b='DUPA'; b+=null; print(b);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Add number to null")
    {
        const char* code = "a=1; a=a+null; print(a);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Add string to null")
    {
        const char* code = "b='DUPA'; b=null+b; print(b);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("NaN")
    {
        const char* code = "f=123; n1=0/0; n2=n1; \n"
            "print(n1==f, n1==n2, n1!=f, n1!=n2, n1<n2, n1?'T':'F');";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n0\n1\n1\n0\nT\n");
    }
    SECTION("Comparing number to null")
    {
        const char* code = "num0=0; num1=1; \n"
            "print(num0==nul, num0!=nul, num1==nul, num1!=nul, nul==nul, nul!=nul);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n1\n0\n1\n1\n0\n");
    }
    SECTION("Comparing string to null")
    {
        const char* code = "s0=''; s1='AAA'; \n"
            "print(s0==nul, s0!=nul, s1==nul, s1!=nul, nul==nul, nul!=nul);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n1\n0\n1\n1\n0\n");
    }
    SECTION("Inequality comparison number with null")
    {
        const char* code = "print(123 > nul);";
        REQUIRE_THROWS_AS(env.Execute(code), ExecutionError);
    }
    SECTION("Inequality comparison string with null")
    {
        const char* code = "print('ABC' <= nul);";
        REQUIRE_THROWS_AS(env.Execute(code), ExecutionError);
    }
    SECTION("Null assignment")
    {
        const char* code = 
            "print(a);"
            "a=1; print(a);"
            "a=null; print(a);"
            "a='A'; print(a);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "null\n1\nnull\nA\n");
    }
    SECTION("Null in operators")
    {
        const char* code = "print(a / 2);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "print(null < 5);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
        code = "print(a & 0xFF);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Null in comparison")
    {
        const char* code =
            "        print(a == a); print(a != a); print(a == null); print(null != a);"
            "a=1;    print(a == a); print(a != a); print(a == null); print(null != a);"
            "a=null; print(a == a); print(a != a); print(a == null); print(null != a);";
        env.Execute(code);
        REQUIRE(env.GetOutput() ==
            "1\n0\n1\n0\n"
            "1\n0\n0\n1\n"
            "1\n0\n1\n0\n");
    }
    SECTION("Passing null to function")
    {
        const char* code = "function f(a) { print(a); print(a != null); }"
            "f(1); f(x); f(null);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n1\n" "null\n0\n" "null\n0\n");
    }
    SECTION("Returning null from function")
    {
        const char* code = "function f(){return;} function g(){return null;}"
            "x=1; print(x); x=f(); print(x);"
            "x=1; print(x); x=g(); print(x);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\nnull\n1\nnull\n");
    }
    SECTION("Condition on null")
    {
        const char* code = "x=null;"
            "if(null) print(true); else print(false);"
            "if(x) print(true); else print(false);"
            "if(y) print(true); else print(false);"
            "if(true) print(true); else print(false);";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n0\n0\n1\n");
    }
    SECTION("Add to null")
    {
        const char* code = "print(x + 2);";
        REQUIRE_THROWS_AS(env.Execute(code), ExecutionError);
    }
    SECTION("Assign add null")
    {
        const char* code = "a += 3; a += 2; print(a); b += 'AAA'; b += 'BBB'; print(b);";
        REQUIRE_THROWS_AS(env.Execute(code), ExecutionError);
    }
    SECTION("Increment null")
    {
        const char* code = "a = null; print(a++); print(++a);";
        REQUIRE_THROWS_AS(env.Execute(code), ExecutionError);
    }
    SECTION("Stack overflow")
    {
        const char* code = "function fib(x) { return fib(x+1) + fib(x+2); } \n"
            "fib(1);";
        REQUIRE_THROWS_AS( env.Execute(code), ExecutionError );
    }
    SECTION("Throw statement")
    {
        const char* code = "throw 1; \n";
        REQUIRE_THROWS_AS( env.Execute(code), Value );
    }
    SECTION("Try catch")
    {
        const char* code = "try { throw 1; print('Should not get here'); } catch(ex) print(ex); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n");
    }
    SECTION("Throwing from function and loop")
    {
        const char* code = "function f() { \n"
            "  for(i = 0; i < 10; ++i) { \n"
            "    if(i == 6) throw 'AAA'; \n"
            " } \n"
            "} \n"
            "try { f(); } \n"
            "catch(ex) { print(ex); } \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "AAA\n");
    }
    SECTION("Catching execution error")
    {
        const char* code = "try { a='AAA' + 5 + null; } \n"
            "catch(ex) { \n"
            "  print(ex.Type); \n"
            "  print(ex.Index < 1000); \n"
            "  print(ex.Row); \n"
            "  print(ex.Column < 1000); \n"
            "  print(typeOf(ex.Message)); \n"
            "} \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "ExecutionError\n1\n1\n1\nString\n");
    }
    SECTION("Try finally without exception")
    {
        const char* code = "try print('A'); finally print('B'); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "A\nB\n");
    }
    SECTION("Try catch finally with exception")
    {
        const char* code = "try { print('A'); throw 1; } catch(ex) print('EX'); finally print('B'); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "A\nEX\nB\n");
    }
    SECTION("Try finally for break")
    {
        const char* code = "for(i : [1, 2, 3, 4, 5]) {\n"
            "  try {\n"
            "    print(i * 100); \n"
            "    if(i == 4) break; \n"
            "  } \n"
            "  finally { \n"
            "    print(i); \n"
            "  } \n"
            "} \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "100\n1\n200\n2\n300\n3\n400\n4\n");
    }
    SECTION("Try finally for continue")
    {
        const char* code = "for(i : [1, 2, 3, 4, 5]) {\n"
            "  try {\n"
            "    if(i == 4) continue; \n"
            "    print(i * 100); \n"
            "  } \n"
            "  finally { \n"
            "    print(i); \n"
            "  } \n"
            "} \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "100\n1\n200\n2\n300\n3\n4\n500\n5\n");
    }
    SECTION("Try catch finally for return")
    {
        const char* code = "function fn() {\n"
            "  try {\n"
            "    print('Try'); \n"
            "    return 1; \n"
            "  } \n"
            "  catch(ex) print('Catch'); \n"
            "  finally print('Finally'); \n"
            "} \n"
            "print(fn()); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "Try\nFinally\n1\n");
    }
    SECTION("Break in catch")
    {
        const char* code = "a=[1, 2, 3]; \n"
            "for(i = 0; i < 10; ++i) { \n"
            "  try { \n"
            "    print(i); \n"
            "    a[i] += 100; \n"
            "  } \n"
            "  catch(ex) break; \n"
            "} \n"
            "print(a[2]); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "0\n1\n2\n3\n103\n");
    }
    SECTION("Rethrowing an exception")
    {
        const char* code =
            "try { \n"
            "  try { \n"
            "    throw [555, 666, 777]; \n"
            "  } \n"
            "  catch(ex) { print('CATCH'); throw ex; } \n"
            "} \n"
            "catch(ex) print(ex[1]); \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "CATCH\n666\n");
    }
    SECTION("Throwing from finally with no exception")
    {
        const char* code =
            "function f() { \n"
            "  try { \n"
            "    print('AAA'); \n"
            "  } \n"
            "  finally { throw 123; } \n"
            "} \n"
            "try { f(); } catch(ex) { print(ex); } \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "AAA\n123\n");
    }
    SECTION("Try with finally and exception")
    {
        const char* code =
            "function f() { \n"
            "  try { \n"
            "    print('try before throw'); \n"
            "    throw 1; \n"
            "    print('try after throw'); \n"
            "  } \n"
            "  finally { print('try finally'); } \n"
            "} \n"
            "try { f(); } catch(ex) { print('catch', ex); } \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "try before throw\ntry finally\ncatch\n1\n");
    }
    SECTION("Throwing from finally with exception - exception precedence")
    {
        const char* code =
            "function f() { \n"
            "  try { \n"
            "    throw 1; \n"
            "    print('AAA'); \n"
            "  } \n"
            "  finally { throw 123; } \n"
            "} \n"
            "try { f(); } catch(ex) { print(ex); } \n";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n");
    }
    SECTION("Return from global")
    {
        const char* code = "s='A'; print(s); return s;";
        Value val = env.Execute(code);
        REQUIRE(env.GetOutput() == "A\n");
        REQUIRE(val.GetType() == ValueType::String);
        REQUIRE(val.GetString() == "A");
    }
    // To make sure the memory is correct even after code finished executing.
    SECTION("Return object from global")
    {
        const char* code = "s={a: 1, b: '2'}; print(s.a, s.b); return s;";
        Value val = env.Execute(code);
        REQUIRE(env.GetOutput() == "1\n2\n");
        REQUIRE(val.GetType() == ValueType::Object);
        const Object* obj = val.GetObject_();
        REQUIRE(obj->TryGetValue("a") != nullptr);
        REQUIRE(obj->TryGetValue("a")->GetType() == ValueType::Number);
        REQUIRE(obj->TryGetValue("a")->GetNumber() == 1.0);
        REQUIRE(obj->TryGetValue("b") != nullptr);
        REQUIRE(obj->TryGetValue("b")->GetType() == ValueType::String);
        REQUIRE(obj->TryGetValue("b")->GetString() == "2");
    }
    SECTION("Two executions in same environment")
    {
        const char* code = "print('A');";
        env.Execute(code);
        code = "print('B');";
        env.Execute(code);
        REQUIRE(env.GetOutput() == "A\nB\n");
    }
}
