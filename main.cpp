
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
    std::cerr << "uncaught error line " << e.getPlace().textline << ": (" << e.prettyMessage() << std::endl;
}

#if !defined(NO_READLINE)
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
                    env.global(varname) = val;
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

int main(int argc, char* argv[])
{
    std::string filename;
    std::string filedata;
    MSL::Environment env;
    if(argc > 1)
    {
        filename = argv[1];
        if(readfile(filename, filedata))
        {
            try
            {
                env.execute(filedata);
            }
            catch(MSL::Error::Exception& e)
            {
                printException(e);
            }
        }
        else
        {
            fprintf(stderr, "failed to read file '%s'\n", filename.data());
            return 0;
        }
    }
    else
    {
        #if defined(NO_READLINE)
        fprintf(stderr, "need a filename\n");
        #else
            return repl(env);
        #endif
    }
    return 1;
}


