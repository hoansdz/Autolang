#ifndef INTERPRETER_CPP
#define INTERPRETER_CPP

#include <chrono>
#include <sstream>
#include <iostream>
#include <functional>
#include "Debugger.hpp"
#include "Interpreter.hpp"
#include "DefaultOperator.hpp"

AVM::AVM(AVMReadFileMode& mode, bool allowDebug) : allowDebug(allowDebug)
{
	auto startCompiler = std::chrono::high_resolution_clock::now();

	AutoLang::DefaultClass::init(data);
	AutoLang::DefaultFunction::init(data);
	data.mainFunctionId = data.registerFunction(nullptr, false, ".main", {}, {}, nullptr);

	std::cout << "Init time : " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startCompiler).count() << " ms" << '\n';
	if (AutoLang::build(data, mode)) {
		data.main = &data.functions[data.mainFunctionId];
		initGlobalVariables();
		run();
	}
	// log();
	auto end = std::chrono::high_resolution_clock::now();
	auto durationCompiler = std::chrono::duration_cast<std::chrono::milliseconds>(end - startCompiler);
	std::cout << "Total time : " << durationCompiler.count() << " ms" << '\n';
	while (allowDebug)
	{
		std::string command;
		std::getline(std::cin, command);
		std::istringstream iss(command);
		std::string word;
		if (iss >> word)
		{
			if (word == "log")
			{
				if (iss >> word)
				{
					std::string name = std::move(word);
					auto &vec = data.funcMap[name];
					if (vec.size() == 0)
					{
						std::cout << "Cannot find " << name;
						continue;
					}
					if (vec.size() == 1)
					{
						log(&data.functions[vec[0]]);
						std::cout << '\n';
						continue;
					}
					continue;
				}
				else
				{
					std::cout << "Please log function" << '\n';
					continue;
				}
			}
			else if (word == "e")
			{
				return;
			}
		}
		else
		{
			std::cout << "wtf" << '\n';
		}
	}
}

void AVM::run()
{
	std::cerr << "-------------------" << '\n';
	std::cerr << "Runtime" << '\n';
	auto start = std::chrono::high_resolution_clock::now();
	//[0, 5] is temp area
	stackAllocator.top = 5;
	run(data.main, stackAllocator.top, 0);
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << '\n'
			  << "Total runtime : " << duration.count() << " ms" << '\n';
}

