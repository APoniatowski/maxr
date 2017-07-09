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

#ifndef game_data_modelH
#define game_data_modelH

#include <vector>
#include <memory>
#include <forward_list>

#include "utility/flatset.h"
#include "units/unit.h"
#include "utility/crossplattformrandom.h"
#include "utility/serialization/serialization.h"
#include "game/data/map/map.h"
#include "game/data/player/player.h"
#include "game/logic/pathcalculator.h"
#include "game/logic/movejob.h"

class cPlayerBasicData;
class cGameSettings;
class cStaticMap;
class cBuilding;
class cVehicle;
struct sID;
class cPosition;
class cUnit;
class cGameTimer;

class cModel
{
	friend class cDebugOutputWidget;
public:
	cModel();
	~cModel();

	cCrossPlattformRandom randomGenerator;

	unsigned int getGameId() const { return gameId; }
	void initGameId();

	void advanceGameTime();
	unsigned int getGameTime() const;

	uint32_t getChecksum() const;

	void setUnitsData(std::shared_ptr<cUnitsData> unitsData);
	std::shared_ptr<const cUnitsData> getUnitsData() const { return unitsData; };

	std::shared_ptr<const cGameSettings> getGameSettings() const { return gameSettings; };
	std::shared_ptr<cGameSettings> getGameSettings() { return gameSettings; };
	void setGameSettings(const cGameSettings& gameSettings);

	std::shared_ptr<const cMap> getMap() const { return map; };
	std::shared_ptr<cMap> getMap() { return map; };
	void setMap(std::shared_ptr<cStaticMap> map);

	cPlayer* getPlayer(int playerNr);
	const cPlayer* getPlayer(int playerNr) const;
	const cPlayer* getPlayer(std::string player) const;
	const std::vector<std::shared_ptr<cPlayer>>& getPlayerList() const { return /*static_cast<std::vector<std::shared_ptr<const cPlayer>>>*/(playerList); }; //TODO: cast to const cPlayer
	std::vector<std::shared_ptr<cPlayer>>& getPlayerList() { return playerList; };
	void setPlayerList(const std::vector<cPlayerBasicData>& splayers);

	cUnit* getUnitFromID(unsigned int id) const;
	cVehicle* getVehicleFromID(unsigned int id) const;
	cBuilding* getBuildingFromID(unsigned int id) const;

	//TODO: check if init and addToMap are needed
	cVehicle& addVehicle(const cPosition& position, const sID& id, cPlayer* player, bool init = false, bool addToMap = true);
	cBuilding& addBuilding(const cPosition& position, const sID& id, cPlayer* player, bool init = false);
	void deleteUnit(cUnit* unit);
	void deleteRubble(cBuilding* rubble);

	void addMoveJob(cVehicle& vehicle, const std::forward_list<cPosition>& path);

	mutable cSignal<void()> gameTimeChanged;
	mutable cSignal<void(const cVehicle& vehicle)> triggeredAddTracks;

	template<typename T>
	void save(T& archive)
	{
		archive << NVP(gameId);
		archive << NVP(gameTime);
		archive << serialization::makeNvp("gameSettings", *gameSettings);
		archive << serialization::makeNvp("map", *map);
		archive << serialization::makeNvp("unitsData", *unitsData);
		archive << serialization::makeNvp("numPlayers", (int)playerList.size());
		for (auto player : playerList)
		{
			archive << serialization::makeNvp("player", *player);
		}
		archive << serialization::makeNvp("numMoveJobs", (int)moveJobs.size());
		for (auto moveJob : moveJobs)
		{
			archive << serialization::makeNvp("moveJob", *moveJob);
		}
		//archive & NVP(neutralBuildings);
		archive << NVP(nextUnitId);
	};
	template<typename T>
	void load(T& archive)
	{
		archive >> NVP(gameId);
		archive >> NVP(gameTime);

		assert(gameSettings != nullptr);
		archive >> serialization::makeNvp("gameSettings", *gameSettings);

		if (map == nullptr)
		{
			auto staticMap = std::make_shared<cStaticMap>();
			map = std::make_shared<cMap>(staticMap);
		}
		archive >> serialization::makeNvp("map", *map);
		map->reset();

		if (unitsData == nullptr)
		{
			unitsData = std::make_shared<cUnitsData>();
		}
		archive >> serialization::makeNvp("unitsData", *unitsData);
		//TODO: check UIData available

		int numPlayers;
		archive >> NVP(numPlayers);
		playerList.resize(numPlayers);
		for (auto& player : playerList)
		{
			if (player == nullptr)
			{
				cPlayerBasicData basicPlayerData;
				player = std::make_shared<cPlayer>(basicPlayerData, *unitsData);
			}
			player->initMaps(*map);
			archive >> serialization::makeNvp("player", *player);
		}
		refreshMapPointer();
		for (auto& player : playerList)
		{
			player->refreshBase(*map);
		}
		int numMoveJobs;
		archive >> NVP(numMoveJobs);
		for (auto moveJob : moveJobs)
		{
			delete moveJob;
		}
		moveJobs.clear();
		moveJobs.resize(numMoveJobs);
		for (auto& moveJob : moveJobs)
		{
			moveJob = new cMoveJob();
			archive >> serialization::makeNvp("moveJob", *moveJob);
		}
		archive >> NVP(nextUnitId);
	}
	SERIALIZATION_SPLIT_MEMBER();
private:
	void refreshMapPointer();
	void runMoveJobs();

	unsigned int gameId; //this id can be used to check, which log files, and save file belong to the same game.

	unsigned int gameTime;

	std::shared_ptr<cGameSettings> gameSettings;
	std::shared_ptr<cMap> map;
	std::vector<std::shared_ptr<cPlayer>> playerList;

	cFlatSet<std::shared_ptr<cBuilding>, sUnitLess<cBuilding>> neutralBuildings;

	int nextUnitId;

	std::shared_ptr<cUnitsData> unitsData;

	std::vector<cMoveJob*> moveJobs; //TODO: serialize

	//jobs
	//casualtiesTracker
	//turnnr
	//turn u. deadline timer
	
	//effect list

	//signalConnectionManager?

};

#endif
