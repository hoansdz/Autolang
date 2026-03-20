#ifndef CLASS_INFO_CPP
#define CLASS_INFO_CPP

#include "ClassInfo.hpp"

namespace AutoLang {

AccessNode *ClassInfo::findDeclaration(in_func, uint32_t line,
                                       LexerStringId nameId, bool isStatic) {
	// Find static member
	{
		auto it = staticMember.find(nameId);
		if (it != staticMember.end()) {
			return context.varPool.push(line, it->second, false,
			                            it->second->nullable);
		}
		// Not found
	}
	// if (declarationThis) {
	const auto &name = context.lexerString[nameId];
	auto clazz = compile.classes[declarationThis->classId];
	auto it = clazz->memberMap.find(name);
	if (it != clazz->memberMap.end()) {
		auto node = member[it->second];
		if (isStatic)
			ParserError(line, name + " is not static");
		return context.getPropPool.push(
		    line, node, declarationThis->classId,
		    context.varPool.push(line, declarationThis, false, false), nameId,
		    false, node->nullable, false);
	}
	// }
	return nullptr;
}

ClassInfo::~ClassInfo() {}

} // namespace AutoLang

#endif