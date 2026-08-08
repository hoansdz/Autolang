#ifndef CAST_NODE_CPP
#define CAST_NODE_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "frontend/parser/node/CreateNode.hpp"
#include "frontend/parser/node/NodePutBytecode.hpp"

namespace Autolang {

ExprNode *CastNode::resolve(in_func) {
	value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	if (value->classId == classId) {
		switch (classId) {
			case Autolang::DefaultClass::intClassId:
			case Autolang::DefaultClass::floatClassId: {
				break;
			}
			default: {
				auto result = value;
				value = nullptr;
				ExprNode::deleteNode(this);
				return result;
			}
		}
	}
	if (classId == DefaultClass::anyClassId) {
		return this;
	}
	try {
		switch (value->kind) {
			case NodeType::CONST_VAL: {
				auto node = static_cast<ConstValueNode *>(value);
				switch (classId) {
					case Autolang::DefaultClass::intClassId: {
						auto result = toInt(in_data, node);
						value = nullptr;
						ExprNode::deleteNode(this);
						return result;
					}
					case Autolang::DefaultClass::floatClassId: {
						auto result = toFloat(in_data, node);
						value = nullptr;
						ExprNode::deleteNode(this);
						return result;
					}
					case Autolang::DefaultClass::boolClassId: {
						auto result = toBool(in_data, node);
						value = nullptr;
						ExprNode::deleteNode(this);
						return result;
					}
					default:
						break;
				}
				throw std::runtime_error("");
			}
			case NodeType::CAST: {
				if (value->classId == classId) {
					auto result = value;
					value = nullptr;
					ExprNode::deleteNode(this);
					return result;
				}
				break;
			}
			default:
				break;
		}
	} catch (const std::runtime_error &err) {
		goto errCast;
	}
	return this;
errCast:;
	throwError("Cannot cast " + compile.classes[value->classId]->getName(compile) + " to " +
	           compile.classes[classId]->getName(compile));
}

void CastNode::optimize(in_func) {
	value->optimize(in_data);
	if (value->classId == classId) {
		return;
	}
	if (classId == DefaultClass::anyClassId) {
		return;
	}
	switch (classId) {
		case Autolang::DefaultClass::intClassId: {
			switch (value->classId) {
				case Autolang::DefaultClass::intClassId:
				case Autolang::DefaultClass::floatClassId: {
					return;
				}
				case Autolang::DefaultClass::boolClassId: {
					return;
				}
				default: {
					goto errCast;
				}
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (value->classId) {
				case Autolang::DefaultClass::intClassId:
				case Autolang::DefaultClass::floatClassId:
				case Autolang::DefaultClass::boolClassId: {
					return;
				}
				default: {
					throwError("Cannot cast " +
					           compile.classes[value->classId]->getName(compile) + " to " +
					           compile.classes[classId]->getName(compile));
				}
			}
			break;
		}
		case Autolang::DefaultClass::boolClassId: {
			switch (value->classId) {
				case Autolang::DefaultClass::intClassId:
				case Autolang::DefaultClass::floatClassId: {
					throwError(
					    "Type Error: Cannot cast Int/Float to Bool. Use "
					    "explicit comparison like 'value != 0' instead.");
				}
				case Autolang::DefaultClass::boolClassId: {
					return;
				}
				default: {
					throwError("Cannot cast " +
					           compile.classes[value->classId]->getName(compile) + " to " +
					           compile.classes[classId]->getName(compile));
				}
			}
			break;
		}
		default:
			// Extended class
			if (compile.classes[classId]->inheritance.get(value->classId) ||
			    compile.classes[value->classId]->inheritance.get(classId)) {
				return;
			}
			throwError("Cannot cast " + compile.classes[value->classId]->getName(compile) +
			           " to " + compile.classes[classId]->getName(compile));
	}
errCast:;
	throwError("Cannot cast " + compile.classes[value->classId]->getName(compile) + " to " +
	           compile.classes[classId]->getName(compile));
}

void CastNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	loadOpcodeLine(in_data, bytecodes);
	value->putBytecodes(in_data, bytecodes);
	switch (classId) {
		case Autolang::DefaultClass::intClassId: {
			switch (value->classId) {
				case Autolang::DefaultClass::intClassId: {
					bytecodes.emplace_back(Opcode::TO_INT);
					return;
				}
				case Autolang::DefaultClass::floatClassId: {
					bytecodes.emplace_back(Opcode::FLOAT_TO_INT);
					return;
				}
				case Autolang::DefaultClass::boolClassId: {
					bytecodes.emplace_back(Opcode::BOOL_TO_INT);
					return;
				}
				default: {
					throwError("Internal Compiler Error: Unsupported source type for Int cast");
				}
			}
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (value->classId) {
				case Autolang::DefaultClass::intClassId: {
					bytecodes.emplace_back(Opcode::INT_TO_FLOAT);
					return;
				}
				case Autolang::DefaultClass::floatClassId: {
					bytecodes.emplace_back(Opcode::TO_FLOAT);
					return;
				}
				case Autolang::DefaultClass::boolClassId: {
					bytecodes.emplace_back(Opcode::BOOL_TO_FLOAT);
					return;
				}
				default: {
					throwError("Internal Compiler Error: Unsupported source type for Float cast");
				}
			}
		}
		default:
			// Extended class
			return;
	}
}

ExprNode *CastNode::copy(in_func) {
	return context.castPool.push(
	    static_cast<HasClassIdNode *>(value->copy(in_data)), classId);
}

CastNode::~CastNode() { deleteNode(value); }

ExprNode *RuntimeCastNode::resolve(in_func) {
	value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	return this;
}

void RuntimeCastNode::optimize(in_func) {
	value->optimize(in_data);
	if (value->classId == classId ||
	    compile.classes[value->classId]->inheritance.get(classId)) {
		return;
	}
	if (classId == DefaultClass::anyClassId ||
	    value->classId == DefaultClass::anyClassId ||
	    compile.classes[classId]->inheritance.get(value->classId)) {
		return;
	}
	throwError("Cannot cast " + compile.classes[value->classId]->getName(compile) + " to " +
	           compile.classes[classId]->getName(compile) +
	           ": no inheritance relationship");
}

void RuntimeCastNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	loadOpcodeLine(in_data, bytecodes);
	value->putBytecodes(in_data, bytecodes);
	if (value->classId == classId ||
	    compile.classes[value->classId]->inheritance.get(classId) ||
	    classId == DefaultClass::anyClassId) {
		return;
	}
	bytecodes.emplace_back(nullable ? Opcode::SAFE_CAST : Opcode::UNSAFE_CAST);
	put_opcode_u32(bytecodes, classId);
}

ExprNode *RuntimeCastNode::copy(in_func) {
	return context.runtimeCastPool.push(
	    static_cast<HasClassIdNode *>(value->copy(in_data)), classId, nullable);
}

RuntimeCastNode::~RuntimeCastNode() { deleteNode(value); }

} // namespace Autolang
#endif