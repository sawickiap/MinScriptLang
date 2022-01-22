
#include <array>
#include "priv.h"


namespace MSL
{
    static auto stdobjproperties_string = std::to_array<StandardObjectPropertyFunc>(
    {
        {"count", Builtins::protofn_string_length},
        {"length", Builtins::protofn_string_length},
        {"size", Builtins::protofn_string_length},
        {"chars", Builtins::protofn_string_chars},
        {"rstrip", Builtins::protofn_string_stripright},
        {"lstrip", Builtins::protofn_string_stripleft},
        {"strip", Builtins::protofn_string_strip},
    });

    static auto stdobjmethods_string = std::to_array<StandardObjectMemberFunc>(
    {
        {"resize", Builtins::memberfn_string_resize},
        {"startsWith", Builtins::memberfn_string_startswith},
        /*
        {"endsWith", Builtins::memberfn_string_endswith},
        {"includes", Builtins::memberfn_string_includes},
        {"contains", Builtins::memberfn_string_includes},
        {"lstrip", Builtins::memberfn_string_leftstrip},
        {"rstrip", Builtins::memberfn_string_rightstrip},
        {"strip", Builtins::memberfn_string_leftstrip},
        {"split", Builtins::memberfn_string_leftstrip},
        */
    });

    static auto stdobjproperties_object = std::to_array<StandardObjectPropertyFunc>(
    {
        {"count", Builtins::protofn_object_count},
    });

    static auto stdobjmethods_object = std::to_array<StandardObjectMemberFunc>(
    {
        {nullptr, nullptr},
    });

    static auto stdobjproperties_array = std::to_array<StandardObjectPropertyFunc>(
    {
        {"count", Builtins::protofn_array_length},
        {"length", Builtins::protofn_array_length},

    });

    static auto stdobjmethods_array = std::to_array<StandardObjectMemberFunc>(
    {
        {"push", Builtins::memberfn_array_push},
        {"add", Builtins::memberfn_array_push},
        {"pop", Builtins::memberfn_array_pop},
        {"insert", Builtins::memberfn_array_insert},
        {"remove", Builtins::memberfn_array_remove},
        {"each", Builtins::memberfn_array_each},
        {"map", Builtins::memberfn_array_map},

    });

    template<typename Type, size_t sz, typename DestType>
    bool get_thing(const std::array<Type, sz>& ary, std::string_view name, DestType& dest)
    {
        size_t i;
        for(i=0; i<sz; i++)
        {
            if(ary[i].name == nullptr)
            {
                break;
            }
            if(name == ary[i].name)
            {
                dest = ary[i];
                return true;
            }
        }
        return false;
    }

    template<typename Type, size_t sz>
    bool get_property(const std::array<Type, sz>& ary, std::string_view name, StandardObjectPropertyFunc& dest)
    {
        return get_thing<Type, sz>(ary, name, dest);
    }

    template<typename Type, size_t sz>
    bool get_method(const std::array<Type, sz>& ary, std::string_view name, StandardObjectMemberFunc& dest)
    {
        return get_thing<Type, sz>(ary, name, dest);
    }

    static std::shared_ptr<Object> objectFromException(const Error::RuntimeError& err)
    {
        auto obj = std::make_shared<Object>();
        obj->put("type", Value{ std::string(err.name()) });
        obj->put("index", Value{ Number::makeInteger(err.location().textindex) });
        obj->put("line", Value{ Number::makeInteger(err.location().textline) });
        obj->put("column", Value{ Number::makeInteger(err.location().textcolumn) });
        obj->put("message", Value{ std::string{ err.message() } });
        return obj;
    }

    Context::PushLocalScope::PushLocalScope(Context& ctx, Object* localscope, ThisObject&& thisobj, const Location& loc)
    : m_context{ ctx }
    {
        if(ctx.m_localscopes.size() == LOCAL_SCOPE_STACK_MAX_SIZE)
        {
            throw Error::RuntimeError{ loc, "stack overflow" };
        }
        ctx.m_localscopes.push_back(localscope);
        ctx.m_thislist.push_back(std::move(thisobj));
    }

    Context::PushLocalScope::~PushLocalScope()
    {
        m_context.m_thislist.pop_back();
        m_context.m_localscopes.pop_back();
        //GC::Collector::GC.collect();
    }

    Context::Context(Environment::Impl& env, Object& globalScope) : m_env{ env }, m_globalscope{ globalScope }
    {
    }

    Environment::Impl& Context::env()
    {
        return m_env;
    }

    Environment::Impl& Context::env() const
    {
        return m_env;
    }

    bool Context::isLocal() const
    {
        return !m_localscopes.empty();
    }

    Object* Context::getCurrentLocalScope()
    {
        assert(isLocal());
        return m_localscopes.back();
    }

    const ThisObject& Context::getThis()
    {
        //assert(isLocal());
        return m_thislist.back();
    }

    Object& Context::getInnermostScope() const
    {
        return isLocal() ? *m_localscopes.back() : m_globalscope;
    }

    namespace LeftValue
    {
        Value* Getter::getValueRef(const Location& loc) const
        {
            Value* val;
            const ObjectMember* leftmemberval;
            const ArrayItem* leftarrayitem;
            leftmemberval = std::get_if<ObjectMember>(this);
            if(leftmemberval)
            {
                if((val = leftmemberval->objectval->tryGet(leftmemberval->keyval)))
                {
                    return val;
                }
                throw Error::RuntimeError(loc, "cannot get reference to lvalue of non-existing members");
            }
            leftarrayitem = std::get_if<ArrayItem>(this);
            if(leftarrayitem)
            {
                MINSL_EXECUTION_CHECK(leftarrayitem->indexval < leftarrayitem->arrayval->size(), loc, "index out of bounds");
                return &leftarrayitem->arrayval->at(leftarrayitem->indexval);
            }
            throw Error::RuntimeError(loc, "lvalue required");
        }

