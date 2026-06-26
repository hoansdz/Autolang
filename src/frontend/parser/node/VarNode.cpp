#ifndef VAR_NODE_CPP
#define VAR_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

void VarNode::optimize(in_func) {
	// std::cerr << "loaded " << declaration->name << " "
	//           << compile.classes[declaration->classId]->name << "\n";
	classId = declaration->classId;
	isVal = declaration->isVal;
	classDeclaration = declaration->classDeclaration;
	if (nullable) {
		nullable = declaration->nullable; // #
	}
}

ExprNode *VarNode::copy(in_func) {
	DeclarationNode *newDeclaration;
	auto funcInfo = context.getCurrentFunctionInfo(in_data);
	auto it = funcInfo->reflectDeclarationMap.find(declaration);
	if (it != funcInfo->reflectDeclarationMap.end()) {
		newDeclaration = it->second;
	} else {
		newDeclaration =
		    static_cast<DeclarationNode *>(declaration->copy(in_data));
	}
	auto newNode =
	    context.varPool.push(line, newDeclaration, isStore, nullable);
	newNode->classId = classId;
	return newNode;
}

bool VarNode::isStaticValue() { return declaration && declaration->isGlobal; }

void VarNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (isStore) {
		bytecodes.emplace_back(declaration->isGlobal ? Opcode::STORE_GLOBAL
		                                             : Opcode::STORE_LOCAL);
		put_opcode_u32(bytecodes, declaration->id);
	} else {
		bytecodes.emplace_back(declaration->isGlobal ? Opcode::LOAD_GLOBAL
		                                             : Opcode::LOAD_LOCAL);
		put_opcode_u32(bytecodes, declaration->id);
		if (cloneable) {
			switch (classId) {
				case DefaultClass::intClassId:
				case DefaultClass::floatClassId: {
					bytecodes.emplace_back(Opcode::CLONE);
					break;
				}
				default: {
					break;
				}
			}
		}
	}
}
} // namespace AutoLang

#endif