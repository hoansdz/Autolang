#ifndef REFLECTION_ENGINE_HPP
#define REFLECTION_ENGINE_HPP

#include "ReflectionMeta.hpp"
#include "frontend/ACompiler.hpp"
#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/ClassInfo.hpp"
#include "frontend/parser/FunctionInfo.hpp"
#include "frontend/parser/GenericData.hpp"
#include "frontend/parser/Parameter.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/FunctionFlags.hpp"
#include <algorithm>
#include <cstring>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#if defined(__PYBIND11__)
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;
#endif

namespace Autolang {

struct NativeReflectionAdapter {
	using TypeResult = TypeMeta;
	using VarResult = VarMeta;
	using FunctionResult = FunctionMeta;
	using GenericResult = GenericMeta;
	using ClassResult = ClassMeta;
	using LibraryResult = LibraryMeta;
	using AliasResult = AliasMeta;
	using ListType = std::vector<TypeResult>;
	using ListVar = std::vector<VarResult>;
	using ListFunction = std::vector<FunctionResult>;
	using ListGeneric = std::vector<GenericResult>;
	using ListClass = std::vector<ClassResult>;
	using ListLibrary = std::vector<LibraryResult>;
	using ListAlias = std::vector<AliasResult>;
	using ListString = std::vector<std::string>;
	using MapFunction = ankerl::unordered_dense::map<std::string, std::vector<FunctionMeta>>;
	using OptionalClass = std::optional<ClassMeta>;
	using OptionalFunction = std::optional<FunctionMeta>;

	template <typename ListT, typename ItemT>
	static void appendList(ListT &list, ItemT &&item) {
		list.push_back(std::forward<ItemT>(item));
	}

	template <typename ItemT>
	static void appendMap(MapFunction &map, const std::string &key, ItemT &&item) {
		map[key].push_back(std::forward<ItemT>(item));
	}

	static TypeMeta makeType(std::optional<uint32_t> classId, const std::string &name,
	                         bool nullable, bool isGeneric, ListType &&inputs) {
		TypeMeta tm;
		tm.classId = classId;
		tm.name = name;
		tm.nullable = nullable;
		tm.isGeneric = isGeneric;
		tm.inputs = std::move(inputs);
		return tm;
	}

	static VarMeta makeVar(const std::string &name, TypeMeta &&type,
	                       bool isStatic, bool isVal, bool isLateinit, bool hasDefault) {
		VarMeta vm;
		vm.name = name;
		vm.type = std::move(type);
		vm.isStatic = isStatic;
		vm.isVal = isVal;
		vm.isLateinit = isLateinit;
		vm.hasDefault = hasDefault;
		return vm;
	}

	static FunctionMeta makeFunction(uint32_t id, const std::string &name,
	                                 const std::string &library,
	                                 std::optional<uint32_t> ownerClassId,
	                                 bool isStatic, bool hasTrailingClosure,
	                                 ListVar &&params,
	                                 TypeMeta &&returnType) {
		FunctionMeta fm;
		fm.id = id;
		fm.name = name;
		fm.library = library;
		fm.ownerClassId = ownerClassId;
		fm.isStatic = isStatic;
		fm.hasTrailingClosure = hasTrailingClosure;
		fm.params = std::move(params);
		fm.returnType = std::move(returnType);
		return fm;
	}

	static GenericMeta makeGeneric(const std::string &name, bool mustBeExtends,
	                               std::optional<uint32_t> extendClassId) {
		GenericMeta gm;
		gm.name = name;
		gm.mustBeExtends = mustBeExtends;
		gm.extendClassId = extendClassId;
		return gm;
	}

	static ClassMeta makeClass(uint32_t classId, const std::string &name,
	                           const std::string &library,
	                           ListGeneric &&generics,
	                           ListVar &&properties,
	                           MapFunction &&functions,
	                           ListString &&enumEntries) {
		ClassMeta cm;
		cm.classId = classId;
		cm.name = name;
		cm.library = library;
		cm.generics = std::move(generics);
		cm.properties = std::move(properties);
		cm.functions = std::move(functions);
		cm.enumEntries = std::move(enumEntries);
		return cm;
	}

