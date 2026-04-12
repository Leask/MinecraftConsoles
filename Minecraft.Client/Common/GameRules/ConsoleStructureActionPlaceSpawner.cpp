#include "stdafx.h"
#include "../../../Minecraft.World/StringHelpers.h"
#include "ConsoleStructureActionPlaceSpawner.h"
#include "../../../Minecraft.World/net.minecraft.world.level.levelgen.structure.h"
#include "../../../Minecraft.World/net.minecraft.world.level.h"
#include "../../../Minecraft.World/net.minecraft.world.level.tile.entity.h"

ConsoleStructureActionPlaceSpawner::ConsoleStructureActionPlaceSpawner()
{
	m_tile = Tile::mobSpawner_Id;
	m_entityId = L"Pig";
}

ConsoleStructureActionPlaceSpawner::~ConsoleStructureActionPlaceSpawner()
{
}

void ConsoleStructureActionPlaceSpawner::writeAttributes(DataOutputStream *dos, UINT numAttrs)
{
	ConsoleStructureActionPlaceBlock::writeAttributes(dos, numAttrs + 1);

	ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_entity);
	dos->writeUTF(m_entityId);
}

void ConsoleStructureActionPlaceSpawner::addAttribute(const wstring &attributeName, const wstring &attributeValue)
{
	if(attributeName.compare(L"entity") == 0)
	{
		m_entityId = attributeValue;
#ifndef _CONTENT_PACKAGE
		wprintf(L"ConsoleStructureActionPlaceSpawner: Adding parameter entity=%ls\n",m_entityId.c_str());
#endif
	}
	else
	{
		ConsoleStructureActionPlaceBlock::addAttribute(attributeName, attributeValue);
	}
}

bool ConsoleStructureActionPlaceSpawner::placeSpawnerInLevel(StructurePiece *structure, Level *level, BoundingBox *chunkBB)
{
	int worldX = structure->getWorldX( m_x, m_z );
	int worldY = structure->getWorldY( m_y );
	int worldZ = structure->getWorldZ( m_x, m_z );

	if ( chunkBB->isInside( worldX, worldY, worldZ ) )
	{
		if ( level->getTileEntity( worldX, worldY, worldZ ) != nullptr )
		{
			// Remove the current tile entity
			level->removeTileEntity( worldX, worldY, worldZ );
			level->setTileAndData( worldX, worldY, worldZ, 0, 0, Tile::UPDATE_ALL );
		}

		level->setTileAndData( worldX, worldY, worldZ, m_tile, 0, Tile::UPDATE_ALL );
		shared_ptr<MobSpawnerTileEntity> entity = dynamic_pointer_cast<MobSpawnerTileEntity>(level->getTileEntity( worldX, worldY, worldZ ));

#ifndef _CONTENT_PACKAGE
		wprintf(L"ConsoleStructureActionPlaceSpawner - placing a %ls spawner at (%d,%d,%d)\n", m_entityId.c_str(), worldX, worldY, worldZ);
#endif
		if( entity != nullptr )
		{
			entity->setEntityId(m_entityId);
		}
		return true;
	}
	return false;
}