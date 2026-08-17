#ifndef GENERIC_DATA_HPP
#define GENERIC_DATA_HPP

#include "frontend/parser/node/Node.hpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace Autolang {

struct GenericData {
	HashMap<DeclarationOffset, DeclarationOffset>
	    newPositionOfStaticDeclaration;
	HashMap<ClassDeclaration *, ExprNode *> mustRenameNodes;
	SmallVector<GenericDeclarationNode *, 2> genericDeclarations;
	SmallVector<std::pair<DeclarationNode *, HasClassIdNode *>, 4>
	    staticDeclaration;
	HashMap<LexerStringId, Offset> genericDeclarationMap;
	HashMap<DeclarationNode *, DeclarationNode *> reflectDeclarationMap;
	SmallVector<DeclarationNode *, 4> allDeclaration;
	inline GenericDeclarationNode *findDeclaration(LexerStringId nameId) {
		auto it = genericDeclarationMap.find(nameId);
		if (it == genericDeclarationMap.end()) {
			return nullptr;
		}
		return genericDeclarations[it->second];
	}
	~GenericData() {
		for (auto declaration : genericDeclarations) {
			delete declaration;
		}
	}
};

} // namespace Autolang

#endif