        Value Getter::getValue(const Location& loc) const
        {
            char ch;
            const Value* val;
            const ArrayItem* leftarrayitem;
            const ObjectMember* leftmemberval;
            const StringCharacter* leftstrchar;
            if((leftmemberval = std::get_if<ObjectMember>(this)))
            {
                val = leftmemberval->objectval->tryGet(leftmemberval->keyval);
                if(val)
                {
                    return *val;
                }
                throw Error::RuntimeError(loc, "cannot get lvalue of non-existing member");
            }
            leftstrchar = std::get_if<StringCharacter>(this);
            if(leftstrchar)
            {
                MINSL_EXECUTION_CHECK(leftstrchar->indexval < leftstrchar->stringval->length(), loc, "index out of bounds");
                ch = (*leftstrchar->stringval)[leftstrchar->indexval];
                return Value{ std::string{ &ch, &ch + 1 } };
            }
            leftarrayitem = std::get_if<ArrayItem>(this);
            if(leftarrayitem)
            {
                MINSL_EXECUTION_CHECK(leftarrayitem->indexval < leftarrayitem->arrayval->size(), loc, "index out of bounds");
                return Value{ leftarrayitem->arrayval->at(leftarrayitem->indexval) };
            }
            assert(0);
            return {};
        }
    }

    namespace Runtime
    {
        Block::Block(const Location& loc) : Statement{ loc }
        {
        }

        Block::~Block()
        {
        }

        Statement::Statement(const Location& loc) : m_place{ loc }
        {
        }

        Statement::~Statement()
        {
        }

        const Location& Statement::location() const
        {
            return m_place;
        }

        void Statement::assign(const LeftValue::Getter& lhs, Value&& rhs) const
        {
            const LeftValue::StringCharacter* leftstrchar;
            const LeftValue::ArrayItem* leftarrayitem;
            const LeftValue::ObjectMember* leftobjmember;
            leftobjmember = std::get_if<LeftValue::ObjectMember>(&lhs);
            if(leftobjmember)
            {
                if(rhs.isNull())
                {
                    leftobjmember->objectval->removeEntry(leftobjmember->keyval);
                }
                else
                {
                    leftobjmember->objectval->put(leftobjmember->keyval, std::move(rhs));
                }
                return;
            }
            leftarrayitem = std::get_if<LeftValue::ArrayItem>(&lhs);
            if(leftarrayitem)
            {
                MINSL_EXECUTION_CHECK(leftarrayitem->indexval < leftarrayitem->arrayval->size(), location(), "index out of bounds");
                leftarrayitem->arrayval->at(leftarrayitem->indexval) = std::move(rhs);
                return;
            }
            leftstrchar = std::get_if<LeftValue::StringCharacter>(&lhs);
            if(leftstrchar)
            {
                MINSL_EXECUTION_CHECK(leftstrchar->indexval < leftstrchar->stringval->length(), location(), "index out of bounds");
                MINSL_EXECUTION_CHECK(rhs.isString(), location(), "expected string");
                MINSL_EXECUTION_CHECK(rhs.string().length() == 1, location(), "character literal too long");
                (*leftstrchar->stringval)[leftstrchar->indexval] = rhs.string()[0];
                return;
            }
            else
            {
                assert(0);
            }
        }

        Value Condition::execute(Context& ctx)
        {
            if(m_condexpr->evaluate(ctx, nullptr).isTrue())
            {
                return m_truestmt->execute(ctx);
            }
            else if(m_falsestmt)
            {
                return m_falsestmt->execute(ctx);
            }
            return {};
        }

        Value WhileLoop::execute(Context& ctx)
        {
            switch(m_type)
            {
                case WhileLoop::Type::While:
                    {
                        while(m_condexpr->evaluate(ctx, nullptr).isTrue())
                        {
                            try
                            {
                                m_body->execute(ctx);
                            }
                            catch(const BreakException&)
                            {
                                break;
                            }
                            catch(const ContinueException&)
                            {
                                continue;
                            }
                        }
                    }
                    break;
                case WhileLoop::Type::DoWhile:
                    {
                        do
                        {
                            try
                            {
                                m_body->execute(ctx);
                            }
                            catch(const BreakException&)
                            {
                                break;
                            }
                            catch(const ContinueException&)
                            {
                                continue;
                            }
                        } while(m_condexpr->evaluate(ctx, nullptr).isTrue());
                    }
                    break;
                default:
                    {
                        assert(0);
                    }
                    break;
            }
            return {};
        }

        Value ForLoop::execute(Context& ctx)
        {
            if(m_initexpr)
            {
                m_initexpr->execute(ctx);
            }
            while(m_condexpr ? m_condexpr->evaluate(ctx, nullptr).isTrue() : true)
            {
                try
                {
                    m_body->execute(ctx);
                }
                catch(const BreakException&)
                {
                    break;
                }
                catch(const ContinueException&)
                {
                }
                if(m_iterexpr)
                {
                    m_iterexpr->execute(ctx);
                }
            }
            return {};
        }


