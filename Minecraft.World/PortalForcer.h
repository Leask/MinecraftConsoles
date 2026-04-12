#pragma once

#include "Pos.h"

class Random;
#if defined(_NATIVE_DESKTOP)
class Level;
#else
class ServerLevel;
#endif

class PortalForcer
{
public:
	class PortalPosition : public Pos
	{
	public:
		int64_t lastUsed;

		PortalPosition(int x, int y, int z, int64_t time);
	};

private:
#if defined(_NATIVE_DESKTOP)
	Level *level;
#else
	ServerLevel *level;
#endif
	Random *random;
	unordered_map<int64_t, PortalPosition *> cachedPortals;
	vector<int64_t> cachedPortalKeys;

public:
#if defined(_NATIVE_DESKTOP)
	PortalForcer(Level *level);
#else
	PortalForcer(ServerLevel *level);
#endif
	~PortalForcer();

	void force(shared_ptr<Entity> e, double xOriginal, double yOriginal, double zOriginal, float yRotOriginal);
	bool findPortal(shared_ptr<Entity> e, double xOriginal, double yOriginal, double zOriginal, float yRotOriginal);
	bool createPortal(shared_ptr<Entity> e);
	void tick(int64_t time);
};
