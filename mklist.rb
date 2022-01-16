
types = ["None", "Identifier", "Number", "String", "End", "Comma", "QuestionMark", "Colon", "Semicolon", "RoundBracketOpen", "RoundBracketClose", "SquareBracketOpen", "SquareBracketClose", "CurlyBracketOpen", "CurlyBracketClose", "Asterisk", "Slash", "Percent", "Plus", "Dash", "Equals", "ExclamationMark", "Tilde", "Less", "Greater", "Amperstand", "Caret", "Pipe", "Dot", "DoublePlus", "DoubleDash", "PlusEquals", "DashEquals", "AsteriskEquals", "SlashEquals", "PercentEquals", "DoubleLessEquals", "DoubleGreaterEquals", "AmperstandEquals", "CaretEquals", "PipeEquals", "DoubleLess", "DoubleGreater", "LessEquals", "GreaterEquals", "DoubleEquals", "ExclamationEquals", "DoubleAmperstand", "DoublePipe", "Null", "False", "True", "If", "Else", "While", "Do", "For", "Break", "Continue", "Switch", "Case", "Default", "Function", "Return", "Local", "This", "Global", "Class", "Throw", "Try", "Catch", "Finally", "Count"]

symstrings = [
        "", "", "", "", "",
        ",", "?", ":", ";", "(", ")", "[", "]", "{", "}", "*", "/", "%", "+", "-", "=", "!", "~", "<", ">", "&", "^", "|", ".",
        "++", "--", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
        "null", "false", "true", "if", "else", "while", "do", "for", "break", "continue", "switch", "case", "default",
        "function", "return", "local", "this", "global", "class", "throw", "try", "catch", "finally"
]

begin
  i = 0
  while i != symstrings.length do
    sym = symstrings[i]
    typ = types[i]
    printf("    { Token::Type::%s, %p},\n", typ, sym);
    i += 1
  end
end