        Value RangeBasedForLoop::execute(Context& ctx)
        {
            int ch;
            size_t i;
            size_t count;
            bool usekey;
            Array* arr;
            Value rangeval;
            rangeval = m_rangeexpr->evaluate(ctx, nullptr);
            Object& innermostctx = ctx.getInnermostScope();
            usekey = !m_keyvar.empty();
            if(rangeval.isString())
            {
                const auto& rangestr = rangeval.string();
                count = rangestr.length();
                for(i = 0; i < count; ++i)
                {
                    if(usekey)
                    {
                        assign(LeftValue::Getter{ LeftValue::ObjectMember{ &innermostctx, m_keyvar } }, Value{ Number::makeInteger(i) });
                    }
                    ch = rangestr[i];
                    assign(LeftValue::Getter{ LeftValue::ObjectMember{ &innermostctx, m_valuevar } }, Value{ std::string{ &ch, &ch + 1 } });
                    try
                    {
                        m_body->execute(ctx);
                    }
                    catch(const BreakException&)
                    {
                        break;
                    }
                    catch(const ContinueException&)
                    {
                    }
                }
            }
            else if(rangeval.isObject())
            {
                for(const auto& [key, value] : rangeval.object()->m_entrymap)
                {
                    if(usekey)
                    {
                        assign(LeftValue::Getter{ LeftValue::ObjectMember{ &innermostctx, m_keyvar } }, Value{ std::string{ key } });
                    }
                    assign(LeftValue::Getter{ LeftValue::ObjectMember{ &innermostctx, m_valuevar } }, Value{ value });
                    try
                    {
                        m_body->execute(ctx);
                    }
                    catch(const BreakException&)
                    {
                        break;
                    }
                    catch(const ContinueException&)
                    {
                    }
                }
            }
            else if(rangeval.isArray())
            {
                arr = rangeval.array();
                for(i = 0, count = arr->size(); i < count; ++i)
                {
                    if(usekey)
                    {
                        assign(LeftValue::Getter{ LeftValue::ObjectMember{ &innermostctx, m_keyvar } }, Value{ Number::makeInteger(i) });
                    }
                    assign(LeftValue::Getter{ LeftValue::ObjectMember{ &innermostctx, m_valuevar } }, Value{ arr->at(i) });
                    try
                    {
                        m_body->execute(ctx);
                    }
                    catch(const BreakException&)
                    {
                        break;
                    }
                    catch(const ContinueException&)
                    {
                    }
                }
            }
            else
            {
                throw Error::RuntimeError(location(), "range-based loop can not be used with this object");
            }
            if(usekey)
            {
                assign(LeftValue::Getter{ LeftValue::ObjectMember{ &innermostctx, m_keyvar } }, Value{});
            }
            assign(LeftValue::Getter{ LeftValue::ObjectMember{ &innermostctx, m_valuevar } }, Value{});
            return {};
        }

        Value LoopBreakStatement::execute(Context& ctx)
        {
            (void)ctx;
            switch(m_type)
            {
                case LoopBreakStatement::Type::Break:
                    {
                        throw BreakException{};
                    }
                    break;
                case LoopBreakStatement::Type::Continue:
                    {
                        throw ContinueException{};
                    }
                    break;
                default:
                    {
                        assert(0);
                    }
                    break;
            }
            return {};
        }

        Value ReturnStatement::execute(Context& ctx)
        {
            if(m_retvalue)
            {
                throw ReturnException{ location(), m_retvalue->evaluate(ctx, nullptr) };
            }
            else
            {
                throw ReturnException{ location(), Value{} };
            }
            return {};
        }

        Value Block::execute(Context& ctx)
        {
            Value rv;
            for(const auto& stmt : m_statements)
            {
                rv = stmt->execute(ctx);
            }
            return rv;
        }

        Value SwitchStatement::execute(Context& ctx)
        {
            Value condval;
            size_t itemidx;
            size_t defitemidx;
            size_t itemcnt;
            defitemidx = SIZE_MAX;
            condval = m_cond->evaluate(ctx, nullptr);
            itemcnt = m_itemvals.size();
            for(itemidx = 0; itemidx < itemcnt; ++itemidx)
            {
                if(m_itemvals[itemidx])
                {
                    if(m_itemvals[itemidx]->m_val.isEqual(condval))
                    {
                        break;
                    }
                }
                else
                {
                    defitemidx = itemidx;
                }
            }
            if(itemidx == itemcnt && defitemidx != SIZE_MAX)
            {
                itemidx = defitemidx;
            }
            if(itemidx != itemcnt)
            {
                for(; itemidx < itemcnt; ++itemidx)
                {
                    try
                    {
                        m_itemblocks[itemidx]->execute(ctx);
                    }
                    catch(const BreakException&)
                    {
                        break;
                    }
                }
            }
            return {};
        }

        Value ThrowStatement::execute(Context& ctx)
        {
            throw m_thrownexpr->evaluate(ctx, nullptr);
            return {};
        }

        Value TryStatement::execute(Context& ctx)
        {
            /**
            // Careful with this function!
            // It contains logic that was difficult to get right.
            */
            try
            {
                m_tryblock->execute(ctx);
            }
            catch(Value& val)
            {
                if(m_catchblock)
                {
                    auto& innermostctx = ctx.getInnermostScope();
                    assign(LeftValue::Getter{ LeftValue::ObjectMember{ &innermostctx, m_exvarname } }, std::move(val));
                    m_catchblock->execute(ctx);
                    assign(LeftValue::Getter{ LeftValue::ObjectMember{ &innermostctx, m_exvarname } }, Value{});
                    if(m_finallyblock)
                    {
                        m_finallyblock->execute(ctx);
                    }
                }
                else
                {
                    assert(m_finallyblock);
                    // One exception is on the fly - new one is ignored, old one is thrown again.
                    try
                    {
                        m_finallyblock->execute(ctx);
                    }
                    catch(const Value&)
                    {
                        throw val;
                    }
                    catch(const Error::RuntimeError&)
                    {
                        throw val;
                    }
                    throw val;
                }
                return {};
            }
            catch(const Error::RuntimeError& err)
            {
                if(m_catchblock)
                {
                    auto& innermostctx = ctx.getInnermostScope();
                    assign(LeftValue::Getter{LeftValue::ObjectMember{&innermostctx, m_exvarname}}, Value{objectFromException(err)});
                    m_catchblock->execute(ctx);
                    assign(LeftValue::Getter{LeftValue::ObjectMember{&innermostctx, m_exvarname}}, Value{});
                    if(m_finallyblock)
                    {
                        m_finallyblock->execute(ctx);
                    }
                }
                else
                {
                    assert(m_finallyblock);
                    // One exception is on the fly - new one is ignored, old one is thrown again.
                    try
                    {
                        m_finallyblock->execute(ctx);
                    }
                    catch(const Value&)
                    {
                        throw err;
                    }
                    catch(const Error::RuntimeError&)
                    {
                        throw err;
                    }
                    throw err;
                }
                return {};
            }
            catch(const BreakException&)
            {
                if(m_finallyblock)
                {
                    m_finallyblock->execute(ctx);
                }
                throw;
            }
            catch(const ContinueException&)
            {
                if(m_finallyblock)
                {
                    m_finallyblock->execute(ctx);
                }
                throw;
            }
            catch(ReturnException&)
            {
                if(m_finallyblock)
                {
                    m_finallyblock->execute(ctx);
                }
                throw;
            }
            catch(const Error::ParsingError&)
            {
                assert(0 && "ParsingError not expected during execution.");
            }
            if(m_finallyblock)
            {
                m_finallyblock->execute(ctx);
            }
            return {};
        }

