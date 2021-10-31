#include "PCH.h"
#include "../3rdParty/Catch2/catch.hpp"
#include "../MinScriptLang.h"

using namespace MinScriptLang;

static void WriteDataToFile(const char* filePath, const char* data, size_t byteCount)
{
    FILE* f = nullptr;
    errno_t err = fopen_s(&f, filePath, "wb");
    if(err == 0)
    {
        fwrite(data, 1, byteCount, f);
        fclose(f);
    }
    else
        assert(err == 0);
}

TEST_CASE("Basic")
{
    Environment env;
    SECTION("Comments empty statements")
    {
        const char* code = "// Single line comment\n"
            "/* multi line comment \n"
            "*/ ;;;; \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput().empty());
    }
    SECTION("Unexpected end of comment")
    {
        const char* code = "/* foo";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
    }
    SECTION("Number formats")
    {
        const char* code = "print(123); print(-00444); print(+0xaF250); print(-0xFF); print(0xAA00FF5544CD);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "123\n-444\n717392\n-255\n1.86921e+14\n");
    }
    SECTION("Floating point number variants")
    {
        const char* code = "print(01.00); print(10.5); print(23.); print(.25); print(1e3); print(1e+2); print(.001e-1); print(3.E+0);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n10.5\n23\n0.25\n1000\n100\n0.0001\n3\n");
    }
    SECTION("Invalid floating point number variant 1")
    {
        const char* code = "print(.);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
    }
    SECTION("Invalid floating point number variant 2")
    {
        const char* code = "print(2.0e);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
    }
    SECTION("Characters after number")
    {
        const char* code = "123print(1);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
    }
    SECTION("Missing semicolon")
    {
        const char* code = "print(1)";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
    }
    SECTION("Garbage after last statement")
    {
        const char* code = "print(1); $~!@#$%^^&*()}";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
    }
    SECTION("Print Add Sub")
    {
        const char* code = "print(2 + 6 - 3);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "5\n");
    }
    SECTION("Mul Div Mod arithmetic operator precedence")
    {
        const char* code = "print(2 + 3 * 4); print(2 - 10 / 2 + 7 % 3); print(60 / 3 * 2);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "14\n-2\n40\n");
    }
    SECTION("Round brackets expression grouping")
    {
        const char* code = "print(2 + 3 * ((4))); print((2 + 3) * 4);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "14\n20\n");
    }
    SECTION("Curly brackets block")
    {
        const char* code = "print(1); { print(2); print(3); } print(4); { }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n2\n3\n4\n");
    }
    SECTION("Constants null false true")
    {
        const char* code = "print(null); print(false); print(true); print(true + true - false);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "null\n0\n1\n2\n");
    }
    SECTION("If conditions")
    {
        const char* code = "if(true) print(true); if(false) print(2); if(2); if(2-2) { } else { print(10); print(11); }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n10\n11\n");
    }
    SECTION("If conditions nested")
    {
        const char* code = "if(false) print(1); else if(false) print(2); else print(3); \n"
            "if(true) if(false) print(1); else print(2);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "3\n2\n");
    }
    SECTION("Ternary operator")
    {
        const char* code = "print(false ? 1 : 0); print(true ? false ? 1 : 2 : true ? 3 : 4);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n2\n");
    }
    SECTION("Variables")
    {
        const char* code = "a=1; print(a); b=a+1; print(b); c=d=b*b; print(b, c, d);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n2\n2\n4\n4\n");
    }
    SECTION("Variable name")
    {
        const char* code = "a1234_3252saczf434=1; print(a1234_3252saczf434+1);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "2\n");
    }
    SECTION("while loop")
    {
        const char* code = "a=6; while(a) { print(a); a = a - 2; }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "6\n4\n2\n");
    }
    SECTION("do while loop")
    {
        const char* code = "a=6; do { print(a); a = a - 2; } while(a);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "6\n4\n2\n");
    }
    SECTION("for loop")
    {
        const char* code = "for(a=10; a; a=a-2) { print(a); }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "10\n8\n6\n4\n2\n");
    }
    SECTION("For loop with empty statement")
    {
        const char* code = "for(a=0; a<10; ++a) ; print('a');";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "a\n");
    }
    SECTION("for loop with empty sections")
    {
        const char* code = "a=10; for(; a;) { print(a); a=a-2; }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "10\n8\n6\n4\n2\n");
    }
    SECTION("Preincrementation")
    {
        const char* code = "for(a=-4; a; ++a) { print(a); }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "-4\n-3\n-2\n-1\n");
    }
    SECTION("Logical operators")
    {
        const char* code = "print(!true); print(!!0);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n0\n");
    }
    SECTION("Bitwise shift")
    {
        const char* code = "a=2; print(a<<0); print(a<<2); print(a<<33); print(a>>1); print(a>>2); print(-a<<3); print(-256>>4);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "2\n8\n1.71799e+10\n1\n0\n-16\n-16\n");
    }
    SECTION("Bitwise not")
    {
        const char* code = "a=226; a=~a+1; print(a);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "-226\n");
    }
    SECTION("Comparisons")
    {
        const char* code = "a=-10; b=2; c=1000000000000; print(a<b); print(c<=b); print(c>a); print(b>=b); \n"
            "print(a==a); print(a==b); print(a!=b); print(b==4/2);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n0\n1\n1\n"
            "1\n0\n1\n1\n");
    }
    SECTION("Bitwise operators")
    {
        const char* code = "a=4294967295; /*0xFFFFFFFF*/ print(a&10); print(a&1|4); print(16 | a & 65535 ^ 12345);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "10\n5\n53206\n");
    }
    SECTION("Logical operators")
    {
        const char* code = "a=3; print(1==1 && a==3); print(false || true); print(false || true && false);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n1\n0\n");
    }
    SECTION("Logical operators short circuit")
    {
        const char* code = "(print(101)||true)&&print(102); (print(201)||false)&&print(202);"
            "print(301)||print(302); (print(401)||true)||print(402);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "101\n102\n201\n"
            "301\n302\n401\n");
    }
    SECTION("Logical operators returning value not bool")
    {
        const char* code = "five=5; six=6; zero=0; nth=null; \n"
            "print(five && zero); print(zero && five); print(zero && zero); print(five && six); print(zero && nth); print(nth && zero); \n"
            "print(five || zero); print(zero || five); print(zero || zero); print(five || six); print(zero || nth); print(nth || zero); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n0\n0\n6\n0\nnull\n"
            "5\n5\n0\n5\nnull\n0\n");
    }
    SECTION("Comma operator")
    {
        const char* code = "a=(1,2,4); print(a, 3, 5); print((a, 3, 5));";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "4\n3\n5\n5\n");
    }
    SECTION("Legit for loop")
    {
        const char* code = "for(i=0; i<5; ++i) print(i);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n1\n2\n3\n4\n");
    }
    SECTION("Loop break")
    {
        const char* code = "for(i=0;; ++i) { print(i); if(i>=5) break; }"
            "i=5; while(true) { print(i); break; }"
            "do { print(i); if(--i==0) break; } while(true);"
            "i=0; for(;;) { print(i); if(++i==4) { break; } }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n1\n2\n3\n4\n5\n"
            "5\n"
            "5\n4\n3\n2\n1\n"
            "0\n1\n2\n3\n");
    }
    SECTION("Loop continue")
    {
        const char* code = "for(i=0; i<10; ++i) { if(i>5) continue; print(i); }"
            "i=-10; while(true) { ++i; if(i<-5) { continue; } print(i); if(i>0) break; }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n1\n2\n3\n4\n5\n"
            "-5\n-4\n-3\n-2\n-1\n0\n1\n");
    }
    SECTION("Error break without a loop")
    {
        const char* code = "if(true) { do { print(1); }while(false); break; }";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Postincrementation postdecrementation")
    {
        const char* code = "for(i=0; i<3; i++) print(i); print(i++); print(i); print(i--); print(i);"
            "";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n1\n2\n3\n4\n4\n3\n");
    }
    SECTION("Compound assignment")
    {
        const char* code = "a=1; a+=3; print(a); a-=10; print(a); a*=-2; print(a); a/=4; print(a); a%=2; print(a); a<<=2; print(a); a>>=1; print(a);"
            "a=6; a|=8; print(a); a&=3; print(a); a^=3; print(a);"
            "a=1; b=2; b*=a+=7;  print(a);  print(b);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "4\n-6\n12\n3\n1\n4\n2\n"
            "14\n2\n1\n"
            "8\n16\n");
    }
    SECTION("Chained assignment")
    {
        const char* code = "a=b=c=2; b=3; print(a, b, c);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "2\n3\n2\n");
    }
    SECTION("Operator sequence Expr3")
    {
        const char* code = "a=1; ++----++++a; print(a);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "2\n");
    }
    SECTION("Operator sequence arithmetic")
    {
        const char* code = "a=1; b=2; c=a+b+b-2+b*3; print(c);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "9\n");
    }
    SECTION("Operator sequence logical")
    {
        const char* code = "a=false; b=true; c=a&&b&&a&&b||a||a&&a; print(c);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n");
    }
    SECTION("String")
    {
        const char* code = "a=\"aaa\"; b='bbb\n"
            "ccc'; print(a, a?1:0); empty=''; print(empty?1:0); \n"
            "print('aa' 'bb' /* comment */ \"cc\");";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "aaa\n1\n0\naabbcc\n");
    }
    SECTION("String escape sequences")
    {
        const char* code = "print('\\\\ \\\" \\' \\b \\f \\n \\r \\t \\? \\a \\v \\/ \\0a');";
        env.Execute(code, strlen(code));
        const char expectedOutput[] = "\\ \" ' \b \f \n \r \t \? \a \v / \0a\n";
        REQUIRE((env.GetOutput().length() == _countof(expectedOutput) - 1 &&
            memcmp(env.GetOutput().data(), expectedOutput, _countof(expectedOutput) - 1) == 0));
    }
    SECTION("String escape sequences numeric")
    {
        const char* code = "print('\\xA5 \\xa5');";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "\xa5 \xa5\n");
    }
    SECTION("String escape sequences Unicode")
    {
        const char* code = "print('\\u0104 \\U00000104');";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "\xC4\x84 \xC4\x84\n");
    }
    SECTION("String escape sequences wrong")
    {
        const char* code = " '\\256' ;";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
        code = " '\\Z' ;";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
        code = " '\\xZ' ;";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
        code = " '\\x1' ;";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
        code = " '\\u01";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
        code = " '\\U0000010G';";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
    }
    SECTION("String concatenation and appending")
    {
        const char* code = "s='aa' + 'bb'; s += 'cc'; s += s; print(s);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "aabbccaabbcc\n");
    }
    SECTION("Mixed types in conditional operator")
    {
        const char* code = "b=true; v=b?123:'aaa'; print(v);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "123\n");
    }
    SECTION("Type mismatch in operators")
    {
        const char* code = "print('aaa' + 123);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "v='aaa'; v += 1;";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "v=1 ^ 'aaa';";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "v='bbb' - 'aaa';";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "v='bbb' < null;";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Equality of numbers")
    {
        const char* code =
            "n1=1; n2=n1; n3=3; \n"
            "print(n1==n1, n1==n2, n1==n3, n1!=n1, n1!=n2, n1!=n3); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n1\n0\n0\n0\n1\n");
    }
    SECTION("Equality of strings")
    {
        const char* code =
            "s1='A'; s2=s1; s3='C'; \n"
            "print(s1==s1, s1==s2, s1==s3, s1!=s1, s1!=s2, s1!=s3); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n1\n0\n0\n0\n1\n");
    }
    SECTION("Equality of functions")
    {
        const char* code =
            "f1=function(){}; f2=f1; f3=function(){}; \n"
            "print(f1==f1, f1==f2, f1==f3, f1!=f1, f1!=f2, f1!=f3); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n1\n0\n0\n0\n1\n");
    }
    SECTION("Equality of system functions")
    {
        const char* code =
            "sf1=print; sf2=sf1; sf3=print; \n"
            "print(sf1==sf1, sf1==sf2, sf1==sf3, sf1!=sf1, sf1!=sf2, sf1!=sf3); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n1\n1\n0\n0\n0\n");
    }
    SECTION("String comparisons equality")
    {
        const char* code = "a='aa'; b='bb'; n=1; \n"
            "print(a==a); print(a==b); print(b!=b); print(b!=a); print(a==n); print(n!=b);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n0\n0\n1\n0\n1\n");
    }
    SECTION("String comparisons inequality")
    {
        const char* code = "a='aa'; b='bb'; \n"
            "print(a<b); print(a<a); print(a<=a); print(b>=a); print(b>b);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n0\n1\n1\n0\n");
    }
    SECTION("String indexing")
    {
        const char* code = "s='ABCDEF'; print(s[1]); i=3; t=s[i-1]; print(t + t);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "B\nCC\n");
    }
    SECTION("String Count")
    {
        const char* code = "s='ABCD'; \n"
            "for(i=0; i<s.Count; ++i) { print(i, s[i]); }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\nA\n1\nB\n2\nC\n3\nD\n");
    }
    SECTION("Invalid string indexing")
    {
        const char* code = "s='ABCDEF'; print(s[-1]);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "s='ABCDEF'; print(s[2.5]);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "s='ABCDEF'; print(s['a']);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "s='ABCDEF'; b='aaa'; print(s[b]);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "s='ABCDEF'; print(s[10]);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Invalid string member")
    {
        const char* code = "s='ABCD'; s.FOO;";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("String indexing as l_value")
    {
        const char* code = "s='ABCDEF'; print(s); s[0]='a'; print(s); s[5]='z'; i=2; s[i*i]='w'; print(s);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "ABCDEF\naBCDEF\naBCDwz\n");
    }
    SECTION("Invalid string indexing as l-value")
    {
        const char* code = "'ABCDEF'[0] = 'a';";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "s='ABCDEF'; s[-1]='a';";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "s='ABCDEF'; s[0.5]='a';";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "s='ABCDEF'; s[10]='a';";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "s='ABCDEF'; s['x']='a';";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("String copy to variable")
    {
        const char* code = "a='ABC'; b=a; b[0]='X'; print(a); print(b); b='DEF'; print(a); print(b);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "ABC\nXBC\nABC\nDEF\n");
    }
    SECTION("String passing as parameter")
    {
        const char* code =
            "function f(x){ print(x); x=x+x; print(x); x='A'; print(x); }"
            "a='ABC'; f('---'); f(a); print(a); f(a); print(a);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "---\n------\nA\nABC\nABCABC\nA\nABC\nABC\nABCABC\nA\nABC\n");
    }
    SECTION("Range-based for loop on string")
    {
        const char* code =
            "s='ABCD'; \n"
            "for(ch : s) print(ch); \n"
            "for(i, ch : s) print(i, ch); \n"
            "print(i, ch);\n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() ==
            "A\nB\nC\nD\n"
            "0\nA\n1\nB\n2\nC\n3\nD\n"
            "null\nnull\n");
    }
    SECTION("Switch basic")
    {
        const char* code = "switch(123) { case 1: print(1); case 'a': print('a'); case 123: print(123); default: print('Boo!'); }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "123\nBoo!\n");
    }
    SECTION("Switch with break")
    {
        const char* code = "switch(123) { case 1: print(1);break; case 'a': print('a');break; case 123: print(123);break; default: print('Boo!');break; }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "123\n");
    }
    SECTION("Switch with default active")
    {
        const char* code = "switch(124) { default: print('Boo!');break; case 1: print(1);break; case 'a': print('a');break; case 123: print(123);break; }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "Boo!\n");
    }
    SECTION("Switch with nothing active")
    {
        const char* code = "switch(124) { case 1: print(1);break; case 'a': print('a');break; case 123: print(123);break; }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "");
    }
    SECTION("Switch inside a loop")
    {
        const char* code = "for(i=0; i<5; ++i) { switch(i) { case 0:print(0);break; case 1:print('1');continue; case 2:print(2);break; case 3:default:print('Other'); } }";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n1\n2\nOther\nOther\n");
    }
    SECTION("Invalid switch")
    {
        const char* code = "switch(1) { case 2+2: }";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
        code = "i=1; switch(1) { case i: }";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
        code = "switch(1) { case 1: case 1: }";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
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
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() ==
            "null\n1\nnull\n2\n2\n3\n"
            "10\n1\n1\n2\n2\n3\n"
            "null\n1\nnull\n2\n2\n3\n");
    }
    SECTION("invalid local env")
    {
        const char* code = "local.a=1;";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "env.a=1;";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("local global in function definition")
    {
        const char* code = "function Outer() { \n"
            "   function InnerNone() { print('InnerNone'); } \n"
            "   local.InnerLocal = function() { print('InnerLocal'); }; \n"
            "   global.InnerGlobal = function() { print('InnerGlobal'); }; \n"
            "   InnerNone(); InnerLocal(); InnerGlobal(); \n"
            "} Outer(); InnerGlobal(); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "InnerNone\nInnerLocal\nInnerGlobal\nInnerGlobal\n");
    }
    SECTION("Increment number by null")
    {
        const char* code = "a=1; a+=null; print(a);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Increment string by null")
    {
        const char* code = "b='DUPA'; b+=null; print(b);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Add number to null")
    {
        const char* code = "a=1; a=a+null; print(a);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Add string to null")
    {
        const char* code = "b='DUPA'; b=null+b; print(b);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("NaN")
    {
        const char* code = "f=123; n1=0/0; n2=n1; \n"
            "print(n1==f, n1==n2, n1!=f, n1!=n2, n1<n2, n1?'T':'F');";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n0\n1\n1\n0\nT\n");
    }
    SECTION("Comparing number to null")
    {
        const char* code = "num0=0; num1=1; \n"
            "print(num0==nul, num0!=nul, num1==nul, num1!=nul, nul==nul, nul!=nul);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n1\n0\n1\n1\n0\n");
    }
    SECTION("Comparing string to null")
    {
        const char* code = "s0=''; s1='AAA'; \n"
            "print(s0==nul, s0!=nul, s1==nul, s1!=nul, nul==nul, nul!=nul);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n1\n0\n1\n1\n0\n");
    }
    SECTION("Inequality comparison number with null")
    {
        const char* code = "print(123 > nul);";
        REQUIRE_THROWS_AS(env.Execute(code, strlen(code)), ExecutionError);
    }
    SECTION("Inequality comparison string with null")
    {
        const char* code = "print('ABC' <= nul);";
        REQUIRE_THROWS_AS(env.Execute(code, strlen(code)), ExecutionError);
    }
}

