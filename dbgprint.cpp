
#include "priv.h"

#define DEBUG_PRINT_FORMAT_STR_BEG \
    "(%4u,%4u) %s%.*s"

#define DEBUG_PRINT_ARGS_BEG \
    getPlace().textline, getPlace().textcolumn, getIndent(indentLevel), (int)prefix.length(), prefix.data()


namespace MSL
{
    namespace AST
    {
        static void printString(std::string_view str)
        {
            int c;
            size_t i;
            fputc('"', stdout);
            for(i=0; i<str.size(); i++)
            {
                c = str[i];
                if((c < 32) || (c > 127) || (c == '\"') || (c == '\\'))
                {
                    switch(c)
                    {
                        case '\'':
                            printf("\\\'");
                            break;
                        case '\"':
                            printf("\\\"");
                            break;
                        case '\\':
                            printf("\\\\");
                            break;
                        case '\b':
                            printf("\\b");
                            break;
                        case '\f':
                            printf("\\f");
                            break;
                        case '\n':
                            printf("\\n");
                            break;
                        case '\r':
                            printf("\\r");
                            break;
                        case '\t':
                            printf("\\t");
                            break;
                        default:
                            {
                                constexpr const char* const hexchars = "0123456789ABCDEF";
                                fputc('\\', stdout);
                                if(c <= 255)
                                {
                                    fputc('x', stdout);
                                    fputc(char(hexchars[(c >> 4) & 0xf]), stdout);
                                    fputc(char(hexchars[c & 0xf]), stdout);
                                }
                                else
                                {
                                    fputc('u', stdout);
                                    fputc(char(hexchars[(c >> 12) & 0xf]), stdout);
                                    fputc(char(hexchars[(c >> 8) & 0xf]), stdout);
                                    fputc(char(hexchars[(c >> 4) & 0xf]), stdout);
                                    fputc(char(hexchars[c & 0xf]), stdout);
                                }
                            }
                            break;
                    }
                }
                else
                {
                    fputc(c, stdout);
                }
            }
            fputc('"', stdout);
        }

        static const char* getIndent(uint32_t indentLevel)
        {
            static const char* silly = "                                                                                                                                                                                                                                                                ";
            return silly + (256 - std::min<uint32_t>(indentLevel, 128) * 2);
        }

        void EmptyStatement::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Empty\n", DEBUG_PRINT_ARGS_BEG);
        }

