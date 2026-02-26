#ifndef ACOMPILER_CPP
#define ACOMPILER_CPP

#include "ACompiler.hpp"
#include "frontend/libs/math.hpp"
#include "frontend/libs/stdlib.hpp"
#include "frontend/libs/time.hpp"
#include "frontend/libs/vm.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>

namespace AutoLang {

LibraryData *ACompiler::requestImport(LibraryData *currentLibrary,
                                      const char *path) {
	auto it = builtInLibrariesMap.find(std::string(path));
	if (it != builtInLibrariesMap.end()) {
		return builtInLibraries[it->second];
	}

#ifdef __EMSCRIPTEN__
	if (path[0] == '.')
		return nullptr;
	return nullptr;
#else
	if (path[0] != '.')
		return nullptr;
	std::filesystem::path input = path;
	std::filesystem::path currentPath;
	if (currentLibrary->isFile) {
		currentPath = std::filesystem::path(currentLibrary->path).parent_path();
	} else {
		currentPath = std::filesystem::current_path();
	}
	std::filesystem::path resolved = (currentPath / input).lexically_normal();
	if (!std::filesystem::exists(resolved)) {
		return nullptr;
	}
	// std::cout<<input<<"\n"<<currentPath<<"\n"<<resolved<<"\n";
	LibraryData *library =
	    new LibraryData(resolved.string(), 0, currentLibrary->nativeFuncMap);
	generatedLibraries.push_back(library);
	Lexer::loadFile(&parserContext, library);
	return library;
#endif
}

void ACompiler::lexerTextToToken(LibraryData *library) {
	switch (state) {
		case CompilerState::BYTECODE_READY:
			throw std::logic_error("Bytecode already generated. Call run() to "
			                       "execute or refresh() to register.");
			break;
		case CompilerState::ERROR:
			throw std::logic_error("Compiler is in ERROR state. Call refresh() "
			                       "before register source.");
		default:
			break;
	}
	auto &context = parserContext;
	auto &compile = vm.data;

	loadSource(library);
	library->rawData.clear();
}

void ACompiler::loadSource(LibraryData *library) {
	auto &context = parserContext;
	auto &compile = vm.data;
	std::vector<Offset> importOffset;
	lexerData(in_data, *this, library, &importOffset);
	// for (auto& token : lexerContext.tokens) {
	// 	std::cout<<token.toString(context)<<" ";
	// }
	// std::cerr<<"h "<<source.mode.path<<": "<<source.end<<"\n";
	if (library->lexerContext.hasError) {
		state = CompilerState::ERROR;
		return;
	}
	for (auto &pos : importOffset) {
		auto lib = loadImport(in_data, library, library->lexerContext.tokens,
		                      *this, (size_t)pos);
		for (auto &[key, l] : lib->dependencies) {
			library->dependencies[key] = l;
		}
		library->dependencies[lib->path] = lib;
	}

	state = CompilerState::ANALYZED;
}

void ACompiler::registerFromSource(const char *path, bool autoImport,
                                   const ANativeMap &nativeFuncMap) {
	uint32_t flags = LibraryFlags::IS_BUILT_IN;
	if (autoImport) {
		flags |= LibraryFlags::AUTO_IMPORT;
	}
	uint32_t libraryOffset = builtInLibraries.size();
	auto lib = new LibraryData(path, flags, nativeFuncMap);
	builtInLibraries.push_back(lib);
	builtInLibrariesMap[path] = libraryOffset;
	Lexer::loadFile(&parserContext, lib);
	if (autoImport) {
		autoImportMap[path] = lib;
	}
}

void ACompiler::registerFromSource(const char *path, const char *data,
                                   bool autoImport,
                                   const ANativeMap &nativeFuncMap) {

	uint32_t flags = LibraryFlags::IS_BUILT_IN;
	if (autoImport) {
		flags |= LibraryFlags::AUTO_IMPORT;
	}
	uint32_t libraryOffset = builtInLibraries.size();
	auto lib = new LibraryData(path, flags, nativeFuncMap);
	lib->rawData = data;
	builtInLibraries.push_back(lib);
	builtInLibrariesMap[path] = libraryOffset;
	if (autoImport) {
		autoImportMap[path] = lib;
	}
}

void ACompiler::loadBuiltInFunctions() {
	for (auto *library : builtInLibraries) {
		if (!library->lexerContext.tokens.empty())
			continue;
		lexerTextToToken(library);
	}
	parserContext.importMap.clear();
}

void ACompiler::loadMainSource(const char *path,
                               const ANativeMap &nativeFuncMap) {
	if (!loadedBuiltIn) {
		loadBuiltInFunctions();
	}
	LibraryData *library = new LibraryData(path, 0, nativeFuncMap);
	generatedLibraries.push_back(library);
	Lexer::loadFile(&parserContext, library);
	loadMainSource(library);
}

void ACompiler::loadMainSource(const char *path, const char *data,
                               const ANativeMap &nativeFuncMap) {
	if (!loadedBuiltIn) {
		loadBuiltInFunctions();
	}
	LibraryData *library = new LibraryData(path, 0, nativeFuncMap);
	library->rawData = data;
	generatedLibraries.push_back(library);
	loadMainSource(library);
}

void ACompiler::loadMainSource(LibraryData *library) {
	auto &context = parserContext;
	auto &compile = vm.data;

	mainSource = library;

	std::string autoImportStr;
	for (auto &[k, lib] : autoImportMap) {
		autoImportStr += "@import(\"" + lib->path + "\")";
		// library->dependencies[k] = lib;
		// library->lexerContext.tokens.insert(
		//     library->lexerContext.tokens.begin(),
		//     lib->lexerContext.tokens.begin(),
		//     lib->lexerContext.tokens.end());
		// for (auto &[key, l] : lib->dependencies) {
		// 	library->dependencies[key] = l;
		// }
	}
	library->rawData = autoImportStr + library->rawData;

	std::vector<Offset> importOffset;
	lexerData(in_data, *this, library, &importOffset);

	if (mainSource->lexerContext.hasError) {
		state = CompilerState::ERROR;
		return;
	}

	library->lexerContext.tokens.pop_back();

	for (auto &pos : importOffset) {
		auto lib = loadImport(in_data, library, library->lexerContext.tokens,
		                      *this, (size_t)pos);
		for (auto &[key, l] : lib->dependencies) {
			library->dependencies[key] = l;
		}
		library->dependencies[lib->path] = lib;
	}

	auto &newEstimate = mainSource->lexerContext.estimate;

	for (auto &[name, lib] : library->dependencies) {
		const auto &libEstimate = lib->lexerContext.estimate;

		newEstimate.declaration += libEstimate.declaration;
		newEstimate.classes += libEstimate.classes;
		newEstimate.functions += libEstimate.functions;
		newEstimate.constructorNode += libEstimate.constructorNode;
		newEstimate.ifNode += libEstimate.ifNode;
		newEstimate.whileNode += libEstimate.whileNode;
		newEstimate.returnNode += libEstimate.returnNode;
		newEstimate.setNode += libEstimate.setNode;
		newEstimate.binaryNode += libEstimate.binaryNode;
		newEstimate.tryCatchNode += libEstimate.tryCatchNode;
		newEstimate.throwNode += libEstimate.throwNode;
	}

	context.mode = library;
	context.tokens = mainSource->lexerContext.tokens;
	context.mainLexerContext = &library->lexerContext;
	context.loadingLibs.reserve(8);
	context.loadingLibs.push_back(library);
	loadedMainSource = true;
}

void ACompiler::generateBytecodes() {
	switch (state) {
		case CompilerState::BYTECODE_READY:
			throw std::logic_error("Bytecode already generated. Call run() to "
			                       "execute instead of generating again.");
			break;
		case CompilerState::ERROR:
			throw std::logic_error("Compiler is in ERROR state. Call refresh() "
			                       "before generate bytecode.");
		case CompilerState::READY:
			throw std::logic_error(
			    "No bytecode generated. Compile source before generating.");
		default:
			break;
	}

	if (!loadedMainSource) {
		throw std::logic_error(
		    "No main source generated. Compile main source before generating.");
	}

	auto &context = parserContext;
	auto &compile = vm.data;

	estimate(in_data, mainSource->lexerContext);

	// for (auto& token : context.tokens) {
	// 	std::cerr<<token.toString(context)<<" ";
	// }
	// std::cerr<<"\n";

	auto startParserTime = std::chrono::high_resolution_clock::now();
	context.currentTokenPos = 0;
	size_t &i = context.currentTokenPos;
	while (i < context.tokens.size()) {
		try {
			auto node = loadLine(in_data, i);
			ensureEndline(in_data, i);
			++i;
			if (node == nullptr) {
				continue;
			}
			if (context.annotationFlags || context.modifierflags) {
				ExprNode::deleteNode(node);
				ensureNoKeyword(in_data, i);
				ensureNoAnnotations(in_data, i);
			}
			context.getCurrentFunctionInfo(in_data)->block.nodes.push_back(
			    node);
		} catch (const std::runtime_error &err) {
			context.hasError = true;
			std::cout << "Unexpected exception: " << err.what() << '\n';
		} catch (const ParserError &err) {
			context.hasError = true;
			context.logMessage(err.line, err.message);
			if (i >= context.tokens.size())
				break;
			uint32_t line = context.tokens[i].line;
			Lexer::Token *_;
			while (nextTokenSameLine(&_, context.tokens, i, line)) {
			}
		}
	}
	// auto parserTime = std::chrono::high_resolution_clock::now();
	// std::cerr << "Generated node : "
	//           << std::chrono::duration_cast<std::chrono::milliseconds>(
	//                  parserTime - startParserTime)
	//                  .count()
	//           << " ms" << '\n';

	if (context.hasError) {
		state = CompilerState::ERROR;
		return;
	}

	try {
		printDebug("-----------------AST Node-----------------\n");

		printDebug("Load all class declarations");
		// Load class declaration (generics as T won't have class id)
		for (auto *declaration : context.allClassDeclarations) {
			declaration->load<true>(in_data);
		}

		printDebug("Start optimize classes");
		size_t sizeNewClasses = context.newClasses.getSize();
		for (size_t i = 0; i < sizeNewClasses; ++i) {
			context.newClasses[i]->optimize(in_data);
			// auto classInfo =
			// context.classInfo[context.newClasses[i]->classId];
		}

		printDebug("Start optimize declaration nodes in functions");
		for (int i = 0; i < context.declarationNodePool.index; ++i) {
			context.declarationNodePool.objects[i].optimize(in_data);
		}
		for (auto *node : context.declarationNodePool.vecs) {
			node->optimize(in_data);
		}

		printDebug("Start optimize constructor nodes");
		for (int i = 0; i < sizeNewClasses; ++i) {
			auto *node = context.newClasses[i];
			auto clazz = compile.classes[node->classId];
			auto classInfo = context.classInfo[clazz->id];
			if (classInfo->genericData)
				continue;
			if (classInfo->primaryConstructor) {
				classInfo->primaryConstructor->resolve(in_data);
				classInfo->primaryConstructor->optimize(in_data);
			} else {
				for (auto *constructor : classInfo->secondaryConstructor) {
					constructor->resolve(in_data);
					constructor->optimize(in_data);
				}
			}
		}
		printDebug("Start optimize functions");
		size_t sizeNewFunctions = context.newFunctions.getSize();
		for (int i = 0; i < sizeNewFunctions; ++i) {
			auto *createFunctionNode = context.newFunctions[i];
			if (createFunctionNode->contextCallClassId) {
				auto classInfo =
				    context.classInfo[*createFunctionNode->contextCallClassId];
				if (classInfo->genericData)
					continue;
			}
			createFunctionNode->optimize(in_data);
		}

		printDebug("Start load virtual");
		for (size_t i = 0; i < sizeNewClasses; ++i) {
			auto *newClass = context.newClasses[i];
			auto classInfo = context.classInfo[newClass->classId];
			if (classInfo->genericData)
				continue;
			newClass->loadSuper(in_data);
		}

		printDebug("Start optimize detach member default value");
		for (size_t i = 0; i < sizeNewClasses; ++i) {
			auto *newClass = context.newClasses[i];
			auto classInfo = context.classInfo[newClass->classId];
			if (classInfo->genericData)
				continue;
			newClass->body.resolve(in_data);
			newClass->body.optimize(in_data);
		}

		printDebug("Start optimize static nodes");
		for (auto &node : context.staticNode) {
			ParserContext::mode = node->mode;
			node = node->resolve(in_data);
			node->optimize(in_data);
		}

		printDebug("Start put bytecodes constructor");
		for (int i = 0; i < sizeNewClasses; ++i) {
			auto *node = context.newClasses[i];
			auto classInfo =
			    context.classInfo[compile.classes[node->classId]->id];
			if (classInfo->genericData)
				continue;
			if (classInfo->primaryConstructor) {
				// Put initial bytecodes, example val a = 5 => SetNode
				//  node->body.optimize(in_data);
				auto func =
				    compile.functions[classInfo->primaryConstructor->funcId];
				auto &bytecodes = func->bytecodes;
				node->body.resolve(in_data);
				node->body.putBytecodes(in_data, bytecodes);
				node->body.rewrite(in_data, bytecodes);

				classInfo->primaryConstructor->body.resolve(in_data);
				classInfo->primaryConstructor->body.optimize(in_data);
				classInfo->primaryConstructor->body.putBytecodes(
				    in_data, func->bytecodes);
				classInfo->primaryConstructor->body.rewrite(in_data,
				                                            func->bytecodes);
			} else {
				for (auto &constructor : classInfo->secondaryConstructor) {
					auto func = compile.functions[constructor->funcId];
					// Put initial bytecodes, example val a = 5 => SetNode a
					// and value 5
					//  node->body.optimize(in_data);
					node->body.resolve(in_data);
					node->body.putBytecodes(in_data, func->bytecodes);
					node->body.rewrite(in_data, func->bytecodes);
					// Put constructor bytecodes
					constructor->body.resolve(in_data);
					constructor->body.optimize(in_data);
					constructor->body.putBytecodes(in_data, func->bytecodes);
					constructor->body.rewrite(in_data, func->bytecodes);
				}
			}
		}

		printDebug("Start put bytecodes static nodes");
		for (auto &node : context.staticNode) {
			node->putBytecodes(in_data,
			                   context.getMainFunction(in_data)->bytecodes);
			node->rewrite(in_data, context.getMainFunction(in_data)->bytecodes);
		}

		printDebug("Inference function type");
		for (FunctionId id : context.mustInferenceFunctionType) {
			auto *funcInfo = context.functionInfo[id];
			if (!funcInfo->inferenceNode->loaded) {
				funcInfo->inferenceNode->resolve(in_data);
				funcInfo->inferenceNode->optimize(in_data);
				funcInfo->inferenceNode->loaded = true;
			}
		}

		printDebug("Start optimize bytecodes in main");
		for (auto *&node : context.getMainFunctionInfo(in_data)->block.nodes) {
			ParserContext::mode = node->mode;
			node = node->resolve(in_data);
		}
		context.getMainFunctionInfo(in_data)->block.optimize(in_data);
		printDebug("Start put bytecodes in functions");
		for (int i = 0; i < sizeNewFunctions; ++i) {
			auto *node = context.newFunctions[i];
			if (node->contextCallClassId) {
				auto classInfo = context.classInfo[*node->contextCallClassId];
				if (classInfo->genericData)
					continue;
			}
			auto func = compile.functions[node->id];
			auto funcInfo = context.functionInfo[node->id];
			if ((func->functionFlags & FunctionFlags::FUNC_OVERRIDE) &&
			    !(func->functionFlags & FunctionFlags::FUNC_IS_VIRTUAL)) {
				throw ParserError(
				    node->line,
				    "Function " +
				        compile.functions[node->id]->toString(compile) +
				        " is marked @override");
			}
			if (func->functionFlags & FunctionFlags::FUNC_IS_NATIVE)
				continue;
			if (!funcInfo->inferenceNode) {
				node->body.resolve(in_data);
				node->body.optimize(in_data);
			}
			node->body.putBytecodes(in_data, func->bytecodes);
			node->body.rewrite(in_data, func->bytecodes);
		}
		printDebug("Start put bytecodes in main");
		context.getMainFunctionInfo(in_data)->block.putBytecodes(
		    in_data, context.getMainFunction(in_data)->bytecodes);
		context.getMainFunctionInfo(in_data)->block.rewrite(
		    in_data, context.getMainFunction(in_data)->bytecodes);

		printDebug("Real Declarations: " +
		           std::to_string(context.declarationNodePool.index +
		                          context.declarationNodePool.vecs.size()));
		printDebug("Real New classes: " +
		           std::to_string(context.newClasses.getSize()));
		printDebug("Real New functions: " +
		           std::to_string(context.newFunctions.getSize()));
		printDebug("Real ClassInfo: " +
		           std::to_string(context.classInfo.size()));
		printDebug("Real FunctionInfo: " +
		           std::to_string(context.functionInfo.size()));

		printDebug("Real Classes: " + std::to_string(compile.classes.size()));
		printDebug("Real Functions: " +
		           std::to_string(compile.functions.size()));
		printDebug("Real ClassMap: " + std::to_string(compile.classMap.size()));
		printDebug("Real FuncMap: " + std::to_string(compile.funcMap.size()));
		// size_t total = 0;
		// for (auto& [k, v] : compile.funcMap)
		// 	++total;

		// printDebug("TOTAL FUNC IN MAP: " + std::to_string(total));
	} catch (const ParserError &err) {
		context.hasError = true;
		context.logMessage(err.line, err.message);
	} catch (const std::exception &err) {
		context.hasError = true;
		std::cout << "Unexpected exception: " << err.what() << '\n';
	}

	// auto resolveTime = std::chrono::high_resolution_clock::now();
	// std::cerr << "Optimize and Putbytecode time : "
	//           << std::chrono::duration_cast<std::chrono::milliseconds>(
	//                  resolveTime - parserTime)
	//                  .count()
	//           << " ms" << '\n';

	if (context.hasError) {
		state = CompilerState::ERROR;
		return;
	}

	state = CompilerState::BYTECODE_READY;
}

void ACompiler::run() {
	switch (state) {
		case CompilerState::BYTECODE_READY:
			break;
		case CompilerState::ERROR:
			throw std::logic_error("Compiler is in ERROR state. Call "
			                       "refresh() before running.");

		case CompilerState::READY:
			throw std::logic_error("Bytecode not generated. Call "
			                       "generateBytecodes() before running.");

		case CompilerState::ANALYZED:
			throw std::logic_error(
			    "Source is analyzed but bytecode not generated. "
			    "Call generateBytecodes() before running.");
	}
	vm.start();
}

void ACompiler::refresh() {
	parserContext.refresh(vm.data);
	mainSource->lexerContext.refresh();
	for (auto *lib : generatedLibraries) {
		delete lib;
	}
	generatedLibraries.clear();
	vm.restart();
	vm.data.destroy();
	// AutoLang::DefaultClass::init(vm.data);
	// AutoLang::Libs::stdlib::init(*this);
	// AutoLang::DefaultClass::init(*this);
	// AutoLang::DefaultFunction::init(*this);
	vm.data.mainFunctionId = vm.data.registerFunction(
	    nullptr, ".main", nullptr, 0,
	    static_cast<uint32_t>(FunctionFlags::FUNC_IS_STATIC));
	parserContext.functionInfo.push_back(
	    parserContext.functionInfoAllocator.push());
	new (&vm.data.functions[vm.data.mainFunctionId]->bytecodes)
	    std::vector<uint8_t>();
	state = CompilerState::ANALYZED;
	if (vm.globalVariables) {
		delete[] vm.globalVariables;
		vm.globalVariables = nullptr;
	}
}

ACompiler::ACompiler() {
	// auto startCompiler = std::chrono::high_resolution_clock::now();

	AutoLang::Libs::stdlib::init(*this);
	AutoLang::DefaultClass::init(*this);
	AutoLang::DefaultFunction::init(*this);
	AutoLang::Libs::time::init(*this);
	AutoLang::Libs::vm::init(*this);

	vm.data.mainFunctionId = vm.data.registerFunction(
	    nullptr, ".main", nullptr, 0,
	    static_cast<uint32_t>(FunctionFlags::FUNC_IS_STATIC));
	parserContext.functionInfo.push_back(
	    parserContext.functionInfoAllocator.push());
	new (&vm.data.functions[vm.data.mainFunctionId]->bytecodes)
	    std::vector<uint8_t>();
	// AutoLang::DefaultClass::init(vm.data);
	// AutoLang::DefaultFunction::init(vm.data);

	parserContext.init(vm.data);

	// std::cout << "Init time : "
	//           << std::chrono::duration_cast<std::chrono::milliseconds>(
	//                  std::chrono::high_resolution_clock::now() -
	//                  startCompiler) .count()
	//           << " ms" << '\n';
	state = CompilerState::READY;
	AutoLang::Libs::Math::init(*this);
	state = CompilerState::READY;
}

ACompiler::~ACompiler() {
	auto &context = parserContext;
	auto &compile = vm.data;
	freeData(in_data);
	parserContext.refresh(vm.data);
	if (mainSource) {
		mainSource->lexerContext.refresh();
	}
	vm.data.destroy();
	for (auto library : generatedLibraries) {
		delete library;
	}
	for (auto library : builtInLibraries) {
		delete library;
	}
}

} // namespace AutoLang

#endif