        Value Script::execute(Context& ctx)
        {
            try
            {
                return Block::execute(ctx);
            }
            catch(BreakException)
            {
                throw Error::RuntimeError{ location(), "use of 'break' outside of a loop" };
            }
            catch(ContinueException)
            {
                throw Error::RuntimeError{ location(), "use of 'continue' outside of a loop" };
            }
            return {};
        }

        Value Identifier::evaluate(Context& ctx, ThisObject* othis)
        {
            size_t i;
            size_t count;
            Value* val;
            const std::shared_ptr<Object>* thisobj;
            MINSL_EXECUTION_CHECK(m_scope != Identifier::Scope::Local || ctx.isLocal(), location(), "no local scope");
            if(ctx.isLocal())
            {
                // Local variable
                if((m_scope == Identifier::Scope::None || m_scope == Identifier::Scope::Local))
                {
                    if(val = ctx.getCurrentLocalScope()->tryGet(m_ident); val)
                    {
                        return *val;
                    }
                }
                // This
                if(m_scope == Identifier::Scope::None)
                {
                    thisobj = std::get_if<std::shared_ptr<Object>>(&ctx.getThis());
                    if(thisobj)
                    {
                        val = (*thisobj)->tryGet(m_ident);
                        if(val)
                        {
                            if(othis)
                            {
                                *othis = ThisObject{ *thisobj };
                            }
                            return *val;
                        }
                    }
                }
            }
            if(m_scope == Identifier::Scope::None || m_scope == Identifier::Scope::Global)
            {
                // Global variable
                val = ctx.m_globalscope.tryGet(m_ident);
                if(val)
                {
                    return *val;
                }
                // Type
                for(i = 0, count = (size_t)Value::Type::Count; i < count; ++i)
                {
                    //if(m_ident == VALUE_TYPE_NAMES[i])
                    if(m_ident == Value::getTypename((Value::Type)i))
                    {
                        return Value{ (Value::Type)i };
                    }
                }
            }
            // Not found - null
            return {};
        }

        LeftValue::Getter Identifier::getLeftValue(Context& ctx) const
        {
            bool islocal;
            const std::shared_ptr<Object>* thisobj;
            islocal = ctx.isLocal();
            MINSL_EXECUTION_CHECK(m_scope != Identifier::Scope::Local || islocal, location(), "no local scope");

            if(islocal)
            {
                // Local variable
                if((m_scope == Identifier::Scope::None || m_scope == Identifier::Scope::Local) && ctx.getCurrentLocalScope()->hasKey(m_ident))
                {
                    return LeftValue::Getter{ LeftValue::ObjectMember{ &*ctx.getCurrentLocalScope(), m_ident } };
                }
                // This
                if(m_scope == Identifier::Scope::None)
                {
                    thisobj = std::get_if<std::shared_ptr<Object>>(&ctx.getThis());
                    if(thisobj && (*thisobj)->hasKey(m_ident))
                    {
                        return LeftValue::Getter{ LeftValue::ObjectMember{ (*thisobj).get(), m_ident } };
                    }
                }
            }

            // Global variable
            if((m_scope == Identifier::Scope::None || m_scope == Identifier::Scope::Global) && ctx.m_globalscope.hasKey(m_ident))
            {
                return LeftValue::Getter{ LeftValue::ObjectMember{ &ctx.m_globalscope, m_ident } };
            }
            // Not found: return reference to smallest scope.
            if((m_scope == Identifier::Scope::None || m_scope == Identifier::Scope::Local) && islocal)
            {
                return LeftValue::Getter{ LeftValue::ObjectMember{ ctx.getCurrentLocalScope(), m_ident } };
            }
            return LeftValue::Getter{ LeftValue::ObjectMember{ &ctx.m_globalscope, m_ident } };
        }

        Value ThisExpression::evaluate(Context& ctx, ThisObject* othis)
        {
            (void)othis;
            MINSL_EXECUTION_CHECK(ctx.isLocal() && std::get_if<std::shared_ptr<Object>>(&ctx.getThis()), location(), "use of 'this' not possible in this context");
            return Value{ std::shared_ptr<Object>{ *std::get_if<std::shared_ptr<Object>>(&ctx.getThis()) } };
        }

