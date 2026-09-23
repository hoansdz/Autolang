#ifndef NODE_CPP
#define NODE_CPP

#include "frontend/parser/node/Node.cpp"
#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace Autolang {

ExprNode::ExprNode(NodeType kind, uint32_t line) : line(line), kind(kind) {
	mode = ParserContext::mode;
}

void ExprNode::loadOpcodeLine(in_func, std::vector<uint8_t> &bytecodes) {
	if (context.currentFunctionId == context.mainFunctionId) {
		compile.allMainFunctionOpcodeLines.emplace_back(
		    line, bytecodes.size() - context.currentBytecodePos,
		    mode->path.c_str());
		// std::cerr << compile.allMainFunctionOpcodeLines.back().path << ":"
		//           << compile.allMainFunctionOpcodeLines.back().line << "\n";
		return;
	}
	if (!context.currentAllOpcodeLine->empty() &&
	    context.currentAllOpcodeLine->back().line == line) {
		return;
	}
	context.currentAllOpcodeLine->emplace_back(
	    line, bytecodes.size() - context.currentBytecodePos);
}

void ExprNode::throwError(std::string message) {
	ParserContext::mode = mode;
	throw ParserError(line, message);
}

void ExprNode::warning(in_func, std::string message) {
	auto lastMode = context.mode;
	context.mode = mode;
	context.warning(line, message);
	context.mode = lastMode;
}

bool ExprNode::canCast(in_func, ClassDeclaration *from, ClassDeclaration *to) {
	if (from->classId == DefaultClass::functionClassId) {
		if (to->classId != DefaultClass::functionClassId) {
			return false;
		}
		if (from->inputClassId.size() != to->inputClassId.size()) {
			return false;
		}
		for (int i = 0; i < from->inputClassId.size(); ++i) {
			if (from->inputClassId[i]->isSame(to->inputClassId[i])) {
				continue;
			}
			return false;
		}
		return true;
	}
	if (to->classId != DefaultClass::functionClassId) {
		if (!from->classId || !to->classId) return false;
		if (!to->nullable && from->nullable) return false;
		return canCast(in_data, *from->classId, *to->classId);
	}
	return false;
}

bool ExprNode::canCast(in_func, ClassId from, ClassId to) {
	if (from == to) {
		return true;
	}
	switch (to) {
		case DefaultClass::anyClassId:
			return true;
		case DefaultClass::intClassId:
			return from == DefaultClass::boolClassId;
		case DefaultClass::floatClassId: {
			return from == DefaultClass::intClassId ||
			       from == DefaultClass::boolClassId;
		}
		default: {
			if (from >= compile.classes.size() || !compile.classes[from]) return false;
			auto clazz = compile.classes[from];
			if (clazz->inheritance.get(to)) return true;
			if (to < compile.classes.size() && compile.classes[to]) {
				auto toClazz = compile.classes[to];
				if (toClazz->genericBaseClassId != 0 &&
				    clazz->genericBaseClassId == toClazz->genericBaseClassId) {
					auto infoFrom = context.classInfo[from];
					auto infoTo = context.classInfo[to];
					if (infoFrom && infoTo &&
					    infoFrom->genericTypeId.size() == infoTo->genericTypeId.size()) {
						bool allMatch = true;
						for (size_t i = 0; i < infoFrom->genericTypeId.size(); ++i) {
							auto gFrom = infoFrom->genericTypeId[i];
							auto gTo = infoTo->genericTypeId[i];
							if (!gTo->nullable && gFrom->nullable) {
								allMatch = false;
								break;
							}
							if (!canCast(in_data, *gFrom->classId, *gTo->classId)) {
								allMatch = false;
								break;
							}
						}
						if (allMatch) return true;
					}
				}
			}
			return false;
		}
	}
}

ClassId ExprNode::getCommonSuperType(in_func, ClassId a, ClassId b) {
	if (a == b) return a;
	if (a == DefaultClass::nullClassId) return b;
	if (b == DefaultClass::nullClassId) return a;
	if (a == DefaultClass::anyClassId || b == DefaultClass::anyClassId) {
		return DefaultClass::anyClassId;
	}
	// Numeric promotion
	if ((a == DefaultClass::intClassId || a == DefaultClass::boolClassId) &&
	    b == DefaultClass::floatClassId) {
		return DefaultClass::floatClassId;
	}
	if ((b == DefaultClass::intClassId || b == DefaultClass::boolClassId) &&
	    a == DefaultClass::floatClassId) {
		return DefaultClass::floatClassId;
	}
	if (a == DefaultClass::boolClassId && b == DefaultClass::intClassId) {
		return DefaultClass::intClassId;
	}
	if (b == DefaultClass::boolClassId && a == DefaultClass::intClassId) {
		return DefaultClass::intClassId;
	}

	// Check direct inheritance / covariance
	if (canCast(in_data, a, b)) {
		return b;
	}
	if (canCast(in_data, b, a)) {
		return a;
	}

	// Walk ancestor chains to find lowest common ancestor
	if (a < compile.classes.size() && compile.classes[a] &&
	    b < compile.classes.size() && compile.classes[b]) {
		SmallVector<ClassId, 8> ancestorsA;
		ClassId cur = a;
		while (cur != 0 && cur < compile.classes.size() && compile.classes[cur]) {
			ancestorsA.push_back(cur);
			cur = compile.classes[cur]->parentId;
		}
		cur = b;
		while (cur != 0 && cur < compile.classes.size() && compile.classes[cur]) {
			for (auto anc : ancestorsA) {
				if (cur == anc) {
					return cur;
				}
			}
			cur = compile.classes[cur]->parentId;
		}
	}

	return DefaultClass::anyClassId;
}

