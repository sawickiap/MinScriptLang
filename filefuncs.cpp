
#include <fstream>
#include <filesystem>
#include "priv.h"

namespace MSL
{
    namespace Builtins
    {
        namespace
        {
            Value filefunc_readfile(Environment& env, const Location& loc, Value::List&& args)
            {
                (void)env;
                Util::ArgumentCheck ac(loc, args, "File.readFile");
                auto filename = ac.checkArgument(0, {Value::Type::String});
                auto sz = ac.checkOptional(1, {Value::Type::Number});
                auto data = sz ? Util::readFile(filename.string(), sz->number().toInteger()) : Util::readFile(filename.string());
                if(!data)
                {
                    throw Error::IOError(loc, Util::joinArgs("failed to read from '", filename.string(), "'"));
                }
                return Value{std::move(data.value())};
            }

            Value filefunc_readdir(Environment& env, const Location& loc, Value::List&& args)
            {
                Value path;
                (void)env;
                Util::ArgumentCheck ac(loc, args, "File.readDirectory");
                path = ac.checkArgument(0, {Value::Type::String});
                auto ary = std::make_shared<Array>();
                try
                {
                    for(const auto& entry : std::filesystem::directory_iterator(path.string()))
                    {
                        ary->push_back(Value(entry.path().filename().string()));
                    }
                }
                catch(std::runtime_error& e)
                {
                    throw Error::OSError(loc, e.what());
                }
                return Value(std::move(ary));
            }


            Value filefunc_exists(Environment& env, const Location& loc, Value::List&& args)
            {
                Value path;
                (void)env;
                Util::ArgumentCheck ac(loc, args, "File.exists");
                path = ac.checkArgument(0, {Value::Type::String});
                return Value(Number::makeInteger(std::filesystem::exists(path.string())));
            }

            Value filefunc_size(Environment& env, const Location& loc, Value::List&& args)
            {
                Value path;
                (void)env;
                std::error_code ec;
                Util::ArgumentCheck ac(loc, args, "File.size");
                path = ac.checkArgument(0, {Value::Type::String});
                auto rs = Number::makeInteger(std::filesystem::file_size(path.string(), ec));
                if(ec)
                {
                    throw Error::OSError(loc, ec.message());
                }
                return Value(rs);
            }

            Value filefunc_readlink(Environment& env, const Location& loc, Value::List&& args)
            {
                Value path;
                (void)env;
                std::error_code ec;
                Util::ArgumentCheck ac(loc, args, "File.readLink");
                path = ac.checkArgument(0, {Value::Type::String});
                auto rs = std::filesystem::read_symlink(path.string(), ec);
                if(ec)
                {
                    rs = std::filesystem::absolute(path.string(), ec);
                    if(ec)
                    {
                        throw Error::OSError(loc, ec.message());
                    }
                }
                return Value(rs.string());
            }

            Value filefunc_basename(Environment& env, const Location& loc, Value::List&& args)
            {
                Value path;
                (void)env;
                Util::ArgumentCheck ac(loc, args, "File.basename");
                path = ac.checkArgument(0, {Value::Type::String});
                return Value(std::filesystem::path(path.string()).filename().string());
            }

            Value filefunc_dirname(Environment& env, const Location& loc, Value::List&& args)
            {
                Value path;
                (void)env;
                Util::ArgumentCheck ac(loc, args, "File.dirname");
                path = ac.checkArgument(0, {Value::Type::String});
                return Value(std::filesystem::path(path.string()).parent_path().string());
            }

            Value filefunc_extname(Environment& env, const Location& loc, Value::List&& args)
            {
                Value path;
                (void)env;
                Util::ArgumentCheck ac(loc, args, "File.extname");
                path = ac.checkArgument(0, {Value::Type::String});
                return Value(std::filesystem::path(path.string()).extension().string());
            }

        }

        void makeFileNamespace(Environment& env)
        {
            auto obj = std::make_shared<Object>();
            {
                obj->put("readFile", Value{filefunc_readfile});
                obj->put("read", Value{filefunc_readfile});
                obj->put("readDirectory", Value(HostFunction(filefunc_readdir)));
                obj->put("readLink", Value(HostFunction(filefunc_readlink)));
                obj->put("exists", Value(HostFunction(filefunc_exists)));
                obj->put("size", Value(HostFunction(filefunc_size)));
                obj->put("basename", Value(HostFunction(filefunc_basename)));
                obj->put("dirname", Value(HostFunction(filefunc_dirname)));
                obj->put("extname", Value(HostFunction(filefunc_extname)));
            }
            env.setGlobal("File", Value{std::move(obj)});
        }
    }
}
