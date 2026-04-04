#ifndef AVM_HPP
#define AVM_HPP

#include "shared/AClass.hpp"
#include "shared/CompiledProgram.hpp"
#include "shared/FixedPool.hpp"
#include "shared/FixedPoolLoaded.hpp"
#include "shared/Function.hpp"
#include "shared/ObjectManager.hpp"
#include "shared/StackAllocator.hpp"
#include "backend/vm/Opcode.hpp"
#include <chrono>
#include <string>

namespace AutoLang {

template <typename K, typename V>
size_t estimateUnorderedMapSize(const HashMap<K, V> &map);

enum class VMState { READY, RUNNING, HALTED, WAITING, ERROR };

#ifndef MAX_STACK_OBJECT
#define MAX_STACK_OBJECT 256
#endif

#ifndef MAX_STACK_ALLOCATOR
#define MAX_STACK_ALLOCATOR 512
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
	ANotifier *notifier;

  public:
	inline uint32_t get_u32(uint8_t *code, uint32_t &ip);
	void log(Function *currentFunction);

	Stack<AObject *, MAX_STACK_OBJECT> stack;
	FixedPoolLoaded<CallFrame, MAX_CALL_FRAME> callFrames;
	StackAllocator stackAllocator = StackAllocator(MAX_STACK_ALLOCATOR);
	AObject **tempAllocateArea = new AObject *[3]{};
	inline AObject *getConstObject(uint32_t id);

	inline void initGlobalVariables();
	AObject **globalVariables = nullptr;
	inline void setGlobalVariables(uint32_t i, AObject *object);
	void restart();

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
	template <size_t size> inline bool fastOperate(ANativeFunction native);
	template <bool loadVirtual, bool hasValue, bool isConstructor>
	inline bool callFunction(CallFrame *&currentCallFrame,
	                         Function *currentFunction, uint8_t *bytecodes,
	                         uint32_t &i);
	template <bool hasValue>
	inline bool callNativeFunction(CallFrame *&currentCallFrame,
	                        Function *currentFunction, uint8_t *bytecodes,
	                        uint32_t &i);
	inline bool callFunctionObject(AObject *obj);
	inline bool callFunction(Function *currentFunction);
	inline bool callFunction(CallFrame *currentCallFrame,
	                         uint32_t argumentCount);
	inline void resume();
	void run();
	void input(AObject *inputData);

	//   public:
	VMState state = VMState::READY;
	[[nodiscard]] explicit AVM(bool allowDebug);
	void start();
	void log();
	CompiledProgram data;
	~AVM();
	// friend ACompiler;
};

} // namespace AutoLang

#endif