#ifndef CLASS_INFO_CPP
#define CLASS_INFO_CPP

#include "ClassInfo.hpp"
#include "frontend/parser/ParserContext.hpp"

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
	auto classInfo = context.classInfo[declarationThis->classId];
	auto it = classInfo->memberMap.find(nameId);
	if (it != classInfo->memberMap.end()) {
		auto node = member[it->second];
		if (isStatic)
			ParserError(line, context.lexerString[nameId] + " is not static");
		return context.getPropPool.push(
		    line, node, declarationThis->classId,
		    context.varPool.push(line, declarationThis, false, false), nameId,
		    false, node->nullable, false);
	}
	// }
	return nullptr;
}

DeclarationNode *ClassInfo::findAllMember(in_func, uint32_t line,
                                          LexerStringId nameId, bool isStatic) {
	// Find static member
	{
		auto it = staticMember.find(nameId);
		if (it != staticMember.end()) {
			return it->second;
		}
		// Not found
	}
	// if (declarationThis) {
	auto classInfo = context.classInfo[declarationThis->classId];
	auto it = classInfo->memberMap.find(nameId);
	if (it != classInfo->memberMap.end()) {
		auto node = member[it->second];
		if (isStatic)
			ParserError(line, context.lexerString[nameId] + " is not static");
		return member[it->second];
	}
	// }
	return nullptr;
}

ClassInfo::~ClassInfo() {}

} // namespace AutoLang

#endif