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

#ifndef game_data_units_idH
#define game_data_units_idH

#include "game/serialization/serialization.h"

#include <string>

struct sID
{
	sID() : firstPart(0), secondPart(0) {}
	sID(int first, int second) : firstPart(first), secondPart(second) {}

	std::string getText() const;

	bool isAVehicle() const { return firstPart == 0; }
	bool isABuilding() const { return firstPart == 1; }

	bool operator== (const sID& ID) const;
	bool operator!= (const sID& rhs) const { return !(*this == rhs); }
	bool operator< (const sID& rhs) const { return less_vehicleFirst(rhs); }
	bool less_vehicleFirst(const sID& ID) const;
	bool less_buildingFirst(const sID& ID) const;

	uint32_t getChecksum(uint32_t crc) const;

	template<typename T>
	void serialize(T& archive)
	{
		archive & NVP(firstPart);
		archive & NVP(secondPart);
	}

public:
	int firstPart;
	int secondPart;
};

#endif
