#include "PCH.h"
#include "../MinScriptLang.h"

int main()
{
    using namespace MinScriptLang;

    Environment env;
    const char* code =
        "// Single line comment \n"
        "/* multi line comment \n"
        "*/ print(2 + 2 * 2); \n"
        "1024; \n"
        "; \n"
        "print( 2 / 2 ); \n"
        "print( /* nothing here */ );\n";
    try
    {
        env.Execute(code, strlen(code));
    }
    catch(const ParsingError& e)
    {
        printf("PARSING ERROR:\nindex=%zu, row=%zu, col=%zu: %s\n", e.GetPlace().Index, e.GetPlace().Row, e.GetPlace().Column, e.GetMessage().c_str());
    }
    catch(const ExecutionError& e)
    {
        printf("EXECUTION ERROR:\nindex=%zu, row=%zu, col=%zu: %s\n", e.GetPlace().Index, e.GetPlace().Row, e.GetPlace().Column, e.GetMessage().c_str());
    }
    catch(const Error& e)
    {
        printf("ERROR:\nindex=%zu, row=%zu, col=%zu: %s\n", e.GetPlace().Index, e.GetPlace().Row, e.GetPlace().Column, e.GetMessage().c_str());
    }
}