/***************************************************************************
 *      Mechanized Assault and Exploration Reloaded Projectfile            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef drawingcacheH
#define drawingcacheH

#include <memory>

#include "utility/autosurface.h"
#include "main.h" // for sID

class cPlayer;
class cVehicle;
class cBuilding;
class cAnimationTimer;
class cMap;

/**
* Stores all properties, which determine the look of the unit.
* Overlays and alpha effects are not cached!
*/
struct sDrawingCacheEntry
{
	//building proberties
	bool BaseN;
	bool BaseBN;
	bool BaseE;
	bool BaseBE;
	bool BaseS;
	bool BaseBS;
	bool BaseW;
	bool BaseBW;
	int clan;

	//vehicle properties
	unsigned int frame;
	int flightHigh;
	bool big;
	bool isBuilding;
	bool isClearing;
	bool stealth;
	bool water;

	//common properties
	sID id;
	cPlayer* owner;
	int dir;
	double zoom;

	AutoSurface surface;

	/**
	* sets all properties and initialises the surface.
	*/
	void init (const cVehicle& vehicle, const cMap& map, const cPlayer* player, unsigned long long animationTime, double zoom);
	void init (const cBuilding& building, double zoom);
};

class cDrawingCache
{
public:
	cDrawingCache (std::shared_ptr<cAnimationTimer> animationTimer);
	~cDrawingCache();

	void setPlayer (const cPlayer* player);

	/**
	* This method looks for a cached image, that matches the properties of the passed building.
	* @return a pointer to a surface, which contains the already rendered image of the building or NULL when no matching cache entry exists.
	*/
	SDL_Surface* getCachedImage (const cBuilding& building, double zoom);
	SDL_Surface* getCachedImage (const cVehicle& vehicle, double zoom, const cMap& map);
	/**
	* This method creates a new chace entry, when there is space in the cache.
	* When there is no free space, an old entry is reused.
	* When there is no free space and no old entries, NULL is returned.
	* @return a surface to which the building has to be drawn, after calling this function. Returns NULL when the cache is full.
	*/
	SDL_Surface* createNewEntry (const cBuilding& building, double zoom);
	SDL_Surface* createNewEntry (const cVehicle& vehicle, double zoom, const cMap& map);
	/**
	* Deletes all cache entries.
	*/
	void flush();

	void resetStatistics();
	int getMaxCacheSize() const;
	void setMaxCacheSize (unsigned int newSize);
	int getCacheSize() const;
	int getCacheHits() const;
	int getCacheMisses() const;
	int getNotCached() const;
private:
	std::shared_ptr<cAnimationTimer> animationTimer;
	const cPlayer* player;

	unsigned int maxCacheSize;
	unsigned int cacheSize;
	sDrawingCacheEntry* cachedImages;
	bool canCache (const cBuilding& building);
	bool canCache (const cVehicle& vehicle);

	//statistics
	int cacheHits;
	int cacheMisses;
	int notCached;
};

#endif
