
#include <string>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstdint>
#include <cmath>
#if __has_include(<readline/readline.h>)
#include <readline/readline.h>
#include <readline/history.h>
#else
    #define NO_READLINE
#endif

#include "msl.h"
#include "optionparser.h"

struct StdoutWriter: public MSL::AST::DebugWriter
{
    StdoutWriter()
    {
    }

    void flush() override
    {
        std::cout << std::flush;
    }

    void writeValue(const MSL::Value& v) override
    {
        v.toStream(std::cout);
    }

    void writeString(std::string_view str) override
    {
        std::cout << str;
    }

    void writeReprString(std::string_view str) override
    {
        MSL::Util::reprString(std::cout, str);
    }

    void writeChar(int ch) override
    {
        std::cout << char(ch);
    }

    void writeNumber(double n) override
    {
        std::cout << n;
    }
    
};

void WriteDataToFile(const char* filePath, const char* data, size_t byteCount)
{
    FILE* f;
    f  = fopen(filePath, "wb");
    if(f != nullptr)
    {
        fwrite(data, 1, byteCount, f);
        fclose(f);
    }
}

bool readfile(const std::string& fname, std::string& destbuf)
{
    char c;
    FILE* fh;
    destbuf.clear();
    if((fh = fopen(fname.c_str(), "rb")) == nullptr)
    {
        return false;
    }
    while((c = fgetc(fh)) != EOF)
    {
        destbuf.append(&c, 1);
    }
    fclose(fh);
    return true;
}

void printException(const MSL::Error::Exception& e)
{
    std::cerr << "uncaught error at " << e.location().textline << "," << e.location().textcolumn << ": (" << e.name() << ") " << e.message() << std::endl;
}

void setARGV(MSL::Environment& env, const std::vector<std::string>& va)
{
    auto a = std::make_shared<MSL::Array>();
    for(auto s: va)
    {
        a->push_back(MSL::Value{std::string(s)});
    }
    env.setGlobal("ARGV", MSL::Value{std::move(a)});
}

#if !defined(NO_READLINE)
/*
* returns true if $line is more than just space.
* used to ignore blank lines.
*/
static bool notjustspace(const char* line)
{
    int c;
    size_t i;
    for(i=0; (c = line[i]) != 0; i++)
    {
        if(!isspace(c))
        {
            return true;
        }
    }
    return false;
}

int repl(MSL::Environment& env)
{
    size_t varid;
    size_t nowid;
    char* line;
    std::string exeme;
    std::string varname;
    varid = 0;
    while(true)
    {
        line = readline ("> ");
        fflush(stdout);
        if(line == NULL)
        {
            fprintf(stderr, "readline() returned NULL\n");
        }
        if((line != NULL) && (line[0] != '\0') && notjustspace(line))
        {
            exeme = line;
            /*
            * this appends a semicolon to the code line.
            * this is a dumb hack, until the parser recognized linefeeds as line terminators.
            */
            if(*(exeme.end()-1) != ';')
            {
                exeme.push_back(';');
            }
            add_history(line);
            try
            {
                auto val = env.execute(exeme);
                nowid = varid;
                varid++;
                //if(!val.isNull())
                {
                    varname = MSL::Util::joinArgs("$", nowid);
                    env.setGlobal(varname, std::move(val));
                    std::cout << varname << " = ";
                    val.reprToStream(std::cout);
                    std::cout << std::endl;
                }
            }
            catch(MSL::Error::Exception& e)
            {
                printException(e);
            }
        }
    }
}
#endif

int dumpSyntaxTreeCode(MSL::Environment& env, std::string_view code)
{
    (void)env;
    StdoutWriter sw;
    MSL::AST::Script ast(MSL::Location{});
    try
    {
        MSL::Tokenizer tkz(code); 
        MSL::Parser prs(tkz);
        prs.parseScript(ast);
        ast.debugPrint(sw, 0, "");
    }
    catch(MSL::Error::Exception& e)
    {
        printException(e);
        return 1;
    }
    return 0;
}

int dumpSyntaxTreeFile(MSL::Environment& env, const std::string& filename)
{
    std::string fdata;
    if(readfile(filename, fdata))
    {
        return dumpSyntaxTreeCode(env, fdata);
    }
    else
    {
        std::cerr << "dumpsyntax: cannot read file '" << filename << "'" << std::endl;
    }
    return 1;
}

int main(int argc, char* argv[])
{
    bool havecodechunk;
    bool forcerepl;
    bool dumpsyntax;
    bool alsoprint;
    std::string filename;
    std::string filedata;
    std::string codechunk;
    std::vector<std::string> rest;
    MSL::Environment env;
    OptionParser prs;
    alsoprint = false;
    dumpsyntax = false;
    forcerepl = false;
    havecodechunk = false;
    prs.on({"-i", "--repl"}, "force run REPL", [&]
    {
        forcerepl = true; 
    });
    prs.on({"-e?", "--eval=?"}, "evaluate <arg>, then exit", [&](const auto& v)
    {
        codechunk = v.str();
        havecodechunk = true;
        prs.stopParsing();
    });
    prs.on({"-p", "--print"}, "when using '-e', print the result", [&]
    {
        alsoprint = true;
    });
    prs.on({"-d", "--dump"}, "dump source through debugPrint() - can also be used with '-e'", [&]
    {
        dumpsyntax = true;
    });
    try
    {
        prs.parse(argc, argv);
    }
    catch(OptionParser::Error& e)
    {
        std::cerr << "failed to process options: " << e.what() << std::endl;
        return 1;
    }
    rest = prs.positional();
    setARGV(env, rest);
    if(havecodechunk)
    {
        if(dumpsyntax)
        {
            return dumpSyntaxTreeCode(env, codechunk);
        }
        else
        {
            try
            {
                auto v = env.execute(codechunk);
                if(alsoprint)
                {
                    v.toStream(std::cout);
                }
            }
            catch(MSL::Error::Exception& e)
            {
                printException(e);
                return 1;
            }
        }
        return 0;
    }
    if(rest.size() > 0 && !forcerepl)
    {
        filename = rest[0];
        if(dumpsyntax)
        {
            return dumpSyntaxTreeFile(env, filename);
        }
        if(readfile(filename, filedata))
        {
            try
            {
                env.execute(filedata);
            }
            catch(MSL::Error::Exception& e)
            {
                printException(e);
                return 1;
            }
        }
        else
        {
            fprintf(stderr, "failed to read file '%s'\n", filename.data());
            return 1;
        }
    }
    else
    {
        #if defined(NO_READLINE)
            if(forcerepl)
            {
                std::cerr << "no support for readline compiled, so no REPL. sorry" << std::endl;
            }
            else
            {
                std::cerr << "need a filename" << std::endl;
            }
        #else
            return repl(env);
        #endif
    }
    return 0;
}


