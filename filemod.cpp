
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
                filename = Util::checkArgument(place, "File.read", args, 0, Value::Type::String);
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

            Value filefunc_readdir(Environment& env, const Location& place, Value::List&& args)
            {
                Value path;
                (void)env;
                path = Util::checkArgument(place, "File.readDirectory", args, 0, Value::Type::String);
                auto ary = std::make_shared<Array>();
                try
                {
                    for(const auto& entry : std::filesystem::directory_iterator(path.getString()))
                    {
                        ary->push_back(Value(entry.path().filename().string()));
                    }
                }
                catch(std::runtime_error& e)
                {
                    throw Error::OSError(place, e.what());
                }
                return Value(std::move(ary));
            }


            Value filefunc_exists(Environment& env, const Location& place, Value::List&& args)
            {
                Value path;
                (void)env;
                path = Util::checkArgument(place, "File.exists", args, 0, Value::Type::String);
                return Value(double(std::filesystem::exists(path.getString())));
            }

            Value filefunc_size(Environment& env, const Location& place, Value::List&& args)
            {
                double rs;
                Value path;
                (void)env;
                std::error_code ec;
                path = Util::checkArgument(place, "File.size", args, 0, Value::Type::String);
                rs = double(std::filesystem::file_size(path.getString(), ec));
                if(ec)
                {
                    throw Error::OSError(place, ec.message());
                }
                return Value(rs);
            }

            Value filefunc_readlink(Environment& env, const Location& place, Value::List&& args)
            {
                Value path;
                (void)env;
                std::error_code ec;
                path = Util::checkArgument(place, "File.readLink", args, 0, Value::Type::String);
                auto rs = std::filesystem::read_symlink(path.getString(), ec);
                if(ec)
                {
                    rs = std::filesystem::absolute(path.getString(), ec);
                    if(ec)
                    {
                        throw Error::OSError(place, ec.message());
                    }
                }
                return Value(rs.string());
            }

            Value filefunc_basename(Environment& env, const Location& place, Value::List&& args)
            {
                Value path;
                (void)env;
                path = Util::checkArgument(place, "File.basename", args, 0, Value::Type::String);
                return Value(std::filesystem::path(path.getString()).filename().string());
            }

            Value filefunc_dirname(Environment& env, const Location& place, Value::List&& args)
            {
                Value path;
                (void)env;
                path = Util::checkArgument(place, "File.dirname", args, 0, Value::Type::String);
                return Value(std::filesystem::path(path.getString()).parent_path().string());
            }

            Value filefunc_extname(Environment& env, const Location& place, Value::List&& args)
            {
                Value path;
                (void)env;
                path = Util::checkArgument(place, "File.extname", args, 0, Value::Type::String);
                return Value(std::filesystem::path(path.getString()).extension().string());
            }

        }

        void makeFileNamespace(Environment& env)
        {
            auto obj = std::make_shared<Object>();
            {
                obj->put("readFile", Value{filefunc_readfile});
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
