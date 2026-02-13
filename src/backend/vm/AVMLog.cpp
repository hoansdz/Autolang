#ifndef AVMLOG_CPP
#define AVMLOG_CPP

#include "backend/vm/AVM.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/DefaultFunction.hpp"

namespace AutoLang {

void AVM::log() {
	log(data.main);
	std::cerr << "-------------------" << '\n';
	std::cerr << "ConstPool: " << data.constPool.size() << " elements" << '\n';
	for (int i = 0; i < data.constPool.size(); ++i) {
		tempAllocateArea[0] = data.constPool[i];
		std::cerr << '[' << i << "] ";
		AutoLang::DefaultFunction::println(*notifier, tempAllocateArea, 1);
	}
	std::cerr << "-------------------" << '\n';
	std::cerr << "Function: " << data.functions.size() << " elements" << '\n';
	for (auto &func : data.functions) {
		std::cerr << func->toString(data) << std::endl;
	}
	/*uint32_t totalSize = bytecodes.size() +
	    sizeof(AVM) +
	    data.functions.size() * sizeof(Function) +
	    data.classes.size() * sizeof(ClassInfo) +
	    data.constPool.size() * sizeof(AObject*)  +
	    data.constPool.size() * sizeof(AObject)  +
	    estimateUnorderedMapSize(data.funcMap) +
	    estimateUnorderedMapSize(data.classMap) +
	    data.main->maxDeclaration * sizeof(AObject*)
	    ;
	std::cerr<<"TotalSize: "<<static_cast<double>(totalSize) / 1024<<" kb\n";*/
}

std::string Function::toString(CompiledProgram &data) {
	bool isFirst = true;
	std::string result = "[" + std::to_string(id) + "] " + name + ": (";
	for (int i = 0; i < argSize; ++i) {
		ClassId classId = args[i];
		if (isFirst) {
			isFirst = false;
		} else {
			result += ", ";
		}
		result += data.classes[classId]->name;
	}
	result += ")->";
	result += data.classes[returnId]->name;
	if (functionFlags & FunctionFlags::FUNC_RETURN_NULLABLE)
		result += "?";
	return result;
}

void AClass::log(CompiledProgram &data) {
	std::cerr << "[" << id << "]: Class " + name
	          << (inheritance.empty() ? ""
	                                  : std::string(" extends ") +
	                                        data.classes[*parentId]->name)
	          << "\n";
	for (auto &[name, offset] : memberMap) {
		std::cerr << "[" << offset << "] " << name << ": "
		          << data.classes[memberId[offset]]->name << "\n";
	}
	for (auto &[name, vecs] : funcMap) {
		for (auto funcId : vecs) {
			std::cerr << data.functions[funcId]->toString(data) << "\n";
		}
	}
	if (!vtable.empty()) {
		std::cerr << "Vtable\n";
		for (int i = 0; i < vtable.size(); ++i) {
			std::cerr << "[" << i << "]: " << vtable[i] << "\n";
		}
	}
}

#define BYTECODE_PRINT_SINGLE(bytecode)                                        \
	case AutoLang::Opcode::bytecode:                                           \
		std::cerr << #bytecode << "\n";                                        \
		break;

void AVM::log(Function *currentFunction) {
	std::cerr << currentFunction->toString(data) << '\n';
	std::cerr << currentFunction->argSize << " arguments\n";
	std::cerr << "Total " << currentFunction->maxDeclaration
	          << " declarations\n";
	if (currentFunction->functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
		std::cerr << "Has native function" << '\n';
		return;
	}
	auto *bytecodes = currentFunction->bytecodes.data();
	auto size = currentFunction->bytecodes.size();
	std::cerr << "Size: " << size << " bytes" << '\n';
	uint32_t i = 0;
	while (i < size) {
		std::cerr << "[" << i << "] ";
		uint8_t b = bytecodes[i++];
		switch (b) {
			case AutoLang::Opcode::CALL_FUNCTION: {
				uint32_t funcId = get_u32(bytecodes, i);
				std::cerr << "CALL_FUNCTION	 " << data.functions[funcId]->name
				          << '\n';
				break;
			}
			case AutoLang::Opcode::CALL_VOID_FUNCTION: {
				uint32_t funcId = get_u32(bytecodes, i);
				std::cerr << "CALL_VOID_FUNCTION	 "
				          << data.functions[funcId]->name << '\n';
				break;
			}
			case AutoLang::Opcode::CALL_VTABLE_FUNCTION: {
				uint32_t funcPos = get_u32(bytecodes, i);
				uint32_t argumentCount = get_u32(bytecodes, i);
				std::cerr << "CALL_VTABLE_FUNCTION	 " << funcPos << " "
				          << argumentCount << '\n';
				break;
			}
			case AutoLang::Opcode::CALL_VTABLE_VOID_FUNCTION: {
				uint32_t funcPos = get_u32(bytecodes, i);
				uint32_t argumentCount = get_u32(bytecodes, i);
				std::cerr << "CALL_VTABLE_VOID_FUNCTION	 " << funcPos << " "
				          << argumentCount << '\n';
				break;
			}
			case AutoLang::Opcode::CALL_DATA_CONTRUCTOR: {
				uint32_t funcPos = get_u32(bytecodes, i);
				std::cerr << "CALL_DATA_CONTRUCTOR	 " << funcPos << "\n";
				break;
			}
			case AutoLang::Opcode::LOAD_CONST:
				std::cerr << "LOAD_CONST	 " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::IS:
				std::cerr << "IS	 " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::SAFE_CAST:
				std::cerr << "SAFE_CAST	 " << data.classes[get_u32(bytecodes, i)]->name << '\n';
				break;
			case AutoLang::Opcode::UNSAFE_CAST:
				std::cerr << "UNSAFE_CAST	 " << data.classes[get_u32(bytecodes, i)]->name << '\n';
				break;
			case AutoLang::Opcode::ADD_TRY_BLOCK:
				std::cerr << "ADD_TRY_BLOCK	 " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::REMOVE_TRY_AND_JUMP:
				std::cerr << "REMOVE_TRY_AND_JUMP	 " << get_u32(bytecodes, i)
				          << '\n';
				break;
			case AutoLang::Opcode::LOAD_CONST_PRIMARY:
				std::cerr << "CONST_PRIMARY	 " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::RETURN_LOCAL:
				std::cerr << "RETURN_LOCAL	 " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::CREATE_OBJECT:
				std::cerr << "CREATE_OBJECT	 "
				          << data.classes[get_u32(bytecodes, i)]->name
				          << "     " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::LOAD_GLOBAL:
				std::cerr << "LOAD_GLOBAL	 " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::STORE_GLOBAL:
				std::cerr << "STORE_GLOBAL	 " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::LOAD_LOCAL:
				std::cerr << "LOAD_LOCAL	 " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::STORE_LOCAL:
				std::cerr << "STORE_LOCAL	 " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::LOAD_MEMBER:
				std::cerr << "LOAD_MEMBER	 " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::LOAD_MEMBER_IF_NNULL:
				std::cerr << "LOAD_MEMBER_IF_NNULL	 " << get_u32(bytecodes, i)
				          << '\n';
				break;
			case AutoLang::Opcode::LOAD_MEMBER_CAN_RET_NULL:
				std::cerr << "LOAD_MEMBER_CAN_RET_NULL	 "
				          << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::STORE_MEMBER:
				std::cerr << "STORE_MEMBER	 " << get_u32(bytecodes, i) << '\n';
				break;
				BYTECODE_PRINT_SINGLE(POP)
				BYTECODE_PRINT_SINGLE(POP_NO_RELEASE)
				BYTECODE_PRINT_SINGLE(RETURN)
				BYTECODE_PRINT_SINGLE(RETURN_VALUE)
				BYTECODE_PRINT_SINGLE(INT_FROM_INT)
				BYTECODE_PRINT_SINGLE(FLOAT_FROM_FLOAT)
				BYTECODE_PRINT_SINGLE(REMOVE_TRY)
				BYTECODE_PRINT_SINGLE(LOAD_EXCEPTION)
				BYTECODE_PRINT_SINGLE(WAIT_INPUT)
				BYTECODE_PRINT_SINGLE(THROW_EXCEPTION)
			case AutoLang::Opcode::JUMP:
				std::cerr << "JUMP	 " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::JUMP_IF_FALSE:
				std::cerr << "JUMP_IF_FALSE	 " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::JUMP_IF_NULL:
				std::cerr << "JUMP_IF_NULL	 " << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::JUMP_AND_DELETE_IF_NULL:
				std::cerr << "JUMP_AND_DELETE_IF_NULL	 "
				          << get_u32(bytecodes, i) << '\n';
				break;
			case AutoLang::Opcode::JUMP_AND_SET_IF_NULL:
				std::cerr << "JUMP_AND_SET_IF_NULL	 " << get_u32(bytecodes, i)
				          << '\n';
				break;
			case AutoLang::Opcode::JUMP_IF_NON_NULL:
				std::cerr << "JUMP_IF_NON_NULL	 " << get_u32(bytecodes, i)
				          << '\n';
				break;
				BYTECODE_PRINT_SINGLE(TO_INT)
				BYTECODE_PRINT_SINGLE(TO_FLOAT)
				BYTECODE_PRINT_SINGLE(TO_STRING)
				BYTECODE_PRINT_SINGLE(PLUS_PLUS)
				BYTECODE_PRINT_SINGLE(MINUS_MINUS)
			case AutoLang::Opcode::AND_AND:
				std::cerr << "AND	 " << '\n';
				break;
			case AutoLang::Opcode::OR_OR:
				std::cerr << "OR	 " << '\n';
				break;
				BYTECODE_PRINT_SINGLE(PLUS)
				BYTECODE_PRINT_SINGLE(MINUS)
				BYTECODE_PRINT_SINGLE(MUL)
				BYTECODE_PRINT_SINGLE(DIVIDE)
				BYTECODE_PRINT_SINGLE(IS_NULL)
				BYTECODE_PRINT_SINGLE(IS_NON_NULL)
				BYTECODE_PRINT_SINGLE(LOAD_TRUE)
				BYTECODE_PRINT_SINGLE(LOAD_FALSE)
				BYTECODE_PRINT_SINGLE(LOAD_NULL)
				BYTECODE_PRINT_SINGLE(PLUS_EQUAL)
				BYTECODE_PRINT_SINGLE(MINUS_EQUAL)
				BYTECODE_PRINT_SINGLE(MUL_EQUAL)
				BYTECODE_PRINT_SINGLE(DIVIDE_EQUAL)
				BYTECODE_PRINT_SINGLE(NEGATIVE)
				BYTECODE_PRINT_SINGLE(MOD)
				BYTECODE_PRINT_SINGLE(BITWISE_AND)
				BYTECODE_PRINT_SINGLE(BITWISE_OR)
				BYTECODE_PRINT_SINGLE(NOT)
				BYTECODE_PRINT_SINGLE(EQUAL_VALUE)
				BYTECODE_PRINT_SINGLE(NOTEQ_VALUE)
				BYTECODE_PRINT_SINGLE(EQUAL_POINTER)
				BYTECODE_PRINT_SINGLE(NOTEQ_POINTER)
				BYTECODE_PRINT_SINGLE(LESS_THAN_EQ)
				BYTECODE_PRINT_SINGLE(LESS_THAN)
				BYTECODE_PRINT_SINGLE(GREATER_THAN_EQ)
				BYTECODE_PRINT_SINGLE(GREATER_THAN)
				BYTECODE_PRINT_SINGLE(FLOAT_TO_INT)
				BYTECODE_PRINT_SINGLE(INT_TO_FLOAT)
				BYTECODE_PRINT_SINGLE(BOOL_TO_INT)
				BYTECODE_PRINT_SINGLE(BOOL_TO_FLOAT)
			default:
				throw std::runtime_error("Bytecode not defined " +
				                         std::to_string(b));
				break;
		}
	}
}

} // namespace AutoLang

#endif