        void Condition::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "If\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            m_condexpr->debugPrint(indentLevel, "ConditionExpression: ");
            m_statements[0]->debugPrint(indentLevel, "TrueStatement: ");
            if(m_statements[1])
            {
                m_statements[1]->debugPrint(indentLevel, "FalseStatement: ");
            }
        }

        void WhileLoop::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            const char* name;
            name = nullptr;
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


        void ForLoop::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "For\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            if(m_initexpr)
            {
                m_initexpr->debugPrint(indentLevel, "InitExpression: ");
            }
            else
            {
                printf(DEBUG_PRINT_FORMAT_STR_BEG "(Init expression empty)\n", DEBUG_PRINT_ARGS_BEG);
            }
            if(m_condexpr)
            {
                m_condexpr->debugPrint(indentLevel, "ConditionExpression: ");
            }
            else
            {
                printf(DEBUG_PRINT_FORMAT_STR_BEG "(Condition expression empty)\n", DEBUG_PRINT_ARGS_BEG);
            }
            if(m_iterexpr)
            {
                m_iterexpr->debugPrint(indentLevel, "IterationExpression: ");
            }
            else
            {
                printf(DEBUG_PRINT_FORMAT_STR_BEG "(Iteration expression empty)\n", DEBUG_PRINT_ARGS_BEG);
            }
            m_body->debugPrint(indentLevel, "Body: ");
        }


        void RangeBasedForLoop::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            if(!m_keyvar.empty())
            {
                printf(DEBUG_PRINT_FORMAT_STR_BEG "Range-based for: %s, %s\n", DEBUG_PRINT_ARGS_BEG, m_keyvar.c_str(),
                       m_valuevar.c_str());
            }
            else
            {
                printf(DEBUG_PRINT_FORMAT_STR_BEG "Range-based for: %s\n", DEBUG_PRINT_ARGS_BEG, m_valuevar.c_str());
            }
            ++indentLevel;
            m_rangeexpr->debugPrint(indentLevel, "RangeExpression: ");
            m_body->debugPrint(indentLevel, "Body: ");
        }

        void LoopBreakStatement::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            static const char* LOOP_BREAK_TYPE_NAMES[] = { "Break", "Continue" };
            printf(DEBUG_PRINT_FORMAT_STR_BEG "%s\n", DEBUG_PRINT_ARGS_BEG, LOOP_BREAK_TYPE_NAMES[(size_t)m_type]);
        }


        void ReturnStatement::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "return\n", DEBUG_PRINT_ARGS_BEG);
            if(m_retvalue)
            {
                m_retvalue->debugPrint(indentLevel + 1, "ReturnedValue: ");
            }
        }

        void Block::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Block\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            for(const auto& stmtPtr : m_statements)
            {
                stmtPtr->debugPrint(indentLevel, std::string_view{});
            }
        }

        void SwitchStatement::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            size_t i;
            printf(DEBUG_PRINT_FORMAT_STR_BEG "switch\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            m_cond->debugPrint(indentLevel, "Condition: ");
            for(i = 0; i < m_itemvals.size(); ++i)
            {
                if(m_itemvals[i])
                {
                    m_itemvals[i]->debugPrint(indentLevel, "ItemValue: ");
                }
                else
                {
                    printf(DEBUG_PRINT_FORMAT_STR_BEG "Default\n", DEBUG_PRINT_ARGS_BEG);
                }
                if(m_itemblocks[i])
                {
                    m_itemblocks[i]->debugPrint(indentLevel, "ItemBlock: ");
                }
                else
                {
                    printf(DEBUG_PRINT_FORMAT_STR_BEG "(Empty block)\n", DEBUG_PRINT_ARGS_BEG);
                }
            }
        }

        void ThrowStatement::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "throw\n", DEBUG_PRINT_ARGS_BEG);
            m_thrownexpr->debugPrint(indentLevel + 1, "ThrownExpression: ");
        }

        void TryStatement::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "try\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            m_tryblock->debugPrint(indentLevel, "TryBlock: ");
            if(m_catchblock)
            {
                m_catchblock->debugPrint(indentLevel, "CatchBlock: ");
            }
            if(m_finallyblock)
            {
                m_finallyblock->debugPrint(indentLevel, "FinallyBlock: ");
            }
        }

        void ConstantValue::debugPrint(uint32_t indentLevel, std::string_view prefix) const
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
                    printf(DEBUG_PRINT_FORMAT_STR_BEG "Constant string: ", DEBUG_PRINT_ARGS_BEG);
                    printString(m_val.getString());
                    printf("\n");
                    break;
                default:
                    assert(0 && "ConstantValue should not be used with this type.");
                    break;
            }
        }

        void Identifier::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            static const char* PREFIX[] = { "", "local.", "global." };
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Identifier: %s%s\n", DEBUG_PRINT_ARGS_BEG, PREFIX[(size_t)m_scope], m_ident.c_str());
        }

        void ThisExpression::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "This\n", DEBUG_PRINT_ARGS_BEG);
        }

        void UnaryOperator::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            static const char* UNARY_OPERATOR_TYPE_NAMES[]
            = { "Preincrementation", "Predecrementation", "Postincrementation", "Postdecrementation", "Plus", "Minus",
                "Logical NOT",       "Bitwise NOT" };

            printf(DEBUG_PRINT_FORMAT_STR_BEG "UnaryOperator %s\n", DEBUG_PRINT_ARGS_BEG, UNARY_OPERATOR_TYPE_NAMES[(uint32_t)m_type]);
            ++indentLevel;
            m_operand->debugPrint(indentLevel, "Operand: ");
        }

        void MemberAccessOperator::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "MemberAccessOperator Member=%s\n", DEBUG_PRINT_ARGS_BEG, m_membername.c_str());
            m_operand->debugPrint(indentLevel + 1, "Operand: ");
        }

        void BinaryOperator::debugPrint(uint32_t indentLevel, std::string_view prefix) const
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

        void TernaryOperator::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "TernaryOperator\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            m_oplist[0]->debugPrint(indentLevel, "ConditionExpression: ");
            m_oplist[1]->debugPrint(indentLevel, "TrueExpression: ");
            m_oplist[2]->debugPrint(indentLevel, "FalseExpression: ");
        }


        void CallOperator::debugPrint(uint32_t indentLevel, std::string_view prefix) const
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

        void FunctionDefinition::debugPrint(uint32_t indentLevel, std::string_view prefix) const
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

        void ObjectExpression::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Object\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            for(const auto& [name, value] : m_exprmap)
            {
                value->debugPrint(indentLevel, name);
            }
        }

        void ArrayExpression::debugPrint(uint32_t indentLevel, std::string_view prefix) const
        {
            size_t i;
            size_t count;
            std::string itemPrefix;
            printf(DEBUG_PRINT_FORMAT_STR_BEG "Array\n", DEBUG_PRINT_ARGS_BEG);
            ++indentLevel;
            for(i = 0, count = m_exprlist.size(); i < count; ++i)
            {
                itemPrefix = Util::Format("%zu: ", i);
                m_exprlist[i]->debugPrint(indentLevel, itemPrefix);
            }
        }
    }
}