	static LibraryMeta makeLibrary(const std::string &name, bool autoImport, bool loaded) {
		LibraryMeta lm;
		lm.name = name;
		lm.autoImport = autoImport;
		lm.loaded = loaded;
		return lm;
	}

	static AliasMeta makeAlias(const std::string &name, const std::string &target) {
		AliasMeta am;
		am.name = name;
		am.target = target;
		return am;
	}

	static OptionalClass makeNullClass() {
		return std::nullopt;
	}

	static OptionalFunction makeNullFunction() {
		return std::nullopt;
	}
};

#if defined(__PYBIND11__)
struct PyReflectionAdapter {
	using TypeResult = py::dict;
	using VarResult = py::dict;
	using FunctionResult = py::dict;
	using GenericResult = py::dict;
	using ClassResult = py::dict;
	using LibraryResult = py::dict;
	using AliasResult = py::dict;
	using ListType = py::list;
	using ListVar = py::list;
	using ListFunction = py::list;
	using ListGeneric = py::list;
	using ListClass = py::list;
	using ListLibrary = py::list;
	using ListAlias = py::list;
	using ListString = py::list;
	using MapFunction = py::dict;
	using OptionalClass = py::object;
	using OptionalFunction = py::object;

	template <typename ItemT>
	static void appendList(py::list &list, ItemT &&item) {
		list.append(std::forward<ItemT>(item));
	}

	template <typename ItemT>
	static void appendMap(py::dict &dict, const std::string &key, ItemT &&item) {
		if (!dict.contains(key.c_str())) {
			dict[key.c_str()] = py::list();
		}
		dict[key.c_str()].cast<py::list>().append(std::forward<ItemT>(item));
	}

	static py::dict makeType(std::optional<uint32_t> classId, const std::string &name,
	                         bool nullable, bool isGeneric, ListType &&inputs) {
		py::dict d;
		if (classId.has_value()) {
			d["classId"] = *classId;
		} else {
			d["classId"] = py::none();
		}
		d["name"] = name;
		d["nullable"] = nullable;
		d["isGeneric"] = isGeneric;
		d["inputs"] = std::move(inputs);
		return d;
	}

	static py::dict makeVar(const std::string &name, TypeResult &&type,
	                        bool isStatic, bool isVal, bool isLateinit, bool hasDefault) {
		py::dict d;
		d["name"] = name;
		d["type"] = std::move(type);
		d["isStatic"] = isStatic;
		d["isVal"] = isVal;
		d["isLateinit"] = isLateinit;
		d["hasDefault"] = hasDefault;
		return d;
	}

	static py::dict makeFunction(uint32_t id, const std::string &name,
	                             const std::string &library,
	                             std::optional<uint32_t> ownerClassId,
	                             bool isStatic, bool hasTrailingClosure,
	                             ListVar &&params,
	                             TypeResult &&returnType) {
		py::dict d;
		d["id"] = id;
		d["name"] = name;
		d["library"] = library;
		if (ownerClassId.has_value()) {
			d["ownerClassId"] = *ownerClassId;
		} else {
			d["ownerClassId"] = py::none();
		}
		d["isStatic"] = isStatic;
		d["hasTrailingClosure"] = hasTrailingClosure;
		d["params"] = std::move(params);
		d["returnType"] = std::move(returnType);
		return d;
	}

	static py::dict makeGeneric(const std::string &name, bool mustBeExtends,
	                            std::optional<uint32_t> extendClassId) {
		py::dict d;
		d["name"] = name;
		d["mustBeExtends"] = mustBeExtends;
		if (extendClassId.has_value()) {
			d["extendClassId"] = *extendClassId;
		} else {
			d["extendClassId"] = py::none();
		}
		return d;
	}