        Value UnaryOperator::evaluate(Context& ctx, ThisObject* othis)
        {
            Value* pval;
            Value val;
            Value result;
            (void)othis;
            // Those require l-value.
            if(m_type == UnaryOperator::Type::Preincrementation || m_type == UnaryOperator::Type::Predecrementation
               || m_type == UnaryOperator::Type::Postincrementation || m_type == UnaryOperator::Type::Postdecrementation)
            {
                pval = m_operand->getLeftValue(ctx).getValueRef(location());
                MINSL_EXECUTION_CHECK(pval->isNumber(), location(), "expected numeric value");
                auto r = pval->number();
                switch(m_type)
                {
                    case UnaryOperator::Type::Preincrementation:
                        {
                            if(r.isInteger())
                            {
                                pval->setNumberValue(Number::makeInteger(r.toInteger() + 1));
                            }
                            else
                            {
                                //pval->setNumberValue(pval->number() + 1.0);
                                pval->setNumberValue(Number::makeFloat(r.toFloat() + 1.0));
                            }
                            return *pval;
                        }
                        break;
                    case UnaryOperator::Type::Predecrementation:
                        {
                            //pval->setNumberValue(pval->number() - 1.0);
                            if(r.isInteger())
                            {
                                pval->setNumberValue(Number::makeInteger(r.toInteger() - 1));
                            }
                            else
                            {
                                pval->setNumberValue(Number::makeFloat(r.toFloat() - 1));
                            }
                            return *pval;
                        }
                        break;
                    case UnaryOperator::Type::Postincrementation:
                        {
                            result = *pval;
                            //pval->setNumberValue(pval->number() + 1.0);
                            if(r.isInteger())
                            {
                                pval->setNumberValue(Number::makeInteger(pval->number().toInteger() + 1));
                            }
                            else
                            {
                                pval->setNumberValue(Number::makeFloat(pval->number().toFloat() + 1.0));
                                
                            }
                            return result;
                        }
                        break;
                    case UnaryOperator::Type::Postdecrementation:
                        {
                            result = *pval;
                            //pval->setNumberValue(pval->number() - 1.0);
                            if(r.isInteger())
                            {
                                pval->setNumberValue(Number::makeInteger(pval->number().toInteger() - 1));
                            }
                            else
                            {
                                pval->setNumberValue(Number::makeFloat(pval->number().toFloat() - 1.0));
                            }
                            return result;
                        }
                        break;
                    default:
                        {
                            assert(0);
                            return {};
                        }
                        break;
                }
            }
            // Those use r-value.
            else if(m_type == UnaryOperator::Type::Plus || m_type == UnaryOperator::Type::Minus
                    || m_type == UnaryOperator::Type::LogicalNot || m_type == UnaryOperator::Type::BitwiseNot)
            {
                val = m_operand->evaluate(ctx, nullptr);
                MINSL_EXECUTION_CHECK(val.isNumber(), location(), "expected numeric value");
                switch(m_type)
                {
                    case UnaryOperator::Type::Plus:
                        {
                            return val;
                        }
                        break;
                    case UnaryOperator::Type::Minus:
                        {
                            if(val.number().isInteger())
                            {
                                return Value{ Number::makeInteger(-val.number().toInteger()) };
                            }
                            return Value{ Number::makeFloat(-val.number().toFloat()) };
                        }
                        break;
                    case UnaryOperator::Type::LogicalNot:
                        {
                            return Value{ Number::makeInteger(val.isTrue() ? 0 : 1) };
                        }
                        break;
                    case UnaryOperator::Type::BitwiseNot:
                        {
                            return BitwiseNot(std::move(val));
                        }
                        break;
                    default:
                        {
                            assert(0);
                            return {};
                        }
                        break;
                }
            }
            assert(0);
            return {};
        }

        LeftValue::Getter UnaryOperator::getLeftValue(Context& ctx) const
        {
            LeftValue::Getter lval;
            Value* val;
            const LeftValue::ObjectMember* leftmemberval;
            if(m_type == UnaryOperator::Type::Preincrementation || m_type == UnaryOperator::Type::Predecrementation)
            {
                lval = m_operand->getLeftValue(ctx);
                leftmemberval = std::get_if<LeftValue::ObjectMember>(&lval);
                MINSL_EXECUTION_CHECK(leftmemberval, location(), "lvalue required");
                val = leftmemberval->objectval->tryGet(leftmemberval->keyval);
                MINSL_EXECUTION_CHECK(val != nullptr, location(), "no such name");
                MINSL_EXECUTION_CHECK(val->isNumber(), location(), "expected numeric value");
                switch(m_type)
                {
                    case UnaryOperator::Type::Preincrementation:
                        {
                            if(val->number().isInteger())
                            {
                                val->setNumberValue(Number::makeInteger(val->number().toInteger() + 1));
                            }
                            else
                            {
                                val->setNumberValue(Number::makeFloat(val->number().toFloat() + 1.0));
                            }
                            return lval;
                        }
                        break;
                    case UnaryOperator::Type::Predecrementation:
                        {
                            if(val->number().isInteger())
                            {
                                val->setNumberValue(Number::makeInteger(val->number().toInteger() - 1));
                            }
                            else
                            {
                                val->setNumberValue(Number::makeFloat(val->number().toFloat() - 1.0));
                            }
                            return lval;
                        }
                        break;
                    default:
                        {
                            assert(0);
                        }
                        break;
                }
            }
            throw Error::RuntimeError(location(), "lvalue required");
        }


