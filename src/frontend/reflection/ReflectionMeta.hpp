#ifndef REFLECTION_META_HPP
#define REFLECTION_META_HPP

#include "shared/Type.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <third_party/ankerl/unordered_dense.h>

namespace Autolang {

struct TypeMeta {
	std::optional<uint32_t> classId;
	std::string name;
	bool nullable = false;
	bool isGeneric = false;
	std::vector<TypeMeta> inputs;
};

struct VarMeta {
	std::string name;
	TypeMeta type;
	bool isStatic = false;
	bool isVal = false;
	bool isLateinit = false;
	bool hasDefault = false;
};

struct FunctionMeta {
	uint32_t id = 0;
	std::string name;
	std::string library;
	std::optional<uint32_t> ownerClassId;
	bool isStatic = false;
	bool hasTrailingClosure = false;
	bool isInfix = false;
	int infixPrecedence = -1;
	std::vector<VarMeta> params;
	TypeMeta returnType;
};

struct GenericMeta {
	std::string name;
	bool mustBeExtends = false;
	std::optional<uint32_t> extendClassId;
};

struct ClassMeta {
	uint32_t classId = 0;
	std::string name;
	std::string library;
	std::vector<GenericMeta> generics;
	std::vector<VarMeta> properties;
	ankerl::unordered_dense::map<std::string, std::vector<FunctionMeta>> functions;
	std::vector<std::string> enumEntries;
};

struct LibraryMeta {
	std::string name;
	bool autoImport = false;
	bool loaded = false;
};

struct AliasMeta {
	std::string name;
	std::string target;
};

struct IntrinsicMeta {
	std::string name;
	std::string returnClass;
	bool nullable = false;
	bool elementFromArguments = false;
	int minArgs = 0;
	int maxArgs = -1;
};

} // namespace Autolang

#endif
