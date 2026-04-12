#pragma once

#include "ConsoleStructureActionPlaceBlock.h"

class StructurePiece;
class Level;
class BoundingBox;
class GRFObject;

class ConsoleStructureActionPlaceSpawner : public ConsoleStructureActionPlaceBlock
{
private:
	wstring m_entityId;
public:
	ConsoleStructureActionPlaceSpawner();
	~ConsoleStructureActionPlaceSpawner();

	virtual ConsoleGameRules::EGameRuleType getActionType() { return ConsoleGameRules::eGameRuleType_PlaceSpawner; }
	
	virtual void writeAttributes(DataOutputStream *dos, UINT numAttrs);
	virtual void addAttribute(const wstring &attributeName, const wstring &attributeValue);

	bool placeSpawnerInLevel(StructurePiece *structure, Level *level, BoundingBox *chunkBB);
};