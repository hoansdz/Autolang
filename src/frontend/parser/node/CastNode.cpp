#ifndef CAST_NODE_CPP
#define CAST_NODE_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/CreateNode.hpp"
#include "frontend/parser/node/NodePutBytecode.hpp"

namespace AutoLang {

ExprNode *CastNode::resolve(in_func) {
	if (value->classId == classId) {
		switch (classId) {
			case AutoLang::DefaultClass::intClassId:
			case AutoLang::DefaultClass::floatClassId: {
				break;
			}
			default: {
				auto result = value;
				result->mode = mode;
				value = nullptr;
				ExprNode::deleteNode(this);
				return result;
			}
		}
	}
	try {
		switch (value->kind) {
			case NodeType::CONST: {
				auto node = static_cast<ConstValueNode *>(value);
				switch (classId) {
					case AutoLang::DefaultClass::intClassId: {
						toInt(node);
						auto result = value;
						value = nullptr;
						ExprNode::deleteNode(this);
						return result;
					}
					case AutoLang::DefaultClass::floatClassId: {
						toFloat(node);
						auto result = value;
						value = nullptr;
						ExprNode::deleteNode(this);
						return result;
					}
					case AutoLang::DefaultClass::boolClassId: {
						toBool(node);
						auto result = value;
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
		throwError("Cannot castb " + compile.classes[value->classId]->name +
		           " to " + compile.classes[classId]->name);
	}
	return this;
}

void CastNode::optimize(in_func) {
	value->optimize(in_data);
	if (value->classId == classId) {
		return;
	}
	switch (classId) {
		case AutoLang::DefaultClass::intClassId: {
			switch (value->classId) {
				case AutoLang::DefaultClass::intClassId:
				case AutoLang::DefaultClass::floatClassId:
				case AutoLang::DefaultClass::boolClassId: {
					return;
				}
				default: {
					throwError("Cannot castc " +
					           compile.classes[value->classId]->name + " to " +
					           compile.classes[classId]->name);
				}
			}
			break;
		}
		case AutoLang::DefaultClass::floatClassId: {
			switch (value->classId) {
				case AutoLang::DefaultClass::intClassId:
				case AutoLang::DefaultClass::floatClassId:
				case AutoLang::DefaultClass::boolClassId: {
					return;
				}
				default: {
					throwError("Cannot castd " +
					           compile.classes[value->classId]->name + " to " +
					           compile.classes[classId]->name);
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
			throwError("Cannot caste " + compile.classes[value->classId]->name +
			           " to " + compile.classes[classId]->name);
	}
}

void CastNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	value->putBytecodes(in_data, bytecodes);
	switch (classId) {
		case AutoLang::DefaultClass::intClassId: {
			switch (value->classId) {
				case AutoLang::DefaultClass::intClassId: {
					bytecodes.emplace_back(Opcode::INT_FROM_INT);
					return;
				}
				case AutoLang::DefaultClass::floatClassId: {
					bytecodes.emplace_back(Opcode::FLOAT_TO_INT);
					return;
				}
				case AutoLang::DefaultClass::boolClassId: {
					bytecodes.emplace_back(Opcode::BOOL_TO_INT);
					return;
				}
				default: {
					throwError("Bug: Unknown cast type");
				}
			}
		}
		case AutoLang::DefaultClass::floatClassId: {
			switch (value->classId) {
				case AutoLang::DefaultClass::intClassId: {
					bytecodes.emplace_back(Opcode::INT_TO_FLOAT);
					return;
				}
				case AutoLang::DefaultClass::floatClassId: {
					bytecodes.emplace_back(Opcode::FLOAT_FROM_FLOAT);
					return;
				}
				case AutoLang::DefaultClass::boolClassId: {
					bytecodes.emplace_back(Opcode::BOOL_TO_FLOAT);
					return;
				}
				default: {
					throwError("Bug: Unknown cast type");
				}
			}
		}
		default:
			// Extended class
			return;
	}
}

} // namespace AutoLang
#endif