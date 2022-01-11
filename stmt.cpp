
#include "msl.h"

#define DEBUG_PRINT_FORMAT_STR_BEG "(%u,%u) %s%.*s"
#define DEBUG_PRINT_ARGS_BEG \
    getPlace().textrow, getPlace().textcolumn, GetDebugPrintIndent(indentLevel), (int)prefix.length(), prefix.data()


namespace MSL
{
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
        if(const ObjectMemberLValue* objMemberLval = std::get_if<ObjectMemberLValue>(this))
        {
            if(Value* val = objMemberLval->objectval->tryGet(objMemberLval->keyval))
                return val;
            MINSL_EXECUTION_FAIL(place, ERROR_MESSAGE_OBJECT_MEMBER_DOESNT_EXIST);
        }
        if(const ArrayItemLValue* arrItemLval = std::get_if<ArrayItemLValue>(this))
        {
            MINSL_EXECUTION_CHECK(arrItemLval->indexval < arrItemLval->arrayval->m_items.size(), place, ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
            return &arrItemLval->arrayval->m_items[arrItemLval->indexval];
        }
        MINSL_EXECUTION_FAIL(place, "lvalue required");
    }

    Value LValue::getValue(const PlaceInCode& place) const
    {
        if(const ObjectMemberLValue* objMemberLval = std::get_if<ObjectMemberLValue>(this))
        {
            if(const Value* val = objMemberLval->objectval->tryGet(objMemberLval->keyval))
                return *val;
            MINSL_EXECUTION_FAIL(place, ERROR_MESSAGE_OBJECT_MEMBER_DOESNT_EXIST);
        }
        if(const StringCharacterLValue* strCharLval = std::get_if<StringCharacterLValue>(this))
        {
            MINSL_EXECUTION_CHECK(strCharLval->indexval < strCharLval->stringval->length(), place, ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
            const char ch = (*strCharLval->stringval)[strCharLval->indexval];
            return Value{ std::string{ &ch, &ch + 1 } };
        }
        if(const ArrayItemLValue* arrItemLval = std::get_if<ArrayItemLValue>(this))
        {
            MINSL_EXECUTION_CHECK(arrItemLval->indexval < arrItemLval->arrayval->m_items.size(), place, ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
            return Value{ arrItemLval->arrayval->m_items[arrItemLval->indexval] };
        }
        assert(0);
        return {};
    }


    namespace AST
    {

        void Statement::assign(const LValue& lhs, Value&& rhs) const
        {
            if(const ObjectMemberLValue* objMemberLhs = std::get_if<ObjectMemberLValue>(&lhs))
            {
                if(rhs.isNull())
                    objMemberLhs->objectval->removeEntry(objMemberLhs->keyval);
                else
                    objMemberLhs->objectval->entry(objMemberLhs->keyval) = std::move(rhs);
            }
            else if(const ArrayItemLValue* arrItemLhs = std::get_if<ArrayItemLValue>(&lhs))
            {
                MINSL_EXECUTION_CHECK(arrItemLhs->indexval < arrItemLhs->arrayval->m_items.size(), getPlace(), ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
                arrItemLhs->arrayval->m_items[arrItemLhs->indexval] = std::move(rhs);
            }
            else if(const StringCharacterLValue* strCharLhs = std::get_if<StringCharacterLValue>(&lhs))
            {
                MINSL_EXECUTION_CHECK(strCharLhs->indexval < strCharLhs->stringval->length(), getPlace(), ERROR_MESSAGE_INDEX_OUT_OF_BOUNDS);
                MINSL_EXECUTION_CHECK(rhs.isString(), getPlace(), ERROR_MESSAGE_EXPECTED_STRING);
                MINSL_EXECUTION_CHECK(rhs.getString().length() == 1, getPlace(), ERROR_MESSAGE_EXPECTED_SINGLE_CHARACTER_STRING);
                (*strCharLhs->stringval)[strCharLhs->indexval] = rhs.getString()[0];
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
            size_t i;
            size_t count;
            const Value rangeVal = m_rangeexpr->evaluate(ctx, nullptr);
            Object& innermostCtxObj = ctx.getInnermostScope();
            const bool useKey = !m_keyvar.empty();

            if(rangeVal.isString())
            {
                const std::string& rangeStr = rangeVal.getString();
                size_t count = rangeStr.length();
                for(i = 0; i < count; ++i)
                {
                    if(useKey)
                        assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, m_keyvar } }, Value{ (double)i });
                    const char ch = rangeStr[i];
                    assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, m_valuevar } }, Value{ std::string{ &ch, &ch + 1 } });
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
            else if(rangeVal.isObject())
            {
                for(const auto& [key, value] : rangeVal.getObject()->m_items)
                {
                    if(useKey)
                        assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, m_keyvar } }, Value{ std::string{ key } });
                    assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, m_valuevar } }, Value{ value });
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
            else if(rangeVal.isArray())
            {
                const Array* const arr = rangeVal.getArray();
                for(i = 0, count = arr->m_items.size(); i < count; ++i)
                {
                    if(useKey)
                        assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, m_keyvar } }, Value{ (double)i });
                    assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, m_valuevar } }, Value{ arr->m_items[i] });
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
                MINSL_EXECUTION_FAIL(getPlace(), "range-based loop can not be used with this object");

            if(useKey)
                assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, m_keyvar } }, Value{});
            assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, m_valuevar } }, Value{});
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
            printf(DEBUG_PRINT_FORMAT_STR_BEG "switch\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            m_cond->debugPrint(indentLevel, "Condition: ");
            for(size_t i = 0; i < m_itemvals.size(); ++i)
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
            const Value condVal = m_cond->evaluate(ctx, nullptr);
            size_t itemIndex, defaultItemIndex = SIZE_MAX;
            const size_t itemCount = m_itemvals.size();
            for(itemIndex = 0; itemIndex < itemCount; ++itemIndex)
            {
                if(m_itemvals[itemIndex])
                {
                    if(m_itemvals[itemIndex]->m_val.isEqual(condVal))
                        break;
                }
                else
                    defaultItemIndex = itemIndex;
            }
            if(itemIndex == itemCount && defaultItemIndex != SIZE_MAX)
                itemIndex = defaultItemIndex;
            if(itemIndex != itemCount)
            {
                for(; itemIndex < itemCount; ++itemIndex)
                {
                    try
                    {
                        m_itemblocks[itemIndex]->execute(ctx);
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
                    Object& innermostCtxObj = ctx.getInnermostScope();
                    assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, m_exvarname } }, std::move(val));
                    m_catchblock->execute(ctx);
                    assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, m_exvarname } }, Value{});
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
                    Object& innermostCtxObj = ctx.getInnermostScope();
                    assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, m_exvarname } },
                           Value{ ConvertExecutionErrorToObject(err) });
                    m_catchblock->execute(ctx);
                    assign(LValue{ ObjectMemberLValue{ &innermostCtxObj, m_exvarname } }, Value{});
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
            MINSL_EXECUTION_CHECK(m_scope != Identifier::Scope::Local || ctx.isLocal(), getPlace(), ERROR_MESSAGE_NO_LOCAL_SCOPE);

            if(ctx.isLocal())
            {
                // Local variable
                if((m_scope == Identifier::Scope::None || m_scope == Identifier::Scope::Local))
                    if(Value* val = ctx.getCurrentLocalScope()->tryGet(m_ident); val)
                        return *val;
                // This
                if(m_scope == Identifier::Scope::None)
                {
                    if(const std::shared_ptr<Object>* thisObj = std::get_if<std::shared_ptr<Object>>(&ctx.getThis()); thisObj)
                    {
                        if(Value* val = (*thisObj)->tryGet(m_ident); val)
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
                Value* val = ctx.m_globalscope.tryGet(m_ident);
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
            const bool isLocal = ctx.isLocal();
            MINSL_EXECUTION_CHECK(m_scope != Identifier::Scope::Local || isLocal, getPlace(), ERROR_MESSAGE_NO_LOCAL_SCOPE);

            if(isLocal)
            {
                // Local variable
                if((m_scope == Identifier::Scope::None || m_scope == Identifier::Scope::Local) && ctx.getCurrentLocalScope()->hasKey(m_ident))
                    return LValue{ ObjectMemberLValue{ &*ctx.getCurrentLocalScope(), m_ident } };
                // This
                if(m_scope == Identifier::Scope::None)
                {
                    if(const std::shared_ptr<Object>* thisObj = std::get_if<std::shared_ptr<Object>>(&ctx.getThis());
                       thisObj && (*thisObj)->hasKey(m_ident))
                        return LValue{ ObjectMemberLValue{ (*thisObj).get(), m_ident } };
                }
            }

            // Global variable
            if((m_scope == Identifier::Scope::None || m_scope == Identifier::Scope::Global) && ctx.m_globalscope.hasKey(m_ident))
                return LValue{ ObjectMemberLValue{ &ctx.m_globalscope, m_ident } };

            // Not found: return reference to smallest scope.
            if((m_scope == Identifier::Scope::None || m_scope == Identifier::Scope::Local) && isLocal)
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
            (void)othis;
            // Those require l-value.
            if(m_type == UnaryOperator::Type::Preincrementation || m_type == UnaryOperator::Type::Predecrementation
               || m_type == UnaryOperator::Type::Postincrementation || m_type == UnaryOperator::Type::Postdecrementation)
            {
                Value* val = m_operand->getLeftValue(ctx).getValueRef(getPlace());
                MINSL_EXECUTION_CHECK(val->type() == Value::Type::Number, getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                switch(m_type)
                {
                    case UnaryOperator::Type::Preincrementation:
                        val->setNumberValue(val->getNumber() + 1.0);
                        return *val;
                    case UnaryOperator::Type::Predecrementation:
                        val->setNumberValue(val->getNumber() - 1.0);
                        return *val;
                    case UnaryOperator::Type::Postincrementation:
                    {
                        Value result = *val;
                        val->setNumberValue(val->getNumber() + 1.0);
                        return result;
                    }
                    case UnaryOperator::Type::Postdecrementation:
                    {
                        Value result = *val;
                        val->setNumberValue(val->getNumber() - 1.0);
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
                Value val = m_operand->evaluate(ctx, nullptr);
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
            if(m_type == UnaryOperator::Type::Preincrementation || m_type == UnaryOperator::Type::Predecrementation)
            {
                LValue lval = m_operand->getLeftValue(ctx);
                const ObjectMemberLValue* objMemberLval = std::get_if<ObjectMemberLValue>(&lval);
                MINSL_EXECUTION_CHECK(objMemberLval, getPlace(), "lvalue required");
                Value* val = objMemberLval->objectval->tryGet(objMemberLval->keyval);
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
            MINSL_EXECUTION_FAIL(getPlace(), "lvalue required");
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
            const Value* memberVal;
            vobj = m_operand->evaluate(ctx, nullptr);
            if(vobj.isObject())
            {
                memberVal = vobj.getObject()->tryGet(m_membername);
                if(memberVal)
                {
                    if(othis)
                        *othis = ThisType{ vobj.getObjectRef() };
                    return *memberVal;
                }
                if(m_membername == "count")
                    return Builtins::memberfn_object_count(ctx, getPlace(), std::move(vobj));
                return {};
            }
            if(vobj.isString())
            {
                if((m_membername == "count") || (m_membername == "length") || (m_membername == "size"))
                {
                    return Builtins::memberfn_string_count(ctx, getPlace(), std::move(vobj));
                }
                else if(m_membername == "resize")
                {
                    return Value{ SystemFunction::StringResizeFunc };
                }
                MINSL_EXECUTION_FAIL(getPlace(), "no such String member");
            }
            if(vobj.isArray())
            {
                if(othis)
                    *othis = ThisType{ vobj.getArrayRef() };
                if(m_membername == "count")
                    return Builtins::memberfn_array_count(ctx, getPlace(), std::move(vobj));
                else if(m_membername == "add")
                    return Value{ SystemFunction::ArrayAddFunc };
                else if(m_membername == "insert")
                    return Value{ SystemFunction::ArrayInsertFunc };
                else if(m_membername == "remove")
                    return Value{ SystemFunction::ArrayRemoveFunc };
                MINSL_EXECUTION_FAIL(getPlace(), "no such Array member");
            }
            MINSL_EXECUTION_FAIL(getPlace(), "member access in something not an object");
        }

        LValue MemberAccessOperator::getLeftValue(ExecutionContext& ctx) const
        {
            Value vobj = m_operand->evaluate(ctx, nullptr);
            MINSL_EXECUTION_CHECK(vobj.isObject(), getPlace(), ERROR_MESSAGE_EXPECTED_OBJECT);
            return LValue{ ObjectMemberLValue{ vobj.getObject(), m_membername } };
        }

        Value UnaryOperator::BitwiseNot(Value&& operand) const
        {
            const int64_t operandInt = (int64_t)operand.getNumber();
            const int64_t resultInt = ~operandInt;
            return Value{ (double)resultInt };
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
            Value lhs = m_oplist[0]->evaluate(ctx, nullptr);

            // Logical operators with short circuit for right hand side operand.
            if(m_type == BinaryOperator::Type::LogicalAnd)
            {
                if(!lhs.isTrue())
                    return lhs;
                return m_oplist[1]->evaluate(ctx, nullptr);
            }
            if(m_type == BinaryOperator::Type::LogicalOr)
            {
                if(lhs.isTrue())
                    return lhs;
                return m_oplist[1]->evaluate(ctx, nullptr);
            }

            // Remaining operators use both operands as r-values.
            Value rhs = m_oplist[1]->evaluate(ctx, nullptr);

            const Value::Type lhsType = lhs.type();
            const Value::Type rhsType = rhs.type();

            // These ones support various types.
            if(m_type == BinaryOperator::Type::Add)
            {
                if(lhsType == Value::Type::Number && rhsType == Value::Type::Number)
                    return Value{ lhs.getNumber() + rhs.getNumber() };
                if(lhsType == Value::Type::String && rhsType == Value::Type::String)
                    return Value{ lhs.getString() + rhs.getString() };
                MINSL_EXECUTION_FAIL(getPlace(), ERROR_MESSAGE_INCOMPATIBLE_TYPES);
            }
            if(m_type == BinaryOperator::Type::Equal)
            {
                return Value{ lhs.isEqual(rhs) ? 1.0 : 0.0 };
            }
            if(m_type == BinaryOperator::Type::NotEqual)
            {
                return Value{ !lhs.isEqual(rhs) ? 1.0 : 0.0 };
            }
            if(m_type == BinaryOperator::Type::Less || m_type == BinaryOperator::Type::LessEqual
               || m_type == BinaryOperator::Type::Greater || m_type == BinaryOperator::Type::GreaterEqual)
            {
                bool result = false;
                MINSL_EXECUTION_CHECK(lhsType == rhsType, getPlace(), ERROR_MESSAGE_INCOMPATIBLE_TYPES);
                if(lhsType == Value::Type::Number)
                {
                    switch(m_type)
                    {
                        case BinaryOperator::Type::Less:
                            result = lhs.getNumber() < rhs.getNumber();
                            break;
                        case BinaryOperator::Type::LessEqual:
                            result = lhs.getNumber() <= rhs.getNumber();
                            break;
                        case BinaryOperator::Type::Greater:
                            result = lhs.getNumber() > rhs.getNumber();
                            break;
                        case BinaryOperator::Type::GreaterEqual:
                            result = lhs.getNumber() >= rhs.getNumber();
                            break;
                        default:
                            assert(0);
                    }
                }
                else if(lhsType == Value::Type::String)
                {
                    switch(m_type)
                    {
                        case BinaryOperator::Type::Less:
                            result = lhs.getString() < rhs.getString();
                            break;
                        case BinaryOperator::Type::LessEqual:
                            result = lhs.getString() <= rhs.getString();
                            break;
                        case BinaryOperator::Type::Greater:
                            result = lhs.getString() > rhs.getString();
                            break;
                        case BinaryOperator::Type::GreaterEqual:
                            result = lhs.getString() >= rhs.getString();
                            break;
                        default:
                            assert(0);
                    }
                }
                else
                    MINSL_EXECUTION_FAIL(getPlace(), "incompatible types for binary operation");
                return Value{ result ? 1.0 : 0.0 };
            }
            if(m_type == BinaryOperator::Type::Indexing)
            {
                if(lhsType == Value::Type::String)
                {
                    MINSL_EXECUTION_CHECK(rhsType == Value::Type::Number, getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                    size_t index = 0;
                    MINSL_EXECUTION_CHECK(NumberToIndex(index, rhs.getNumber()), getPlace(), "string index out of bounds");
                    MINSL_EXECUTION_CHECK(index < lhs.getString().length(), getPlace(), "string index out of bounds");
                    return Value{ std::string(1, lhs.getString()[index]) };
                }
                if(lhsType == Value::Type::Object)
                {
                    MINSL_EXECUTION_CHECK(rhsType == Value::Type::String, getPlace(), ERROR_MESSAGE_EXPECTED_STRING);
                    if(Value* val = lhs.getObject()->tryGet(rhs.getString()))
                    {
                        if(othis)
                            *othis = ThisType{ lhs.getObjectRef() };
                        return *val;
                    }
                    return {};
                }
                if(lhsType == Value::Type::Array)
                {
                    MINSL_EXECUTION_CHECK(rhsType == Value::Type::Number, getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                    size_t index;
                    MINSL_EXECUTION_CHECK(NumberToIndex(index, rhs.getNumber()) && index < lhs.getArray()->m_items.size(),
                                          getPlace(), "array index out of bounds");
                    return lhs.getArray()->m_items[index];
                }
                MINSL_EXECUTION_FAIL(getPlace(), "cannot index this type");
            }

            // Remaining operators require numbers.
            CheckNumberOperand(m_oplist[0].get(), lhs);
            CheckNumberOperand(m_oplist[1].get(), rhs);

            switch(m_type)
            {
                case BinaryOperator::Type::Mul:
                    return Value{ lhs.getNumber() * rhs.getNumber() };
                case BinaryOperator::Type::Div:
                    return Value{ lhs.getNumber() / rhs.getNumber() };
                case BinaryOperator::Type::Mod:
                    return Value{ fmod(lhs.getNumber(), rhs.getNumber()) };
                case BinaryOperator::Type::Sub:
                    return Value{ lhs.getNumber() - rhs.getNumber() };
                case BinaryOperator::Type::ShiftLeft:
                    return ShiftLeft(std::move(lhs), std::move(rhs));
                case BinaryOperator::Type::ShiftRight:
                    return ShiftRight(std::move(lhs), std::move(rhs));
                case BinaryOperator::Type::BitwiseAnd:
                    return Value{ (double)((int64_t)lhs.getNumber() & (int64_t)rhs.getNumber()) };
                case BinaryOperator::Type::BitwiseXor:
                    return Value{ (double)((int64_t)lhs.getNumber() ^ (int64_t)rhs.getNumber()) };
                case BinaryOperator::Type::BitwiseOr:
                    return Value{ (double)((int64_t)lhs.getNumber() | (int64_t)rhs.getNumber()) };
                default:
                    break;
            }

            assert(0);
            return {};
        }

        LValue BinaryOperator::getLeftValue(ExecutionContext& ctx) const
        {
            if(m_type == BinaryOperator::Type::Indexing)
            {
                Value* leftValRef = m_oplist[0]->getLeftValue(ctx).getValueRef(getPlace());
                const Value indexVal = m_oplist[1]->evaluate(ctx, nullptr);
                if(leftValRef->type() == Value::Type::String)
                {
                    MINSL_EXECUTION_CHECK(indexVal.isNumber(), getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                    size_t charIndex;
                    MINSL_EXECUTION_CHECK(NumberToIndex(charIndex, indexVal.getNumber()), getPlace(), "string index out of bounds");
                    return LValue{ StringCharacterLValue{ &leftValRef->getString(), charIndex } };
                }
                if(leftValRef->type() == Value::Type::Object)
                {
                    MINSL_EXECUTION_CHECK(indexVal.isString(), getPlace(), ERROR_MESSAGE_EXPECTED_STRING);
                    return LValue{ ObjectMemberLValue{ leftValRef->getObject(), indexVal.getString() } };
                }
                if(leftValRef->type() == Value::Type::Array)
                {
                    MINSL_EXECUTION_CHECK(indexVal.isNumber(), getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
                    size_t itemIndex;
                    MINSL_EXECUTION_CHECK(NumberToIndex(itemIndex, indexVal.getNumber()), getPlace(), "array index out of bounds");
                    return LValue{ ArrayItemLValue{ leftValRef->getArray(), itemIndex } };
                }
            }
            return Operator::getLeftValue(ctx);
        }

        Value BinaryOperator::ShiftLeft(const Value& lhs, const Value& rhs) const
        {
            const int64_t lhsInt = (int64_t)lhs.getNumber();
            const int64_t rhsInt = (int64_t)rhs.getNumber();
            const int64_t resultInt = lhsInt << rhsInt;
            return Value{ (double)resultInt };
        }

        Value BinaryOperator::ShiftRight(const Value& lhs, const Value& rhs) const
        {
            const int64_t lhsInt = (int64_t)lhs.getNumber();
            const int64_t rhsInt = (int64_t)rhs.getNumber();
            const int64_t resultInt = lhsInt >> rhsInt;
            return Value{ (double)resultInt };
        }

        Value BinaryOperator::Assignment(LValue&& lhs, Value&& rhs) const
        {
            // This one is able to create new value.
            if(m_type == BinaryOperator::Type::Assignment)
            {
                assign(lhs, Value{ rhs });
                return rhs;
            }

            // Others require existing value.
            Value* const lhsValPtr = lhs.getValueRef(getPlace());

            if(m_type == BinaryOperator::Type::AssignmentAdd)
            {
                if(lhsValPtr->type() == Value::Type::Number && rhs.isNumber())
                    lhsValPtr->setNumberValue(lhsValPtr->getNumber() + rhs.getNumber());
                else if(lhsValPtr->type() == Value::Type::String && rhs.isString())
                    lhsValPtr->getString() += rhs.getString();
                else
                    MINSL_EXECUTION_FAIL(getPlace(), ERROR_MESSAGE_INCOMPATIBLE_TYPES);
                return *lhsValPtr;
            }

            // Remaining ones work on numbers only.
            MINSL_EXECUTION_CHECK(lhsValPtr->type() == Value::Type::Number, getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
            MINSL_EXECUTION_CHECK(rhs.isNumber(), getPlace(), ERROR_MESSAGE_EXPECTED_NUMBER);
            switch(m_type)
            {
                case BinaryOperator::Type::AssignmentSub:
                    lhsValPtr->setNumberValue(lhsValPtr->getNumber() - rhs.getNumber());
                    break;
                case BinaryOperator::Type::AssignmentMul:
                    lhsValPtr->setNumberValue(lhsValPtr->getNumber() * rhs.getNumber());
                    break;
                case BinaryOperator::Type::AssignmentDiv:
                    lhsValPtr->setNumberValue(lhsValPtr->getNumber() / rhs.getNumber());
                    break;
                case BinaryOperator::Type::AssignmentMod:
                    lhsValPtr->setNumberValue(fmod(lhsValPtr->getNumber(), rhs.getNumber()));
                    break;
                case BinaryOperator::Type::AssignmentShiftLeft:
                    *lhsValPtr = ShiftLeft(*lhsValPtr, rhs);
                    break;
                case BinaryOperator::Type::AssignmentShiftRight:
                    *lhsValPtr = ShiftRight(*lhsValPtr, rhs);
                    break;
                case BinaryOperator::Type::AssignmentBitwiseAnd:
                    lhsValPtr->setNumberValue((double)((int64_t)lhsValPtr->getNumber() & (int64_t)rhs.getNumber()));
                    break;
                case BinaryOperator::Type::AssignmentBitwiseXor:
                    lhsValPtr->setNumberValue((double)((int64_t)lhsValPtr->getNumber() ^ (int64_t)rhs.getNumber()));
                    break;
                case BinaryOperator::Type::AssignmentBitwiseOr:
                    lhsValPtr->setNumberValue((double)((int64_t)lhsValPtr->getNumber() | (int64_t)rhs.getNumber()));
                    break;
                default:
                    assert(0);
            }

            return *lhsValPtr;
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
            printf(DEBUG_PRINT_FORMAT_STR_BEG "CallOperator\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            m_oplist[0]->debugPrint(indentLevel, "Callee: ");
            for(size_t i = 1, count = m_oplist.size(); i < count; ++i)
                m_oplist[i]->debugPrint(indentLevel, "Argument: ");
        }

        Value CallOperator::evaluate(ExecutionContext& ctx, ThisType* othis) const
        {
            (void)othis;
            ThisType th = ThisType{};
            Value callee = m_oplist[0]->evaluate(ctx, &th);
            const size_t argCount = m_oplist.size() - 1;
            std::vector<Value> arguments(argCount);
            for(size_t i = 0; i < argCount; ++i)
                arguments[i] = m_oplist[i + 1]->evaluate(ctx, nullptr);

            // Calling an object: Call its function under '' key.
            if(callee.isObject())
            {
                std::shared_ptr<Object> calleeObj = callee.getObjectRef();
                if(Value* defaultVal = calleeObj->tryGet(std::string{}); defaultVal && defaultVal->type() == Value::Type::Function)
                {
                    callee = *defaultVal;
                    th = ThisType{ std::move(calleeObj) };
                }
            }

            if(callee.type() == Value::Type::Function)
            {
                const AST::FunctionDefinition* const funcDef = callee.getFunction();
                MINSL_EXECUTION_CHECK(argCount == funcDef->m_paramlist.size(), getPlace(), "inexact number of arguments");
                Object localScope;
                // Setup parameters
                for(size_t argIndex = 0; argIndex != argCount; ++argIndex)
                    localScope.entry(funcDef->m_paramlist[argIndex]) = std::move(arguments[argIndex]);
                ExecutionContext::LocalScopePush localContextPush{ ctx, &localScope, std::move(th), getPlace() };
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
                    MINSL_EXECUTION_FAIL(getPlace(), ERROR_MESSAGE_BREAK_WITHOUT_LOOP);
                }
                catch(ContinueException)
                {
                    MINSL_EXECUTION_FAIL(getPlace(), ERROR_MESSAGE_CONTINUE_WITHOUT_LOOP);
                }
                return {};
            }
            if(callee.type() == Value::Type::HostFunction)
                return callee.getHostFunction()(ctx.m_env.getOwner(), getPlace(), std::move(arguments));
            if(callee.type() == Value::Type::SystemFunction)
            {
                switch(callee.getSysFunction())
                {
                    case SystemFunction::StringResizeFunc:
                        return Builtins::memberfn_string_resize(ctx, getPlace(), th, std::move(arguments));
                    case SystemFunction::ArrayAddFunc:
                        return Builtins::memberfn_array_add(ctx, getPlace(), th, std::move(arguments));
                    case SystemFunction::ArrayInsertFunc:
                        return Builtins::memberfn_array_insert(ctx, getPlace(), th, std::move(arguments));
                    case SystemFunction::ArrayRemoveFunc:
                        return Builtins::memberfn_array_remove(ctx, getPlace(), th, std::move(arguments));
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
                        return Builtins::ctor_null(ctx, getPlace(), std::move(arguments));
                    case Value::Type::Number:
                        return Builtins::ctor_number(ctx, getPlace(), std::move(arguments));
                    case Value::Type::String:
                        return Builtins::ctor_string(ctx, getPlace(), std::move(arguments));
                    case Value::Type::Object:
                        return Builtins::ctor_object(ctx, getPlace(), std::move(arguments));
                    case Value::Type::Array:
                        return Builtins::ctor_array(ctx, getPlace(), std::move(arguments));
                    case Value::Type::Type:
                        return Builtins::ctor_type(ctx, getPlace(), std::move(arguments));
                    case Value::Type::Function:
                    case Value::Type::SystemFunction:
                        return Builtins::ctor_function(ctx, getPlace(), std::move(arguments));
                    default:
                        assert(0);
                        return {};
                }
            }

            MINSL_EXECUTION_FAIL(getPlace(), "invalid function call");
        }

        void FunctionDefinition::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Function(", DEBUG_PRINT_ARGS_BEG);
            if(!m_paramlist.empty())
            {
                printf("%s", m_paramlist[0].c_str());
                for(size_t i = 1, count = m_paramlist.size(); i < count; ++i)
                    printf(", %s", m_paramlist[i].c_str());
            }
            printf(")\n");
            m_body.debugPrint(indentLevel + 1, "Body: ");
        }

        bool FunctionDefinition::areParamsUnique() const
        {
            // Warning! O(n^2) algorithm.
            for(size_t i = 0, count = m_paramlist.size(); i < count; ++i)
                for(size_t j = i + 1; j < count; ++j)
                    if(m_paramlist[i] == m_paramlist[j])
                        return false;
            return true;
        }

        void ObjectExpression::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Object\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            for(const auto& [name, value] : m_items)
                value->debugPrint(indentLevel, name);
        }

        Value ObjectExpression::evaluate(ExecutionContext& ctx, ThisType* othis) const
        {
            std::shared_ptr<Object> obj;
            (void)othis;
            if(m_baseexpr)
            {
                Value baseObj = m_baseexpr->evaluate(ctx, nullptr);
                if(baseObj.type() != Value::Type::Object)
                    throw ExecutionError{ getPlace(), ERROR_MESSAGE_BASE_MUST_BE_OBJECT };
                obj = CopyObject(*baseObj.getObject());
            }
            else
                obj = std::make_shared<Object>();
            for(const auto& [name, valueExpr] : m_items)
            {
                Value val = valueExpr->evaluate(ctx, nullptr);
                if(val.type() != Value::Type::Null)
                    obj->entry(name) = std::move(val);
                else if(m_baseexpr)
                    obj->removeEntry(name);
            }
            return Value{ std::move(obj) };
        }

        void ArrayExpression::debugPrint(uint32_t indentLevel, const std::string_view& prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Array\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            std::string itemPrefix;
            for(size_t i = 0, count = m_items.size(); i < count; ++i)
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
                result->m_items.push_back(item->evaluate(ctx, nullptr));
            return Value{ std::move(result) };
        }

    }
}