	static py::dict makeClass(uint32_t classId, const std::string &name,
	                          const std::string &library,
	                          ListGeneric &&generics,
	                          ListVar &&properties,
	                          MapFunction &&functions,
	                          ListString &&enumEntries) {
		py::dict d;
		d["classId"] = classId;
		d["name"] = name;
		d["library"] = library;
		d["generics"] = std::move(generics);
		d["properties"] = std::move(properties);
		d["functions"] = std::move(functions);
		d["enumEntries"] = std::move(enumEntries);
		return d;
	}

	static py::dict makeLibrary(const std::string &name, bool autoImport, bool loaded) {
		py::dict d;
		d["name"] = name;
		d["autoImport"] = autoImport;
		d["loaded"] = loaded;
		return d;
	}

	static py::dict makeAlias(const std::string &name, const std::string &target) {
		py::dict d;
		d["name"] = name;
		d["target"] = target;
		return d;
	}

	static OptionalClass makeNullClass() {
		return py::none();
	}

	static OptionalFunction makeNullFunction() {
		return py::none();
	}
};
#endif

class ReflectionEngine {
  public:
	static inline std::string resolveLibrary(const ACompiler &compiler, const char *path) {
		if (!path || path[0] == '\0') {
			return "";
		}
		if (compiler.mainSource && compiler.mainSource->path == path) {
			return "";
		}
		if (std::strcmp(path, "stdlib") == 0) {
			return "stdlib";
		}
		auto it = compiler.builtInLibrariesMap.find(path);
		if (it != compiler.builtInLibrariesMap.end()) {
			return path;
		}
		return "";
	}

	static inline std::string resolveClassLibrary(const ACompiler &compiler, ClassId cid) {
		if (cid < compiler.parserContext.classInfo.size() && compiler.parserContext.classInfo[cid]) {
			ClassInfo *cInfo = compiler.parserContext.classInfo[cid];
			if (cInfo->mode) {
				return resolveLibrary(compiler, cInfo->mode->path.c_str());
			}
		}
		return "";
	}

	template <typename Adapter>
	static typename Adapter::TypeResult buildTypeMeta(const ACompiler &compiler, ClassDeclaration *cd) {
		typename Adapter::ListType inputs;
		if (!cd) {
			return Adapter::makeType(static_cast<uint32_t>(DefaultClass::voidClassId), "Void",
			                         false, false, std::move(inputs));
		}

		bool isGeneric = cd->isGenericDeclaration;
		bool nullable = cd->nullable;
		std::string name;
		std::optional<uint32_t> classId = std::nullopt;

		if (isGeneric) {
			classId = std::nullopt;
			if (cd->baseClassLexerStringId < compiler.parserContext.lexerString.size()) {
				name = std::string(compiler.parserContext.lexerString[cd->baseClassLexerStringId]);
			}
		} else {
			if (cd->baseClassLexerStringId < compiler.parserContext.lexerString.size()) {
				name = std::string(compiler.parserContext.lexerString[cd->baseClassLexerStringId]);
			}
			if (cd->classId.has_value()) {
				classId = *cd->classId;
				if (*classId < compiler.vm.data.classes.size() && compiler.vm.data.classes[*classId]) {
					AClass *clazz = compiler.vm.data.classes[*classId];
					if (clazz->genericBaseClassId != 0 && clazz->genericBaseClassId != DefaultClass::nullClassId) {
						classId = clazz->genericBaseClassId;
					}
					if (name.empty()) {
						name = clazz->getName(const_cast<CompiledProgram &>(compiler.vm.data));
					}
				}
			} else {
				auto itDef = compiler.parserContext.defaultClassMap.find(cd->baseClassLexerStringId);
				if (itDef != compiler.parserContext.defaultClassMap.end()) {
					classId = itDef->second;
				}
			}
			auto bracketPos = name.find('<');
			if (bracketPos != std::string::npos) {
				name = name.substr(0, bracketPos);
			}
		}

		for (auto *inCd : cd->inputClassId) {
			Adapter::appendList(inputs, buildTypeMeta<Adapter>(compiler, inCd));
		}

		return Adapter::makeType(classId, name, nullable, isGeneric, std::move(inputs));
	}

