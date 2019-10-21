#include "PCH.h"
#include "../3rdParty/Catch2/catch.hpp"
#include "../MinScriptLang.h"

using namespace MinScriptLang;

TEST_CASE("Basic")
{
    Environment env;
    SECTION("Comments empty statements")
    {
        const char* code = "// Single line comment\n/* multi line comment \n*/ ;;;; \n";
        env.Execute(code, strlen(code));
        REQUIRE(env.GetOutput().empty());
    }
    SECTION("Unexpected end of comment")
    {
        const char* code = "/* foo";
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
}
