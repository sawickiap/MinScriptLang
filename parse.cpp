
#include "msl.h"

namespace MSL
{
    #define MUST_PARSE(result, errorMessage)                                \
        do                                                                  \
        {                                                                   \
            if(!(result))                                                   \
                throw Error::ParsingError(currentTokenLocation(), (errorMessage)); \
        } while(false)

    void Parser::parseScript(AST::Script& outscr)
    {
        bool isend;
        Token token;
        for(;;)
        {
            m_tokenizer.getNextToken(token);
            isend = token.type() == Token::Type::End;
            if(token.type() == Token::Type::String && !m_toklist.empty() && m_toklist.back().type() == Token::Type::String)
            {
                m_toklist.back().string() += token.string();
            }
            else
            {
                m_toklist.push_back(std::move(token));
            }
            if(isend)
            {
                break;
            }
        }

        parseBlock(outscr);
        if(m_toklist[m_tokidx].type() != Token::Type::End)
        {
            throw Error::ParsingError(currentTokenLocation(), "parsing error: expected End");
        }
        //outscr.debugPrint(0, "Script: "); // #DELME
    }

    void Parser::parseBlock(AST::Block& outBlock)
    {
        while(m_toklist[m_tokidx].type() != Token::Type::End)
        {
            auto stmt = tryParseStatement();
            if(!stmt)
            {
                break;
            }
            outBlock.m_statements.push_back(std::move(stmt));
        }
    }

    bool Parser::tryParseSwitchItem(AST::SwitchStatement& swstmt)
    {
        auto loc = currentTokenLocation();
        // 'default' ':' Block
        if(tryParseSymbol(Token::Type::Default))
        {
            MUST_PARSE(tryParseSymbol(Token::Type::Colon), "expected ':'");
            swstmt.m_itemvals.push_back(std::unique_ptr<AST::ConstantValue>{});
            swstmt.m_itemblocks.push_back(std::make_unique<AST::Block>(loc));
            parseBlock(*swstmt.m_itemblocks.back());
            return true;
        }
        // 'case' ConstantExpr ':' Block
        if(tryParseSymbol(Token::Type::Case))
        {
            auto cval = tryParseConstVal();
            MUST_PARSE(cval, "expected constant value");
            swstmt.m_itemvals.push_back(std::move(cval));
            MUST_PARSE(tryParseSymbol(Token::Type::Colon), "expected ':'");
            swstmt.m_itemblocks.push_back(std::make_unique<AST::Block>(loc));
            parseBlock(*swstmt.m_itemblocks.back());
            return true;
        }
        return false;
    }