AObject *AVM::run(Function *currentFunction, const size_t currentTop, size_t maxThisAreaSize)
{
	auto *bytecodes = currentFunction->bytecodes.data();
	size_t i = 0;
	const size_t size = currentFunction->bytecodes.size();
	// printDebug('\n');
	while (i < size)
	{
		// std::cerr<<i<<'\n';
		switch (bytecodes[i++])
		{
		case AutoLang::Opcode::CALL_FUNCTION:
		{
			// std::cerr<<"loading: function"<<'\n';;
			Function *func = &data.functions[get_u32(bytecodes, i)];
			stackAllocator.top += currentFunction->maxDeclaration;
			uint32_t top = stackAllocator.top;
			// Ensure
			stackAllocator.ensure(func->maxDeclaration);
			for (size_t size = func->args.size; size-- > 0;)
			{
				auto object = stack.pop();
				if (object != nullptr)
					object->retain();
				stackAllocator[size] = object;
			}
			AObject *value;
			if (func->bytecodes.size() != 0)
			{
				value = run(func, top, currentFunction->maxDeclaration);
			}

			if (func->native)
			{
				value = func->native(data.manager, stackAllocator, func->args.size);
			}
			stack.push(value);
			stackAllocator.clear(data.manager, top, top + func->maxDeclaration - 1);
			stackAllocator.freeTo(currentTop);
			break;
		}
		case AutoLang::Opcode::CALL_VOID_FUNCTION:
		{
			// std::cerr<<"loading: function"<<'\n';;
			Function *func = &data.functions[get_u32(bytecodes, i)];
			stackAllocator.top += currentFunction->maxDeclaration;
			uint32_t top = stackAllocator.top;
			// Ensure
			stackAllocator.ensure(func->maxDeclaration);
			for (size_t size = func->args.size; size-- > 0;)
			{
				auto object = stack.pop();
				if (object != nullptr)
					object->retain();
				stackAllocator[size] = object;
			}
			if (func->bytecodes.size() != 0)
			{
				run(func, top, currentFunction->maxDeclaration);
			}

			if (func->native)
			{
				func->native(data.manager, stackAllocator, func->args.size);
			}
			stackAllocator.clear(data.manager, top, top + func->maxDeclaration - 1);
			stackAllocator.freeTo(currentTop);
			break;
		}
		case AutoLang::Opcode::LOAD_CONST:
		{
			stack.push(getConstObject(get_u32(bytecodes, i)));
			break;
		}
		case AutoLang::Opcode::LOAD_CONST_PRIMARY:
		{
			stack.push(data.constPool[get_u32(bytecodes, i)]);
			break;
		}
		case AutoLang::Opcode::POP:
		{
			data.manager.release(stack.pop());
			break;
		}
		case AutoLang::Opcode::RETURN_LOCAL:
		{
			AObject **last = &stackAllocator[get_u32(bytecodes, i)];
			AObject *obj = *last;
			if (obj != nullptr)
			{
				--obj->refCount;
			}
			stack.push(obj);
			*last = nullptr;
			return obj;
		}
		case AutoLang::Opcode::CREATE_OBJECT:
		{
			uint32_t type = get_u32(bytecodes, i);
			size_t count = static_cast<size_t>(get_u32(bytecodes, i));
			stack.push(data.manager.get(type, count));
			break;
		}
		case AutoLang::Opcode::LOAD_GLOBAL:
		{
			stack.push(globalVariables[get_u32(bytecodes, i)]);
			break;
		}
		case AutoLang::Opcode::STORE_GLOBAL:
		{
			setGlobalVariables(get_u32(bytecodes, i), stack.pop());
			break;
		}
		case AutoLang::Opcode::LOAD_LOCAL:
		{
			stack.push(stackAllocator[get_u32(bytecodes, i)]);
			break;
		}
		case AutoLang::Opcode::STORE_LOCAL:
		{
			stackAllocator.set(data.manager, get_u32(bytecodes, i), stack.pop());
			break;
		}
		case AutoLang::Opcode::LOAD_MEMBER:
		{
			stack.push((*stack.pop()->member)[get_u32(bytecodes, i)]);
			break;
		}
		case AutoLang::Opcode::STORE_MEMBER:
		{
			AObject **last = &stack.pop()->member->data[get_u32(bytecodes, i)];
			if (*last != nullptr)
			{
				data.manager.release(*last);
			}
			*last = stack.pop();
			(*last)->retain();
			break;
		}
		case AutoLang::Opcode::RETURN:
		{
			return nullptr;
		}
		case AutoLang::Opcode::RETURN_VALUE:
		{
			return stack.pop();
		}
		case AutoLang::Opcode::JUMP_IF_FALSE:
		{
			uint32_t pos = get_u32(bytecodes, i);
			if (!stack.pop()->b)
				i = pos;
			break;
		}
		case AutoLang::Opcode::JUMP:
		{
			i = get_u32(bytecodes, i);
			break;
		}
		case AutoLang::Opcode::JUMP_IF_NULL:
		{
			uint32_t pos = get_u32(bytecodes, i);
			if (stack.pop() == AutoLang::DefaultClass::nullObject)
				i = pos;
			break;
		}
		case AutoLang::Opcode::JUMP_IF_NON_NULL:
		{
			uint32_t pos = get_u32(bytecodes, i);
			if (stack.pop() != AutoLang::DefaultClass::nullObject)
				i = pos;
			break;
		}
		case AutoLang::Opcode::TO_INT:
		{
			operate<AutoLang::DefaultFunction::to_int, 1>(currentTop);
			break;
		}
		case AutoLang::Opcode::TO_FLOAT:
		{
			operate<AutoLang::DefaultFunction::to_float, 1>(currentTop);
			break;
		}
		case AutoLang::Opcode::TO_STRING:
		{
			operate<AutoLang::DefaultFunction::to_string, 1>(currentTop);
			break;
		}
		case AutoLang::Opcode::PLUS_PLUS:
		{
			operate<AutoLang::DefaultFunction::plus_plus, 1>(currentTop);
			break;
		}
		case AutoLang::Opcode::MINUS_MINUS:
		{
			operate<AutoLang::DefaultFunction::minus_minus, 1>(currentTop);
			break;
		}
		case AutoLang::Opcode::PLUS:
		{
			operate<AutoLang::DefaultFunction::plus, 2>(currentTop);
			break;
		}
		case AutoLang::Opcode::MINUS:
		{
			operate<AutoLang::DefaultFunction::minus, 2>(currentTop);
			break;
		}
		case AutoLang::Opcode::MUL:
		{
			operate<AutoLang::DefaultFunction::mul, 2>(currentTop);
			break;
		}
		case AutoLang::Opcode::DIVIDE:
		{
			operate<AutoLang::DefaultFunction::divide, 2>(currentTop);
			break;
		}
		case AutoLang::Opcode::PLUS_EQUAL:
			operateWithoutPush<AutoLang::DefaultFunction::plus_eq, 2>(currentTop);
			break;
		case AutoLang::Opcode::MINUS_EQUAL:
			operateWithoutPush<AutoLang::DefaultFunction::minus_eq, 2>(currentTop);
			break;
		case AutoLang::Opcode::MUL_EQUAL:
			operateWithoutPush<AutoLang::DefaultFunction::mul_eq, 2>(currentTop);
			break;
		case AutoLang::Opcode::DIVIDE_EQUAL:
			operateWithoutPush<AutoLang::DefaultFunction::divide_eq, 2>(currentTop);
			break;
		case AutoLang::Opcode::MOD:
		{
			operate<AutoLang::DefaultFunction::mod, 2>(currentTop);
			break;
		}
		case AutoLang::Opcode::BITWISE_AND:
		{
			operate<AutoLang::DefaultFunction::bitwise_and, 2>(currentTop);
			break;
		}
		case AutoLang::Opcode::BITWISE_OR:
		{
			operate<AutoLang::DefaultFunction::bitwise_or, 2>(currentTop);
			break;
		}
		case AutoLang::Opcode::NEGATIVE:
		{
			operate<AutoLang::DefaultFunction::negative, 1>(currentTop);
			break;
		}
		case AutoLang::Opcode::NOT:
		{
			operate<AutoLang::DefaultFunction::op_not, 1>(currentTop);
			break;
		}
		case AutoLang::Opcode::AND_AND:
		{
			operate<AutoLang::DefaultFunction::op_and_and, 2>(currentTop);
			break;
		}
		case AutoLang::Opcode::OR_OR:
		{
			operate<AutoLang::DefaultFunction::op_or_or, 2>(currentTop);
			break;
		}
		case AutoLang::Opcode::EQUAL_VALUE:
			operate<AutoLang::DefaultFunction::op_eqeq, 2>(currentTop);
			break;
		case AutoLang::Opcode::NOTEQ_VALUE:
			operate<AutoLang::DefaultFunction::op_not_eq, 2>(currentTop);
			break;
		case AutoLang::Opcode::IS_NULL:
		{
			stack.push(ObjectManager::createBoolObject(stack.pop() == AutoLang::DefaultClass::nullObject));
			break;
		}
		case AutoLang::Opcode::IS_NON_NULL:
		{
			stack.push(ObjectManager::createBoolObject(stack.pop() != AutoLang::DefaultClass::nullObject));
			break;
		}
		case AutoLang::Opcode::LOAD_NULL:
		{
			stack.push(AutoLang::DefaultClass::nullObject);
			break;
		}
		case AutoLang::Opcode::LOAD_TRUE:
		{
			stack.push(AutoLang::DefaultClass::trueObject);
			break;
		}
		case AutoLang::Opcode::LOAD_FALSE:
		{
			stack.push(AutoLang::DefaultClass::falseObject);
			break;
		}
		case AutoLang::Opcode::EQUAL_POINTER:
		{
			operate<AutoLang::DefaultFunction::op_eq_pointer, 2>(currentTop);
			break;
		}
		case AutoLang::Opcode::NOTEQ_POINTER:
		{
			operate<AutoLang::DefaultFunction::op_not_eq_pointer, 2>(currentTop);
			break;
		}
		case AutoLang::Opcode::LESS_THAN_EQ:
			operate<AutoLang::DefaultFunction::op_less_than_eq, 2>(currentTop);
			break;
		case AutoLang::Opcode::LESS_THAN:
			operate<AutoLang::DefaultFunction::op_less_than, 2>(currentTop);
			break;
		case AutoLang::Opcode::GREATER_THAN_EQ:
			operate<AutoLang::DefaultFunction::op_greater_than_eq, 2>(currentTop);
			break;
		case AutoLang::Opcode::GREATER_THAN:
			operate<AutoLang::DefaultFunction::op_greater_than, 2>(currentTop);
			break;
		case AutoLang::Opcode::FLOAT_TO_INT: {
			auto obj = stack.pop();
			stack.push(data.manager.createIntObject(static_cast<int64_t>(obj->f)));
			++obj->refCount;
			data.manager.release(obj);
			break;
		}
		case AutoLang::Opcode::INT_TO_FLOAT: {
			auto obj = stack.pop();
			stack.push(data.manager.createFloatObject(static_cast<double>(obj->i)));
			++obj->refCount;
			data.manager.release(obj);
			break;
		}
		case AutoLang::Opcode::BOOL_TO_INT: {
			auto obj = stack.pop();
			stack.push(data.manager.createIntObject(static_cast<int64_t>(obj->b)));
			++obj->refCount;
			data.manager.release(obj);
			break;
		}
		case AutoLang::Opcode::BOOL_TO_FLOAT: {
			auto obj = stack.pop();
			stack.push(data.manager.createFloatObject(static_cast<double>(obj->b)));
			++obj->refCount;
			data.manager.release(obj);
			break;
		}
		case AutoLang::Opcode::INT_TO_STRING: {
			auto obj = stack.pop();
			stack.push(data.manager.create(AString::from(obj->i)));
			++obj->refCount;
			data.manager.release(obj);
			break;
		}
		case AutoLang::Opcode::FLOAT_TO_STRING: {
			auto obj = stack.pop();
			stack.push(data.manager.create(AString::from(obj->f)));
			++obj->refCount;
			data.manager.release(obj);
			break;
		}
		default:
			throw std::runtime_error("Bytecode not be defined");
		}
	}
	return nullptr;
}

