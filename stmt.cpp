
#include <array>
#include "msl.h"

#define DEBUG_PRINT_FORMAT_STR_BEG "(%u,%u) %s%.*s"
#define DEBUG_PRINT_ARGS_BEG \
    getPlace().textrow, getPlace().textcolumn, GetDebugPrintIndent(indentLevel), (int)prefix.length(), prefix.data()


namespace MSL
{
    struct StandardObjectMemberFunc
    {
        const char* name;
        MemberMethodFunction* func;
    };

    struct StandardObjectPropertyFunc
    {
        const char* name;
        MemberPropertyFunction* func;
    };

    static constexpr auto stdobjproperties_string = std::to_array<StandardObjectPropertyFunc>(
    {
        {"count", Builtins::protofn_string_length},
        {"length", Builtins::protofn_string_length},
        {"size", Builtins::protofn_string_length},
    });

    static constexpr auto stdobjmethods_string = std::to_array<StandardObjectMemberFunc>(
    {
        {"resize", Builtins::memberfn_string_resize},
    });

    static constexpr auto stdobjproperties_object = std::to_array<StandardObjectPropertyFunc>(
    {
        {"count", Builtins::protofn_object_count},
    });

    static constexpr auto stdobjmethods_object = std::to_array<StandardObjectMemberFunc>(
    {
        {nullptr, nullptr},
    });

    static constexpr auto stdobjproperties_array = std::to_array<StandardObjectPropertyFunc>(
    {
        {"count", Builtins::protofn_array_length},
        {"length", Builtins::protofn_array_length},

    });

