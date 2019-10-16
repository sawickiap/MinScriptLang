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
}
