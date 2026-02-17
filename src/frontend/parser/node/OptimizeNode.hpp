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
struct CompiledProgram;

ConstValueNode* plus(in_func, ConstValueNode* left, ConstValueNode* right);
ConstValueNode* minus(in_func, ConstValueNode* left, ConstValueNode* right);
ConstValueNode* mul(in_func, ConstValueNode* left, ConstValueNode* right);
ConstValueNode* divide(in_func, ConstValueNode* left, ConstValueNode* right);
ConstValueNode* mod(in_func, ConstValueNode* left, ConstValueNode* right);
ConstValueNode* bitwise_and(in_func, ConstValueNode* left, ConstValueNode* right);
ConstValueNode* bitwise_or(in_func, ConstValueNode* left, ConstValueNode* right);
ConstValueNode* op_less_than(in_func, ConstValueNode* left, ConstValueNode* right);
ConstValueNode* op_greater_than(in_func, ConstValueNode* left, ConstValueNode* right);
ConstValueNode* op_less_than_eq(in_func, ConstValueNode* left, ConstValueNode* right);
ConstValueNode* op_greater_than_eq(in_func, ConstValueNode* left, ConstValueNode* right);
ConstValueNode* op_eqeq(in_func, ConstValueNode* left, ConstValueNode* right);
ConstValueNode* op_not_eq(in_func, ConstValueNode* left, ConstValueNode* right);

void toInt(in_func, ConstValueNode* value);
void toFloat(in_func, ConstValueNode* value);
void toBool(in_func, ConstValueNode* value);
void toString(in_func, ConstValueNode* value);

}

#endif