    static constexpr auto stdobjmethods_array = std::to_array<StandardObjectMemberFunc>(
    {
        {"add", Builtins::memberfn_array_add},
        {"insert", Builtins::memberfn_array_insert},
        {"remove", Builtins::memberfn_array_remove},
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

    static std::shared_ptr<Object> ConvertExecutionErrorToObject(const ExecutionError& err)
    {
        auto obj = std::make_shared<Object>();
        obj->entry("type") = Value{ "ExecutionError" };
        obj->entry("index") = Value{ (double)err.getPlace().textindex };
        obj->entry("row") = Value{ (double)err.getPlace().textrow };
        obj->entry("column") = Value{ (double)err.getPlace().textcolumn };
        obj->entry("message") = Value{ std::string{ err.getMessage() } };
        return obj;
    }

    static const char* GetDebugPrintIndent(uint32_t indentLevel)
    {
        static const char* silly = "                                                                                                                                                                                                                                                                ";
        return silly + (256 - std::min<uint32_t>(indentLevel, 128) * 2);
    }

    Value* LValue::getValueRef(const PlaceInCode& place) const
    {
        Value* val;
        const ObjectMemberLValue* leftmemberval;
        const ArrayItemLValue* leftarrayitem;
        leftmemberval = std::get_if<ObjectMemberLValue>(this);
        if(leftmemberval)
        {
            if((val = leftmemberval->objectval->tryGet(leftmemberval->keyval)))
            {
                return val;
            }
            throw ExecutionError(place, "cannot get reference to lvalue of non-existing members");
        }
        leftarrayitem = std::get_if<ArrayItemLValue>(this);
        if(leftarrayitem)
        {
            MINSL_EXECUTION_CHECK(leftarrayitem->indexval < leftarrayitem->arrayval->m_items.size(), place, ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
            return &leftarrayitem->arrayval->m_items[leftarrayitem->indexval];
        }
        throw ExecutionError(place, "lvalue required");
    }

    Value LValue::getValue(const PlaceInCode& place) const
    {
        char ch;
        const Value* val;
        const ArrayItemLValue* leftarrayitem;
        const ObjectMemberLValue* leftmemberval;
        const StringCharacterLValue* leftstrchar;
        if((leftmemberval = std::get_if<ObjectMemberLValue>(this)))
        {
            val = leftmemberval->objectval->tryGet(leftmemberval->keyval);
            if(val)
            {
                return *val;
            }
            throw ExecutionError(place, "cannot get lvalue of non-existing member");
        }
        if((leftstrchar = std::get_if<StringCharacterLValue>(this)))
        {
            MINSL_EXECUTION_CHECK(leftstrchar->indexval < leftstrchar->stringval->length(), place, ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
            ch = (*leftstrchar->stringval)[leftstrchar->indexval];
            return Value{ std::string{ &ch, &ch + 1 } };
        }
        if((leftarrayitem = std::get_if<ArrayItemLValue>(this)))
        {
            MINSL_EXECUTION_CHECK(leftarrayitem->indexval < leftarrayitem->arrayval->m_items.size(), place, ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
            return Value{ leftarrayitem->arrayval->m_items[leftarrayitem->indexval] };
        }
        assert(0);
        return {};
    }

    namespace AST
    {
        void Statement::assign(const LValue& lhs, Value&& rhs) const
        {
            const StringCharacterLValue* leftstrchar;
            const ArrayItemLValue* leftarrayitem;
            const ObjectMemberLValue* leftobjmember;
            if((leftobjmember = std::get_if<ObjectMemberLValue>(&lhs)))
            {
                if(rhs.isNull())
                    leftobjmember->objectval->removeEntry(leftobjmember->keyval);
                else
                    leftobjmember->objectval->entry(leftobjmember->keyval) = std::move(rhs);
            }
            else if((leftarrayitem = std::get_if<ArrayItemLValue>(&lhs)))
            {
                MINSL_EXECUTION_CHECK(leftarrayitem->indexval < leftarrayitem->arrayval->m_items.size(), getPlace(), ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
                leftarrayitem->arrayval->m_items[leftarrayitem->indexval] = std::move(rhs);
            }
            else if((leftstrchar = std::get_if<StringCharacterLValue>(&lhs)))
            {
                MINSL_EXECUTION_CHECK(leftstrchar->indexval < leftstrchar->stringval->length(), getPlace(), ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
                MINSL_EXECUTION_CHECK(rhs.isString(), getPlace(), ERROR_MESSAGE_EXPECTED_STRING);
                MINSL_EXECUTION_CHECK(rhs.getString().length() == 1, getPlace(), ERROR_MESSAGE_EXPECTED_SINGLE_CHARACTER_STRING);
                (*leftstrchar->stringval)[leftstrchar->indexval] = rhs.getString()[0];
            }
            else
                assert(0);
        }

        void EmptyStatement::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Empty\n", DEBUG_PRINT_ARGS_BEG);
        }

        void Condition::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "If\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            m_condexpr->debugPrint(indentLevel, "ConditionExpression: ");
            m_statements[0]->debugPrint(indentLevel, "TrueStatement: ");
            if(m_statements[1])
                m_statements[1]->debugPrint(indentLevel, "FalseStatement: ");
        }

        void Condition::execute(ExecutionContext& ctx) const
        {
            if(m_condexpr->evaluate(ctx, nullptr).isTrue())
                m_statements[0]->execute(ctx);
            else if(m_statements[1])
                m_statements[1]->execute(ctx);
        }

        void WhileLoop::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            const char* name = nullptr;
            switch(m_type)
            {
                case WhileLoop::Type::While:
                    name = "While";
                    break;
                case WhileLoop::Type::DoWhile:
                    name = "DoWhile";
                    break;
                default:
                    assert(0);
            }
            printf(DEBUG_PRINT_FORMAT_STR_BEG "%s\n", DEBUG_PRINT_ARGS_BEG, name);
            ++indentLevel;
            m_condexpr->debugPrint(indentLevel, "ConditionExpression: ");
            m_body->debugPrint(indentLevel, "Body: ");
        }

        void WhileLoop::execute(ExecutionContext& ctx) const
        {
            switch(m_type)
            {
                case WhileLoop::Type::While:
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
                    break;
                case WhileLoop::Type::DoWhile:
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
                    break;
                default:
                    assert(0);
            }
        }

        void ForLoop::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "For\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            if(m_initexpr)
                m_initexpr->debugPrint(indentLevel, "InitExpression: ");
            else
                printf(DEBUG_PRINT_FORMAT_STR_BEG "(Init expression empty)\n", DEBUG_PRINT_ARGS_BEG);
            if(m_condexpr)
                m_condexpr->debugPrint(indentLevel, "ConditionExpression: ");
            else
                printf(DEBUG_PRINT_FORMAT_STR_BEG "(Condition expression empty)\n", DEBUG_PRINT_ARGS_BEG);
            if(m_iterexpr)
                m_iterexpr->debugPrint(indentLevel, "IterationExpression: ");
            else
                printf(DEBUG_PRINT_FORMAT_STR_BEG "(Iteration expression empty)\n", DEBUG_PRINT_ARGS_BEG);
            m_body->debugPrint(indentLevel, "Body: ");
        }

        void ForLoop::execute(ExecutionContext& ctx) const
        {
            if(m_initexpr)
                m_initexpr->execute(ctx);
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
                    m_iterexpr->execute(ctx);
            }
        }

        void RangeBasedForLoop::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            if(!m_keyvar.empty())
                printf(DEBUG_PRINT_FORMAT_STR_BEG "Range-based for: %s, %s\n", DEBUG_PRINT_ARGS_BEG, m_keyvar.c_str(),
                       m_valuevar.c_str());
            else
                printf(DEBUG_PRINT_FORMAT_STR_BEG "Range-based for: %s\n", DEBUG_PRINT_ARGS_BEG, m_valuevar.c_str());
            ++indentLevel;
            m_rangeexpr->debugPrint(indentLevel, "RangeExpression: ");
            m_body->debugPrint(indentLevel, "Body: ");
        }

        void RangeBasedForLoop::execute(ExecutionContext& ctx) const
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
                const auto& rangeStr = rangeval.getString();
                count = rangeStr.length();
                for(i = 0; i < count; ++i)
                {
                    if(usekey)
                        assign(LValue{ ObjectMemberLValue{ &innermostctx, m_keyvar } }, Value{ (double)i });
                    ch = rangeStr[i];
                    assign(LValue{ ObjectMemberLValue{ &innermostctx, m_valuevar } }, Value{ std::string{ &ch, &ch + 1 } });
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
                for(const auto& [key, value] : rangeval.getObject()->m_items)
                {
                    if(usekey)
                        assign(LValue{ ObjectMemberLValue{ &innermostctx, m_keyvar } }, Value{ std::string{ key } });
                    assign(LValue{ ObjectMemberLValue{ &innermostctx, m_valuevar } }, Value{ value });
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
                arr = rangeval.getArray();
                for(i = 0, count = arr->m_items.size(); i < count; ++i)
                {
                    if(usekey)
                        assign(LValue{ ObjectMemberLValue{ &innermostctx, m_keyvar } }, Value{ (double)i });
                    assign(LValue{ ObjectMemberLValue{ &innermostctx, m_valuevar } }, Value{ arr->m_items[i] });
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
                throw ExecutionError(getPlace(), "range-based loop can not be used with this object");

            if(usekey)
                assign(LValue{ ObjectMemberLValue{ &innermostctx, m_keyvar } }, Value{});
            assign(LValue{ ObjectMemberLValue{ &innermostctx, m_valuevar } }, Value{});
        }

        void LoopBreakStatement::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            static const char* LOOP_BREAK_TYPE_NAMES[] = { "Break", "Continue" };
            printf(DEBUG_PRINT_FORMAT_STR_BEG "%s\n", DEBUG_PRINT_ARGS_BEG, LOOP_BREAK_TYPE_NAMES[(size_t)m_type]);
        }

        void LoopBreakStatement::execute(ExecutionContext& ctx) const
        {
            (void)ctx;
            switch(m_type)
            {
                case LoopBreakStatement::Type::Break:
                    throw BreakException{};
                case LoopBreakStatement::Type::Continue:
                    throw ContinueException{};
                default:
                    assert(0);
            }
        }

        void ReturnStatement::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "return\n", DEBUG_PRINT_ARGS_BEG);
            if(m_retvalue)
                m_retvalue->debugPrint(indentLevel + 1, "ReturnedValue: ");
        }

        void ReturnStatement::execute(ExecutionContext& ctx) const
        {
            if(m_retvalue)
                throw ReturnException{ getPlace(), m_retvalue->evaluate(ctx, nullptr) };
            else
                throw ReturnException{ getPlace(), Value{} };
        }

        void Block::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Block\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            for(const auto& stmtPtr : m_statements)
                stmtPtr->debugPrint(indentLevel, std::string_view{});
        }

        void Block::execute(ExecutionContext& ctx) const
        {
            for(const auto& stmtPtr : m_statements)
                stmtPtr->execute(ctx);
        }