	template <typename Adapter>
	static typename Adapter::FunctionResult
	buildFunctionMeta(const ACompiler &compiler, uint32_t fid,
	                  std::optional<uint32_t> ownerClassIdOpt,
	                  const std::string &nameOverride) {
		std::string name = nameOverride;
		Function *func = (fid < compiler.vm.data.functions.size()) ? compiler.vm.data.functions[fid] : nullptr;
		FunctionInfo *fInfo = (fid < compiler.parserContext.functionInfo.size()) ? compiler.parserContext.functionInfo[fid] : nullptr;

		if (name.empty()) {
			if (func) {
				name = compiler.vm.data.stringArena.get(func->nameStringOffset);
			} else if (fInfo && fInfo->tokenIndex < compiler.parserContext.tokens.size()) {
				name = std::string(compiler.parserContext.lexerString[compiler.parserContext.tokens[fInfo->tokenIndex].indexData]);
			}
		}

		std::string library;
		if (func && func->path) {
			library = resolveLibrary(compiler, func->path);
		} else if (fInfo && fInfo->clazz && fInfo->clazz->id < compiler.parserContext.classInfo.size() &&
		           compiler.parserContext.classInfo[fInfo->clazz->id] &&
		           compiler.parserContext.classInfo[fInfo->clazz->id]->mode) {
			library = resolveLibrary(compiler, compiler.parserContext.classInfo[fInfo->clazz->id]->mode->path.c_str());
		}

		std::optional<uint32_t> ownerClassId = ownerClassIdOpt;
		if (!ownerClassId.has_value()) {
			if (fInfo && fInfo->clazz) {
				ownerClassId = fInfo->clazz->id;
			}
		}

		bool isStatic = false;
		if (ownerClassId.has_value()) {
			if (func && (func->functionFlags & FunctionFlags::FUNC_IS_STATIC)) {
				isStatic = true;
			} else if (*ownerClassId < compiler.parserContext.classInfo.size() && compiler.parserContext.classInfo[*ownerClassId]) {
				ClassInfo *cInfo = compiler.parserContext.classInfo[*ownerClassId];
				auto itStr = compiler.parserContext.lexerStringMap.find(name);
				if (itStr != compiler.parserContext.lexerStringMap.end()) {
					if (cInfo->staticFunc.find(itStr->second) != cInfo->staticFunc.end()) {
						isStatic = true;
					}
				}
			}
		}

		bool hasTrailingClosure = compiler.isFunctionTrailingClosure(fid);

		typename Adapter::ListVar params;
		if (fInfo && fInfo->parameter) {
			size_t startIndex = 0;
			if (!isStatic && ownerClassId.has_value() && !fInfo->parameter->parameters.empty()) {
				DeclarationNode *firstParam = fInfo->parameter->parameters[0];
				if (firstParam && (firstParam->name == "this" || firstParam->baseName == lexerIdthis)) {
					startIndex = 1;
				}
			}
			for (size_t i = startIndex; i < fInfo->parameter->parameters.size(); ++i) {
				DeclarationNode *pNode = fInfo->parameter->parameters[i];
				if (!pNode) continue;
				std::string pName;
				if (!pNode->name.empty()) {
					pName = std::string(pNode->name);
				} else if (pNode->baseName < compiler.parserContext.lexerString.size()) {
					pName = std::string(compiler.parserContext.lexerString[pNode->baseName]);
				}
				auto pType = buildTypeMeta<Adapter>(compiler, pNode->classDeclaration);
				bool hasDef = (i >= fInfo->parameter->defaultValuePos);
				Adapter::appendList(params, Adapter::makeVar(pName, std::move(pType), false, pNode->isVal, false, hasDef));
			}
		} else if (func) {
			size_t startIndex = (!isStatic && ownerClassId.has_value() && func->argSize > 0) ? 1 : 0;
			for (size_t i = startIndex; i < func->argSize; ++i) {
				ClassId argCid = func->args ? func->args[i] : DefaultClass::voidClassId;
				std::string argName = "arg" + std::to_string(i - startIndex);
				std::string typeName;
				if (argCid < compiler.vm.data.classes.size() && compiler.vm.data.classes[argCid]) {
					typeName = compiler.vm.data.classes[argCid]->getName(const_cast<CompiledProgram &>(compiler.vm.data));
				}
				typename Adapter::ListType inps;
				auto pType = Adapter::makeType(argCid, typeName, false, false, std::move(inps));
				Adapter::appendList(params, Adapter::makeVar(argName, std::move(pType), false, true, false, false));
			}
		}

		typename Adapter::TypeResult returnType;
		if (fInfo && fInfo->returnClass) {
			returnType = buildTypeMeta<Adapter>(compiler, fInfo->returnClass);
		} else if (func) {
			std::string retName;
			if (func->returnId < compiler.vm.data.classes.size() && compiler.vm.data.classes[func->returnId]) {
				retName = compiler.vm.data.classes[func->returnId]->getName(const_cast<CompiledProgram &>(compiler.vm.data));
			}
			typename Adapter::ListType inps;
			returnType = Adapter::makeType(func->returnId, retName, false, false, std::move(inps));
		} else {
			typename Adapter::ListType inps;
			returnType = Adapter::makeType(static_cast<uint32_t>(DefaultClass::voidClassId), "Void", false, false, std::move(inps));
		}

		return Adapter::makeFunction(fid, name, library, ownerClassId, isStatic,
		                             hasTrailingClosure, std::move(params), std::move(returnType));
	}

