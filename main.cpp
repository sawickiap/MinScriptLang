
#include <string>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstdint>
#include <cmath>
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
            catch(MSL::Error& e)
            {
                fprintf(stderr, "error: %s\n", e.podMessage());
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
        fprintf(stderr, "need a filename\n");
    }
    return 1;
}