        void SwitchStatement::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            size_t i;
            printf(DEBUG_PRINT_FORMAT_STR_BEG "switch\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            m_cond->debugPrint(indentLevel, "Condition: ");
            for(i = 0; i < m_itemvals.size(); ++i)
            {
                if(m_itemvals[i])
                    m_itemvals[i]->debugPrint(indentLevel, "ItemValue: ");
                else
                    printf(DEBUG_PRINT_FORMAT_STR_BEG "Default\n", DEBUG_PRINT_ARGS_BEG);
                if(m_itemblocks[i])
                    m_itemblocks[i]->debugPrint(indentLevel, "ItemBlock: ");
                else
                    printf(DEBUG_PRINT_FORMAT_STR_BEG "(Empty block)\n", DEBUG_PRINT_ARGS_BEG);
            }
        }

        void SwitchStatement::execute(ExecutionContext& ctx) const
        {
            Value condVal;
            size_t itemidx;
            size_t defitemidx;
            size_t itemcnt;
            defitemidx = SIZE_MAX;
            condVal = m_cond->evaluate(ctx, nullptr);

            itemcnt = m_itemvals.size();

            for(itemidx = 0; itemidx < itemcnt; ++itemidx)
            {
                if(m_itemvals[itemidx])
                {
                    if(m_itemvals[itemidx]->m_val.isEqual(condVal))
                        break;
                }
                else
                    defitemidx = itemidx;
            }
            if(itemidx == itemcnt && defitemidx != SIZE_MAX)
                itemidx = defitemidx;
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
        }