	template <typename Adapter>
	static typename Adapter::ClassResult buildClassMeta(const ACompiler &compiler, uint32_t cid) {
		std::string name;
		if (cid < compiler.vm.data.classes.size() && compiler.vm.data.classes[cid]) {
			name = compiler.vm.data.classes[cid]->getName(const_cast<CompiledProgram &>(compiler.vm.data));
		}

		std::string library = resolveClassLibrary(compiler, cid);

		typename Adapter::ListGeneric generics;
		typename Adapter::ListVar properties;
		typename Adapter::MapFunction functions;
		typename Adapter::ListString enumEntries;

		ClassInfo *cInfo = (cid < compiler.parserContext.classInfo.size()) ? compiler.parserContext.classInfo[cid] : nullptr;
		if (cInfo) {
			if (cInfo->genericData) {
				for (auto *genDecl : cInfo->genericData->genericDeclarations) {
					if (!genDecl) continue;
					std::string gName;
					if (genDecl->nameId < compiler.parserContext.lexerString.size()) {
						gName = std::string(compiler.parserContext.lexerString[genDecl->nameId]);
					}
					bool mustExtends = genDecl->condition.has_value();
					std::optional<uint32_t> extClassId = std::nullopt;
					if (mustExtends && genDecl->condition->classDeclaration) {
						if (genDecl->condition->classDeclaration->classId) {
							extClassId = *genDecl->condition->classDeclaration->classId;
						} else {
							auto itDef = compiler.parserContext.defaultClassMap.find(
							    genDecl->condition->classDeclaration->baseClassLexerStringId);
							if (itDef != compiler.parserContext.defaultClassMap.end()) {
								extClassId = itDef->second;
							}
						}
					}
					Adapter::appendList(generics, Adapter::makeGeneric(gName, mustExtends, extClassId));
				}
			}

			std::unordered_set<std::string> seenProps;
			for (auto *decl : cInfo->member) {
				if (!decl) continue;
				std::string propName;
				if (!decl->name.empty()) {
					propName = std::string(decl->name);
				} else if (decl->baseName < compiler.parserContext.lexerString.size()) {
					propName = std::string(compiler.parserContext.lexerString[decl->baseName]);
				}
				auto dotPos = propName.find_last_of('.');
				if (dotPos != std::string::npos) {
					propName = propName.substr(dotPos + 1);
				}
				if (seenProps.insert(propName).second) {
					auto propType = buildTypeMeta<Adapter>(compiler, decl->classDeclaration);
					Adapter::appendList(properties, Adapter::makeVar(propName, std::move(propType), false,
					                                                decl->isVal, decl->isLateInit, false));
				}
			}

			for (const auto &[nameId, decl] : cInfo->staticMember) {
				if (!decl) continue;
				std::string propName;
				if (!decl->name.empty()) {
					propName = std::string(decl->name);
				} else if (decl->baseName < compiler.parserContext.lexerString.size()) {
					propName = std::string(compiler.parserContext.lexerString[decl->baseName]);
				}
				auto dotPos = propName.find_last_of('.');
				if (dotPos != std::string::npos) {
					propName = propName.substr(dotPos + 1);
				}
				if (seenProps.insert(propName).second) {
					auto propType = buildTypeMeta<Adapter>(compiler, decl->classDeclaration);
					Adapter::appendList(properties, Adapter::makeVar(propName, std::move(propType), true,
					                                                decl->isVal, decl->isLateInit, false));
				}
			}

			std::unordered_set<FunctionId> seenFuncs;

			for (const auto &[nameId, fids] : cInfo->allFunction) {
				std::string fName;
				if (nameId < compiler.parserContext.lexerString.size()) {
					fName = std::string(compiler.parserContext.lexerString[nameId]);
				}
				if (fName.empty()) continue;

				for (auto fid : fids) {
					if (seenFuncs.insert(fid).second) {
						Adapter::appendMap(functions, fName, buildFunctionMeta<Adapter>(compiler, fid, cid, fName));
					}
				}
			}

			for (const auto &[nameId, nodes] : cInfo->genericFunctionMap) {
				std::string fName;
				if (nameId < compiler.parserContext.lexerString.size()) {
					fName = std::string(compiler.parserContext.lexerString[nameId]);
				}
				if (fName.empty()) continue;

				for (auto *cNode : nodes) {
					if (!cNode) continue;
					if (seenFuncs.insert(cNode->id).second) {
						Adapter::appendMap(functions, fName, buildFunctionMeta<Adapter>(compiler, cNode->id, cid, fName));
					}
				}
			}

			for (const auto &[nameId, constNode] : cInfo->constValue) {
				if (nameId < compiler.parserContext.lexerString.size()) {
					Adapter::appendList(enumEntries, std::string(compiler.parserContext.lexerString[nameId]));
				}
			}
		}

		return Adapter::makeClass(cid, name, library, std::move(generics),
		                          std::move(properties), std::move(functions), std::move(enumEntries));
	}

