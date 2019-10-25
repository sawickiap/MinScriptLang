#include "PCH.h"
#include "../3rdParty/Catch2/catch.hpp"
#include "../MinScriptLang.h"

using namespace MinScriptLang;

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
        const char* code = "print(null); print(false); print(true); print(true + true - null - false);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "0\n0\n1\n2\n");
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
    SECTION("String")
    {
        const char* code = "a=\"aaa\"; b='bbb\n"
            "ccc'; print(a, a?1:0); empty=''; print(empty?1:0);";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput() == "aaa\n1\n0\n");
    }

}