AObject *AVM::getConstObject(uint32_t id)
{
	AObject *obj = data.constPool[id];
	switch (obj->type)
	{
	case AutoLang::DefaultClass::intClassId:
		return data.manager.create(obj->i);
	case AutoLang::DefaultClass::floatClassId:
		return data.manager.create(obj->f);
	default:
		return obj;
	}
}

void AVM::initGlobalVariables()
{
	globalVariables = new AObject *[data.main->maxDeclaration]{};
}

void AVM::setGlobalVariables(uint32_t i, AObject *object)
{
	AObject **last = &globalVariables[i];
	if (*last != nullptr)
	{
		data.manager.release(*last);
	}
	*last = object;
	object->retain();
}

template <AObject *(*native)(NativeFuncInput), size_t size>
void AVM::operate(size_t currentTop)
{
	stackAllocator.top = 0;
	inputArgument<size>();
	stack.push(native(data.manager, stackAllocator, size));
	stackAllocator.top = currentTop;
	stackAllocator.clearTemp<size>(data.manager);
}

template <AObject *(*native)(NativeFuncInput), size_t size>
void AVM::operateWithoutPush(size_t currentTop)
{
	stackAllocator.top = 0;
	inputArgument<size>();
	native(data.manager, stackAllocator, size);
	stackAllocator.top = currentTop;
	stackAllocator.clearTemp<size>(data.manager);
}