        void ThrowStatement::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "throw\n", DEBUG_PRINT_ARGS_BEG);
            m_thrownexpr->debugPrint(indentLevel + 1, "ThrownExpression: ");
        }

        void ThrowStatement::execute(ExecutionContext& ctx) const
        {
            throw m_thrownexpr->evaluate(ctx, nullptr);
        }

        void TryStatement::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "try\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            m_tryblock->debugPrint(indentLevel, "TryBlock: ");
            if(m_catchblock)
                m_catchblock->debugPrint(indentLevel, "CatchBlock: ");
            if(m_finallyblock)
                m_finallyblock->debugPrint(indentLevel, "FinallyBlock: ");
        }

        void TryStatement::execute(ExecutionContext& ctx) const
        {
            // Careful with this function! It contains logic that was difficult to get right.
            try
            {
                m_tryblock->execute(ctx);
            }
            catch(Value& val)
            {
                if(m_catchblock)
                {
                    auto& innermostctx = ctx.getInnermostScope();
                    assign(LValue{ ObjectMemberLValue{ &innermostctx, m_exvarname } }, std::move(val));
                    m_catchblock->execute(ctx);
                    assign(LValue{ ObjectMemberLValue{ &innermostctx, m_exvarname } }, Value{});
                    if(m_finallyblock)
                        m_finallyblock->execute(ctx);
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
                    catch(const ExecutionError&)
                    {
                        throw val;
                    }
                    throw val;
                }
                return;
            }
            catch(const ExecutionError& err)
            {
                if(m_catchblock)
                {
                    auto& innermostctx = ctx.getInnermostScope();
                    assign(LValue{ ObjectMemberLValue{ &innermostctx, m_exvarname } },
                           Value{ ConvertExecutionErrorToObject(err) });
                    m_catchblock->execute(ctx);
                    assign(LValue{ ObjectMemberLValue{ &innermostctx, m_exvarname } }, Value{});
                    if(m_finallyblock)
                        m_finallyblock->execute(ctx);
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
                    catch(const ExecutionError&)
                    {
                        throw err;
                    }
                    throw err;
                }
                return;
            }
            catch(const BreakException&)
            {
                if(m_finallyblock)
                    m_finallyblock->execute(ctx);
                throw;
            }
            catch(const ContinueException&)
            {
                if(m_finallyblock)
                    m_finallyblock->execute(ctx);
                throw;
            }
            catch(ReturnException&)
            {
                if(m_finallyblock)
                    m_finallyblock->execute(ctx);
                throw;
            }
            catch(const ParsingError&)
            {
                assert(0 && "ParsingError not expected during execution.");
            }
            if(m_finallyblock)
                m_finallyblock->execute(ctx);
        }

        void Script::execute(ExecutionContext& ctx) const
        {
            try
            {
                Block::execute(ctx);
            }
            catch(BreakException)
            {
                throw ExecutionError{ getPlace(), ERROR_MESSAGE_BREAK_WITHOUT_LOOP };
            }
            catch(ContinueException)
            {
                throw ExecutionError{ getPlace(), ERROR_MESSAGE_CONTINUE_WITHOUT_LOOP };
            }
        }

        void ConstantValue::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            switch(m_val.type())
            {
                case Value::Type::Null:
                    printf(DEBUG_PRINT_FORMAT_STR_BEG "Constant null\n", DEBUG_PRINT_ARGS_BEG);
                    break;
                case Value::Type::Number:
                    printf(DEBUG_PRINT_FORMAT_STR_BEG "Constant number: %g\n", DEBUG_PRINT_ARGS_BEG, m_val.getNumber());
                    break;
                case Value::Type::String:
                    printf(DEBUG_PRINT_FORMAT_STR_BEG "Constant string: %s\n", DEBUG_PRINT_ARGS_BEG, m_val.getString().c_str());
                    break;
                default:
                    assert(0 && "ConstantValue should not be used with this type.");
            }
        }

        void Identifier::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            static const char* PREFIX[] = { "", "local.", "global." };
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Identifier: %s%s\n", DEBUG_PRINT_ARGS_BEG, PREFIX[(size_t)m_scope], m_ident.c_str());
        }

        Value Identifier::evaluate(ExecutionContext& ctx, ThisType* othis) const
        {
            size_t i;
            size_t count;
            Value* val;
            const std::shared_ptr<Object>* thisObj;
            MINSL_EXECUTION_CHECK(m_scope != Identifier::Scope::Local || ctx.isLocal(), getPlace(), ERROR_MESSAGE_NO_LOCAL_SCOPE);

            if(ctx.isLocal())
            {
                // Local variable
                if((m_scope == Identifier::Scope::None || m_scope == Identifier::Scope::Local))
                    if(val = ctx.getCurrentLocalScope()->tryGet(m_ident); val)
                        return *val;
                // This
                if(m_scope == Identifier::Scope::None)
                {
                    thisObj = std::get_if<std::shared_ptr<Object>>(&ctx.getThis());
                    if(thisObj)
                    {
                        val = (*thisObj)->tryGet(m_ident);
                        if(val)
                        {
                            if(othis)
                                *othis = ThisType{ *thisObj };
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
                    return *val;
                // Type
                for(i = 0, count = (size_t)Value::Type::Count; i < count; ++i)
                {
                    if(m_ident == VALUE_TYPE_NAMES[i])
                        return Value{ (Value::Type)i };
                }
                // System function
                for(i = 0, count = (size_t)SystemFunction::FuncCount; i < count; ++i)
                {
                    if(m_ident == SYSTEM_FUNCTION_NAMES[i])
                    {
                        return Value{ (SystemFunction)i };
                    }
                }
            }

            // Not found - null
            return {};
        }

        LValue Identifier::getLeftValue(ExecutionContext& ctx) const
        {
            bool islocal;
            const std::shared_ptr<Object>* thisObj;
            islocal = ctx.isLocal();
            MINSL_EXECUTION_CHECK(m_scope != Identifier::Scope::Local || islocal, getPlace(), ERROR_MESSAGE_NO_LOCAL_SCOPE);

            if(islocal)
            {
                // Local variable
                if((m_scope == Identifier::Scope::None || m_scope == Identifier::Scope::Local) && ctx.getCurrentLocalScope()->hasKey(m_ident))
                    return LValue{ ObjectMemberLValue{ &*ctx.getCurrentLocalScope(), m_ident } };
                // This
                if(m_scope == Identifier::Scope::None)
                {
                    if((thisObj = std::get_if<std::shared_ptr<Object>>(&ctx.getThis()));
                       thisObj && (*thisObj)->hasKey(m_ident))
                        return LValue{ ObjectMemberLValue{ (*thisObj).get(), m_ident } };
                }
            }

            // Global variable
            if((m_scope == Identifier::Scope::None || m_scope == Identifier::Scope::Global) && ctx.m_globalscope.hasKey(m_ident))
                return LValue{ ObjectMemberLValue{ &ctx.m_globalscope, m_ident } };

            // Not found: return reference to smallest scope.
            if((m_scope == Identifier::Scope::None || m_scope == Identifier::Scope::Local) && islocal)
                return LValue{ ObjectMemberLValue{ ctx.getCurrentLocalScope(), m_ident } };
            return LValue{ ObjectMemberLValue{ &ctx.m_globalscope, m_ident } };
        }

        void ThisExpression::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "This\n", DEBUG_PRINT_ARGS_BEG);
        }

        Value ThisExpression::evaluate(ExecutionContext& ctx, ThisType* othis) const
        {
            (void)othis;
            MINSL_EXECUTION_CHECK(ctx.isLocal() && std::get_if<std::shared_ptr<Object>>(&ctx.getThis()), getPlace(), ERROR_MESSAGE_NO_THIS);
            return Value{ std::shared_ptr<Object>{ *std::get_if<std::shared_ptr<Object>>(&ctx.getThis()) } };
        }

        void UnaryOperator::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            static const char* UNARY_OPERATOR_TYPE_NAMES[]
            = { "Preincrementation", "Predecrementation", "Postincrementation", "Postdecrementation", "Plus", "Minus",
                "Logical NOT",       "Bitwise NOT" };

            printf(DEBUG_PRINT_FORMAT_STR_BEG "UnaryOperator %s\n", DEBUG_PRINT_ARGS_BEG,
                   UNARY_OPERATOR_TYPE_NAMES[(uint32_t)m_type]);
            ++indentLevel;
            m_operand->debugPrint(indentLevel, "Operand: ");
        }

        Value UnaryOperator::evaluate(ExecutionContext& ctx, ThisType* othis) const
        {
            Value* pval;
            Value val;
            Value result;
            (void)othis;
            // Those require l-value.
            if(m_type == UnaryOperator::Type::Preincrementation || m_type == UnaryOperator::Type::Predecrementation
               || m_type == UnaryOperator::Type::Postincrementation || m_type == UnaryOperator::Type::Postdecrementation)
            {
                pval = m_operand->getLeftValue(ctx).getValueRef(getPlace());
                MINSL_EXECUTION_CHECK(pval->type() == Value::Type::Number, getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                switch(m_type)
                {
                    case UnaryOperator::Type::Preincrementation:
                        pval->setNumberValue(pval->getNumber() + 1.0);
                        return *pval;
                    case UnaryOperator::Type::Predecrementation:
                        pval->setNumberValue(pval->getNumber() - 1.0);
                        return *pval;
                    case UnaryOperator::Type::Postincrementation:
                    {
                        result = *pval;
                        pval->setNumberValue(pval->getNumber() + 1.0);
                        return result;
                    }
                    case UnaryOperator::Type::Postdecrementation:
                    {
                        result = *pval;
                        pval->setNumberValue(pval->getNumber() - 1.0);
                        return result;
                    }
                    default:
                        assert(0);
                        return {};
                }
            }
            // Those use r-value.
            else if(m_type == UnaryOperator::Type::Plus || m_type == UnaryOperator::Type::Minus
                    || m_type == UnaryOperator::Type::LogicalNot || m_type == UnaryOperator::Type::BitwiseNot)
            {
                val = m_operand->evaluate(ctx, nullptr);
                MINSL_EXECUTION_CHECK(val.isNumber(), getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                switch(m_type)
                {
                    case UnaryOperator::Type::Plus:
                        return val;
                    case UnaryOperator::Type::Minus:
                        return Value{ -val.getNumber() };
                    case UnaryOperator::Type::LogicalNot:
                        return Value{ val.isTrue() ? 0.0 : 1.0 };
                    case UnaryOperator::Type::BitwiseNot:
                        return BitwiseNot(std::move(val));
                    default:
                        assert(0);
                        return {};
                }
            }
            assert(0);
            return {};
        }

        LValue UnaryOperator::getLeftValue(ExecutionContext& ctx) const
        {
            LValue lval;
            Value* val;
            const ObjectMemberLValue* leftmemberval;
            if(m_type == UnaryOperator::Type::Preincrementation || m_type == UnaryOperator::Type::Predecrementation)
            {
                lval = m_operand->getLeftValue(ctx);
                leftmemberval = std::get_if<ObjectMemberLValue>(&lval);
                MINSL_EXECUTION_CHECK(leftmemberval, getPlace(), "lvalue required");
                val = leftmemberval->objectval->tryGet(leftmemberval->keyval);
                MINSL_EXECUTION_CHECK(val != nullptr, getPlace(), ERROR_MESSAGE_VARIABLE_DOESNT_EXIST);
                MINSL_EXECUTION_CHECK(val->type() == Value::Type::Number, getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                switch(m_type)
                {
                    case UnaryOperator::Type::Preincrementation:
                        val->setNumberValue(val->getNumber() + 1.0);
                        return lval;
                    case UnaryOperator::Type::Predecrementation:
                        val->setNumberValue(val->getNumber() - 1.0);
                        return lval;
                    default:
                        assert(0);
                }
            }
            throw ExecutionError(getPlace(), "lvalue required");
        }

        void MemberAccessOperator::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "MemberAccessOperator Member=%s\n", DEBUG_PRINT_ARGS_BEG, m_membername.c_str());
            m_operand->debugPrint(indentLevel + 1, "Operand: ");
        }

        /**!
        // TODO:
        // turn this into a table lookup.
        */
        Value MemberAccessOperator::evaluate(ExecutionContext& ctx, ThisType* othis) const
        {
            Value vobj;
            const Value* memberval;
            StandardObjectPropertyFunc prop;
            StandardObjectMemberFunc meth;
            vobj = m_operand->evaluate(ctx, nullptr);
            if(vobj.isObject())
            {
                memberval = vobj.getObject()->tryGet(m_membername);
                if(memberval)
                {
                    if(othis)
                        *othis = ThisType{ vobj.getObjectRef() };
                    return *memberval;
                }
                if(get_property(stdobjproperties_object, m_membername, prop))
                {
                    return prop.func(ctx, getPlace(), std::move(vobj));
                }
                else if(get_method(stdobjmethods_object, m_membername, meth))
                {
                    return Value{ meth.func };
                }
                return {};
            }
            if(vobj.isString())
            {
                if(get_property(stdobjproperties_string, m_membername, prop))
                {
                    return prop.func(ctx, getPlace(), std::move(vobj));
                }
                else if(get_method(stdobjmethods_string, m_membername, meth))
                {
                    return Value{ meth.func };
                }                
                throw ExecutionError(getPlace(), "no such String member");
            }
            if(vobj.isArray())
            {
                if(othis)
                {
                    *othis = ThisType{ vobj.getArrayRef() };
                }
                if(get_property(stdobjproperties_array, m_membername, prop))
                {
                    return prop.func(ctx, getPlace(), std::move(vobj));
                }
                else if(get_method(stdobjmethods_array, m_membername, meth))
                {
                    return Value{ meth.func };
                }
                throw ExecutionError(getPlace(), "no such Array member");
            }
            throw ExecutionError(getPlace(), "member access in something not an object");
        }

        LValue MemberAccessOperator::getLeftValue(ExecutionContext& ctx) const
        {
            Value vobj;
            vobj = m_operand->evaluate(ctx, nullptr);
            MINSL_EXECUTION_CHECK(vobj.isObject(), getPlace(), ERROR_MESSAGE_EXPECTED_OBJECT);
            return LValue{ ObjectMemberLValue{ vobj.getObject(), m_membername } };
        }

        Value UnaryOperator::BitwiseNot(Value&& operand) const
        {
            int64_t operval;
            int64_t resval;

            operval = (int64_t)operand.getNumber();
            resval = ~operval;
            return Value{ (double)resval };
        }

        void BinaryOperator::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            static const char* BINARY_OPERATOR_TYPE_NAMES[] = {
                "Mul",
                "Div",
                "Mod",
                "Add",
                "Sub",
                "Shift left",
                "Shift right",
                "Assignment",
                "AssignmentAdd",
                "AssignmentSub",
                "AssignmentMul",
                "AssignmentDiv",
                "AssignmentMod",
                "AssignmentShiftLeft",
                "AssignmentShiftRight",
                "AssignmentBitwiseAnd",
                "AssignmentBitwiseXor",
                "AssignmentBitwiseOr",
                "Less",
                "Greater",
                "LessEqual",
                "GreaterEqual",
                "Equal",
                "NotEqual",
                "BitwiseAnd",
                "BitwiseXor",
                "BitwiseOr",
                "LogicalAnd",
                "LogicalOr",
                "Comma",
                "Indexing",
            };

            printf(DEBUG_PRINT_FORMAT_STR_BEG "BinaryOperator %s\n", DEBUG_PRINT_ARGS_BEG,
                   BINARY_OPERATOR_TYPE_NAMES[(uint32_t)m_type]);
            ++indentLevel;
            m_oplist[0]->debugPrint(indentLevel, "LeftOperand: ");
            m_oplist[1]->debugPrint(indentLevel, "RightOperand: ");
        }

        Value BinaryOperator::evaluate(ExecutionContext& ctx, ThisType* othis) const
        {
            size_t index;
            bool result;
            Value left;
            Value right;
            Value::Type typleft;
            Value::Type typright;

            // This operator is special, discards result of left operand.
            if(m_type == BinaryOperator::Type::Comma)
            {
                m_oplist[0]->execute(ctx);
                return m_oplist[1]->evaluate(ctx, othis);
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
                    // Getting these explicitly so the order of thier evaluation is defined, unlike in C++ function call arguments.
                    Value rhsVal = m_oplist[1]->evaluate(ctx, nullptr);
                    LValue lhsLval = m_oplist[0]->getLeftValue(ctx);
                    return Assignment(std::move(lhsLval), std::move(rhsVal));
                }
                default:
                    break;
            }

            // Remaining operators use r-values.
            left = m_oplist[0]->evaluate(ctx, nullptr);

            // Logical operators with short circuit for right hand side operand.
            if(m_type == BinaryOperator::Type::LogicalAnd)
            {
                if(!left.isTrue())
                    return left;
                return m_oplist[1]->evaluate(ctx, nullptr);
            }
            if(m_type == BinaryOperator::Type::LogicalOr)
            {
                if(left.isTrue())
                    return left;
                return m_oplist[1]->evaluate(ctx, nullptr);
            }

            // Remaining operators use both operands as r-values.
            right = m_oplist[1]->evaluate(ctx, nullptr);
            typleft = left.type();
            typright = right.type();
            // These ones support various types.
            if(m_type == BinaryOperator::Type::Add)
            {
                if(typleft == Value::Type::Number && typright == Value::Type::Number)
                    return Value{ left.getNumber() + right.getNumber() };
                if(typleft == Value::Type::String && typright == Value::Type::String)
                    return Value{ left.getString() + right.getString() };
                throw ExecutionError(getPlace(), ERROR_MESSAGE_INCOMPATIBLE_TYPES);
            }
            if(m_type == BinaryOperator::Type::Equal)
            {
                return Value{ left.isEqual(right) ? 1.0 : 0.0 };
            }
            if(m_type == BinaryOperator::Type::NotEqual)
            {
                return Value{ !left.isEqual(right) ? 1.0 : 0.0 };
            }
            if(m_type == BinaryOperator::Type::Less || m_type == BinaryOperator::Type::LessEqual
               || m_type == BinaryOperator::Type::Greater || m_type == BinaryOperator::Type::GreaterEqual)
            {
                result = false;
                MINSL_EXECUTION_CHECK(typleft == typright, getPlace(), ERROR_MESSAGE_INCOMPATIBLE_TYPES);
                if(typleft == Value::Type::Number)
                {
                    switch(m_type)
                    {
                        case BinaryOperator::Type::Less:
                            result = left.getNumber() < right.getNumber();
                            break;
                        case BinaryOperator::Type::LessEqual:
                            result = left.getNumber() <= right.getNumber();
                            break;
                        case BinaryOperator::Type::Greater:
                            result = left.getNumber() > right.getNumber();
                            break;
                        case BinaryOperator::Type::GreaterEqual:
                            result = left.getNumber() >= right.getNumber();
                            break;
                        default:
                            assert(0);
                    }
                }
                else if(typleft == Value::Type::String)
                {
                    switch(m_type)
                    {
                        case BinaryOperator::Type::Less:
                            result = left.getString() < right.getString();
                            break;
                        case BinaryOperator::Type::LessEqual:
                            result = left.getString() <= right.getString();
                            break;
                        case BinaryOperator::Type::Greater:
                            result = left.getString() > right.getString();
                            break;
                        case BinaryOperator::Type::GreaterEqual:
                            result = left.getString() >= right.getString();
                            break;
                        default:
                            assert(0);
                    }
                }
                else
                    throw ExecutionError(getPlace(), "incompatible types for binary operation");
                return Value{ result ? 1.0 : 0.0 };
            }
            if(m_type == BinaryOperator::Type::Indexing)
            {
                if(typleft == Value::Type::String)
                {
                    MINSL_EXECUTION_CHECK(typright == Value::Type::Number, getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                    index = 0;
                    MINSL_EXECUTION_CHECK(NumberToIndex(index, right.getNumber()), getPlace(), "string index out of bounds");
                    MINSL_EXECUTION_CHECK(index < left.getString().length(), getPlace(), "string index out of bounds");
                    return Value{ std::string(1, left.getString()[index]) };
                }
                if(typleft == Value::Type::Object)
                {
                    MINSL_EXECUTION_CHECK(typright == Value::Type::String, getPlace(), ERROR_MESSAGE_EXPECTED_STRING);
                    auto val = left.getObject()->tryGet(right.getString());
                    if(val)
                    {
                        if(othis)
                        {
                            *othis = ThisType{ left.getObjectRef() };
                        }
                        return *val;
                    }
                    return {};
                }
                if(typleft == Value::Type::Array)
                {
                    MINSL_EXECUTION_CHECK(typright == Value::Type::Number, getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                    MINSL_EXECUTION_CHECK(NumberToIndex(index, right.getNumber()) && index < left.getArray()->m_items.size(),
                                          getPlace(), "array index out of bounds");
                    return left.getArray()->m_items[index];
                }
                throw ExecutionError(getPlace(), "cannot index this type");
            }

            // Remaining operators require numbers.
            CheckNumberOperand(m_oplist[0].get(), left);
            CheckNumberOperand(m_oplist[1].get(), right);

            switch(m_type)
            {
                case BinaryOperator::Type::Mul:
                    return Value{ left.getNumber() * right.getNumber() };
                case BinaryOperator::Type::Div:
                    return Value{ left.getNumber() / right.getNumber() };
                case BinaryOperator::Type::Mod:
                    return Value{ fmod(left.getNumber(), right.getNumber()) };
                case BinaryOperator::Type::Sub:
                    return Value{ left.getNumber() - right.getNumber() };
                case BinaryOperator::Type::ShiftLeft:
                    return ShiftLeft(std::move(left), std::move(right));
                case BinaryOperator::Type::ShiftRight:
                    return ShiftRight(std::move(left), std::move(right));
                case BinaryOperator::Type::BitwiseAnd:
                    return Value{ (double)((int64_t)left.getNumber() & (int64_t)right.getNumber()) };
                case BinaryOperator::Type::BitwiseXor:
                    return Value{ (double)((int64_t)left.getNumber() ^ (int64_t)right.getNumber()) };
                case BinaryOperator::Type::BitwiseOr:
                    return Value{ (double)((int64_t)left.getNumber() | (int64_t)right.getNumber()) };
                default:
                    break;
            }

            assert(0);
            return {};
        }

        LValue BinaryOperator::getLeftValue(ExecutionContext& ctx) const
        {
            size_t itemidx;
            size_t charidx;
            Value idxval;
            Value* leftref;
            if(m_type == BinaryOperator::Type::Indexing)
            {
                leftref = m_oplist[0]->getLeftValue(ctx).getValueRef(getPlace());
                idxval = m_oplist[1]->evaluate(ctx, nullptr);
                if(leftref->type() == Value::Type::String)
                {
                    MINSL_EXECUTION_CHECK(idxval.isNumber(), getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                    MINSL_EXECUTION_CHECK(NumberToIndex(charidx, idxval.getNumber()), getPlace(), "string index out of bounds");
                    return LValue{ StringCharacterLValue{ &leftref->getString(), charidx } };
                }
                if(leftref->type() == Value::Type::Object)
                {
                    MINSL_EXECUTION_CHECK(idxval.isString(), getPlace(), ERROR_MESSAGE_EXPECTED_STRING);
                    return LValue{ ObjectMemberLValue{ leftref->getObject(), idxval.getString() } };
                }
                if(leftref->type() == Value::Type::Array)
                {
                    MINSL_EXECUTION_CHECK(idxval.isNumber(), getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                    MINSL_EXECUTION_CHECK(NumberToIndex(itemidx, idxval.getNumber()), getPlace(), "array index out of bounds");
                    return LValue{ ArrayItemLValue{ leftref->getArray(), itemidx } };
                }
            }
            return Operator::getLeftValue(ctx);
        }

        Value BinaryOperator::ShiftLeft(const Value& lhs, const Value& rhs) const
        {
            const int64_t leftnum = (int64_t)lhs.getNumber();
            const int64_t rightnum = (int64_t)rhs.getNumber();
            const int64_t resval = leftnum << rightnum;
            return Value{ (double)resval };
        }

        Value BinaryOperator::ShiftRight(const Value& lhs, const Value& rhs) const
        {
            const int64_t leftnum = (int64_t)lhs.getNumber();
            const int64_t rightnum = (int64_t)rhs.getNumber();
            const int64_t resval = leftnum >> rightnum;
            return Value{ (double)resval };
        }

        Value BinaryOperator::Assignment(LValue&& lhs, Value&& rhs) const
        {
            Value* leftvalptr;
            // This one is able to create new value.
            if(m_type == BinaryOperator::Type::Assignment)
            {
                assign(lhs, Value{ rhs });
                return rhs;
            }
            // Others require existing value.
            leftvalptr = lhs.getValueRef(getPlace());
            if(m_type == BinaryOperator::Type::AssignmentAdd)
            {
                if(leftvalptr->type() == Value::Type::Number && rhs.isNumber())
                {
                    leftvalptr->setNumberValue(leftvalptr->getNumber() + rhs.getNumber());
                }
                else if(leftvalptr->type() == Value::Type::String && rhs.isString())
                {
                    leftvalptr->getString() += rhs.getString();
                }
                else
                {
                    throw ExecutionError(getPlace(), ERROR_MESSAGE_INCOMPATIBLE_TYPES);
                }
                return *leftvalptr;
            }

            // Remaining ones work on numbers only.
            MINSL_EXECUTION_CHECK(leftvalptr->type() == Value::Type::Number, getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
            MINSL_EXECUTION_CHECK(rhs.isNumber(), getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
            switch(m_type)
            {
                case BinaryOperator::Type::AssignmentSub:
                    {
                        leftvalptr->setNumberValue(leftvalptr->getNumber() - rhs.getNumber());
                    }
                    break;
                case BinaryOperator::Type::AssignmentMul:
                    {
                        leftvalptr->setNumberValue(leftvalptr->getNumber() * rhs.getNumber());
                    }
                    break;
                case BinaryOperator::Type::AssignmentDiv:
                    {
                        leftvalptr->setNumberValue(leftvalptr->getNumber() / rhs.getNumber());
                    }
                    break;
                case BinaryOperator::Type::AssignmentMod:
                    {
                        leftvalptr->setNumberValue(fmod(leftvalptr->getNumber(), rhs.getNumber()));
                    }
                    break;
                case BinaryOperator::Type::AssignmentShiftLeft:
                    {
                        *leftvalptr = ShiftLeft(*leftvalptr, rhs);
                    }
                    break;
                case BinaryOperator::Type::AssignmentShiftRight:
                    {
                        *leftvalptr = ShiftRight(*leftvalptr, rhs);
                    }
                    break;
                case BinaryOperator::Type::AssignmentBitwiseAnd:
                    {
                        leftvalptr->setNumberValue((double)((int64_t)leftvalptr->getNumber() & (int64_t)rhs.getNumber()));
                    }
                    break;
                case BinaryOperator::Type::AssignmentBitwiseXor:
                    {
                        leftvalptr->setNumberValue((double)((int64_t)leftvalptr->getNumber() ^ (int64_t)rhs.getNumber()));
                    }
                    break;
                case BinaryOperator::Type::AssignmentBitwiseOr:
                    {
                        leftvalptr->setNumberValue((double)((int64_t)leftvalptr->getNumber() | (int64_t)rhs.getNumber()));
                    }
                    break;
                default:
                    assert(0);
            }

            return *leftvalptr;
        }

        void TernaryOperator::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "TernaryOperator\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            m_oplist[0]->debugPrint(indentLevel, "ConditionExpression: ");
            m_oplist[1]->debugPrint(indentLevel, "TrueExpression: ");
            m_oplist[2]->debugPrint(indentLevel, "FalseExpression: ");
        }

        Value TernaryOperator::evaluate(ExecutionContext& ctx, ThisType* othis) const
        {
            return m_oplist[0]->evaluate(ctx, nullptr).isTrue() ? m_oplist[1]->evaluate(ctx, othis) :
                                                                  m_oplist[2]->evaluate(ctx, othis);
        }

        void CallOperator::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            size_t i;
            size_t count;
            printf(DEBUG_PRINT_FORMAT_STR_BEG "CallOperator\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            m_oplist[0]->debugPrint(indentLevel, "Callee: ");
            for(i = 1, count = m_oplist.size(); i < count; ++i)
            {
                m_oplist[i]->debugPrint(indentLevel, "Argument: ");
            }
        }

        Value CallOperator::evaluate(ExecutionContext& ctx, ThisType* othis) const
        {
            (void)othis;
            size_t i;
            size_t argcnt;
            size_t argidx;
            ThisType th;
            th = ThisType{};
            Value callee;
            callee = m_oplist[0]->evaluate(ctx, &th);
            argcnt = m_oplist.size() - 1;
            std::vector<Value> arguments(argcnt);
            for(i = 0; i < argcnt; ++i)
            {
                arguments[i] = m_oplist[i + 1]->evaluate(ctx, nullptr);
            }
            // Calling an object: Call its function under '' key.
            if(callee.isObject())
            {
                auto objcallee = callee.getObjectRef();
                auto defval = objcallee->tryGet(std::string{}); 
                if(defval && defval->type() == Value::Type::Function)
                {
                    callee = *defval;
                    th = ThisType{ std::move(objcallee) };
                }
            }

            if(callee.type() == Value::Type::Function)
            {
                auto funcDef = callee.getFunction();
                MINSL_EXECUTION_CHECK(argcnt == funcDef->m_paramlist.size(), getPlace(), "inexact number of arguments");
                Object ourlocalscope;
                // Setup parameters
                for(argidx = 0; argidx != argcnt; ++argidx)
                {
                    ourlocalscope.entry(funcDef->m_paramlist[argidx]) = std::move(arguments[argidx]);
                }
                ExecutionContext::LocalScopePush ourlocalcontext{ ctx, &ourlocalscope, std::move(th), getPlace() };
                try
                {
                    callee.getFunction()->m_body.execute(ctx);
                }
                catch(ReturnException& returnEx)
                {
                    return std::move(returnEx.thrownvalue);
                }
                catch(BreakException)
                {
                    throw ExecutionError(getPlace(), ERROR_MESSAGE_BREAK_WITHOUT_LOOP);
                }
                catch(ContinueException)
                {
                    throw ExecutionError(getPlace(), ERROR_MESSAGE_CONTINUE_WITHOUT_LOOP);
                }
                return {};
            }
            if(callee.type() == Value::Type::HostFunction)
            {
                return callee.getHostFunction()(ctx.m_env.getOwner(), getPlace(), std::move(arguments));
            }
            if(callee.type() == Value::Type::SystemFunction)
            {
                switch(callee.getSysFunction())
                {
                    case SystemFunction::StringResizeFunc:
                        {
                        return Builtins::memberfn_string_resize(ctx, getPlace(), th, std::move(arguments));
                        }
                    case SystemFunction::ArrayAddFunc:
                        {
                            return Builtins::memberfn_array_add(ctx, getPlace(), th, std::move(arguments));
                        }
                    case SystemFunction::ArrayInsertFunc:
                        {
                            return Builtins::memberfn_array_insert(ctx, getPlace(), th, std::move(arguments));
                        }
                    case SystemFunction::ArrayRemoveFunc:
                        {
                            return Builtins::memberfn_array_remove(ctx, getPlace(), th, std::move(arguments));
                        }
                    default:
                        assert(0);
                        return {};
                }
            }
            if(callee.type() == Value::Type::Type)
            {
                switch(callee.getTypeValue())
                {
                    case Value::Type::Null:
                        {
                            return Builtins::ctor_null(ctx, getPlace(), std::move(arguments));
                        }
                        break;
                    case Value::Type::Number:
                        {
                            return Builtins::ctor_number(ctx, getPlace(), std::move(arguments));
                        }
                        break;
                    case Value::Type::String:
                        {
                            return Builtins::ctor_string(ctx, getPlace(), std::move(arguments));
                        }
                        break;
                    case Value::Type::Object:
                        {
                            return Builtins::ctor_object(ctx, getPlace(), std::move(arguments));
                        }
                        break;
                    case Value::Type::Array:
                        {
                            return Builtins::ctor_array(ctx, getPlace(), std::move(arguments));
                        }
                        break;
                    case Value::Type::Type:
                        {
                            return Builtins::ctor_type(ctx, getPlace(), std::move(arguments));
                        }
                        break;
                    case Value::Type::Function:
                    case Value::Type::SystemFunction:
                        {
                            return Builtins::ctor_function(ctx, getPlace(), std::move(arguments));
                        }
                        break;
/*
    using MemberMethodFunction = Value(AST::ExecutionContext&, const PlaceInCode&, const AST::ThisType&, std::vector<Value>&&);
    using MemberPropertyFunction = Value(AST::ExecutionContext&, const PlaceInCode&, Value&&);
*/
                    #if 1
                    case Value::Type::MemberMethod:
                        {
                            return callee.getMemberFunction()(ctx, getPlace(), th, std::move(arguments));
                        }
                        break;
                    case Value::Type::MemberProperty:
                        {
                            return callee.getPropertyFunction()(ctx, getPlace(), std::move(callee));
                        }
                        break;
                    #endif
                    default:
                        assert(0);
                        return {};
                }
            }
            if(callee.type() == Value::Type::MemberMethod)
            {
                return callee.getMemberFunction()(ctx, getPlace(), th, std::move(arguments));
            }
            #if 1
            if(callee.type() == Value::Type::MemberProperty)
            {
                return callee.getPropertyFunction()(ctx, getPlace(), std::move(callee));
            }
            #endif
            throw ExecutionError(getPlace(), "invalid function call");
        }

        void FunctionDefinition::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            size_t i;
            size_t count;
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Function(", DEBUG_PRINT_ARGS_BEG);
            if(!m_paramlist.empty())
            {
                printf("%s", m_paramlist[0].c_str());
                for(i = 1, count = m_paramlist.size(); i < count; ++i)
                {
                    printf(", %s", m_paramlist[i].c_str());
                }
            }
            printf(")\n");
            m_body.debugPrint(indentLevel + 1, "Body: ");
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

        void ObjectExpression::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Object\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            for(const auto& [name, value] : m_items)
            {
                value->debugPrint(indentLevel, name);
            }
        }

        Value ObjectExpression::evaluate(ExecutionContext& ctx, ThisType* othis) const
        {
            std::shared_ptr<Object> obj;
            (void)othis;
            if(m_baseexpr)
            {
                auto baseObj = m_baseexpr->evaluate(ctx, nullptr);
                if(baseObj.type() != Value::Type::Object)
                {
                    throw ExecutionError{ getPlace(), ERROR_MESSAGE_BASE_MUST_BE_OBJECT };
                }
                obj = CopyObject(*baseObj.getObject());
            }
            else
            {
                obj = std::make_shared<Object>();
            }
            for(const auto& [name, valueExpr] : m_items)
            {
                auto val = valueExpr->evaluate(ctx, nullptr);
                if(val.type() != Value::Type::Null)
                {
                    obj->entry(name) = std::move(val);
                }
                else if(m_baseexpr)
                {
                    obj->removeEntry(name);
                }
            }
            return Value{ std::move(obj) };
        }

        void ArrayExpression::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            size_t i;
            size_t count;
            std::string itemPrefix;
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Array\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            for(i = 0, count = m_items.size(); i < count; ++i)
            {
                itemPrefix = Format("%zu: ", i);
                m_items[i]->debugPrint(indentLevel, itemPrefix);
            }
        }

        Value ArrayExpression::evaluate(ExecutionContext& ctx, ThisType* othis) const
        {
            (void)othis;
            auto result = std::make_shared<Array>();
            for(const auto& item : m_items)
            {
                result->m_items.push_back(item->evaluate(ctx, nullptr));
            }
            return Value{ std::move(result) };
        }
    }
}


