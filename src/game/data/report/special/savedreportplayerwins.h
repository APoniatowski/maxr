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

#ifndef game_data_reports_special_savedreportplayerwinsH
#define game_data_reports_special_savedreportplayerwinsH

#include "game/data/report/savedreport.h"

class cPlayer;

class cSavedReportPlayerWins : public cSavedReport
{
public:
	cSavedReportPlayerWins (const cPlayer&);

	template <typename Archive, ENABLE_ARCHIVE_OUT>
	explicit cSavedReportPlayerWins (Archive& archive)
	{
		serializeThis (archive);
	}

	void serialize (cBinaryArchiveIn& archive) override { cSavedReport::serialize (archive); serializeThis (archive); }
	void serialize (cXmlArchiveIn& archive) override { cSavedReport::serialize (archive); serializeThis (archive); }
	void serialize (cTextArchiveIn& archive) override { cSavedReport::serialize (archive); serializeThis (archive); }

	eSavedReportType getType() const override;

	bool isAlert() const override;

	const cPlayer& getPlayer() const { return *player; }

private:
	template <typename Archive>
	void serializeThis (Archive& archive)
	{
		archive & NVP (player);
	}

	const cPlayer* player;
};

#endif
