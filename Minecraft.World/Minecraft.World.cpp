#include "stdafx.h"

#include "net.minecraft.world.item.h"
#include "net.minecraft.world.item.alchemy.h"
#include "net.minecraft.world.item.crafting.h"
#include "net.minecraft.world.item.enchantment.h"
#include "net.minecraft.world.level.h"
#include "net.minecraft.world.level.chunk.h"
#include "net.minecraft.world.level.chunk.storage.h"
#include "net.minecraft.world.level.levelgen.structure.h"
#include "net.minecraft.world.level.tile.h"
#include "net.minecraft.world.level.tile.entity.h"
#include "net.minecraft.world.entity.h"
#include "net.minecraft.world.entity.monster.h"
#include "net.minecraft.world.entity.npc.h"
#include "net.minecraft.world.effect.h"

#include "Minecraft.World.h"
#if !defined(_NATIVE_DESKTOP)
#include "../Minecraft.Client/ServerLevel.h"
#endif

#include "CommonStats.h"

void MinecraftWorld_RunStaticCtors()
{
	// The ordering of these static ctors can be important. If they are within statement blocks then
	// DO NOT CHANGE the ordering - 4J Stu

#ifdef _NATIVE_DESKTOP
#define NATIVE_DESKTOP_STATIC_CTOR_TRACE(name) \
	fprintf(stderr, "NativeDesktop world static ctor: %s\n", name)
#else
#define NATIVE_DESKTOP_STATIC_CTOR_TRACE(name) ((void)0)
#endif

	NATIVE_DESKTOP_STATIC_CTOR_TRACE("Packet");
	Packet::staticCtor();

	{
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("MaterialColor");
		MaterialColor::staticCtor();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("Material");
		Material::staticCtor();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("Tile");
		Tile::staticCtor();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("HatchetItem");
		HatchetItem::staticCtor();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("PickaxeItem");
		PickaxeItem::staticCtor();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("ShovelItem");
		ShovelItem::staticCtor();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("BlockReplacements");
		BlockReplacements::staticCtor();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("Biome");
		Biome::staticCtor();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("MobEffect");
		MobEffect::staticCtor();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("Item");
		Item::staticCtor();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("FurnaceRecipes");
		FurnaceRecipes::staticCtor();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("Recipes");
		Recipes::staticCtor();	
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("CommonStats");
		GenericStats::setInstance(new CommonStats());
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("Stats");
		Stats::staticCtor();
		//Achievements::staticCtor(); // 4J Stu - This is now called from within the Stats::staticCtor()
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("TileEntity");
		TileEntity::staticCtor();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("EntityIO");
		EntityIO::staticCtor();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("MobCategory");
		MobCategory::staticCtor();

		NATIVE_DESKTOP_STATIC_CTOR_TRACE("Item::staticInit");
		Item::staticInit();
		NATIVE_DESKTOP_STATIC_CTOR_TRACE("LevelChunk");
		LevelChunk::staticCtor();

		NATIVE_DESKTOP_STATIC_CTOR_TRACE("LevelType");
		LevelType::staticCtor();

		{
			NATIVE_DESKTOP_STATIC_CTOR_TRACE("StructureFeatureIO");
			StructureFeatureIO::staticCtor();

			NATIVE_DESKTOP_STATIC_CTOR_TRACE("MineShaftPieces");
			MineShaftPieces::staticCtor();
			NATIVE_DESKTOP_STATIC_CTOR_TRACE("StrongholdFeature");
			StrongholdFeature::staticCtor();
			NATIVE_DESKTOP_STATIC_CTOR_TRACE("VillagePieces::Smithy");
			VillagePieces::Smithy::staticCtor();
			NATIVE_DESKTOP_STATIC_CTOR_TRACE("VillageFeature");
			VillageFeature::staticCtor();
			NATIVE_DESKTOP_STATIC_CTOR_TRACE("RandomScatteredLargeFeature");
			RandomScatteredLargeFeature::staticCtor();
		}
	}
	NATIVE_DESKTOP_STATIC_CTOR_TRACE("EnderMan");
	EnderMan::staticCtor();
	NATIVE_DESKTOP_STATIC_CTOR_TRACE("PotionBrewing");
	PotionBrewing::staticCtor();
	NATIVE_DESKTOP_STATIC_CTOR_TRACE("Enchantment");
	Enchantment::staticCtor();

	NATIVE_DESKTOP_STATIC_CTOR_TRACE("SharedConstants");
	SharedConstants::staticCtor();

	NATIVE_DESKTOP_STATIC_CTOR_TRACE("ServerLevel");
	ServerLevel::staticCtor();
	NATIVE_DESKTOP_STATIC_CTOR_TRACE("SparseLightStorage");
	SparseLightStorage::staticCtor();
	NATIVE_DESKTOP_STATIC_CTOR_TRACE("CompressedTileStorage");
	CompressedTileStorage::staticCtor();
	NATIVE_DESKTOP_STATIC_CTOR_TRACE("SparseDataStorage");
	SparseDataStorage::staticCtor();
	NATIVE_DESKTOP_STATIC_CTOR_TRACE("McRegionChunkStorage");
	McRegionChunkStorage::staticCtor();
	NATIVE_DESKTOP_STATIC_CTOR_TRACE("Villager");
	Villager::staticCtor();
	NATIVE_DESKTOP_STATIC_CTOR_TRACE("GameType");
	GameType::staticCtor();
	NATIVE_DESKTOP_STATIC_CTOR_TRACE("BeaconTileEntity");
	BeaconTileEntity::staticCtor();
	NATIVE_DESKTOP_STATIC_CTOR_TRACE("complete");

#undef NATIVE_DESKTOP_STATIC_CTOR_TRACE
}
