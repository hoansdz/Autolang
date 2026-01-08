#ifndef OPTIMIZE_NODE_HPP
#define OPTIMIZE_NODE_HPP

#define in_func CompiledProgram& compile, ParserContext& context
#define in_data compile, context
#define put_bytecode(byte) context.currentFunction->bytecodes.emplace_back(byte)
#define put_bytecode_u32(i) put_opcode_u32(context.currentFunction->bytecodes, i)

struct CompiledProgram;

namespace AutoLang {
	
struct ConstValueNode;
struct ParserContext;

ConstValueNode* plus(ConstValueNode* left, ConstValueNode* right);
ConstValueNode* minus(ConstValueNode* left, ConstValueNode* right);
ConstValueNode* mul(ConstValueNode* left, ConstValueNode* right);
ConstValueNode* divide(ConstValueNode* left, ConstValueNode* right);
ConstValueNode* mod(ConstValueNode* left, ConstValueNode* right);
ConstValueNode* bitwise_and(ConstValueNode* left, ConstValueNode* right);
ConstValueNode* bitwise_or(ConstValueNode* left, ConstValueNode* right);
ConstValueNode* op_less_than(ConstValueNode* left, ConstValueNode* right);
ConstValueNode* op_greater_than(ConstValueNode* left, ConstValueNode* right);
ConstValueNode* op_less_than_eq(ConstValueNode* left, ConstValueNode* right);
ConstValueNode* op_greater_than_eq(ConstValueNode* left, ConstValueNode* right);
ConstValueNode* op_eqeq(ConstValueNode* left, ConstValueNode* right);
ConstValueNode* op_not_eq(ConstValueNode* left, ConstValueNode* right);
void toInt(ConstValueNode* value);
void toFloat(ConstValueNode* value);

}

#endif