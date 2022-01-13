
#include <fstream>
#include <filesystem>
#include "priv.h"

namespace MSL
{
    namespace Builtins
    {
        namespace
        {
            Value filefunc_readfile(Environment& env, const Location& place, Value::List&& args)
            {
                std::string data;
                Value filename;
                Util::checkArgumentCount(env, place, "File.read", args.size(), 1);
                filename = Util::checkArgument(env, place, "File.read", args, 0, Value::Type::String);
                std::fstream fh(filename.getString(), std::ios::in | std::ios::binary);
                if(!fh.good())
                {
                    throw Error::IOError(place, Util::joinArgs("failed to open '", filename.getString(), "' for reading"));
                }
                fh.seekg(0, std::ios::end);   
                data.reserve(fh.tellg());
                fh.seekg(0, std::ios::beg);
                data.assign((std::istreambuf_iterator<char>(fh)), std::istreambuf_iterator<char>());
                fh.close();
                return Value{std::move(data)};
            }
        }

        void makeFileNamespace(Environment& env)
        {
            auto obj = std::make_shared<Object>();
            {
                obj->entry("readFile") = Value{filefunc_readfile};
            }
            env.global("File") = Value{std::move(obj)};
        }
    }
}
