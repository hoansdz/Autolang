#ifndef AVM_HPP
#define AVM_HPP

#include "shared/AClass.hpp"
#include "shared/CompiledProgram.hpp"
#include "shared/FixedPool.hpp"
#include "shared/FixedPoolLoaded.hpp"
#include "shared/Function.hpp"
#include "shared/ObjectManager.hpp"
#include "shared/StackAllocator.hpp"
#include <string>

namespace AutoLang {

enum Opcode : uint8_t {
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
	JUMP_AND_DELETE_IF_NULL = 56,
	JUMP_AND_SET_IF_NULL = 57,
	LOAD_MEMBER_IF_NNULL = 58,
	LOAD_MEMBER_CAN_RET_NULL = 59,
	POP_NO_RELEASE = 60,
	CALL_VTABLE_FUNCTION = 61,
	CALL_VTABLE_VOID_FUNCTION = 62,
	CALL_DATA_CONTRUCTOR = 63,
	INT_FROM_INT = 64,
	FLOAT_FROM_FLOAT = 65,
	ADD_TRY_BLOCK = 66,
	REMOVE_TRY_AND_JUMP = 67,
	THROW_EXCEPTION = 68,
	REMOVE_TRY = 69,
	IS = 70,
	LOAD_EXCEPTION = 71,
};

template <typename K, typename V>
size_t estimateUnorderedMapSize(const HashMap<K, V> &map);

struct AVMReadFileMode {
	const char *path;
	const char *data;
	uint32_t dataSize;
	bool allowImportOtherFile;
	ANativeMap nativeFuncMap;
};

enum class VMState { INIT, RUNNING, HALTED, WAITING, ERROR };

#ifndef MAX_STACK_OBJECT
#define MAX_STACK_OBJECT 128
#endif

#ifndef MAX_STACK_ALLOCATOR
#define MAX_STACK_ALLOCATOR 256
#endif

#ifndef MAX_CALL_FRAME
#define MAX_CALL_FRAME 256
#endif

struct CallFrame {
	Function *func;
	uint32_t i;
	uint32_t fromStackAllocator;
	uint32_t startStackCount;
	AObject *exception;
	std::vector<uint32_t> catchPosition;
	CallFrame() { catchPosition.reserve(4); }
};

class ANotifier;

class AVM {
  private:
	ANotifier* notifier;
  public:
	inline uint32_t get_u32(uint8_t *code, uint32_t &ip);
	void log(Function *currentFunction);
	Stack<AObject *, MAX_STACK_OBJECT> stack;
	FixedPoolLoaded<CallFrame, MAX_CALL_FRAME> callFrames;
	StackAllocator stackAllocator = StackAllocator(MAX_STACK_ALLOCATOR);
	AObject **tempAllocateArea = new AObject *[3]{};
	inline AObject *getConstObject(uint32_t id);

	uint32_t globalVariableCount;
	inline void initGlobalVariables();
	AObject **globalVariables = nullptr;
	inline void setGlobalVariables(uint32_t i, AObject *object);

	template <size_t index> inline void inputTempAllocateArea() {
		if constexpr (index > 0) {
			AObject *obj = stack.pop();
			tempAllocateArea[std::integral_constant<size_t, index - 1>{}] = obj;
			inputTempAllocateArea<index - 1>();
		}
	}

	template <size_t index> inline void clearTempAllocateArea() {
		if constexpr (index > 0) {
			data.manager.release(
			    tempAllocateArea[std::integral_constant<size_t, index - 1>{}]);
			clearTempAllocateArea<index - 1>();
		}
	}

	bool allowDebug;
	template <ANativeFunction native, size_t size, bool push = true>
	inline bool operate();
	template <bool loadVirtual, bool hasValue, bool isConstructor>
	inline bool callFunction(CallFrame*& currentCallFrame, Function *currentFunction,
	                               uint8_t *bytecodes, uint32_t &i);
	void resume();
	void run();

	//   public:
	VMState state = VMState::INIT;
	[[nodiscard]] explicit AVM(bool allowDebug);
	void start();
	void log();
	CompiledProgram data;
	~AVM();
	// friend ACompiler;
};

} // namespace AutoLang

#endif