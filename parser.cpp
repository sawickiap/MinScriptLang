
#include "msl.h"

namespace MSL
{
    #define MUST_PARSE(result, errorMessage)                                \
        do                                                                  \
        {                                                                   \
            if(!(result))                                                   \
                throw ParsingError(getCurrentTokenPlace(), (errorMessage)); \
        } while(false)

    void Parser::ParseScript(AST::Script& outScript)
    {
        for(;;)
        {
            Token token;
            m_tokenizer.getNextToken(token);
            const bool isEnd = token.symtype == Token::Type::End;
            if(token.symtype == Token::Type::String && !m_toklist.empty() && m_toklist.back().symtype == Token::Type::String)
                m_toklist.back().stringval += token.stringval;
            else
                m_toklist.push_back(std::move(token));
            if(isEnd)
                break;
        }

        parseBlock(outScript);
        if(m_toklist[m_tokidx].symtype != Token::Type::End)
            throw ParsingError(getCurrentTokenPlace(), "parsing error: expected End");

        //outScript.debugPrint(0, "Script: "); // #DELME
    }

    void Parser::parseBlock(AST::Block& outBlock)
    {
        while(m_toklist[m_tokidx].symtype != Token::Type::End)
        {
            std::unique_ptr<AST::Statement> stmt = tryParseStatement();
            if(!stmt)
                break;
            outBlock.m_statements.push_back(std::move(stmt));
        }
    }

    bool Parser::tryParseSwitchItem(AST::SwitchStatement& switchStatement)
    {
        const PlaceInCode place = getCurrentTokenPlace();
        // 'default' ':' Block
        if(tryParseSymbol(Token::Type::Default))
        {
            MUST_PARSE(tryParseSymbol(Token::Type::Colon), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON);
            switchStatement.m_itemvals.push_back(std::unique_ptr<AST::ConstantValue>{});
            switchStatement.m_itemblocks.push_back(std::make_unique<AST::Block>(place));
            parseBlock(*switchStatement.m_itemblocks.back());
            return true;
        }
        // 'case' ConstantExpr ':' Block
        if(tryParseSymbol(Token::Type::Case))
        {
            std::unique_ptr<AST::ConstantValue> constVal;
            MUST_PARSE(constVal = tryParseConstVal(), ERROR_MESSAGE_EXPECTED_CONSTANT_VALUE);
            switchStatement.m_itemvals.push_back(std::move(constVal));
            MUST_PARSE(tryParseSymbol(Token::Type::Colon), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON);
            switchStatement.m_itemblocks.push_back(std::make_unique<AST::Block>(place));
            parseBlock(*switchStatement.m_itemblocks.back());
            return true;
        }
        return false;
    }