        /**!
        // TODO:
        // turn this into a table lookup.
        */
        Value MemberAccessOperator::evaluate(Context& ctx, ThisObject* othis)
        {
            Value vobj;
            const Value* memberval;
            StandardObjectPropertyFunc prop;
            StandardObjectMemberFunc meth;
            vobj = m_operand->evaluate(ctx, nullptr);
            if(vobj.isObject())
            {
                memberval = vobj.object()->tryGet(m_membername);
                if(memberval)
                {
                    if(othis)
                    {
                        *othis = ThisObject{ vobj.objectRef() };
                    }
                    return *memberval;
                }
                if(get_property(stdobjproperties_object, m_membername, prop))
                {
                    return prop.func(ctx, location(), std::move(vobj));
                }
                else if(get_method(stdobjmethods_object, m_membername, meth))
                {
                    return Value{ meth.func };
                }
                return {};
            }
            if(vobj.isString())
            {
                if(othis)
                {
                    *othis = ThisObject{ vobj.string() };
                }
                if(get_property(stdobjproperties_string, m_membername, prop))
                {
                    return prop.func(ctx, location(), std::move(vobj));
                }
                else if(get_method(stdobjmethods_string, m_membername, meth))
                {
                    return Value{ meth.func };
                }                
                throw Error::TypeError(location(), "no such String member");
            }
            if(vobj.isArray())
            {
                if(othis)
                {
                    *othis = ThisObject{ vobj.arrayRef() };
                }
                if(get_property(stdobjproperties_array, m_membername, prop))
                {
                    return prop.func(ctx, location(), std::move(vobj));
                }
                else if(get_method(stdobjmethods_array, m_membername, meth))
                {
                    return Value{ meth.func };
                }
                throw Error::TypeError(location(), "no such Array member");
            }
            throw Error::TypeError(location(), "member access in something not an object");
        }

        LeftValue::Getter MemberAccessOperator::getLeftValue(Context& ctx) const
        {
            Value vobj;
            vobj = m_operand->evaluate(ctx, nullptr);
            MINSL_EXECUTION_CHECK(vobj.isObject(), location(), "expected object");
            return LeftValue::Getter{ LeftValue::ObjectMember{ vobj.object(), m_membername } };
        }

        Value UnaryOperator::BitwiseNot(Value&& operand) const
        {
            Number::IntegerValueType operval;
            Number::IntegerValueType resval;
            operval = operand.number().toInteger();
            resval = ~operval;
            return Value{ Number::makeInteger(resval) };
        }

        Value BinaryOperator::evaluate(Context& ctx, ThisObject* othis)
        {
            size_t index;
            bool result;
            Value left;
            Value right;
            Value rhval;
            Value::Type typleft;
            Value::Type typright;
            LeftValue::Getter lfval;
            // This operator is special, discards result of left operand.
            if(m_type == BinaryOperator::Type::Comma)
            {
                m_leftoper->execute(ctx);
                return m_rightoper->evaluate(ctx, othis);
            }
            // Operators that require l-value.
            switch(m_type)
            {
                case BinaryOperator::Type::Assignment:
                case BinaryOperator::Type::AssignmentAdd:
                case BinaryOperator::Type::AssignmentSub:
                case BinaryOperator::Type::AssignmentMul:
                case BinaryOperator::Type::AssignmentDiv:
                case BinaryOperator::Type::AssignmentMod:
                case BinaryOperator::Type::AssignmentShiftLeft:
                case BinaryOperator::Type::AssignmentShiftRight:
                case BinaryOperator::Type::AssignmentBitwiseAnd:
                case BinaryOperator::Type::AssignmentBitwiseXor:
                case BinaryOperator::Type::AssignmentBitwiseOr:
                    {
                        // Getting these explicitly so the order of their evaluation is defined, unlike
                        // in C++ function call arguments.
                        rhval = m_rightoper->evaluate(ctx, nullptr);
                        LeftValue::Getter lfval = m_leftoper->getLeftValue(ctx);
                        return Assignment(std::move(lfval), std::move(rhval));
                    }
                    break;
                default:
                    {
                    }
                    break;
            }
            // Remaining operators use r-values.
            left = m_leftoper->evaluate(ctx, nullptr);

            // Logical operators with short circuit for right hand side operand.
            if(m_type == BinaryOperator::Type::LogicalAnd)
            {
                if(!left.isTrue())
                {
                    return left;
                }
                return m_rightoper->evaluate(ctx, nullptr);
            }
            if(m_type == BinaryOperator::Type::LogicalOr)
            {
                if(left.isTrue())
                {
                    return left;
                }
                return m_rightoper->evaluate(ctx, nullptr);
            }
            // Remaining operators use both operands as r-values.
            right = m_rightoper->evaluate(ctx, nullptr);
            typleft = left.type();
            typright = right.type();
            // These ones support various types.
            if(m_type == BinaryOperator::Type::Add)
            {
                left.location() = location();
                return left.opPlus(right);
            }
            if(m_type == BinaryOperator::Type::Equal)
            {
                if(left.isEqual(right))
                {
                    return Value{Number::makeInteger(1)};
                }
                return Value{Number::makeInteger(0)};
            }
            if(m_type == BinaryOperator::Type::NotEqual)
            {
                if(!left.isEqual(right))
                {
                    return Value{Number::makeInteger(1)};
                }
                return Value{Number::makeInteger(0)};
            }
            if(m_type == BinaryOperator::Type::Less || m_type == BinaryOperator::Type::LessEqual
               || m_type == BinaryOperator::Type::Greater || m_type == BinaryOperator::Type::GreaterEqual)
            {
                result = false;
                switch(m_type)
                {
                    case BinaryOperator::Type::Less:
                        {
                            result = left.opCompareLessThan(right);
                        }
                        break;
                    case BinaryOperator::Type::LessEqual:
                        {
                            result = left.opCompareLessEqual(right);
                        }
                        break;
                    case BinaryOperator::Type::Greater:
                        {
                            result = left.opCompareGreaterThan(right);
                        }
                        break;
                    case BinaryOperator::Type::GreaterEqual:
                        {
                            result = left.opCompareGreaterEqual(right);
                        }
                        break;
                    default:
                        assert(0);
                }
                if(result)
                {
                    return Value{Number::makeInteger(1)};
                }
                return Value{Number::makeInteger(0)};
            }
            if(m_type == BinaryOperator::Type::Indexing)
            {
                
                if(typleft == Value::Type::String)
                {
                    MINSL_EXECUTION_CHECK(typright == Value::Type::Number, location(), "expected numeric value");
                    index = 0;
                    MINSL_EXECUTION_CHECK(Util::NumberToIndex(index, right.number()), location(), "string index out of bounds");
                    MINSL_EXECUTION_CHECK(index < left.string().length(), location(), "string index out of bounds");
                    return Value{ std::string(1, left.string()[index]) };
                }
                if(typleft == Value::Type::Object)
                {
                    MINSL_EXECUTION_CHECK(typright == Value::Type::String, location(), "expected string value");
                    auto val = left.object()->tryGet(right.string());
                    if(val)
                    {
                        if(othis)
                        {
                            *othis = ThisObject{ left.objectRef() };
                        }
                        return *val;
                    }
                    return {};
                }
                if(typleft == Value::Type::Array)
                {
                    MINSL_EXECUTION_CHECK(typright == Value::Type::Number, location(), "expected numeric value");
                    MINSL_EXECUTION_CHECK(Util::NumberToIndex(index, right.number()) && index < left.array()->size(),
                                          location(), "array index out of bounds");
                    return left.array()->at(index);
                }
                throw Error::TypeError(location(), "cannot index this type");
            }

            switch(m_type)
            {
                case BinaryOperator::Type::Mul:
                    {
                        return left.opMul(right);
                    }
                    break;
                case BinaryOperator::Type::Div:
                    {
                        return left.opDiv(right);
                    }
                    break;
                case BinaryOperator::Type::Mod:
                    {
                        return left.opMod(right);
                    }
                    break;
                case BinaryOperator::Type::Sub:
                    {
                        return left.opMinus(right);
                    }
                    break;
                case BinaryOperator::Type::ShiftLeft:
                    {
                        return left.opShiftLeft(right);
                    }
                    break;
                case BinaryOperator::Type::ShiftRight:
                    {
                        return left.opShiftRight(right);
                    }
                    break;
                case BinaryOperator::Type::BitwiseAnd:
                    {
                        return left.opBitwiseAnd(right);
                    }
                    break;
                case BinaryOperator::Type::BitwiseXor:
                    {
                        return left.opBitwiseXor(right);
                    }
                case BinaryOperator::Type::BitwiseOr:
                    {
                        return left.opBitwiseOr(right);
                    }
                default:
                    {
                    }
                    break;
            }
            assert(0);
            return {};
        }