    void Parser::parseFuncDef(AST::FunctionDefinition& funcdef)
    {
        MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), "expected '(' while parsing function definition");
        if(m_toklist[m_tokidx].type() == Token::Type::Identifier)
        {
            funcdef.m_paramlist.push_back(m_toklist[m_tokidx++].string());
            while(tryParseSymbol(Token::Type::Comma))
            {
                MUST_PARSE(m_toklist[m_tokidx].type() == Token::Type::Identifier, "expected identifier while parsing function definition");
                funcdef.m_paramlist.push_back(m_toklist[m_tokidx++].string());
            }
        }
        MUST_PARSE(funcdef.areParamsUnique(), "expected argument list of function definition to be unique");
        MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), "expected ')' while parsing function definition");
        MUST_PARSE(tryParseSymbol(Token::Type::CurlyBracketOpen), "expected '{' while parsing function definition");
        parseBlock(funcdef.m_body);
        MUST_PARSE(tryParseSymbol(Token::Type::CurlyBracketClose), "expected '}' while parsing function definition");
    }

    bool Parser::peekSymbols(std::initializer_list<Token::Type> symbols)
    {
        size_t i;
        if(m_tokidx + symbols.size() >= m_toklist.size())
        {
            return false;
        }
        for(i = 0; i < symbols.size(); ++i)
        {
            if(m_toklist[m_tokidx + i].type() != symbols.begin()[i])
            {
                return false;
            }
        }
        return true;
    }

    std::unique_ptr<AST::Statement> Parser::tryParseStatement()
    {
        size_t i;
        size_t j;
        size_t count;
        auto loc = currentTokenLocation();

        // Empty statement: ';'
        if(tryParseSymbol(Token::Type::Semicolon))
        {
            return std::make_unique<AST::EmptyStatement>(loc);
        }
        // Block: '{' Block '}'
        if(!peekSymbols({ Token::Type::CurlyBracketOpen, Token::Type::String, Token::Type::Colon }) && tryParseSymbol(Token::Type::CurlyBracketOpen))
        {
            auto block = std::make_unique<AST::Block>(currentTokenLocation());
            parseBlock(*block);
            MUST_PARSE(tryParseSymbol(Token::Type::CurlyBracketClose), "expected '}' after block");
            return block;
        }
        // Condition: 'if' '(' Expr17 ')' Statement [ 'else' Statement ]
        if(tryParseSymbol(Token::Type::If))
        {
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), "expected '(' after 'if'");
            auto condition = std::make_unique<AST::Condition>(loc);
            MUST_PARSE(condition->m_condexpr = TryParseExpr17(), "expected expression after 'if'");
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), "expected ')'");
            MUST_PARSE(condition->m_truestmt = tryParseStatement(), "expected a statement after 'if'");
            if(tryParseSymbol(Token::Type::Else))
            {
                MUST_PARSE(condition->m_falsestmt = tryParseStatement(), "expected statement after 'else'");
            }
            return condition;
        }
        // Loop: 'while' '(' Expr17 ')' Statement
        if(tryParseSymbol(Token::Type::While))
        {
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), "expected '(' after 'while'");
            auto loop = std::make_unique<AST::WhileLoop>(loc, AST::WhileLoop::Type::While);
            MUST_PARSE(loop->m_condexpr = TryParseExpr17(), "expected expression after 'while'");
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), "expected ')'");
            MUST_PARSE(loop->m_body = tryParseStatement(), "expected statement after 'while'");
            return loop;
        }
        // Loop: 'do' Statement 'while' '(' Expr17 ')' ';'    - loop
        if(tryParseSymbol(Token::Type::Do))
        {
            auto loop = std::make_unique<AST::WhileLoop>(loc, AST::WhileLoop::Type::DoWhile);
            MUST_PARSE(loop->m_body = tryParseStatement(), "expect a statement after 'do'");
            MUST_PARSE(tryParseSymbol(Token::Type::While), "expected 'while' after 'do' block");
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), "expected '(' after 'while' in 'do...while'");
            MUST_PARSE(loop->m_condexpr = TryParseExpr17(), "expected expression after ''while in 'do...while'");
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), "expected ')'");
            MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';'");
            return loop;
        }
        if(tryParseSymbol(Token::Type::For))
        {
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), "expected '(' after 'for'");
            // Range-based loop: 'for' '(' TOKEN_IDENTIFIER [ ',' TOKEN_IDENTIFIER ] ':' Expr17 ')' Statement
            if(peekSymbols({ Token::Type::Identifier, Token::Type::Colon })
               || peekSymbols({ Token::Type::Identifier, Token::Type::Comma, Token::Type::Identifier, Token::Type::Colon }))
            {
                auto loop = std::make_unique<AST::RangeBasedForLoop>(loc);
                loop->m_valuevar = tryParseIdentifier();
                MUST_PARSE(!loop->m_valuevar.empty(), "expected identifier in range-based for-loop");
                if(tryParseSymbol(Token::Type::Comma))
                {
                    loop->m_keyvar = std::move(loop->m_valuevar);
                    loop->m_valuevar = tryParseIdentifier();
                    MUST_PARSE(!loop->m_valuevar.empty(), "expected identifier in range-based for-loop");
                }
                MUST_PARSE(tryParseSymbol(Token::Type::Colon), "expected ':' in range-based for-loop");
                MUST_PARSE(loop->m_rangeexpr = TryParseExpr17(), "expected expression in range-based for-loop");
                MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), "expected ':'");
                MUST_PARSE(loop->m_body = tryParseStatement(), "expected ':'");
                return loop;
            }
            // Loop: 'for' '(' Expr17? ';' Expr17? ';' Expr17? ')' Statement
            else
            {
                auto loop = std::make_unique<AST::ForLoop>(loc);
                if(!tryParseSymbol(Token::Type::Semicolon))
                {
                    MUST_PARSE(loop->m_initexpr = TryParseExpr17(), "expected expression for init of for-loop");
                    MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';'");
                }
                if(!tryParseSymbol(Token::Type::Semicolon))
                {
                    MUST_PARSE(loop->m_condexpr = TryParseExpr17(), "expected expression for condition of for-loop");
                    MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';'");
                }
                if(!tryParseSymbol(Token::Type::RoundBracketClose))
                {
                    MUST_PARSE(loop->m_iterexpr = TryParseExpr17(), "expected expression for iterator of for-loop");
                    MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), "expected ')'");
                }
                MUST_PARSE(loop->m_body = tryParseStatement(), "expected statement after for-loop");
                return loop;
            }
        }
        // 'break' ';'
        if(tryParseSymbol(Token::Type::Break))
        {
            MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';'");
            return std::make_unique<AST::LoopBreakStatement>(loc, AST::LoopBreakStatement::Type::Break);
        }
        // 'continue' ';'
        if(tryParseSymbol(Token::Type::Continue))
        {
            MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';' after continue");
            return std::make_unique<AST::LoopBreakStatement>(loc, AST::LoopBreakStatement::Type::Continue);
        }
        // 'return' [ Expr17 ] ';'
        if(tryParseSymbol(Token::Type::Return))
        {
            auto stmt = std::make_unique<AST::ReturnStatement>(loc);
            stmt->m_retvalue = TryParseExpr17();
            MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';' after return");
            return stmt;
        }
        // 'switch' '(' Expr17 ')' '{' SwitchItem+ '}'
        if(tryParseSymbol(Token::Type::Switch))
        {
            auto stmt = std::make_unique<AST::SwitchStatement>(loc);
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), "expected '(' after 'switch'");
            MUST_PARSE(stmt->m_cond = TryParseExpr17(), "expected expression after 'switch'");
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), "expected closing ')' after expression after 'switch'");
            MUST_PARSE(tryParseSymbol(Token::Type::CurlyBracketOpen), "expected '{' after 'switch'");
            while(tryParseSwitchItem(*stmt))
            {
            }
            MUST_PARSE(tryParseSymbol(Token::Type::CurlyBracketClose), "expected closing '}' after 'switch' block");
            // Check uniqueness. Warning: O(n^2) complexity!
            for(i = 0, count = stmt->m_itemvals.size(); i < count; ++i)
            {
                for(j = i + 1; j < count; ++j)
                {
                    if(j != i)
                    {
                        if(!stmt->m_itemvals[i] && !stmt->m_itemvals[j])// 2x default
                        {
                            throw Error::ParsingError(loc, "duplicate 'case' clause");
                        }
                        if(stmt->m_itemvals[i] && stmt->m_itemvals[j])
                        {
                            if(stmt->m_itemvals[i]->m_val.isEqual(stmt->m_itemvals[j]->m_val))
                            {
                                throw Error::ParsingError(stmt->m_itemvals[j]->location(), "expected a constant value for 'case' clause");
                            }
                        }
                    }
                }
            }
            return stmt;
        }
        // 'throw' Expr17 ';'
        if(tryParseSymbol(Token::Type::Throw))
        {
            auto stmt = std::make_unique<AST::ThrowStatement>(loc);
            MUST_PARSE(stmt->m_thrownexpr = TryParseExpr17(), "expected expression after 'throw'");
            MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';'");
            return stmt;
        }
        // 'try' Statement ( ( 'catch' '(' TOKEN_IDENTIFIER ')' Statement [ 'finally' Statement ] ) | ( 'finally' Statement ) )
        if(tryParseSymbol(Token::Type::Try))
        {
            auto stmt = std::make_unique<AST::TryStatement>(loc);
            MUST_PARSE(stmt->m_tryblock = tryParseStatement(), "expected statement after 'try'");
            if(tryParseSymbol(Token::Type::Finally))
            {
                MUST_PARSE(stmt->m_finallyblock = tryParseStatement(), "expected statement after 'finally'");
            }
            else
            {
                MUST_PARSE(tryParseSymbol(Token::Type::Catch), "Expected 'catch' or 'finally'.");
                MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketOpen), "expected '(' after 'catch'");
                MUST_PARSE(!(stmt->m_exvarname = tryParseIdentifier()).empty(), "expected identifier for 'catch' clause");
                MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), "expected terminating ')' for 'catch' clause");
                MUST_PARSE(stmt->m_catchblock = tryParseStatement(), "expected statement after 'catch'");
                if(tryParseSymbol(Token::Type::Finally))
                    MUST_PARSE(stmt->m_finallyblock = tryParseStatement(), "expected statement after 'finally'");
            }
            return stmt;
        }
        // Expression as statement: Expr17 ';'
        auto expr = TryParseExpr17();
        if(expr)
        {
            MUST_PARSE(tryParseSymbol(Token::Type::Semicolon), "expected ';'");
            return expr;
        }
        // Syntactic sugar for functions:
        // 'function' IdentifierValue '(' [ TOKEN_IDENTIFIER ( ',' TOKE_IDENTIFIER )* ] ')' '{' Block '}'
        auto fnsynsugar = tryParseFuncSynSugar();
        if(fnsynsugar.second)
        {
            auto idexpr = std::make_unique<AST::Identifier>(loc, AST::Identifier::Scope::None, std::move(fnsynsugar.first));
            auto assgp = std::make_unique<AST::BinaryOperator>(loc, AST::BinaryOperator::Type::Assignment);
            assgp->m_leftoper = std::move(idexpr);
            assgp->m_rightoper = std::move(fnsynsugar.second);
            return assgp;
        }
        auto cls = tryParseClassSynSugar();
        if(cls)
        {
            return cls;
        }
        return {};
    }

    std::unique_ptr<AST::ConstantValue> Parser::tryParseConstVal()
    {
        const Token& t = m_toklist[m_tokidx];
        switch(t.type())
        {
            case Token::Type::Number:
                ++m_tokidx;
                return std::make_unique<AST::ConstantValue>(t.m_place, Value{ t.number() });
            case Token::Type::String:
                ++m_tokidx;
                return std::make_unique<AST::ConstantValue>(t.m_place, Value{ std::string(t.string()) });
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
        const auto& t = m_toklist[m_tokidx];
        if(t.type() == Token::Type::Local || t.type() == Token::Type::Global)
        {
            ++m_tokidx;
            MUST_PARSE(tryParseSymbol(Token::Type::Dot), "expected '.'");
            MUST_PARSE(m_tokidx < m_toklist.size(), "expected identifier after '.'");
            const auto& tident = m_toklist[m_tokidx++];
            MUST_PARSE(tident.type() == Token::Type::Identifier, "expected identifier after '.'");
            AST::Identifier::Scope identifierScope = AST::Identifier::Scope::Count;
            switch(t.type())
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
            return std::make_unique<AST::Identifier>(t.m_place, identifierScope, std::string(tident.string()));
        }
        if(t.type() == Token::Type::Identifier)
        {
            ++m_tokidx;
            return std::make_unique<AST::Identifier>(t.m_place, AST::Identifier::Scope::None, std::string(t.string()));
        }
        return {};
    }

    std::unique_ptr<AST::ConstantExpression> Parser::tryParseConstExpr()
    {
        auto rconst = tryParseConstVal();
        if(rconst)
        {
            return rconst;
        }
        auto rident = tryParseIdentVal();
        if(rident)
        {
            return rident;
        }
        auto loc = m_toklist[m_tokidx].m_place;
        if(tryParseSymbol(Token::Type::This))
        {
            return std::make_unique<AST::ThisExpression>(loc);
        }
        return {};
    }

    std::pair<std::string, std::unique_ptr<AST::FunctionDefinition>> Parser::tryParseFuncSynSugar()
    {
        std::pair<std::string, std::unique_ptr<AST::FunctionDefinition>> result;
        if(peekSymbols({ Token::Type::Function, Token::Type::Identifier }))
        {
            ++m_tokidx;
            result.first = m_toklist[m_tokidx++].string();
            result.second = std::make_unique<AST::FunctionDefinition>(currentTokenLocation());
            parseFuncDef(*result.second);
            return result;
        }
        return std::make_pair(std::string{}, std::unique_ptr<AST::FunctionDefinition>{});
    }

    std::unique_ptr<AST::Expression> Parser::tryParseObjMember(std::string& outname)
    {
        if(peekSymbols({ Token::Type::String, Token::Type::Colon }) || peekSymbols({ Token::Type::Identifier, Token::Type::Colon }))
        {
            outname = m_toklist[m_tokidx].string();
            m_tokidx += 2;
            return TryParseExpr16();
        }
        return {};
    }

    std::unique_ptr<AST::Expression> Parser::tryParseClassSynSugar()
    {
        std::unique_ptr<AST::Expression> baseexpr;
        auto beginplace = currentTokenLocation();
        if(tryParseSymbol(Token::Type::Class))
        {
            std::string clname = tryParseIdentifier();
            MUST_PARSE(!clname.empty(), "expected identifier after 'class'");
            auto assgp = std::make_unique<AST::BinaryOperator>(beginplace, AST::BinaryOperator::Type::Assignment);
            assgp->m_leftoper = std::make_unique<AST::Identifier>(beginplace, AST::Identifier::Scope::None, std::move(clname));
            
            if(tryParseSymbol(Token::Type::Colon))
            {
                baseexpr = TryParseExpr16();
                MUST_PARSE(baseexpr, "expected expression after ':' in class definition");
            }
            auto objexpr = tryParseObject();
            objexpr->m_baseexpr = std::move(baseexpr);
            assgp->m_rightoper = std::move(objexpr);
            MUST_PARSE(assgp->m_rightoper, "expected object");
            return assgp;
        }
        return std::unique_ptr<AST::ObjectExpression>();
    }

    std::unique_ptr<AST::ObjectExpression> Parser::tryParseObject()
    {
        if(peekSymbols({ Token::Type::CurlyBracketOpen, Token::Type::CurlyBracketClose }) ||// { }
           peekSymbols({ Token::Type::CurlyBracketOpen, Token::Type::String, Token::Type::Colon }) ||// { 'key' :
           peekSymbols({ Token::Type::CurlyBracketOpen, Token::Type::Identifier, Token::Type::Colon }))// { key :
        {
            auto objexpr = std::make_unique<AST::ObjectExpression>(currentTokenLocation());
            tryParseSymbol(Token::Type::CurlyBracketOpen);
            if(!tryParseSymbol(Token::Type::CurlyBracketClose))
            {
                std::string memberName;
                std::unique_ptr<AST::Expression> memberValue;
                MUST_PARSE(memberValue = tryParseObjMember(memberName), "expected object member");
                MUST_PARSE(objexpr->m_exprmap.insert(std::make_pair(std::move(memberName), std::move(memberValue))).second,
                           "duplicate object member");
                if(!tryParseSymbol(Token::Type::CurlyBracketClose))
                {
                    while(tryParseSymbol(Token::Type::Comma))
                    {
                        if(tryParseSymbol(Token::Type::CurlyBracketClose))
                            return objexpr;
                        MUST_PARSE(memberValue = tryParseObjMember(memberName), "expected object member");
                        MUST_PARSE(objexpr->m_exprmap.insert(std::make_pair(std::move(memberName), std::move(memberValue))).second,
                                   "duplicate object member");
                    }
                    MUST_PARSE(tryParseSymbol(Token::Type::CurlyBracketClose), "expected '}' after object definition");
                }
            }
            return objexpr;
        }
        return {};
    }

    std::unique_ptr<AST::ArrayExpression> Parser::tryParseArray()
    {
        if(tryParseSymbol(Token::Type::SquareBracketOpen))
        {
            auto arrexpr = std::make_unique<AST::ArrayExpression>(currentTokenLocation());
            if(!tryParseSymbol(Token::Type::SquareBracketClose))
            {
                auto itemval = TryParseExpr16();
                MUST_PARSE(itemval, "expected expression in array literal");
                arrexpr->m_exprlist.push_back((std::move(itemval)));
                if(!tryParseSymbol(Token::Type::SquareBracketClose))
                {
                    while(tryParseSymbol(Token::Type::Comma))
                    {
                        if(tryParseSymbol(Token::Type::SquareBracketClose))
                            return arrexpr;
                        MUST_PARSE(itemval = TryParseExpr16(), "expected expression in array literal");
                        arrexpr->m_exprlist.push_back((std::move(itemval)));
                    }
                    MUST_PARSE(tryParseSymbol(Token::Type::SquareBracketClose), "expected terminating ']' after array literal");
                }
            }
            return arrexpr;
        }
        return {};
    }

    std::unique_ptr<AST::Expression> Parser::tryParseParentheses()
    {
        const Location loc = currentTokenLocation();
        // '(' Expr17 ')'
        if(tryParseSymbol(Token::Type::RoundBracketOpen))
        {
            std::unique_ptr<AST::Expression> expr;
            MUST_PARSE(expr = TryParseExpr17(), "expected expression");
            MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), "expected ')'");
            return expr;
        }
        if(auto objexpr = tryParseObject())
        {
            return objexpr;
        }
        if(auto arrexpr = tryParseArray())
        {
            return arrexpr;
        }
        // 'function' '(' [ TOKEN_IDENTIFIER ( ',' TOKE_IDENTIFIER )* ] ')' '{' Block '}'
        if(m_toklist[m_tokidx].type() == Token::Type::Function && m_toklist[m_tokidx + 1].type() == Token::Type::RoundBracketOpen)
        {
            ++m_tokidx;
            auto func = std::make_unique<AST::FunctionDefinition>(loc);
            parseFuncDef(*func);
            return func;
        }
        // Constant
        auto constant = tryParseConstExpr();
        if(constant)
        {
            return constant;
        }
        return {};
    }

    std::unique_ptr<AST::Expression> Parser::tryParseUnary()
    {
        auto expr = tryParseParentheses();
        if(!expr)
        {
            return {};
        }
        for(;;)
        {
            auto loc = currentTokenLocation();
            // Postincrementation: Expr0 '++'
            if(tryParseSymbol(Token::Type::DoublePlus))
            {
                auto op = std::make_unique<AST::UnaryOperator>(loc, AST::UnaryOperator::Type::Postincrementation);
                op->m_operand = std::move(expr);
                expr = std::move(op);
            }
            // Postdecrementation: Expr0 '--'
            else if(tryParseSymbol(Token::Type::DoubleDash))
            {
                auto op = std::make_unique<AST::UnaryOperator>(loc, AST::UnaryOperator::Type::Postdecrementation);
                op->m_operand = std::move(expr);
                expr = std::move(op);
            }
            // Call: Expr0 '(' [ Expr16 ( ',' Expr16 )* ')'
            else if(tryParseSymbol(Token::Type::RoundBracketOpen))
            {
                auto op = std::make_unique<AST::CallOperator>(loc);
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
                        MUST_PARSE(expr = TryParseExpr16(), "expected expression");
                        op->m_oplist.push_back(std::move(expr));
                    }
                }
                MUST_PARSE(tryParseSymbol(Token::Type::RoundBracketClose), "expected ')'");
                expr = std::move(op);
            }
            // Indexing: Expr0 '[' Expr17 ']'
            else if(tryParseSymbol(Token::Type::SquareBracketOpen))
            {
                auto op = std::make_unique<AST::BinaryOperator>(loc, AST::BinaryOperator::Type::Indexing);
                op->m_leftoper = std::move(expr);
                MUST_PARSE(op->m_rightoper = TryParseExpr17(), "expected expression");
                MUST_PARSE(tryParseSymbol(Token::Type::SquareBracketClose), "expected ']'");
                expr = std::move(op);
            }
            // Member access: Expr2 '.' TOKEN_IDENTIFIER
            else if(tryParseSymbol(Token::Type::Dot))
            {
                auto op = std::make_unique<AST::MemberAccessOperator>(loc);
                op->m_operand = std::move(expr);
                auto identifier = tryParseIdentVal();
                MUST_PARSE(identifier && identifier->m_scope == AST::Identifier::Scope::None, "expected identifier");
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
        const Location loc = currentTokenLocation();
    #define PARSE_UNARY_OPERATOR(symbol, unaryOperatorType)                               \
        if(tryParseSymbol(symbol))                                                        \
        {                                                                                 \
            auto op = std::make_unique<AST::UnaryOperator>(loc, (unaryOperatorType));        \
            MUST_PARSE(op->m_operand = tryParseOperator(), "expected expression"); \
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
            auto op = std::make_unique<AST::BinaryOperator>(loc, (binaryOperatorType));            \
            op->m_leftoper = std::move(expr);                                                  \
            MUST_PARSE(op->m_rightoper = (exprParseFunc)(), "expected expression"); \
            expr = std::move(op);                                                               \
        }

    std::unique_ptr<AST::Expression> Parser::tryParseBinary()
    {
        auto expr = tryParseOperator();
        if(!expr)
        {
            return {};
        }
        for(;;)
        {
            auto loc = currentTokenLocation();
            if(tryParseSymbol(Token::Type::Asterisk))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Mul, tryParseOperator)
            }
            else if(tryParseSymbol(Token::Type::Slash))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Div, tryParseOperator)
            }
            else if(tryParseSymbol(Token::Type::Percent))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Mod, tryParseOperator)
            }
            else
            {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::tryParseAddSub()
    {
        auto expr = tryParseBinary();
        if(!expr)
        {
            return {};
        }
        for(;;)
        {
            auto loc = currentTokenLocation();
            if(tryParseSymbol(Token::Type::Plus))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Add, tryParseBinary)
            }
            else if(tryParseSymbol(Token::Type::Dash))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Sub, tryParseBinary)
            }
            else
            {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::tryParseAngleSign()
    {
        auto expr = tryParseAddSub();
        if(!expr)
        {
            return {};
        }
        for(;;)
        {
            auto loc = currentTokenLocation();
            if(tryParseSymbol(Token::Type::DoubleLess))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::ShiftLeft, tryParseAddSub)
            }
            else if(tryParseSymbol(Token::Type::DoubleGreater))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::ShiftRight, tryParseAddSub)
            }
            else
            {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::tryParseAngleCompare()
    {
        auto expr = tryParseAngleSign();
        if(!expr)
        {
            return {};
        }
        for(;;)
        {
            const Location loc = currentTokenLocation();
            if(tryParseSymbol(Token::Type::Less))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Less, tryParseAngleSign)
            }
            else if(tryParseSymbol(Token::Type::LessEquals))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::LessEqual, tryParseAngleSign)
            }
            else if(tryParseSymbol(Token::Type::Greater))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Greater, tryParseAngleSign)
            }
            else if(tryParseSymbol(Token::Type::GreaterEquals))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::GreaterEqual, tryParseAngleSign)
            }
            else
            {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::tryParseEquals()
    {
        auto expr = tryParseAngleCompare();
        if(!expr)
        {
            return {};
        }
        for(;;)
        {
            auto loc = currentTokenLocation();
            if(tryParseSymbol(Token::Type::DoubleEquals))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Equal, tryParseAngleCompare)
            }
            else if(tryParseSymbol(Token::Type::ExclamationEquals))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::NotEqual, tryParseAngleCompare)
            }
            else
            {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr11()
    {
        auto expr = tryParseEquals();
        if(!expr)
        {
            return {};
        }
        for(;;)
        {
            auto loc = currentTokenLocation();
            if(tryParseSymbol(Token::Type::Amperstand))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::BitwiseAnd, tryParseEquals)
            }
            else
            {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr12()
    {
        auto expr = TryParseExpr11();
        if(!expr)
        {
            return {};
        }
        for(;;)
        {
            auto loc = currentTokenLocation();
            if(tryParseSymbol(Token::Type::Caret))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::BitwiseXor, TryParseExpr11)
            }
            else
            {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr13()
    {
        auto expr = TryParseExpr12();
        if(!expr)
        {
            return {};
        }
        for(;;)
        {
            const Location loc = currentTokenLocation();
            if(tryParseSymbol(Token::Type::Pipe))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::BitwiseOr, TryParseExpr12)
            }
            else
            {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr14()
    {
        auto expr = TryParseExpr13();
        if(!expr)
        {
            return {};
        }
        for(;;)
        {
            auto loc = currentTokenLocation();
            if(tryParseSymbol(Token::Type::DoubleAmperstand))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::LogicalAnd, TryParseExpr13)
            }
            else
            {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr15()
    {
        auto expr = TryParseExpr14();
        if(!expr)
        {
            return {};
        }
        for(;;)
        {
            auto loc = currentTokenLocation();
            if(tryParseSymbol(Token::Type::DoublePipe))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::LogicalOr, TryParseExpr14)
            }
            else
            {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<AST::Expression> Parser::TryParseExpr16()
    {
        auto expr = TryParseExpr15();
        if(!expr)
        {
            return {};
        }
        // Ternary operator: Expr15 '?' Expr16 ':' Expr16
        if(tryParseSymbol(Token::Type::QuestionMark))
        {
            auto op = std::make_unique<AST::TernaryOperator>(currentTokenLocation());
            op->m_condexpr = std::move(expr);
            MUST_PARSE(op->m_trueexpr = TryParseExpr16(), "expected expression");
            MUST_PARSE(tryParseSymbol(Token::Type::Colon), "expected ':'");
            MUST_PARSE(op->m_falseexpr = TryParseExpr16(), "expected expression");
            return op;
        }
        // Assignment: Expr15 = Expr16, and variants like += -=
    #define TRY_PARSE_ASSIGNMENT(symbol, binaryOperatorType)                                          \
        if(tryParseSymbol(symbol))                                                                    \
        {                                                                                             \
            auto op = std::make_unique<AST::BinaryOperator>(currentTokenLocation(), (binaryOperatorType)); \
            op->m_leftoper = std::move(expr);                                                        \
            MUST_PARSE(op->m_rightoper = TryParseExpr16(), "expected expression");        \
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
        auto expr = TryParseExpr16();
        if(!expr)
        {
            return {};
        }
        for(;;)
        {
            auto loc = currentTokenLocation();
            if(tryParseSymbol(Token::Type::Comma))
            {
                PARSE_BINARY_OPERATOR(AST::BinaryOperator::Type::Comma, TryParseExpr16)
            }
            else
            {
                break;
            }
        }
        return expr;
    }

    bool Parser::tryParseSymbol(Token::Type symbol)
    {
        if(m_tokidx < m_toklist.size() && m_toklist[m_tokidx].type() == symbol)
        {
            ++m_tokidx;
            return true;
        }
        return false;
    }

    std::string Parser::tryParseIdentifier()
    {
        if(m_tokidx < m_toklist.size() && m_toklist[m_tokidx].type() == Token::Type::Identifier)
        {
            return m_toklist[m_tokidx++].string();
        }
        return {};
    }
}