TEST_CASE("Null")
{
    Environment env;
    SECTION("Null assignment")
    {
        const char* code = 
            "print(a);"
            "a=1; print(a);"
            "a=null; print(a);"
            "a='A'; print(a);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "null\n1\nnull\nA\n");
    }
    SECTION("Null in operators")
    {
        const char* code = "print(a / 2);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "print(null < 5);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "print(a & 0xFF);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Null in comparison")
    {
        const char* code =
            "        print(a == a); print(a != a); print(a == null); print(null != a);"
            "a=1;    print(a == a); print(a != a); print(a == null); print(null != a);"
            "a=null; print(a == a); print(a != a); print(a == null); print(null != a);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() ==
            "1\n0\n1\n0\n"
            "1\n0\n0\n1\n"
            "1\n0\n1\n0\n");
    }
    SECTION("Passing null to function")
    {
        const char* code = "function f(a) { print(a); print(a != null); }"
            "f(1); f(x); f(null);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n1\n" "null\n0\n" "null\n0\n");
    }
    SECTION("Returning null from function")
    {
        const char* code = "function f(){return;} function g(){return null;}"
            "x=1; print(x); x=f(); print(x);"
            "x=1; print(x); x=g(); print(x);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\nnull\n1\nnull\n");
    }
    SECTION("Condition on null")
    {
        const char* code = "x=null;"
            "if(null) print(true); else print(false);"
            "if(x) print(true); else print(false);"
            "if(y) print(true); else print(false);"
            "if(true) print(true); else print(false);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n0\n0\n1\n");
    }
    SECTION("Add to null")
    {
        const char* code = "print(x + 2);";
        REQUIRE_THROWS_AS(env.Execute(code, strlen(code)), ExecutionError);
    }
    SECTION("Assign add null")
    {
        const char* code = "a += 3; a += 2; print(a); b += 'AAA'; b += 'BBB'; print(b);";
        REQUIRE_THROWS_AS(env.Execute(code, strlen(code)), ExecutionError);
    }
    SECTION("Increment null")
    {
        const char* code = "a = null; print(a++); print(++a);";
        REQUIRE_THROWS_AS(env.Execute(code, strlen(code)), ExecutionError);
    }
}

