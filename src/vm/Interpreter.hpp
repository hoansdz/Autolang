#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <string>
#include "optimize/FixedArray.hpp"
#include "CompiledProgram.hpp"

namespace AutoLang
{

	enum Opcode : uint8_t
	{
		CALL_FUNCTION = 0,
		LOAD_CONST = 1,
		STORE_GLOBAL = 2,
		LOAD_GLOBAL = 3,
		POP = 4,
		TO_INT = 5,
		TO_FLOAT = 6,
		TO_STRING = 7,
		PLUS = 8,
		MINUS = 9,
		MUL = 10,
		DIVIDE = 11,
		MOD = 12,
		NOT = 13,
		NEGATIVE = 14,
		JUMP = 15,
		JUMP_IF_FALSE = 16,
		AND_AND = 17,
		OR_OR = 18,
		EQUAL_VALUE = 19,
		EQUAL_POINTER = 20,
		NOTEQ_VALUE = 21,
		NOTEQ_POINTER = 22,
		LESS_THAN_EQ = 23,
		LESS_THAN = 24,
		GREATER_THAN_EQ = 25,
		GREATER_THAN = 26,
		LOAD_CONST_PRIMARY = 27,
		PLUS_PLUS = 28,
		MINUS_MINUS = 29,
		RETURN = 30,
		LOAD_LOCAL = 31,
		STORE_LOCAL = 32,
		RETURN_VALUE = 33,
		LOAD_MEMBER = 34,
		STORE_MEMBER = 35,
		RETURN_LOCAL = 36,
		CREATE_OBJECT = 37,
		PLUS_EQUAL = 38,
		MINUS_EQUAL = 39,
		MUL_EQUAL = 40,
		DIVIDE_EQUAL = 41,
		JUMP_IF_NULL = 42,
		JUMP_IF_NON_NULL = 43,
		IS_NULL = 44,
		IS_NON_NULL = 45,
		LOAD_NULL = 46,
		LOAD_TRUE = 47,
		LOAD_FALSE = 48,
		CALL_VOID_FUNCTION = 49,
		BITWISE_AND = 50,
		BITWISE_OR = 51,
		FLOAT_TO_INT = 52,
		INT_TO_FLOAT = 53,
		BOOL_TO_INT = 54,
		BOOL_TO_FLOAT = 55,
		INT_TO_STRING = 56,
		FLOAT_TO_STRING = 57,
	};

}

struct AClass
{
	std::string name;
	uint32_t id;
	std::vector<AClass *> parent;
	std::vector<uint32_t> memberId;
	ankerl::unordered_dense::map<std::string, uint32_t> memberMap;
	ankerl::unordered_dense::map<std::string, std::vector<uint32_t>> funcMap;
	AClass(std::string name, uint32_t id) : name(std::move(name)), id(id) {}
};

struct Function
{
	std::string name;
	AObject *(*native)(NativeFuncInput);
	bool isStatic;
	bool returnNullable;
	FixedArray<uint32_t> args;
	FixedArray<bool> nullableArgs;
	uint32_t returnId;
	std::vector<uint8_t> bytecodes;
	uint32_t maxDeclaration;
	uint32_t id;
	//Support log
	std::string toString(CompiledProgram& data);
	Function(uint32_t id, std::string name, AObject *(*native)(NativeFuncInput), bool isStatic, std::vector<uint32_t> &args, std::vector<bool> &nullableArgs, uint32_t returnId, bool returnNullable) : 
		id(id), name(name), native(native), isStatic(isStatic), returnNullable(returnNullable), args(args), nullableArgs(nullableArgs), returnId(returnId), maxDeclaration(native ? this->args.size : 0) {}
};

template <typename K, typename V>
size_t estimateUnorderedMapSize(const ankerl::unordered_dense::map<K, V> &map);

struct AVMReadFileMode {
	const char* path;
	const char* data;
	uint32_t dataSize;
	bool allowImportOtherFile;
};

class AVM
{
private:
	inline uint32_t get_u32(uint8_t *code, size_t &ip);
	void log();
	void log(Function *currentFunction);
	CompiledProgram data;
	Array<64> stack;
	StackAllocator stackAllocator;
	inline AObject *getConstObject(uint32_t id);

	inline void initGlobalVariables();
	AObject **globalVariables;
	inline void setGlobalVariables(uint32_t i, AObject *object);

	template <size_t index>
	inline void inputArgument()
	{
		if constexpr (index > 0)
		{
			AObject **last = &stackAllocator[std::integral_constant<size_t, index - 1>{}];
			*last = stack.pop();
			(*last)->retain();
			// std::cerr<<"Input: "<<index-1<<", ref: "<<(*last)->refCount<<'\n';
			inputArgument<index - 1>();
		}
	}
	bool allowDebug;
	template <AObject *(*native)(NativeFuncInput), size_t size>
	inline void operateWithoutPush(size_t currentTop);
	template <AObject *(*native)(NativeFuncInput), size_t size>
	inline void operate(size_t currentTop);
	AObject *run(Function *currentFunction, const size_t currentTop, size_t maxThisAreaSize);

public:
	explicit AVM(AVMReadFileMode& mode, bool allowDebug);
	~AVM();
	void run();
};

#endif