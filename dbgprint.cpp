
#include "priv.h"

namespace MSL
{
    namespace AST
    {
        static const char* getIndent(uint32_t idl)
        {
            static const char* silly = "                                                                                                                                                                                                                                                                ";
            return silly + (256 - std::min<uint32_t>(idl, 128) * 2);
        }

        static void printPrefix(DebugWriter& dw, uint32_t idl, const Location& pl, std::string_view prefix)
        {
            enum{ kMaxBuf = 128 };
            char fmt[kMaxBuf+1];
            snprintf(fmt, kMaxBuf, "(%4u,%4u) %s%.*s",
                pl.textline,
                pl.textcolumn,
                getIndent(idl),
                int(prefix.size()),
                prefix.data()
            );
            dw.writeString(fmt);
        }

        void EmptyStatement::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("Empty\n");
        }

        void Condition::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("If\n");
            ++idl;
            m_condexpr->debugPrint(dw, idl, "ConditionExpression: ");
            m_truestmt->debugPrint(dw, idl, "TrueStatement: ");
            if(m_falsestmt)
            {
                m_falsestmt->debugPrint(dw, idl, "FalseStatement: ");
            }
        }

        void WhileLoop::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
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
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString(name);
            dw.writeString("\n");
            ++idl;
            m_condexpr->debugPrint(dw, idl, "ConditionExpression: ");
            m_body->debugPrint(dw, idl, "Body: ");
        }

        void ForLoop::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("For\n");
            ++idl;
            if(m_initexpr)
            {
                m_initexpr->debugPrint(dw, idl, "InitExpression: ");
            }
            else
            {
                printPrefix(dw, idl, getPlace(), prefix);
                dw.writeString("(Init expression empty)\n");
            }
            if(m_condexpr)
            {
                m_condexpr->debugPrint(dw, idl, "ConditionExpression: ");
            }
            else
            {
                printPrefix(dw, idl, getPlace(), prefix);
                dw.writeString("(Condition expression empty)\n");
            }
            if(m_iterexpr)
            {
                m_iterexpr->debugPrint(dw, idl, "IterationExpression: ");
            }
            else
            {
                printPrefix(dw, idl, getPlace(), prefix);
                dw.writeString("(Iteration expression empty)\n");
            }
            m_body->debugPrint(dw, idl, "Body: ");
        }

        void RangeBasedForLoop::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            if(!m_keyvar.empty())
            {
                printPrefix(dw, idl, getPlace(), prefix);
                dw.writeString("Range-based for: ");
                dw.writeString(m_keyvar);
                dw.writeString(", ");
                dw.writeString(m_valuevar);
                dw.writeString("\n");
            }
            else
            {
                printPrefix(dw, idl, getPlace(), prefix);
                dw.writeString("Range-based for: ");
                dw.writeString(m_valuevar);
                dw.writeString("\n");
            }
            ++idl;
            m_rangeexpr->debugPrint(dw, idl, "RangeExpression: ");
            m_body->debugPrint(dw, idl, "Body: ");
        }

        void LoopBreakStatement::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            static const char* LOOP_BREAK_TYPE_NAMES[] = { "Break", "Continue" };
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString(LOOP_BREAK_TYPE_NAMES[size_t(m_type)]);
            dw.writeString("\n");
        }

        void ReturnStatement::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("return\n");
            if(m_retvalue)
            {
                m_retvalue->debugPrint(dw, idl + 1, "ReturnedValue: ");
            }
        }

        void Block::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("Block\n");
            ++idl;
            for(const auto& stmtPtr : m_statements)
            {
                stmtPtr->debugPrint(dw, idl, std::string_view{});
            }
        }

        void SwitchStatement::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            size_t i;
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("switch\n");
            ++idl;
            m_cond->debugPrint(dw, idl, "Condition: ");
            for(i = 0; i < m_itemvals.size(); ++i)
            {
                if(m_itemvals[i])
                {
                    m_itemvals[i]->debugPrint(dw, idl, "ItemValue: ");
                }
                else
                {
                    printPrefix(dw, idl, getPlace(), prefix);
                    dw.writeString("Default\n");
                }
                if(m_itemblocks[i])
                {
                    m_itemblocks[i]->debugPrint(dw, idl, "ItemBlock: ");
                }
                else
                {
                    printPrefix(dw, idl, getPlace(), prefix);
                    dw.writeString("(Empty block)\n");
                }
            }
        }

        void ThrowStatement::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("throw\n");
            m_thrownexpr->debugPrint(dw, idl + 1, "ThrownExpression: ");
        }

        void TryStatement::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("try\n");
            ++idl;
            m_tryblock->debugPrint(dw, idl, "TryBlock: ");
            if(m_catchblock)
            {
                m_catchblock->debugPrint(dw, idl, "CatchBlock: ");
            }
            if(m_finallyblock)
            {
                m_finallyblock->debugPrint(dw, idl, "FinallyBlock: ");
            }
        }

        void ConstantValue::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            switch(m_val.type())
            {
                case Value::Type::Null:
                    {
                        printPrefix(dw, idl, getPlace(), prefix);
                        dw.writeString("Constant null\n");
                    }
                    break;
                case Value::Type::Number:
                    {
                        printPrefix(dw, idl, getPlace(), prefix);
                        dw.writeString("Constant number: ");
                        dw.writeNumber(m_val.number());
                        dw.writeString("\n");
                    }
                    break;
                case Value::Type::String:
                    {
                        printPrefix(dw, idl, getPlace(), prefix);
                        dw.writeString("Constant string: ");
                        dw.writeReprString(m_val.string());
                        dw.writeString("\n");
                    }
                    break;
                default:
                    assert(0 && "ConstantValue should not be used with this type.");
                    break;
            }
        }

        void Identifier::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            static const char* PREFIX[] = { "", "local.", "global." };
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("Identifier: ");
            dw.writeString(PREFIX[size_t(m_scope)]);
            dw.writeString(m_ident);
            dw.writeString("\n");
        }

        void ThisExpression::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("This\n");
        }

        void UnaryOperator::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            static const char* UNARY_OPERATOR_TYPE_NAMES[]
            = { "Preincrementation", "Predecrementation", "Postincrementation", "Postdecrementation", "Plus", "Minus",
                "Logical NOT",       "Bitwise NOT" };
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("UnaryOperator ");
            dw.writeString(UNARY_OPERATOR_TYPE_NAMES[uint32_t(m_type)]);
            dw.writeString("\n");
            ++idl;
            m_operand->debugPrint(dw, idl, "Operand: ");
        }

        void MemberAccessOperator::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("MemberAccessOperator Member=");
            dw.writeString(m_membername);
            dw.writeString("\n");
            m_operand->debugPrint(dw, idl + 1, "Operand: ");
        }

        void BinaryOperator::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
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
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("BinaryOperator ");
            dw.writeString(BINARY_OPERATOR_TYPE_NAMES[uint32_t(m_type)]);
            dw.writeString("\n");
            ++idl;
            m_leftoper->debugPrint(dw, idl, "LeftOperand: ");
            m_rightoper->debugPrint(dw, idl, "RightOperand: ");
        }

        void TernaryOperator::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("TernaryOperator\n");
            ++idl;
            m_condexpr->debugPrint(dw, idl, "ConditionExpression: ");
            m_trueexpr->debugPrint(dw, idl, "TrueExpression: ");
            m_falseexpr->debugPrint(dw, idl, "FalseExpression: ");
        }


        void CallOperator::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            size_t i;
            size_t count;
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("CallOperator\n");
            ++idl;
            m_oplist[0]->debugPrint(dw, idl, "Callee: ");
            for(i = 1, count = m_oplist.size(); i < count; ++i)
            {
                m_oplist[i]->debugPrint(dw, idl, "Argument: ");
            }
        }

        void FunctionDefinition::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            size_t i;
            size_t count;
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("Function(");
            if(!m_paramlist.empty())
            {
                dw.writeString(m_paramlist[0]);
                for(i = 1, count = m_paramlist.size(); i < count; ++i)
                {
                    dw.writeString(", ");
                    dw.writeString(m_paramlist[i]);
                }
            }
            dw.writeString(")\n");
            m_body.debugPrint(dw, idl + 1, "Body: ");
        }

        void ObjectExpression::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("Object\n");
            ++idl;
            for(const auto& [name, value] : m_exprmap)
            {
                value->debugPrint(dw, idl, name);
            }
        }

        void ArrayExpression::debugPrint(DebugWriter& dw, uint32_t idl, std::string_view prefix) const
        {
            size_t i;
            size_t count;
            printPrefix(dw, idl, getPlace(), prefix);
            dw.writeString("Array\n");
            ++idl;
            for(i = 0, count = m_exprlist.size(); i < count; ++i)
            {
                m_exprlist[i]->debugPrint(dw, idl, Util::formatString("%zu: ", i));
            }
        }
    }
}
