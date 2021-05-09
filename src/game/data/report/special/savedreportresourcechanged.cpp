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

#include "game/data/report/special/savedreportresourcechanged.h"

#include "game/data/player/player.h"
#include "game/data/resourcetype.h"
#include "utility/language.h"
#include "utility/string/toString.h"

//------------------------------------------------------------------------------
cSavedReportResourceChanged::cSavedReportResourceChanged (eResourceType resourceType_, int amount_, bool increase_) :
	resourceType (resourceType_),
	amount (amount_),
	increase (increase_)
{}

//------------------------------------------------------------------------------
eSavedReportType cSavedReportResourceChanged::getType() const
{
	return eSavedReportType::ResourceChanged;
}

//------------------------------------------------------------------------------
std::string cSavedReportResourceChanged::getMessage (const cModel&) const
{
	if (increase)
	{
		switch (resourceType)
		{
			case eResourceType::Gold: return lngPack.i18n ("Text~Comp~Adjustments_Gold_Increased", iToStr (amount));
			case eResourceType::Oil: return lngPack.i18n ("Text~Comp~Adjustments_Fuel_Increased", iToStr (amount));
			case eResourceType::Metal: return lngPack.i18n ("Text~Comp~Adjustments_Metal_Increased", iToStr (amount));
		}
	}
	else
	{
		switch (resourceType)
		{
			case eResourceType::Gold: return lngPack.i18n ("Text~Comp~Adjustments_Gold_Decreased", iToStr (amount));
			case eResourceType::Oil: return lngPack.i18n ("Text~Comp~Adjustments_Fuel_Decreased", iToStr (amount));
			case eResourceType::Metal: return lngPack.i18n ("Text~Comp~Adjustments_Metal_Decreased", iToStr (amount));
		}
	}
	throw std::runtime_error("Unknown resourceType " + toString (static_cast<int> (resourceType)));
}

//------------------------------------------------------------------------------
bool cSavedReportResourceChanged::isAlert() const
{
	return false;
}