uint32_t AVM::get_u32(uint8_t *code, size_t &ip)
{
	uint32_t val;
	memcpy(&val, code + ip, 4);
	ip += 4;
	return val;
}

void AVM::log()
{
	// log(data.main);
	std::cerr << "-------------------" << '\n';
	std::cerr << "ConstPool: " << data.constPool.size() << " elements" << '\n';
	stackAllocator.top = 0;
	for (int i = 0; i < data.constPool.size(); ++i)
	{
		stackAllocator[0] = data.constPool[i];
		std::cerr << '[' << i << "] ";
		AutoLang::DefaultFunction::println(data.manager, stackAllocator, 1);
	}
	std::cerr << "-------------------" << '\n';
	std::cerr << "Function: " << data.functions.size() << " elements" << '\n';
	for (auto &func : data.functions)
	{
		
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

#define BYTECODE_PRINT_SINGLE(bytecode) \
	case AutoLang::Opcode::bytecode:    \
		std::cerr << #bytecode << "\n"; \
		break;

void AVM::log(Function *currentFunction)
{
	auto *bytecodes = currentFunction->bytecodes.data();
	auto size = currentFunction->bytecodes.size();
	std::cerr << "Size: " << size << " bytes" << '\n';
	size_t i = 0;
	while (i < size)
	{
		std::cerr << "[" << i << "] ";
		uint8_t b = bytecodes[i++];
		switch (b)
		{
		case AutoLang::Opcode::CALL_FUNCTION:
			std::cerr << "CALL_FUNCTION	 " << data.functions[get_u32(bytecodes, i)].name << '\n';
			break;
		case AutoLang::Opcode::CALL_VOID_FUNCTION:
			std::cerr << "CALL_VOID_FUNCTION	 " << data.functions[get_u32(bytecodes, i)].name << '\n';
			break;
		case AutoLang::Opcode::LOAD_CONST:
			std::cerr << "LOAD_CONST	 " << get_u32(bytecodes, i) << '\n';
			break;
		case AutoLang::Opcode::LOAD_CONST_PRIMARY:
			std::cerr << "CONST_PRIMARY	 " << get_u32(bytecodes, i) << '\n';
			break;
		case AutoLang::Opcode::RETURN_LOCAL:
			std::cerr << "RETURN_LOCAL	 " << get_u32(bytecodes, i) << '\n';
			break;
		case AutoLang::Opcode::CREATE_OBJECT:
			std::cerr << "CREATE_OBJECT	 " << data.classes[get_u32(bytecodes, i)].name << "     " << get_u32(bytecodes, i) << '\n';
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
		case AutoLang::Opcode::STORE_MEMBER:
			std::cerr << "STORE_MEMBER	 " << get_u32(bytecodes, i) << '\n';
			break;
			BYTECODE_PRINT_SINGLE(POP)
			BYTECODE_PRINT_SINGLE(RETURN)
			BYTECODE_PRINT_SINGLE(RETURN_VALUE)
		case AutoLang::Opcode::JUMP_IF_FALSE:
			std::cerr << "JUMP_IF_FALSE	 " << get_u32(bytecodes, i) << '\n';
			break;
		case AutoLang::Opcode::JUMP_IF_NULL:
			std::cerr << "JUMP_IF_NULL	 " << get_u32(bytecodes, i) << '\n';
			break;
		case AutoLang::Opcode::JUMP_IF_NON_NULL:
			std::cerr << "JUMP_IF_NON_NULL	 " << get_u32(bytecodes, i) << '\n';
			break;
			BYTECODE_PRINT_SINGLE(JUMP)
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
			BYTECODE_PRINT_SINGLE(INT_TO_STRING)
			BYTECODE_PRINT_SINGLE(FLOAT_TO_STRING)
		default:
			throw std::runtime_error("Bytecode not defined " + std::to_string(b));
			break;
		}
	}
}

AVM::~AVM()
{
	delete[] globalVariables;
}

std::string Function::toString(CompiledProgram& data) {
	bool isFirst = true;
	std::string result = name + ": (";
	for (int i = 0; i < args.size; ++i)
	{
		uint32_t classId = args[i];
		if (isFirst)
		{
			isFirst = false;
		}
		else
		{
			result += ", ";
		}
		result += data.classes[classId].name;
		if (nullableArgs[i])
			result += "?";
	}
	result += ")->";
	result += data.classes[returnId].name;
	if (returnNullable)
		result += "?";
	return result;
}

template <typename K, typename V>
size_t estimateUnorderedMapSize(const ankerl::unordered_dense::map<K, V> &map)
{
	size_t total = sizeof(map);
	total += map.bucket_count() * sizeof(void *); // bucket array

	for (const auto &[key, value] : map)
	{
		total += sizeof(std::pair<const K, V>);
		total += sizeof(key);
		total += sizeof(value);
	}

	return total;
}

#endif