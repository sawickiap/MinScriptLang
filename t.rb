

words=[
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
]


src=<<'__eos__'
                    Mul,
                    Div,
                    Mod,
                    Add,
                    Sub,
                    ShiftLeft,
                    ShiftRight,
                    Assignment,
                    AssignmentAdd,
                    AssignmentSub,
                    AssignmentMul,
                    AssignmentDiv,
                    AssignmentMod,
                    AssignmentShiftLeft,
                    AssignmentShiftRight,
                    AssignmentBitwiseAnd,
                    AssignmentBitwiseXor,
                    AssignmentBitwiseOr,
                    Less,
                    Greater,
                    LessEqual,
                    GreaterEqual,
                    Equal,
                    NotEqual,
                    BitwiseAnd,
                    BitwiseXor,
                    BitwiseOr,
                    LogicalAnd,
                    LogicalOr,
                    Comma,
                    Indexing,
                    Count
__eos__

enumnames = src.strip.split(",").map(&:strip).reject(&:empty?)

i = 0
while i < words.length do
  word = words[i]
  enum = enumnames[i]
  printf("    {  BinaryOperator::Type::%s, %p },\n", enum, word)
  i += 1
end