TEST_CASE("Functions")
{
    Environment env;
    SECTION("Basic function")
    {
        const char* code = "f = function() { print('Foo'); };\n"
            "f(); f();";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "Foo\nFoo\n");
    }
    SECTION("Local variables")
    {
        const char* code = "a=1; print(a); f=function(){ b=2; print(b); print(a); a=10; print(a); };\n"
            "f(); print(a);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n2\n1\n10\n10\n");
    }
    SECTION("Local variables invalid")
    {
        const char* code = "f=function(){ a=1; print(a); }; f(); print(a);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\nnull\n");
    }
    SECTION("Parameters")
    {
        const char* code = "f=function(a, b){ a='['+a+'] ['+b+']'; print(a); }; \n"
            "f('AAA', 'BBB'); f('CCC', 'DDD');";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "[AAA] [BBB]\n[CCC] [DDD]\n");
    }
    SECTION("Return")
    {
        const char* code = "functionNotReturning = function(){ print('functionNotReturning'); }; \n"
            "functionReturningNull = function(){ print('functionReturningNull'); return; print('DUPA'); }; \n"
            "functionReturningSomething = function(){ print('functionReturningSomething'); return 123; print('DUPA'); }; \n"
            "print(functionNotReturning()); print(functionReturningNull()); print(functionReturningSomething());";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "functionNotReturning\nnull\nfunctionReturningNull\nnull\nfunctionReturningSomething\n123\n");
    }
    SECTION("Return without function")
    {
        const char* code = "return 2;";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Syntactic sugar")
    {
        const char* code = "function add(a, b){ return a+b; }\n"
            "print(add(2, 5));";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "7\n");
    }
    SECTION("Parameters not unique")
    {
        const char* code = "function f(a, a) { }";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
    }
    SECTION("Recursion")
    {
        const char* code = "function factorial(n){ if(n==0) return 1; return n*factorial(n-1); } \n"
            "print(factorial(0)); print(factorial(3)); print(factorial(4));";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n6\n24\n");
    }
    SECTION("Break without loop in function")
    {
        const char* code = "function Bad() { break; } \n"
            "for(i=0; i<10; ++i) { Bad(); }";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
        code = "function Bad() { continue; } \n"
            "for(i=0; i<10; ++i) { Bad(); }";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Wrong number of parameters expected 0 passed 1")
    {
        const char* code = "function f() { print('a'); } f(2);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Wrong number of parameters expected 1 passed 0")
    {
        const char* code = "function f(a) { print(a); } f();";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Wrong number of parameters expected 1 passed 3")
    {
        const char* code = "function f(a) { print(a); } f(1, 2, 3);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Wrong number of parameters expected 4 passed 0")
    {
        const char* code = "function f(a, b, c, d) { print(a, b, c, d); } f();";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Wrong number of parameters expected 4 passed 2")
    {
        const char* code = "function f(a, b, c, d) { print(a, b, c, d); } f(1, 2);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Wrong number of parameters expected 4 passed 5")
    {
        const char* code = "function f(a, b, c, d) { print(a, b, c, d); } f('1', '2', '3' ,'4', '5');";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
}

TEST_CASE("Object")
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
        env.Execute(code, strlen(code));
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
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n2\n3\n");
    }
    SECTION("Compound string in object definition")
    {
        const char* code = "a={ 'AAA' /*comment*/ \"BBB\": 5 }; print(a.AAABBB);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "5\n");
    }
    SECTION("Object definition versus block")
    {
        const char* code = "{ print(1); { print(2); } { } print(3); } { }; {'a':1, 'b':false}; { print(4); } print(5);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n2\n3\n4\n5\n");
    }
    SECTION("Nested object")
    {
        const char* code =
            "o1 = { 'x': 1 }; \n"
            "o2 = { 'y': o1 }; \n"
            "print(o2.y.x); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n");
    }
    SECTION("Nested object definition")
    {
        const char* code =
            "o1 = { 'x': { 'y': 5 } }; \n"
            "o1.x.y += 5; \n"
            "print(o1.x.y); \n";
        env.Execute(code, strlen(code));
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
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "4\n4\n");
    }
    SECTION("Operator sequence Expr2")
    {
        const char* code =
            "o1 = { 'f1': function() { return o2; }, 'val1': 1 }; \n"
            "o2 = { 'f2': function() { return o1; }, 'val2': 2 }; \n"
            "print(o1.f1().f2().val1); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n");
    }
    SECTION("Object indexing")
    {
        const char* code =
            "o1 = { 'x': 1, 'y': 2, 'a b c': 3 }; \n"
            "index = 'y'; \n"
            "print(o1['x'], o1[index], o1['a b' + ' c']); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n2\n3\n");
    }
    SECTION("Object indexing nested")
    {
        const char* code =
            "o1 = { 'x': { 'y': { 'z': 5 } } }; \n"
            "print(o1['x'].y['z']); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "5\n");
    }
    SECTION("Object indexing l-value")
    {
        const char* code =
            "o={}; o['A A'] = 1; o['A A'] += 5; \n"
            "print(o['A A']); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "6\n");
    }
    SECTION("Object indexing nested")
    {
        const char* code =
            "o={}; o['A'] = {}; o['A']['B'] = {}; o['A']['B'].c = 10; \n"
            "print(o.A.B['c']); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "10\n");
    }
    SECTION("Equality of objects")
    {
        const char* code =
            "o1={'a':1, 'b':2}; o2=o1; o3={'a':1, 'b':2}; \n"
            "print(o1==o1, o1==o2, o1==o3, o1!=o1, o1!=o2, o1!=o3); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n1\n0\n0\n0\n1\n");
    }
    SECTION("Count and setting unsetting members")
    {
        const char* code =
            "obj={'a':1, 'b':2, 'd':null}; obj.c=3; obj.b=null; \n"
            "print(obj.Count); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "2\n");
    }
    SECTION("Object with repeating keys")
    {
        const char* code = "obj={'a':1, 'b':2, 'a':3};";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ParsingError );
    }
    SECTION("Range-based for loop for objects")
    {
        const char* code =
            "obj={'a':1, 'bbbb':4, 'cc':10}; \n"
            "sum = 0; for(val: obj) sum += val; print(sum);\n"
            "sumKeyLen = 0; sumVal = 0; for(key, val: obj) { sumKeyLen += key.Count; sumVal += val; } print(sumKeyLen, sumVal); \n"
            "print(key, val); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "15\n7\n15\n"
            "null\nnull\n");
    }
    SECTION("Implicit this")
    {
        const char* code =
            "obj={ 'x': 2, 'f1': function() { x += 1; print(x); } }; \n"
            "obj.f1(); obj['f1'](); (101, '102', obj.f1)(); \n"
            "obj2={'subObj':obj}; obj2.subObj.f1();";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "3\n4\n5\n6\n");
    }
    SECTION("Explicit this")
    {
        const char* code =
            "obj={ 'x': 2, 'f1': function() { this.y = 1; this.x += y; print(this.x); } }; \n"
            "obj.f1(); obj['f1'](); (101, '102', obj.f1)(); \n"
            "obj2={'subObj':obj}; obj2.subObj.f1();";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "3\n4\n5\n6\n");
    }
    SECTION("Returning this")
    {
        const char* code =
            "obj={ 'x': 2, 'f': function() { this.x += 1; return this; } }; \n"
            "obj2 = obj.f(); print(obj2.x); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "3\n");
    }
    SECTION("Passing function together with this")
    {
        const char* code =
            "obj={ 'x': 2, 'f': function() { this.x += 1; print(this.x); } }; \n"
            "function call(a) { a(); } \n"
            "call(obj.f); call(obj.f); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "3\n4\n");
    }
    SECTION("Calling a method from another method")
    {
        const char* code =
            "obj={ 'x': 2, 'f1': function() { f2(); this.f2(); }, 'f2': function() { print(x); print(this.x); } }; \n"
            "obj.f1(); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "2\n2\n2\n2\n");
    }
    SECTION("Invalid use of this in global scope")
    {
        const char* code = "print(this.x);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Invalid use of this in local scope")
    {
        const char* code = "function f() { print(this.x); } \n"
            "f();";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Function syntactic sugar in object definition")
    {
        const char* code = "obj={ var: 2, function fn() { print(var); } }; \n"
            "obj.fn(); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "2\n");
    }
    SECTION("Calling object default function")
    {
        const char* code = "obj={ var: 2, function fn() { print('fn'); }, '':function(a) { print('Default', a); } }; \n"
            "obj(3); \n";
        env.Execute(code, strlen(code));
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
        env.Execute(code, strlen(code));
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
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "3\n1\n");
    }
    SECTION("Null member in derived class")
    {
        const char* code = "class Base { a: 121, b: 122, c: 123 }; \n"
            "class Derived : Base { b: 124, c: null }; \n"
            "print(Derived.a, Derived.b, Derived.c);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "121\n124\nnull\n");
    }
    SECTION("Object conversion to bool")
    {
        const char* code = "o1={a:123}; o2={}; \n"
            "print(o1?'T':'F', o2?'T':'F');";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "T\nT\n");
    }
    SECTION("Object definitions with trailing comma")
    {
        const char* code = "o1={a:123, b:124,}; o2={'a':o1.a, 'b':o1.b,}; \n"
            "print(o1['a'], o1['b'], o2.a, o2.b);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "123\n124\n123\n124\n");
    }
}