    void Parser::parseFuncDef(AST::FunctionDefinition& funcDef)
    {
        MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
        if(m_toklist[m_tokidx].symtype == Token::Type::Identifier)
        {
            funcDef.m_paramlist.push_back(m_toklist[m_tokidx++].stringval);
            while(tryParseSymbol(Token::Type::Comma))
            {
                MUST_PARSE(m_toklist[m_tokidx].symtype == Token::Type::Identifier, ERROR_MESSAGE_EXPECTED_IDENTIFIER);
                funcDef.m_paramlist.push_back(m_toklist[m_tokidx++].stringval);
            }
        }
        MUST_PARSE(funcDef.areParamsUnique(), ERROR_MESSAGE_PARAMETER_NAMES_MUST_BE_UNIQUE);
        MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
        MUST_PARSE(tryParseSymbol(Token::Type::CurlyBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_OPEN);
        parseBlock(funcDef.m_body);
        MUST_PARSE(tryParseSymbol(Token::Type::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE);
    }

    bool Parser::peekSymbols(std::initializer_list<Token::Type> symbols)
    {
        if(m_tokidx + symbols.size() >= m_toklist.size())
            return false;
        for(size_t i = 0; i < symbols.size(); ++i)
            if(m_toklist[m_tokidx + i].symtype != symbols.begin()[i])
                return false;
        return true;
    }

    std::unique_ptr<AST::Statement> Parser::tryParseStatement()
    {
        const PlaceInCode place = getCurrentTokenPlace();

        // Empty statement: ';'
        if(tryParseSymbol(Token::Type::Semicolon))
            return std::make_unique<AST::EmptyStatement>(place);

        // Block: '{' Block '}'
        if(!peekSymbols({ Token::Type::CurlyBracketOpen, Token::Type::String, Token::Type::Colon }) && tryParseSymbol(Token::Type::CurlyBracketOpen))
        {
            auto block = std::make_unique<AST::Block>(getCurrentTokenPlace());
            parseBlock(*block);
            MUST_PARSE(tryParseSymbol(Token::Type::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE);
            return block;
        }

        // Condition: 'if' '(' Expr17 ')' Statement [ 'else' Statement ]
        if(tryParseSymbol(Token::Type::If))
        {
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
            auto condition = std::make_unique<AST::Condition>(place);
            MUST_PARSE(condition->m_condexpr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
            MUST_PARSE(condition->m_statements[0] = tryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            if(tryParseSymbol(Token::Type::Else))
                MUST_PARSE(condition->m_statements[1] = tryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            return condition;
        }

        // Loop: 'while' '(' Expr17 ')' Statement
        if(tryParseSymbol(Token::Type::While))
        {
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
            auto loop = std::make_unique<AST::WhileLoop>(place, AST::WhileLoop::Type::While);
            MUST_PARSE(loop->m_condexpr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
            MUST_PARSE(loop->m_body = tryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            return loop;
        }

        // Loop: 'do' Statement 'while' '(' Expr17 ')' ';'    - loop
        if(tryParseSymbol(Token::Type::Do))
        {
            auto loop = std::make_unique<AST::WhileLoop>(place, AST::WhileLoop::Type::DoWhile);
            MUST_PARSE(loop->m_body = tryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            MUST_PARSE(tryParseSymbol(Token::Type::While), ERROR_MESSAGE_EXPECTED_SYMBOL_WHILE);
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
            MUST_PARSE(loop->m_condexpr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
            MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';'");
            return loop;
        }

        if(tryParseSymbol(Token::Type::For))
        {
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
            // Range-based loop: 'for' '(' TOKEN_IDENTIFIER [ ',' TOKEN_IDENTIFIER ] ':' Expr17 ')' Statement
            if(peekSymbols({ Token::Type::Identifier, Token::Type::Colon })
               || peekSymbols({ Token::Type::Identifier, Token::Type::Comma, Token::Type::Identifier, Token::Type::Colon }))
            {
                auto loop = std::make_unique<AST::RangeBasedForLoop>(place);
                loop->m_valuevar = tryParseIdentifier();
                MUST_PARSE(!loop->m_valuevar.empty(), ERROR_MESSAGE_EXPECTED_IDENTIFIER);
                if(tryParseSymbol(Token::Type::Comma))
                {
                    loop->m_keyvar = std::move(loop->m_valuevar);
                    loop->m_valuevar = tryParseIdentifier();
                    MUST_PARSE(!loop->m_valuevar.empty(), ERROR_MESSAGE_EXPECTED_IDENTIFIER);
                }
                MUST_PARSE(tryParseSymbol(Token::Type::Colon), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON);
                MUST_PARSE(loop->m_rangeexpr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON);
                MUST_PARSE(loop->m_body = tryParseStatement(), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON);
                return loop;
            }
            // Loop: 'for' '(' Expr17? ';' Expr17? ';' Expr17? ')' Statement
            else
            {
                auto loop = std::make_unique<AST::ForLoop>(place);
                if(!tryParseSymbol(Token::Type::Semicolon))
                {
                    MUST_PARSE(loop->m_initexpr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                    MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';'");
                }
                if(!tryParseSymbol(Token::Type::Semicolon))
                {
                    MUST_PARSE(loop->m_condexpr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                    MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';'");
                }
                if(!tryParseSymbol(Token::Type::RoundBracketClose))
                {
                    MUST_PARSE(loop->m_iterexpr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                    MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
                }
                MUST_PARSE(loop->m_body = tryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
                return loop;
            }
        }

        // 'break' ';'
        if(tryParseSymbol(Token::Type::Break))
        {
            MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';'");
            return std::make_unique<AST::LoopBreakStatement>(place, AST::LoopBreakStatement::Type::Break);
        }

        // 'continue' ';'
        if(tryParseSymbol(Token::Type::Continue))
        {
            MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';' after continue");
            return std::make_unique<AST::LoopBreakStatement>(place, AST::LoopBreakStatement::Type::Continue);
        }

        // 'return' [ Expr17 ] ';'
        if(tryParseSymbol(Token::Type::Return))
        {
            auto stmt = std::make_unique<AST::ReturnStatement>(place);
            stmt->m_retvalue = TryParseExpr17();
            MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';' after return");
            return stmt;
        }

        // 'switch' '(' Expr17 ')' '{' SwitchItem+ '}'
        if(tryParseSymbol(Token::Type::Switch))
        {
            auto stmt = std::make_unique<AST::SwitchStatement>(place);
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
            MUST_PARSE(stmt->m_cond = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
            MUST_PARSE(tryParseSymbol(Token::Type::CurlyBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_OPEN);
            while(tryParseSwitchItem(*stmt))
            {
            }
            MUST_PARSE(tryParseSymbol(Token::Type::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE);
            // Check uniqueness. Warning: O(n^2) complexity!
            for(size_t i = 0, count = stmt->m_itemvals.size(); i < count; ++i)
            {
                for(size_t j = i + 1; j < count; ++j)
                {
                    if(j != i)
                    {
                        if(!stmt->m_itemvals[i] && !stmt->m_itemvals[j])// 2x default
                            throw ParsingError(place, ERROR_MESSAGE_EXPECTED_UNIQUE_CONSTANT);
                        if(stmt->m_itemvals[i] && stmt->m_itemvals[j])
                        {
                            if(stmt->m_itemvals[i]->m_val.isEqual(stmt->m_itemvals[j]->m_val))
                                throw ParsingError(stmt->m_itemvals[j]->getPlace(), ERROR_MESSAGE_EXPECTED_UNIQUE_CONSTANT);
                        }
                    }
                }
            }
            return stmt;
        }

        // 'throw' Expr17 ';'
        if(tryParseSymbol(Token::Type::Throw))
        {
            auto stmt = std::make_unique<AST::ThrowStatement>(place);
            MUST_PARSE(stmt->m_thrownexpr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON);
            return stmt;
        }

        // 'try' Statement ( ( 'catch' '(' TOKEN_IDENTIFIER ')' Statement [ 'finally' Statement ] ) | ( 'finally' Statement ) )
        if(tryParseSymbol(Token::Type::Try))
        {
            auto stmt = std::make_unique<AST::TryStatement>(place);
            MUST_PARSE(stmt->m_tryblock = tryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            if(tryParseSymbol(Token::Type::Finally))
                MUST_PARSE(stmt->m_finallyblock = tryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            else
            {
                MUST_PARSE(tryParseSymbol(Token::Type::Catch), "Expected 'catch' or 'finally'.");
                MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_OPEN);
                MUST_PARSE(!(stmt->m_exvarname = tryParseIdentifier()).empty(), ERROR_MESSAGE_EXPECTED_IDENTIFIER);
                MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
                MUST_PARSE(stmt->m_catchblock = tryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
                if(tryParseSymbol(Token::Type::Finally))
                    MUST_PARSE(stmt->m_finallyblock = tryParseStatement(), ERROR_MESSAGE_EXPECTED_STATEMENT);
            }
            return stmt;
        }

        // Expression as statement: Expr17 ';'
        std::unique_ptr<AST::Expression> expr = TryParseExpr17();
        if(expr)
        {
            MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), ERROR_MESSAGE_EXPECTED_SYMBOL_SEMICOLON);
            return expr;
        }

        // Syntactic sugar for functions:
        // 'function' IdentifierValue '(' [ TOKEN_IDENTIFIER ( ',' TOKE_IDENTIFIER )* ] ')' '{' Block '}'
        if(auto fnSyntacticSugar = tryParseFuncSynSugar(); fnSyntacticSugar.second)
        {
            auto identifierExpr
            = std::make_unique<AST::Identifier>(place, AST::Identifier::Scope::None, std::move(fnSyntacticSugar.first));
            auto assignmentOp = std::make_unique<AST::BinaryOperator>(place, AST::BinaryOperator::Type::Assignment);
            assignmentOp->m_oplist[0] = std::move(identifierExpr);
            assignmentOp->m_oplist[1] = std::move(fnSyntacticSugar.second);
            return assignmentOp;
        }

        if(auto cls = tryParseClassSynSugar(); cls)
            return cls;

        return {};
    }

    std::unique_ptr<AST::ConstantValue> Parser::tryParseConstVal()
    {
        const Token& t = m_toklist[m_tokidx];
        switch(t.symtype)
        {
            case Token::Type::Number:
                ++m_tokidx;
                return std::make_unique<AST::ConstantValue>(t.m_place, Value{ t.numberval });
            case Token::Type::String:
                ++m_tokidx;
                return std::make_unique<AST::ConstantValue>(t.m_place, Value{ std::string(t.stringval) });
            case Token::Type::Null:
                ++m_tokidx;
                return std::make_unique<AST::ConstantValue>(t.m_place, Value{});
            case Token::Type::False:
                ++m_tokidx;
                return std::make_unique<AST::ConstantValue>(t.m_place, Value{ 0.0 });
            case Token::Type::True:
                ++m_tokidx;
                return std::make_unique<AST::ConstantValue>(t.m_place, Value{ 1.0 });
            default:
                break;
        }
        return {};
    }

    std::unique_ptr<AST::Identifier> Parser::tryParseIdentVal()
    {
        const Token& t = m_toklist[m_tokidx];
        if(t.symtype == Token::Type::Local || t.symtype == Token::Type::Global)
        {
            ++m_tokidx;
            MUST_PARSE(tryParseSymbol(Token::Type::Dot), ERROR_MESSAGE_EXPECTED_SYMBOL_DOT);
            MUST_PARSE(m_tokidx < m_toklist.size(), ERROR_MESSAGE_EXPECTED_IDENTIFIER);
            const Token& tIdentifier = m_toklist[m_tokidx++];
            MUST_PARSE(tIdentifier.symtype == Token::Type::Identifier, ERROR_MESSAGE_EXPECTED_IDENTIFIER);
            AST::Identifier::Scope identifierScope = AST::Identifier::Scope::Count;
            switch(t.symtype)
            {
                case Token::Type::Local:
                    identifierScope = AST::Identifier::Scope::Local;
                    break;
                case Token::Type::Global:
                    identifierScope = AST::Identifier::Scope::Global;
                    break;
                default:
                    break;
            }
            return std::make_unique<AST::Identifier>(t.m_place, identifierScope, std::string(tIdentifier.stringval));
        }
        if(t.symtype == Token::Type::Identifier)
        {
            ++m_tokidx;
            return std::make_unique<AST::Identifier>(t.m_place, AST::Identifier::Scope::None, std::string(t.stringval));
        }
        return {};
    }

    std::unique_ptr<AST::ConstantExpression> Parser::tryParseConstExpr()
    {
        if(auto r = tryParseConstVal())
            return r;
        if(auto r = tryParseIdentVal())
            return r;
        const PlaceInCode place = m_toklist[m_tokidx].m_place;
        if(tryParseSymbol(Token::Type::This))
            return std::make_unique<AST::ThisExpression>(place);
        return {};
    }

    std::pair<std::string, std::unique_ptr<AST::FunctionDefinition>> Parser::tryParseFuncSynSugar()
    {
        if(peekSymbols({ Token::Type::Function, Token::Type::Identifier }))
        {
            ++m_tokidx;
            std::pair<std::string, std::unique_ptr<AST::FunctionDefinition>> result;
            result.first = m_toklist[m_tokidx++].stringval;
            result.second = std::make_unique<AST::FunctionDefinition>(getCurrentTokenPlace());
            parseFuncDef(*result.second);
            return result;
        }
        return std::make_pair(std::string{}, std::unique_ptr<AST::FunctionDefinition>{});
    }

    std::unique_ptr<AST::Expression> Parser::tryParseObjMember(std::string& outMemberName)
    {
        if(peekSymbols({ Token::Type::String, Token::Type::Colon }) || peekSymbols({ Token::Type::Identifier, Token::Type::Colon }))
        {
            outMemberName = m_toklist[m_tokidx].stringval;
            m_tokidx += 2;
            return TryParseExpr16();
        }
        return {};
    }

    std::unique_ptr<AST::Expression> Parser::tryParseClassSynSugar()
    {
        const PlaceInCode beginPlace = getCurrentTokenPlace();
        if(tryParseSymbol(Token::Type::Class))
        {
            std::string className = tryParseIdentifier();
            MUST_PARSE(!className.empty(), ERROR_MESSAGE_EXPECTED_IDENTIFIER);
            auto assignmentOp = std::make_unique<AST::BinaryOperator>(beginPlace, AST::BinaryOperator::Type::Assignment);
            assignmentOp->m_oplist[0]
            = std::make_unique<AST::Identifier>(beginPlace, AST::Identifier::Scope::None, std::move(className));
            std::unique_ptr<AST::Expression> baseExpr;
            if(tryParseSymbol(Token::Type::Colon))
            {
                baseExpr = TryParseExpr16();
                MUST_PARSE(baseExpr, ERROR_MESSAGE_EXPECTED_EXPRESSION);
            }
            auto objExpr = tryParseObject();
            objExpr->m_baseexpr = std::move(baseExpr);
            assignmentOp->m_oplist[1] = std::move(objExpr);
            MUST_PARSE(assignmentOp->m_oplist[1], ERROR_MESSAGE_EXPECTED_OBJECT);
            return assignmentOp;
        }
        return std::unique_ptr<AST::ObjectExpression>();
    }

    std::unique_ptr<AST::ObjectExpression> Parser::tryParseObject()
    {
        if(peekSymbols({ Token::Type::CurlyBracketOpen, Token::Type::CurlyBracketClose }) ||// { }
           peekSymbols({ Token::Type::CurlyBracketOpen, Token::Type::String, Token::Type::Colon }) ||// { 'key' :
           peekSymbols({ Token::Type::CurlyBracketOpen, Token::Type::Identifier, Token::Type::Colon }))// { key :
        {
            auto objExpr = std::make_unique<AST::ObjectExpression>(getCurrentTokenPlace());
            tryParseSymbol(Token::Type::CurlyBracketOpen);
            if(!tryParseSymbol(Token::Type::CurlyBracketClose))
            {
                std::string memberName;
                std::unique_ptr<AST::Expression> memberValue;
                MUST_PARSE(memberValue = tryParseObjMember(memberName), ERROR_MESSAGE_EXPECTED_OBJECT_MEMBER);
                MUST_PARSE(objExpr->m_items.insert(std::make_pair(std::move(memberName), std::move(memberValue))).second,
                           ERROR_MESSAGE_REPEATING_KEY_IN_OBJECT);
                if(!tryParseSymbol(Token::Type::CurlyBracketClose))
                {
                    while(tryParseSymbol(Token::Type::Comma))
                    {
                        if(tryParseSymbol(Token::Type::CurlyBracketClose))
                            return objExpr;
                        MUST_PARSE(memberValue = tryParseObjMember(memberName), ERROR_MESSAGE_EXPECTED_OBJECT_MEMBER);
                        MUST_PARSE(objExpr->m_items.insert(std::make_pair(std::move(memberName), std::move(memberValue))).second,
                                   ERROR_MESSAGE_REPEATING_KEY_IN_OBJECT);
                    }
                    MUST_PARSE(tryParseSymbol(Token::Type::CurlyBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_CURLY_BRACKET_CLOSE);
                }
            }
            return objExpr;
        }
        return {};
    }

    std::unique_ptr<AST::ArrayExpression> Parser::tryParseArray()
    {
        if(tryParseSymbol(Token::Type::SquareBracketOpen))
        {
            auto arrExpr = std::make_unique<AST::ArrayExpression>(getCurrentTokenPlace());
            if(!tryParseSymbol(Token::Type::SquareBracketClose))
            {
                std::unique_ptr<AST::Expression> itemValue;
                MUST_PARSE(itemValue = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                arrExpr->m_items.push_back((std::move(itemValue)));
                if(!tryParseSymbol(Token::Type::SquareBracketClose))
                {
                    while(tryParseSymbol(Token::Type::Comma))
                    {
                        if(tryParseSymbol(Token::Type::SquareBracketClose))
                            return arrExpr;
                        MUST_PARSE(itemValue = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                        arrExpr->m_items.push_back((std::move(itemValue)));
                    }
                    MUST_PARSE(tryParseSymbol(Token::Type::SquareBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_SQUARE_BRACKET_CLOSE);
                }
            }
            return arrExpr;
        }
        return {};
    }

    std::unique_ptr<AST::Expression> Parser::tryParseParentheses()
    {
        const PlaceInCode place = getCurrentTokenPlace();

        // '(' Expr17 ')'
        if(tryParseSymbol(Token::Type::RoundBracketOpen))
        {
            std::unique_ptr<AST::Expression> expr;
            MUST_PARSE(expr = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
            return expr;
        }

        if(auto objExpr = tryParseObject())
            return objExpr;
        if(auto arrExpr = tryParseArray())
            return arrExpr;

        // 'function' '(' [ TOKEN_IDENTIFIER ( ',' TOKE_IDENTIFIER )* ] ')' '{' Block '}'
        if(m_toklist[m_tokidx].symtype == Token::Type::Function && m_toklist[m_tokidx + 1].symtype == Token::Type::RoundBracketOpen)
        {
            ++m_tokidx;
            auto func = std::make_unique<AST::FunctionDefinition>(place);
            parseFuncDef(*func);
            return func;
        }

        // Constant
        std::unique_ptr<AST::ConstantExpression> constant = tryParseConstExpr();
        if(constant)
            return constant;

        return {};
    }

    std::unique_ptr<AST::Expression> Parser::tryParseUnary()
    {
        std::unique_ptr<AST::Expression> expr = tryParseParentheses();
        if(!expr)
            return {};
        for(;;)
        {
            const PlaceInCode place = getCurrentTokenPlace();
            // Postincrementation: Expr0 '++'
            if(tryParseSymbol(Token::Type::DoublePlus))
            {
                auto op = std::make_unique<AST::UnaryOperator>(place, AST::UnaryOperator::Type::Postincrementation);
                op->m_operand = std::move(expr);
                expr = std::move(op);
            }
            // Postdecrementation: Expr0 '--'
            else if(tryParseSymbol(Token::Type::DoubleDash))
            {
                auto op = std::make_unique<AST::UnaryOperator>(place, AST::UnaryOperator::Type::Postdecrementation);
                op->m_operand = std::move(expr);
                expr = std::move(op);
            }
            // Call: Expr0 '(' [ Expr16 ( ',' Expr16 )* ')'
            else if(tryParseSymbol(Token::Type::RoundBracketOpen))
            {
                auto op = std::make_unique<AST::CallOperator>(place);
                // Callee
                op->m_oplist.push_back(std::move(expr));
                // First argument
                expr = TryParseExpr16();
                if(expr)
                {
                    op->m_oplist.push_back(std::move(expr));
                    // Further arguments
                    while(tryParseSymbol(Token::Type::Comma))
                    {
                        MUST_PARSE(expr = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                        op->m_oplist.push_back(std::move(expr));
                    }
                }
                MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_ROUND_BRACKET_CLOSE);
                expr = std::move(op);
            }
            // Indexing: Expr0 '[' Expr17 ']'
            else if(tryParseSymbol(Token::Type::SquareBracketOpen))
            {
                auto op = std::make_unique<AST::BinaryOperator>(place, AST::BinaryOperator::Type::Indexing);
                op->m_oplist[0] = std::move(expr);
                MUST_PARSE(op->m_oplist[1] = TryParseExpr17(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
                MUST_PARSE(tryParseSymbol(Token::Type::SquareBracketClose), ERROR_MESSAGE_EXPECTED_SYMBOL_SQUARE_BRACKET_CLOSE);
                expr = std::move(op);
            }
            // Member access: Expr2 '.' TOKEN_IDENTIFIER
            else if(tryParseSymbol(Token::Type::Dot))
            {
                auto op = std::make_unique<AST::MemberAccessOperator>(place);
                op->m_operand = std::move(expr);
                auto identifier = tryParseIdentVal();
                MUST_PARSE(identifier && identifier->m_scope == AST::Identifier::Scope::None, ERROR_MESSAGE_EXPECTED_IDENTIFIER);
                op->m_membername = std::move(identifier->m_ident);
                expr = std::move(op);
            }
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::tryParseOperator()
    {
        const PlaceInCode place = getCurrentTokenPlace();
    #define PARSE_UNARY_OPERATOR(symbol, unaryOperatorType)                               \
        if(tryParseSymbol(symbol))                                                        \
        {                                                                                 \
            auto op = std::make_unique<AST::UnaryOperator>(place, (unaryOperatorType));        \
            MUST_PARSE(op->m_operand = tryParseOperator(), ERROR_MESSAGE_EXPECTED_EXPRESSION); \
            return op;                                                                    \
        }
        PARSE_UNARY_OPERATOR(Token::Type::DoublePlus, AST::UnaryOperator::Type::Preincrementation)
        PARSE_UNARY_OPERATOR(Token::Type::DoubleDash, AST::UnaryOperator::Type::Predecrementation)
        PARSE_UNARY_OPERATOR(Token::Type::Plus, AST::UnaryOperator::Type::Plus)
        PARSE_UNARY_OPERATOR(Token::Type::Dash, AST::UnaryOperator::Type::Minus)
        PARSE_UNARY_OPERATOR(Token::Type::ExclamationMark, AST::UnaryOperator::Type::LogicalNot)
        PARSE_UNARY_OPERATOR(Token::Type::Tilde, AST::UnaryOperator::Type::BitwiseNot)
    #undef PARSE_UNARY_OPERATOR
        // Just Expr2 or null if failed.
        return tryParseUnary();
    }

    #define PARSE_BINARY_OPERATOR(binaryOperatorType, exprParseFunc)                            \
        {                                                                                       \
            auto op = std::make_unique<AST::BinaryOperator>(place, (binaryOperatorType));            \
            op->m_oplist[0] = std::move(expr);                                                  \
            MUST_PARSE(op->m_oplist[1] = (exprParseFunc)(), ERROR_MESSAGE_EXPECTED_EXPRESSION); \
            expr = std::move(op);                                                               \
        }

    std::unique_ptr<AST::Expression> Parser::tryParseBinary()
    {
        std::unique_ptr<AST::Expression> expr = tryParseOperator();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = getCurrentTokenPlace();
            if(tryParseSymbol(Token::Type::Asterisk))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Mul, tryParseOperator)
            else if(tryParseSymbol(Token::Type::Slash))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Div, tryParseOperator)
            else if(tryParseSymbol(Token::Type::Percent))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Mod, tryParseOperator)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::tryParseAddSub()
    {
        std::unique_ptr<AST::Expression> expr = tryParseBinary();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = getCurrentTokenPlace();
            if(tryParseSymbol(Token::Type::Plus))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Add, tryParseBinary)
            else if(tryParseSymbol(Token::Type::Dash))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Sub, tryParseBinary)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::tryParseAngleSign()
    {
        std::unique_ptr<AST::Expression> expr = tryParseAddSub();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = getCurrentTokenPlace();
            if(tryParseSymbol(Token::Type::DoubleLess))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::ShiftLeft, tryParseAddSub)
            else if(tryParseSymbol(Token::Type::DoubleGreater))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::ShiftRight, tryParseAddSub)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::tryParseAngleCompare()
    {
        std::unique_ptr<AST::Expression> expr = tryParseAngleSign();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = getCurrentTokenPlace();
            if(tryParseSymbol(Token::Type::Less))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Less, tryParseAngleSign)
            else if(tryParseSymbol(Token::Type::LessEquals))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::LessEqual, tryParseAngleSign)
            else if(tryParseSymbol(Token::Type::Greater))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Greater, tryParseAngleSign)
            else if(tryParseSymbol(Token::Type::GreaterEquals))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::GreaterEqual, tryParseAngleSign)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::tryParseEquals()
    {
        std::unique_ptr<AST::Expression> expr = tryParseAngleCompare();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = getCurrentTokenPlace();
            if(tryParseSymbol(Token::Type::DoubleEquals))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Equal, tryParseAngleCompare)
            else if(tryParseSymbol(Token::Type::ExclamationEquals))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::NotEqual, tryParseAngleCompare)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr11()
    {
        std::unique_ptr<AST::Expression> expr = tryParseEquals();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = getCurrentTokenPlace();
            if(tryParseSymbol(Token::Type::Amperstand))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::BitwiseAnd, tryParseEquals)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr12()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr11();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = getCurrentTokenPlace();
            if(tryParseSymbol(Token::Type::Caret))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::BitwiseXor, TryParseExpr11)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr13()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr12();
        if(!expr)
            return {};

        for(;;)
        {
            const PlaceInCode place = getCurrentTokenPlace();
            if(tryParseSymbol(Token::Type::Pipe))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::BitwiseOr, TryParseExpr12)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr14()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr13();
        if(!expr)
            return {};
        for(;;)
        {
            const PlaceInCode place = getCurrentTokenPlace();
            if(tryParseSymbol(Token::Type::DoubleAmperstand))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::LogicalAnd, TryParseExpr13)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr15()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr14();
        if(!expr)
            return {};
        for(;;)
        {
            const PlaceInCode place = getCurrentTokenPlace();
            if(tryParseSymbol(Token::Type::DoublePipe))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::LogicalOr, TryParseExpr14)
            else
                break;
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr16()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr15();
        if(!expr)
            return {};

        // Ternary operator: Expr15 '?' Expr16 ':' Expr16
        if(tryParseSymbol(Token::Type::QuestionMark))
        {
            auto op = std::make_unique<AST::TernaryOperator>(getCurrentTokenPlace());
            op->m_oplist[0] = std::move(expr);
            MUST_PARSE(op->m_oplist[1] = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            MUST_PARSE(tryParseSymbol(Token::Type::Colon), ERROR_MESSAGE_EXPECTED_SYMBOL_COLON);
            MUST_PARSE(op->m_oplist[2] = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION);
            return op;
        }
        // Assignment: Expr15 = Expr16, and variants like += -=
    #define TRY_PARSE_ASSIGNMENT(symbol, binaryOperatorType)                                          \
        if(tryParseSymbol(symbol))                                                                    \
        {                                                                                             \
            auto op = std::make_unique<AST::BinaryOperator>(getCurrentTokenPlace(), (binaryOperatorType)); \
            op->m_oplist[0] = std::move(expr);                                                        \
            MUST_PARSE(op->m_oplist[1] = TryParseExpr16(), ERROR_MESSAGE_EXPECTED_EXPRESSION);        \
            return op;                                                                                \
        }
        TRY_PARSE_ASSIGNMENT(Token::Type::Equals, AST::BinaryOperator::Type::Assignment)
        TRY_PARSE_ASSIGNMENT(Token::Type::PlusEquals, AST::BinaryOperator::Type::AssignmentAdd)
        TRY_PARSE_ASSIGNMENT(Token::Type::DashEquals, AST::BinaryOperator::Type::AssignmentSub)
        TRY_PARSE_ASSIGNMENT(Token::Type::AsteriskEquals, AST::BinaryOperator::Type::AssignmentMul)
        TRY_PARSE_ASSIGNMENT(Token::Type::SlashEquals, AST::BinaryOperator::Type::AssignmentDiv)
        TRY_PARSE_ASSIGNMENT(Token::Type::PercentEquals, AST::BinaryOperator::Type::AssignmentMod)
        TRY_PARSE_ASSIGNMENT(Token::Type::DoubleLessEquals, AST::BinaryOperator::Type::AssignmentShiftLeft)
        TRY_PARSE_ASSIGNMENT(Token::Type::DoubleGreaterEquals, AST::BinaryOperator::Type::AssignmentShiftRight)
        TRY_PARSE_ASSIGNMENT(Token::Type::AmperstandEquals, AST::BinaryOperator::Type::AssignmentBitwiseAnd)
        TRY_PARSE_ASSIGNMENT(Token::Type::CaretEquals, AST::BinaryOperator::Type::AssignmentBitwiseXor)
        TRY_PARSE_ASSIGNMENT(Token::Type::PipeEquals, AST::BinaryOperator::Type::AssignmentBitwiseOr)
    #undef TRY_PARSE_ASSIGNMENT
        // Just Expr15
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr17()
    {
        std::unique_ptr<AST::Expression> expr = TryParseExpr16();
        if(!expr)
            return {};
        for(;;)
        {
            const PlaceInCode place = getCurrentTokenPlace();
            if(tryParseSymbol(Token::Type::Comma))
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Comma, TryParseExpr16)
            else
                break;
        }
        return expr;
    }

    bool Parser::tryParseSymbol(Token::Type symbol)
    {
        if(m_tokidx < m_toklist.size() && m_toklist[m_tokidx].symtype == symbol)
        {
            ++m_tokidx;
            return true;
        }
        return false;
    }

    std::string Parser::tryParseIdentifier()
    {
        if(m_tokidx < m_toklist.size() && m_toklist[m_tokidx].symtype == Token::Type::Identifier)
            return m_toklist[m_tokidx++].stringval;
        return {};
    }
}

