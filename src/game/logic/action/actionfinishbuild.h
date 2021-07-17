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

#ifndef game_logic_actionFinishBuildH
#define game_logic_actionFinishBuildH

#include "action.h"

class cActionFinishBuild : public cActionT<cAction::eActiontype::ACTION_FINISH_BUILD>
{
public:
	cActionFinishBuild (const cUnit& unit, const cPosition& escapePosition);
	cActionFinishBuild (cBinaryArchiveOut& archive);

	void serialize (cBinaryArchiveIn& archive) override { cAction::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cAction::serialize (archive); serializeThis (archive); }

	void execute (cModel& model) const override;

private:
	void finishABuilding (cModel &model, cVehicle& vehicle) const;
	void finishAVehicle (cModel &model, cBuilding& building) const;

	int unitId;
	cPosition escapePosition;

	template <typename Archive>
	void serializeThis (Archive& archive)
	{
		archive & unitId;
		archive & escapePosition;
	}
};

#endif