        LeftValue::Getter BinaryOperator::getLeftValue(Context& ctx) const
        {
            size_t itemidx;
            size_t charidx;
            Value idxval;
            Value* leftref;
            if(m_type == BinaryOperator::Type::Indexing)
            {
                leftref = m_leftoper->getLeftValue(ctx).getValueRef(location());
                idxval = m_rightoper->evaluate(ctx, nullptr);
                if(leftref->isString())
                {
                    MINSL_EXECUTION_CHECK(idxval.isNumber(), location(), "expected numeric value");
                    MINSL_EXECUTION_CHECK(Util::NumberToIndex(charidx, idxval.number()), location(), "string index out of bounds");
                    return LeftValue::Getter{ LeftValue::StringCharacter{ &leftref->string(), charidx } };
                }
                if(leftref->type() == Value::Type::Object)
                {
                    MINSL_EXECUTION_CHECK(idxval.isString(), location(), "expected string value");
                    return LeftValue::Getter{ LeftValue::ObjectMember{ leftref->object(), idxval.string() } };
                }
                if(leftref->type() == Value::Type::Array)
                {
                    MINSL_EXECUTION_CHECK(idxval.isNumber(), location(), "expected numeric value");
                    MINSL_EXECUTION_CHECK(Util::NumberToIndex(itemidx, idxval.number()), location(), "array index out of bounds");
                    return LeftValue::Getter{ LeftValue::ArrayItem{ leftref->array(), itemidx } };
                }
            }
            return Operator::getLeftValue(ctx);
        }

        Value BinaryOperator::Assignment(LeftValue::Getter&& lhs, Value&& rhs) const
        {
            Value* leftvalptr;
            // This one is able to create new value.
            if(m_type == BinaryOperator::Type::Assignment)
            {
                assign(lhs, Value{ rhs });
                return std::move(rhs);
            }
            // Others require existing value.
            leftvalptr = lhs.getValueRef(location());
            leftvalptr->location() = location();
            if(m_type == BinaryOperator::Type::AssignmentAdd)
            {
                return leftvalptr->opPlusAssign(std::move(rhs));
            }
            switch(m_type)
            {
                case BinaryOperator::Type::AssignmentSub:
                    {
                        return leftvalptr->opMinusAssign(rhs);
                    }
                    break;
                case BinaryOperator::Type::AssignmentMul:
                    {
                        return leftvalptr->opMulAssign(rhs);
                    }
                    break;
                case BinaryOperator::Type::AssignmentDiv:
                    {
                        return leftvalptr->opDivAssign(rhs);
                    }
                    break;
                case BinaryOperator::Type::AssignmentMod:
                    {
                        return leftvalptr->opModAssign(rhs);
                    }
                    break;
                case BinaryOperator::Type::AssignmentShiftLeft:
                    {
                        return leftvalptr->opShiftLeftAssign(rhs);
                    }
                    break;
                case BinaryOperator::Type::AssignmentShiftRight:
                    {
                        return leftvalptr->opShiftRightAssign(rhs);
                    }
                    break;
                case BinaryOperator::Type::AssignmentBitwiseAnd:
                    {
                        return leftvalptr->opBitwiseAndAssign(rhs);
                    }
                    break;
                case BinaryOperator::Type::AssignmentBitwiseXor:
                    {
                        return leftvalptr->opBitwiseXorAssign(rhs);
                    }
                    break;
                case BinaryOperator::Type::AssignmentBitwiseOr:
                    {
                        return leftvalptr->opBitwiseOrAssign(rhs);
                    }
                    break;
                default:
                    {
                        assert(0);
                    }
                    break;
            }
            return *leftvalptr;
        }