ClassDeclaration *ExprNode::getOrCreateClassDeclaration(in_func, ClassId classId, uint32_t line, bool nullable) {
	auto decl = context.classDeclarationAllocator.push();
	decl->classId = classId;
	if (classId < compile.classes.size() && compile.classes[classId]) {
		decl->baseClassLexerStringId = context.createLexerStringIfNotExists(compile.classes[classId]->getName(compile));
	} else {
		decl->baseClassLexerStringId = context.createLexerStringIfNotExists("Any");
	}
	decl->line = line;
	decl->nullable = nullable;
	decl->isGeneric = false;
	return decl;
}

#define STRINGIFY(x) #x
#define GET_NODE_TYPE_CASE(x)                                                  \
	case x:                                                                    \
		return STRINGIFY(x);

std::string ExprNode::getNodeType() {
	switch (kind) {
		GET_NODE_TYPE_CASE(UNKNOW)
		GET_NODE_TYPE_CASE(VAR)
		GET_NODE_TYPE_CASE(BINARY)
		GET_NODE_TYPE_CASE(CONST_VAL)
		GET_NODE_TYPE_CASE(GET_PROP)
		GET_NODE_TYPE_CASE(DECLARATION)
		GET_NODE_TYPE_CASE(CALL)
		GET_NODE_TYPE_CASE(CAST)
		GET_NODE_TYPE_CASE(SET)
		GET_NODE_TYPE_CASE(UNARY)
		GET_NODE_TYPE_CASE(IF)
		GET_NODE_TYPE_CASE(BLOCK)
		GET_NODE_TYPE_CASE(WHILE)
		GET_NODE_TYPE_CASE(FOR)
		GET_NODE_TYPE_CASE(CREATE_FUNC)
		GET_NODE_TYPE_CASE(CREATE_CLASS)
		GET_NODE_TYPE_CASE(CREATE_MAP)
		GET_NODE_TYPE_CASE(PAIR)
		GET_NODE_TYPE_CASE(CREATE_CONSTRUCTOR)
		GET_NODE_TYPE_CASE(RET)
		GET_NODE_TYPE_CASE(SKIP)
		GET_NODE_TYPE_CASE(CLASS_ACCESS)
		GET_NODE_TYPE_CASE(OPTIONAL_ACCESS)
		GET_NODE_TYPE_CASE(NULL_COALESCING)
		GET_NODE_TYPE_CASE(TRY_CATCH)
		GET_NODE_TYPE_CASE(THROW)
		GET_NODE_TYPE_CASE(RUNTIME_CAST)
		GET_NODE_TYPE_CASE(GENERIC_DECLARATION)
		GET_NODE_TYPE_CASE(FUNCTION_ACCESS)
		GET_NODE_TYPE_CASE(WHEN)
		GET_NODE_TYPE_CASE(GET_POINTER)
		default:
			break;
	}

	return "UNKNOWN_NODE_TYPE";
}

// void ExprNode::deleteNode(ExprNode* node) {
// if (!node) return;
// switch (node->kind) {
// 	case NodeType::IF:
// 	case NodeType::WHILE:
// 	case NodeType::TRY_CATCH:
// 	case NodeType::THROW:
// 	case NodeType::RET:
// 	case NodeType::SET:
// 	case NodeType::DECLARATION:
// 	case NodeType::CREATE_FUNC:
// 	case NodeType::CREATE_CLASS:
// 	case NodeType::CREATE_CONSTRUCTOR:
// 	// case NodeType::BINARY:
// 	{
// 		return;
// 	}
// 	default: break;
// }
// // std::cerr<<std::to_string(node->kind)<<'\n';
// delete node;
// }

ReturnNode::~ReturnNode() { deleteNode(value); }

BinaryNode::~BinaryNode() {
	deleteNode(left);
	deleteNode(right);
}

WhileNode::~WhileNode() { deleteNode(condition); }

} // namespace Autolang

#endif