	template <typename Adapter>
	static typename Adapter::ListLibrary getLibraries(const ACompiler &compiler) {
		typename Adapter::ListLibrary result;
		for (const auto &lib : compiler.builtInLibraries) {
			if (!lib) continue;
			bool autoImport = (compiler.autoImportMap.find(lib->path) != compiler.autoImportMap.end());
			bool loaded = (compiler.parserContext.importMap.find(lib->path) != compiler.parserContext.importMap.end());
			Adapter::appendList(result, Adapter::makeLibrary(lib->path, autoImport, loaded));
		}
		return result;
	}

	template <typename Adapter>
	static typename Adapter::ListClass getClasses(const ACompiler &compiler) {
		if (!compiler.loadedBuiltIn) {
			const_cast<ACompiler &>(compiler).loadBuiltInFunctions();
		}

		typename Adapter::ListClass result;
		std::unordered_set<ClassId> seen;

		for (const auto &[nameId, cid] : compiler.parserContext.defaultClassMap) {
			if (cid >= compiler.vm.data.classes.size() || !compiler.vm.data.classes[cid]) {
				continue;
			}
			std::string className = compiler.vm.data.classes[cid]->getName(const_cast<CompiledProgram &>(compiler.vm.data));
			if (className.find('<') != std::string::npos) {
				continue;
			}
			if (seen.insert(cid).second) {
				Adapter::appendList(result, buildClassMeta<Adapter>(compiler, cid));
			}
		}

		return result;
	}