TEST_CASE("Types")
{
    Environment env;
    SECTION("Type identifier")
    {
        const char* code = "t1=Null; t2=Number; t3=String; t4=Object; \n"
            "print(t1, t2, t3, t4);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "Null\nNumber\nString\nObject\n");
    }
    SECTION("TypeOf and comparisons")
    {
        const char* code = "tn1=Number; n2=123; tn2=TypeOf(n2); tnull=TypeOf(nonExistent); \n"
            "print(tn1, tn2, tnull); \n"
            "print(tn1==tn2, tn1!=tn2, tn2==tnull, tn2!=tnull); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "Number\nNumber\nNull\n"
            "1\n0\n0\n1\n");
    }
    SECTION("Type conversion to bool")
    {
        const char* code = "tobj=TypeOf({a:1, b:2}); tnull=TypeOf(nonExistent); \n"
            "print(tobj?1:0, tnull?1:0); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n0\n");
    }
    SECTION("Null construction")
    {
        const char* code = "v1=Null(); v2=Null(nonExistent); \n"
            "print(v1, v2); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "null\nnull\n");
    }
    SECTION("Null construction invalid")
    {
        const char* code = "v1=Null(); v2=Null(123); \n"
            "print(v1, v2); \n";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Number construction")
    {
        const char* code = "v1=2; v2=Number(v1); \n"
            "print(v1, v2); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "2\n2\n");
    }
    SECTION("Number construction invalid")
    {
        const char* code = "v2=Number('A'); \n"
            "print(v2); \n";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("String construction")
    {
        const char* code = "v1='A'; v2=String(v1); \n"
            "print(v1, v2); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "A\nA\n");
    }
    SECTION("String construction invalid")
    {
        const char* code = "v2=String('A', 'B', 123); \n";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Object construction")
    {
        const char* code = "v1={a:1, b:2}; v2=Object(v1); \n"
            "print(v1.a, v2.a, v1==v2); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n1\n0\n");
    }
    SECTION("Object construction invalid")
    {
        const char* code = "v2=Object(123, 'A'); \n";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Function construction")
    {
        const char* code = "function f(){return 123;} v2=Function(f); v3=print;\n"
            "print(f==v2, v2==v3, v2()); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n0\n123\n");
    }
    SECTION("Function construction invalid")
    {
        const char* code = "v2=Function(); \n";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Type construction")
    {
        const char* code = "v1=TypeOf(125); v2=Type(v1); v3=v2(123);\n"
            "print(v1==v2, v3); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n123\n");
    }
    SECTION("Type construction invalid")
    {
        const char* code = "v2=Type(123); \n";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
}

TEST_CASE("Array")
{
    Environment env;
    SECTION("Empty array definition")
    {
        const char* code = "a1=[]; print(a1); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "array\n");
    }
    SECTION("Array definition and indexing as rvalue")
    {
        const char* code = "a1=[1, 2, 3]; \n"
            "print(a1[0], a1[1], a1[2]); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n2\n3\n");
    }
    SECTION("Array definition and indexing as lvalue")
    {
        const char* code = "a1=[1, 2, 3, 4, 5]; \n"
            "a1[0]=10; a1[1]+=10; a1[2]*=10; a1[3]++; ++a1[4];\n"
            "print(a1[0], a1[1], a1[2], a1[3], a1[4]); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "10\n12\n30\n5\n6\n");
    }
    SECTION("Invalid array indexing with other value type")
    {
        const char* code = "a=[1, 2, 3]; print(a[Number]); \n";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Invalid array indexing out of bounds")
    {
        const char* code = "a=[1, 2, 3]; print(a[100]); \n";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
    SECTION("Passing and returning array by reference")
    {
        const char* code = "a1=[1, 2, 3]; a2=a1; \n"
            "function f(arg) { arg[1]++; return arg; } \n"
            "a3=f(a2); \n"
            "print(a1[0], a1[1], a1[2], a3[1], a1==a3, a1!=a3); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n3\n3\n3\n1\n0\n");
    }
    SECTION("Array constructor to copy")
    {
        const char* code = "a1=[1, 2, 3]; a2=Array(a1); a0=Array(); \n"
            "print(a2[0], a2[1], a2[2], a1==a2, a1==a0); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n2\n3\n0\n0\n");
    }
    SECTION("Array definition with trailing comma")
    {
        const char* code = "a=[1, 2, 3,]; print(a[0], a[1], a[2]);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n2\n3\n");
    }
    SECTION("Null as array item")
    {
        const char* code = "a=[1, 2, 3]; a[1]=null; a[2]=null; \n"
            "print(a[0], a[1], a[2]);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\nnull\nnull\n");
    }
    SECTION("Range-based for loop for array")
    {
        const char* code = "a=[1, 2, 3]; \n"
            "for(val : a) print(val); \n"
            "print(val); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "1\n2\n3\n"
            "null\n");
    }
    SECTION("Range-based for loop for array with iterator")
    {
        const char* code = "a=[1, 2, 3]; \n"
            "for(i, val : a) { { print(i, val); } ;;; } \n"
            "print(i, val); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n1\n1\n2\n2\n3\n"
            "null\nnull\n");
    }
    SECTION("Array conversion to bool")
    {
        const char* code = "a0=[]; a3=[1, 2, 3]; n=null;\n"
            "print(a0?'Y':'N', a3?'Y':'N', n?'Y':'N'); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "Y\nY\nN\n");
    }
    SECTION("Array Count property")
    {
        const char* code = "a0=[]; a3=[1, 2, 3]; \n"
            "print(a0.Count, a3.Count); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n3\n");
    }
    SECTION("Array methods Add Insert Remove")
    {
        const char* code = "a=[1, 2, 3]; \n"
            "a.Add(4); a.Add(5); a.Add(6); /* 1, 2, 3, 4, 5, 6 */ \n"
            "a.Remove(0); a.Remove(a.Count-1); a.Remove(1); /* 2, 4, 5 */ \n"
            "a.Insert(3, 100); a.Insert(2, 101); a.Insert(0, 102); /* 102, 2, 4, 101, 5, 100 */ \n"
            "print(a.Count, a[0], a[1], a[2], a[3], a[4], a[5]); \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "6\n102\n2\n4\n101\n5\n100\n");
    }
}

TEST_CASE("Extra slow")
{
    Environment env;
    SECTION("Stack overflow")
    {
        const char* code = "function fib(x) { return fib(x+1) + fib(x+2); } \n"
            "fib(1);";
        REQUIRE_THROWS_AS( env.Execute(code, strlen(code)), ExecutionError );
    }
}