        Value TernaryOperator::evaluate(Context& ctx, ThisObject* othis)
        {
            if(m_condexpr->evaluate(ctx, nullptr).isTrue())
            {
                return m_trueexpr->evaluate(ctx, othis);
            }
            return m_falseexpr->evaluate(ctx, othis);
        }

        Value CallOperator::evaluate(Context& ctx, ThisObject* othis)
        {
            (void)othis;
            size_t i;
            size_t argcnt;
            ThisObject th;
            th = ThisObject{};
            Value callee;
            callee = m_oplist[0]->evaluate(ctx, &th);
            argcnt = m_oplist.size() - 1;
            Value::List arguments(argcnt);
            for(i = 0; i < argcnt; ++i)
            {
                arguments[i] = m_oplist[i + 1]->evaluate(ctx, nullptr);
            }
            // Calling an object: Call its function under '' key.
            if(callee.isObject())
            {
                auto objcallee = callee.objectRef();
                auto defval = objcallee->tryGet("__init__"); 
                if(defval && defval->type() == Value::Type::Function)
                {
                    callee = *defval;
                    th = ThisObject{ std::move(objcallee) };
                }
            }
            if(callee.type() == Value::Type::Function)
            {
                auto funcdef = callee.scriptFunction();
                MINSL_EXECUTION_CHECK(argcnt == funcdef->m_paramlist.size(), location(), "inexact number of arguments");
                return callee.scriptFunction()->call(ctx, std::move(arguments), th);
            }
            if(callee.type() == Value::Type::HostFunction)
            {
                return callee.hostFunction()(ctx.m_env.getOwner(), location(), std::move(arguments));
            }
            if(callee.type() == Value::Type::Type)
            {
                switch(callee.getTypeValue())
                {
                    case Value::Type::Null:
                        {
                            return Builtins::ctor_null(ctx, location(), std::move(arguments));
                        }
                        break;
                    case Value::Type::Number:
                        {
                            return Builtins::ctor_number(ctx, location(), std::move(arguments));
                        }
                        break;
                    case Value::Type::String:
                        {
                            return Builtins::ctor_string(ctx, location(), std::move(arguments));
                        }
                        break;
                    case Value::Type::Object:
                        {
                            return Builtins::ctor_object(ctx, location(), std::move(arguments));
                        }
                        break;
                    case Value::Type::Array:
                        {
                            return Builtins::ctor_array(ctx, location(), std::move(arguments));
                        }
                        break;
                    case Value::Type::Type:
                        {
                            return Builtins::ctor_type(ctx, location(), std::move(arguments));
                        }
                        break;
                    case Value::Type::Function:
                        {
                            return Builtins::ctor_function(ctx, location(), std::move(arguments));
                        }
                        break;
                    case Value::Type::MemberMethod:
                        {
                            return callee.memberFunction()(ctx, location(), th, std::move(arguments));
                        }
                        break;
                    case Value::Type::MemberProperty:
                        {
                            return callee.propertyFunction()(ctx, location(), std::move(callee));
                        }
                        break;
                    default:
                        {
                            assert(0);
                            return {};
                        }
                        break;
                }
            }
            if(callee.type() == Value::Type::MemberMethod)
            {
                return callee.memberFunction()(ctx, location(), th, std::move(arguments));
            }
            if(callee.type() == Value::Type::MemberProperty)
            {
                return callee.propertyFunction()(ctx, location(), std::move(callee));
            }
            throw Error::RuntimeError(location(), "invalid function call");
        }

        Value FunctionDefinition::call(Context& ctx, Value::List&& args, ThisObject& th)
        {
            size_t argidx;
            Object ourlocalscope;
            // Setup parameters
            for(argidx = 0; argidx<args.size(); argidx++)
            {
                ourlocalscope.put(m_paramlist[argidx], std::move(args[argidx]));
            }
            Context::PushLocalScope ourlocalcontext{ ctx, &ourlocalscope, std::move(th), location() };
            try
            {
                m_body.execute(ctx);
            }
            catch(ReturnException& returnEx)
            {
                return std::move(returnEx.thrownvalue);
            }
            catch(BreakException)
            {
                throw Error::RuntimeError(location(), "use of 'break' outside of a loop");
            }
            catch(ContinueException)
            {
                throw Error::RuntimeError(location(), "use of 'continue' outside of a loop");
            }
            return {};
        }

        bool FunctionDefinition::areParamsUnique() const
        {
            size_t i;
            size_t j;
            size_t count;
            // Warning! O(n^2) algorithm.
            for(i = 0, count = m_paramlist.size(); i < count; ++i)
            {
                for(j = i + 1; j < count; ++j)
                {
                    if(m_paramlist[i] == m_paramlist[j])
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        Value ObjectExpression::evaluate(Context& ctx, ThisObject* othis)
        {
            std::shared_ptr<Object> obj;
            (void)othis;
            if(m_baseexpr)
            {
                auto baseobj = m_baseexpr->evaluate(ctx, nullptr);
                if(baseobj.type() != Value::Type::Object)
                {
                    throw Error::TypeError{ location(), "base must be object" };
                }
                obj = Util::CopyObject(*baseobj.object());
            }
            else
            {
                obj = std::make_shared<Object>();
            }
            for(const auto& [name, valexpr] : m_exprmap)
            {
                auto val = valexpr->evaluate(ctx, nullptr);
                if(val.type() != Value::Type::Null)
                {
                    obj->put(name, std::move(val));
                }
                else if(m_baseexpr)
                {
                    obj->removeEntry(name);
                }
            }
            return Value{ std::move(obj) };
        }

        Value ArrayExpression::evaluate(Context& ctx, ThisObject* othis)
        {
            (void)othis;
            auto result = std::make_shared<Array>();
            for(const auto& item : m_exprlist)
            {
                result->push_back(item->evaluate(ctx, nullptr));
            }
            return Value{ std::move(result) };
        }
    }
}