	template <typename Adapter>
	static typename Adapter::OptionalClass getClass(const ACompiler &compiler, uint32_t classId) {
		if (!compiler.loadedBuiltIn) {
			const_cast<ACompiler &>(compiler).loadBuiltInFunctions();
		}

		if (classId >= compiler.vm.data.classes.size() || !compiler.vm.data.classes[classId]) {
			return Adapter::makeNullClass();
		}

		std::string className = compiler.vm.data.classes[classId]->getName(const_cast<CompiledProgram &>(compiler.vm.data));
		if (className.find('<') != std::string::npos) {
			return Adapter::makeNullClass();
		}

		return buildClassMeta<Adapter>(compiler, classId);
	}

	template <typename Adapter>
	static typename Adapter::ListFunction getFunctions(const ACompiler &compiler) {
		if (!compiler.loadedBuiltIn) {
			const_cast<ACompiler &>(compiler).loadBuiltInFunctions();
		}

		typename Adapter::ListFunction result;
		std::unordered_set<FunctionId> seen;

		for (const auto &[nameId, fids] : compiler.parserContext.globalFunction) {
			std::string fName;
			if (nameId < compiler.parserContext.lexerString.size()) {
				fName = std::string(compiler.parserContext.lexerString[nameId]);
			}
			if (fName.empty() || fName[0] == '.') continue;

			for (auto fid : fids) {
				if (seen.insert(fid).second) {
					Adapter::appendList(result, buildFunctionMeta<Adapter>(compiler, fid, std::nullopt, fName));
				}
			}
		}

		for (const auto &[nameId, nodes] : compiler.parserContext.genericFunctionMap) {
			std::string fName;
			if (nameId < compiler.parserContext.lexerString.size()) {
				fName = std::string(compiler.parserContext.lexerString[nameId]);
			}
			if (fName.empty() || fName[0] == '.') continue;

			for (auto *cNode : nodes) {
				if (!cNode) continue;
				if (seen.insert(cNode->id).second) {
					Adapter::appendList(result, buildFunctionMeta<Adapter>(compiler, cNode->id, std::nullopt, fName));
				}
			}
		}

		return result;
	}

	template <typename Adapter>
	static typename Adapter::OptionalFunction getFunction(const ACompiler &compiler, uint32_t funcId) {
		if (!compiler.loadedBuiltIn) {
			const_cast<ACompiler &>(compiler).loadBuiltInFunctions();
		}

		if (funcId >= compiler.vm.data.functions.size() || !compiler.vm.data.functions[funcId]) {
			return Adapter::makeNullFunction();
		}

		return buildFunctionMeta<Adapter>(compiler, funcId, std::nullopt, "");
	}

	template <typename Adapter>
	static typename Adapter::ListAlias getAliases(const ACompiler &compiler) {
		if (!compiler.loadedBuiltIn) {
			const_cast<ACompiler &>(compiler).loadBuiltInFunctions();
		}

		typename Adapter::ListAlias result;
		std::unordered_set<std::string> seen;

		for (const auto &[aliasName, cid] : compiler.parserContext.classAliasMap) {
			std::string nameStr(aliasName);
			if (seen.insert(nameStr).second) {
				std::string target;
				if (cid < compiler.vm.data.classes.size() && compiler.vm.data.classes[cid]) {
					target = compiler.vm.data.classes[cid]->getName(const_cast<CompiledProgram &>(compiler.vm.data));
				}
				Adapter::appendList(result, Adapter::makeAlias(nameStr, target));
			}
		}

		for (const auto &[nameId, typealiasData] : compiler.parserContext.typealiasMap) {
			if (nameId < compiler.parserContext.lexerString.size()) {
				std::string nameStr(compiler.parserContext.lexerString[nameId]);
				if (seen.insert(nameStr).second) {
					std::string target;
					if (typealiasData && typealiasData->classDeclaration) {
						target = typealiasData->classDeclaration->template getName<true>(
						    const_cast<CompiledProgram &>(compiler.vm.data),
						    const_cast<ParserContext &>(compiler.parserContext));
					}
					Adapter::appendList(result, Adapter::makeAlias(nameStr, target));
				}
			}
		}

		return result;
	}
};

} // namespace Autolang

#endif
