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

#include "translations.h"

#include "game/data/gamesettings.h"
#include "game/data/model.h"
#include "game/data/player/clans.h"
#include "game/data/player/player.h"
#include "game/data/report/savedreport.h"
#include "game/data/report/savedreportchat.h"
#include "game/data/report/savedreportsimple.h"
#include "game/data/report/special/savedreporthostcommand.h"
#include "game/data/report/special/savedreportlostconnection.h"
#include "game/data/report/special/savedreportplayerdefeated.h"
#include "game/data/report/special/savedreportplayerendedturn.h"
#include "game/data/report/special/savedreportplayerleft.h"
#include "game/data/report/special/savedreportplayerwins.h"
#include "game/data/report/special/savedreportresourcechanged.h"
#include "game/data/report/special/savedreportturnstart.h"
#include "game/data/report/special/savedreportupgraded.h"
#include "game/data/report/unit/savedreportattacked.h"
#include "game/data/report/unit/savedreportattackingenemy.h"
#include "game/data/report/unit/savedreportcapturedbyenemy.h"
#include "game/data/report/unit/savedreportdestroyed.h"
#include "game/data/report/unit/savedreportdetected.h"
#include "game/data/report/unit/savedreportdisabled.h"
#include "game/data/report/unit/savedreportpathinterrupted.h"
#include "game/data/report/unit/savedreportsurveyoraiconfused.h"
#include "game/data/units/building.h"
#include "game/data/units/commandodata.h"
#include "game/data/units/vehicle.h"
#include "game/logic/endmoveaction.h"
#include "game/logic/movejob.h"
#include "output/video/unifonts.h"
#include "utility/language.h"

#include <cassert>

namespace
{
	//--------------------------------------------------------------------------
	std::string GetModificatorString (int original, int modified)
	{
		const int diff = modified - original;
		if (diff > 0)
			return " +" + std::to_string (diff);
		else if (diff < 0)
			return " -" + std::to_string (-diff);
		else // diff == 0
			return " =" + std::to_string (modified);
	}

	//--------------------------------------------------------------------------
	std::string getRankString (const cCommandoData& commandoData)
	{
		const auto level = cCommandoData::getLevel (commandoData.getSuccessCount());
		const std::string suffix = (level > 0) ? " +" + std::to_string (level) : "";

		if (level < 1)
			return lngPack.i18n ("Text~Comp~CommandoRank_Greenhorn") + suffix;
		else if (level < 3)
			return lngPack.i18n ("Text~Comp~CommandoRank_Average") + suffix;
		else if (level < 6)
			return lngPack.i18n ("Text~Comp~CommandoRank_Veteran") + suffix;
		else if (level < 11)
			return lngPack.i18n ("Text~Comp~CommandoRank_Expert") + suffix;
		else if (level < 19)
			return lngPack.i18n ("Text~Comp~CommandoRank_Elite") + suffix;
		else
			return lngPack.i18n ("Text~Comp~CommandoRank_GrandMaster") + suffix;
	}

	//--------------------------------------------------------------------------
	std::string toTranslatedString (cResearch::eResearchArea area)
	{
		switch (area)
		{
			case cResearch::eResearchArea::AttackResearch: return lngPack.i18n ("Text~Others~Attack");
			case cResearch::eResearchArea::ShotsResearch: return lngPack.i18n ("Text~Others~Shots_7");
			case cResearch::eResearchArea::RangeResearch: return lngPack.i18n ("Text~Others~Range");
			case cResearch::eResearchArea::ArmorResearch: return lngPack.i18n ("Text~Others~Armor_7");
			case cResearch::eResearchArea::HitpointsResearch: return lngPack.i18n ("Text~Others~Hitpoints_7");
			case cResearch::eResearchArea::SpeedResearch: return lngPack.i18n ("Text~Others~Speed");
			case cResearch::eResearchArea::ScanResearch: return lngPack.i18n ("Text~Others~Scan");
			case cResearch::eResearchArea::CostResearch: return lngPack.i18n ("Text~Others~Costs");
		}
		return "";
	}

	//--------------------------------------------------------------------------
	std::string getResearchAreaStatus (const cPlayer& player, cResearch::eResearchArea area)
	{
		const auto workingCenterCount = player.getResearchCentersWorkingOnArea (area);
		if (workingCenterCount > 0)
		{
			const auto remainingTurnCount = player.getResearchState().getRemainingTurns (area, player.getResearchCentersWorkingOnArea (area));
			return toTranslatedString (area) + lngPack.i18n ("Text~Punctuation~Colon") + std::to_string (remainingTurnCount) + "\n";
		}
		return "";
	}

	//--------------------------------------------------------------------------
	std::string getResearchAreaStatus (const cPlayer& player)
	{
		std::string res;
		for (int area = 0; area < cResearch::kNrResearchAreas; area++)
		{
			res += getResearchAreaStatus (player, static_cast<cResearch::eResearchArea> (area));
		}
		return res;
	}
} // namespace

//------------------------------------------------------------------------------
std::string toTranslatedString (eGameSettingsResourceAmount amount)
{
	switch (amount)
	{
		case eGameSettingsResourceAmount::Limited:
			return lngPack.i18n ("Text~Option~Limited");
		case eGameSettingsResourceAmount::Normal:
			return lngPack.i18n ("Text~Option~Normal");
		case eGameSettingsResourceAmount::High:
			return lngPack.i18n ("Text~Option~High");
		case eGameSettingsResourceAmount::TooMuch:
			return lngPack.i18n ("Text~Option~TooMuch");
	}
	assert (false);
	return "";
}

//------------------------------------------------------------------------------
std::string toTranslatedString (eGameSettingsResourceDensity density)
{
	switch (density)
	{
		case eGameSettingsResourceDensity::Sparse:
			return lngPack.i18n ("Text~Option~Sparse");
		case eGameSettingsResourceDensity::Normal:
			return lngPack.i18n ("Text~Option~Normal");
		case eGameSettingsResourceDensity::Dense:
			return lngPack.i18n ("Text~Option~Dense");
		case eGameSettingsResourceDensity::TooMuch:
			return lngPack.i18n ("Text~Option~TooMuch");
	}
	assert (false);
	return "";
}

//------------------------------------------------------------------------------
std::string toTranslatedString (eGameSettingsBridgeheadType type)
{
	switch (type)
	{
		case eGameSettingsBridgeheadType::Definite:
			return lngPack.i18n ("Text~Option~Definite");
		case eGameSettingsBridgeheadType::Mobile:
			return lngPack.i18n ("Text~Option~Mobile");
	}
	assert (false);
	return "";
}

//------------------------------------------------------------------------------
std::string toTranslatedString (eGameSettingsGameType type)
{
	switch (type)
	{
		case eGameSettingsGameType::Simultaneous:
			return lngPack.i18n ("Text~Option~Type_Simu");
		case eGameSettingsGameType::Turns:
			return lngPack.i18n ("Text~Option~Type_Turns");
		case eGameSettingsGameType::HotSeat:
			return "Hot Seat"; // TODO: translation?!
	}
	assert (false);
	return "";
}

//------------------------------------------------------------------------------
std::string toTranslatedString (eGameSettingsVictoryCondition condition)
{
	switch (condition)
	{
		case eGameSettingsVictoryCondition::Turns:
			return lngPack.i18n ("Text~Comp~Turns");
		case eGameSettingsVictoryCondition::Points:
			return lngPack.i18n ("Text~Comp~Points");
		case eGameSettingsVictoryCondition::Death:
			return lngPack.i18n ("Text~Comp~NoLimit");
	}
	assert (false);
	return "";
}

//------------------------------------------------------------------------------
std::string getClanStatsDescription (const cClanUnitStat& clanUnitStat, const cUnitsData& originalData)
{
	const cDynamicUnitData* data = &originalData.getDynamicUnitData (clanUnitStat.getUnitId());

	if (data == nullptr) return "Unknown";

	std::string result = getStaticUnitName (originalData.getStaticUnitData (clanUnitStat.getUnitId())) + lngPack.i18n ("Text~Punctuation~Colon");
	const char* const commaSep = ", ";
	const char* sep = "";

	struct
	{
		eClanModification type;
		std::string text;
		int originalValue;
	} t[] =
		{
			// ToDo / Fixme if #756 fixed, use the non "_7" version of the text files
			{eClanModification::Damage, lngPack.i18n ("Text~Others~Attack_7"), data->getDamage()},
			{eClanModification::Range, lngPack.i18n ("Text~Others~Range"), data->getRange()},
			{eClanModification::Armor, lngPack.i18n ("Text~Others~Armor_7"), data->getArmor()},
			{eClanModification::Hitpoints, lngPack.i18n ("Text~Others~Hitpoints_7"), data->getHitpointsMax()},
			{eClanModification::Scan, lngPack.i18n ("Text~Others~Scan_7"), data->getScan()},
			{eClanModification::Speed, lngPack.i18n ("Text~Others~Speed_7"), data->getSpeedMax() / 4},
		};

	for (auto& e : t)
	{
		if (clanUnitStat.hasModification (e.type) == false) continue;
		result += sep;
		result += e.text;
		result += GetModificatorString (e.originalValue, clanUnitStat.getModificationValue (e.type));
		sep = commaSep;
	}
	if (clanUnitStat.hasModification (eClanModification::Built_Costs))
	{
		result += sep;
		int nrTurns = clanUnitStat.getModificationValue (eClanModification::Built_Costs);
		if (originalData.getStaticUnitData (data->getId()).vehicleData.isHuman == false) nrTurns /= clanUnitStat.getUnitId().isAVehicle() == 0 ? 2 : 3;

		result += std::to_string (nrTurns) + " " + lngPack.i18n ("Text~Comp~Turns");
	}
	return result;
}

//------------------------------------------------------------------------------
std::vector<std::string> getClanStatsDescription (const cClan& clan, const cUnitsData& originalData)
{
	std::vector<std::string> result;
	for (int i = 0; i != clan.getNrUnitStats(); ++i)
	{
		const cClanUnitStat* stat = clan.getUnitStat (i);
		result.push_back (getClanStatsDescription (*stat, originalData));
	}
	return result;
}

//------------------------------------------------------------------------------
std::string getClanName (const cClan& clan)
{
	std::string name = lngPack.getClanName (clan.getClanID());
	if (name.empty()) return clan.getDefaultName();
	return name;
}

//------------------------------------------------------------------------------
std::string getClanDescription (const cClan& clan)
{
	auto description = lngPack.getClanDescription (clan.getClanID());
	if (description.empty()) return clan.getDefaultDescription();
	return description;
}

//------------------------------------------------------------------------------
std::string getStaticUnitName (const cStaticUnitData& unitData)
{
	std::string translatedName = lngPack.getUnitName (unitData.ID);
	if (!translatedName.empty())
		return translatedName;
	return unitData.getDefaultName();
}

//------------------------------------------------------------------------------
std::string getStaticUnitDescription (const cStaticUnitData& unitData)
{
	std::string translatedDescription = lngPack.getUnitDescription (unitData.ID);
	if (!translatedDescription.empty())
		return translatedDescription;
	return unitData.getDefaultDescription();
}
//------------------------------------------------------------------------------
std::string getName (const cUnit& unit)
{
	return unit.getCustomName().value_or (getStaticUnitName (unit.getStaticUnitData()));
}

//------------------------------------------------------------------------------
std::string getDisplayName (const cUnit& unit)
{
	return unit.getDisplayName (getStaticUnitName (unit.getStaticUnitData()));
}

//------------------------------------------------------------------------------
/** Returns a string with the current state */
//------------------------------------------------------------------------------
std::string getStatusStr (const cBuilding& building, const cPlayer* whoWantsToKnow, const cUnitsData& unitsData)
{
	auto font = cUnicodeFont::font.get();
	if (building.isDisabled())
	{
		std::string sText;
		sText = lngPack.i18n ("Text~Comp~Disabled") + " (";
		sText += std::to_string (building.getDisabledTurns()) + ")";
		return sText;
	}
	if (building.isUnitWorking() || building.factoryHasJustFinishedBuilding())
	{
		// Factory:
		if (!building.getStaticUnitData().canBuild.empty() && !building.isBuildListEmpty() && building.getOwner() == whoWantsToKnow)
		{
			const cBuildListItem& buildListItem = building.getBuildListItem (0);
			const std::string& unitName = getStaticUnitName (unitsData.getStaticUnitData (buildListItem.getType()));
			std::string sText;

			if (buildListItem.getRemainingMetal() > 0)
			{
				int iRound;

				iRound = (int) ceilf (buildListItem.getRemainingMetal() / (float) building.getMetalPerRound());
				sText = lngPack.i18n ("Text~Comp~Producing") + lngPack.i18n ("Text~Punctuation~Colon");
				sText += unitName + " (";
				sText += std::to_string (iRound) + ")";

				if (font->getTextWide (sText, eUnicodeFontType::LatinSmallWhite) > 126)
				{
					sText = lngPack.i18n ("Text~Comp~Producing") + lngPack.i18n ("Text~Punctuation~Colon") + "\n";
					sText += unitName + " (";
					sText += std::to_string (iRound) + ")";
				}

				return sText;
			}
			else //new unit is rdy + which kind of unit
			{
				sText = lngPack.i18n ("Text~Comp~Producing_Fin");
				sText += lngPack.i18n ("Text~Punctuation~Colon");
				sText += unitName;

				if (font->getTextWide (sText) > 126)
				{
					sText = lngPack.i18n ("Text~Comp~Producing_Fin") + lngPack.i18n ("Text~Punctuation~Colon");
					sText += "\n";
					sText += unitName;
				}
				return sText;
			}
		}

		// Research Center
		if (building.getStaticData().canResearch && building.getOwner() == whoWantsToKnow && building.getOwner())
		{
			return lngPack.i18n ("Text~Comp~Working") + "\n" + getResearchAreaStatus (*building.getOwner());
		}

		// Goldraffinerie:
		if (building.getStaticData().convertsGold && building.getOwner() == whoWantsToKnow && building.getOwner())
		{
			std::string sText;
			sText = lngPack.i18n ("Text~Comp~Working") + "\n";
			sText += lngPack.i18n ("Text~Title~Credits") + lngPack.i18n ("Text~Punctuation~Colon");
			sText += std::to_string (building.getOwner()->getCredits());
			return sText;
		}
		return lngPack.i18n ("Text~Comp~Working");
	}

	if (building.isAttacking())
		return lngPack.i18n ("Text~Comp~AttackingStatusStr");
	else if (building.isBeeingAttacked())
		return lngPack.i18n ("Text~Comp~IsBeeingAttacked");
	else if (building.isSentryActive())
		return lngPack.i18n ("Text~Comp~Sentry");
	else if (building.isManualFireActive())
		return lngPack.i18n ("Text~Comp~ReactionFireOff");

	//GoldRaf idle + gold-amount
	if (building.getStaticData().convertsGold && building.getOwner() == whoWantsToKnow && building.getOwner() && !building.isUnitWorking())
	{
		std::string sText;
		sText = lngPack.i18n ("Text~Comp~Waits") + "\n";
		sText += lngPack.i18n ("Text~Title~Credits") + lngPack.i18n ("Text~Punctuation~Colon");
		sText += std::to_string (building.getOwner()->getCredits());
		return sText;
	}

	//Research centre idle + projects
	// Research Center
	if (building.getStaticData().canResearch && building.getOwner() == whoWantsToKnow && building.getOwner() && !building.isUnitWorking())
	{
		return lngPack.i18n ("Text~Comp~Waits") + "\n" + getResearchAreaStatus (*building.getOwner());
	}
	return lngPack.i18n ("Text~Comp~Waits");
}

//------------------------------------------------------------------------------
/** Returns a string with the current state */
//------------------------------------------------------------------------------
std::string getStatusStr (const cVehicle& vehicle, const cPlayer* player, const cUnitsData& unitsData)
{
	auto font = cUnicodeFont::font.get();
	if (vehicle.isDisabled())
	{
		std::string sText;
		sText = lngPack.i18n ("Text~Comp~Disabled") + " (";
		sText += std::to_string (vehicle.getDisabledTurns()) + ")";
		return sText;
	}
	else if (vehicle.isSurveyorAutoMoveActive())
		return lngPack.i18n ("Text~Comp~Surveying");
	else if (vehicle.isUnitBuildingABuilding())
	{
		if (vehicle.getOwner() != player)
			return lngPack.i18n ("Text~Comp~Producing");
		else
		{
			std::string sText;
			if (vehicle.getBuildTurns())
			{
				sText = lngPack.i18n ("Text~Comp~Producing");
				sText += lngPack.i18n ("Text~Punctuation~Colon");
				sText += getStaticUnitName (unitsData.getStaticUnitData (vehicle.getBuildingType())) + " (";
				sText += std::to_string (vehicle.getBuildTurns());
				sText += ")";

				if (font->getTextWide (sText) > 126)
				{
					sText = lngPack.i18n ("Text~Comp~Producing") + lngPack.i18n ("Text~Punctuation~Colon");
					sText += "\n";
					sText += getStaticUnitName (unitsData.getStaticUnitData (vehicle.getBuildingType())) + " (";
					sText += std::to_string (vehicle.getBuildTurns());
					sText += ")";
				}
				return sText;
			}
			else //small building is rdy + activate after engineere moves away
			{
				sText = lngPack.i18n ("Text~Comp~Producing_Fin");
				sText += lngPack.i18n ("Text~Punctuation~Colon");
				sText += getStaticUnitName (unitsData.getStaticUnitData (vehicle.getBuildingType()));

				if (font->getTextWide (sText) > 126)
				{
					sText = lngPack.i18n ("Text~Comp~Producing_Fin") + lngPack.i18n ("Text~Punctuation~Colon");
					sText += "\n";
					sText += getStaticUnitName (unitsData.getStaticUnitData (vehicle.getBuildingType()));
				}
				return sText;
			}
		}
	}
	else if (vehicle.isUnitClearingMines())
		return lngPack.i18n ("Text~Comp~Clearing_Mine");
	else if (vehicle.isUnitLayingMines())
		return lngPack.i18n ("Text~Comp~Laying");
	else if (vehicle.isUnitClearing())
	{
		if (vehicle.getClearingTurns())
		{
			return lngPack.i18n ("Text~Comp~Clearing", std::to_string (vehicle.getClearingTurns()));
		}
		else
			return lngPack.i18n ("Text~Comp~Clearing_Fin");
	}
	// generate other infos for normal non-unit-related-events and infiltrators
	std::string sTmp;
	{
		if (vehicle.getMoveJob() && vehicle.getMoveJob()->getEndMoveAction().getType() == eEndMoveActionType::Attack)
			sTmp = lngPack.i18n ("Text~Comp~MovingToAttack");
		else if (vehicle.getMoveJob())
			sTmp = lngPack.i18n ("Text~Comp~Moving");
		else if (vehicle.isAttacking())
			sTmp = lngPack.i18n ("Text~Comp~AttackingStatusStr");
		else if (vehicle.isBeeingAttacked())
			sTmp = lngPack.i18n ("Text~Comp~IsBeeingAttacked");
		else if (vehicle.isManualFireActive())
			sTmp = lngPack.i18n ("Text~Comp~ReactionFireOff");
		else if (vehicle.isSentryActive())
			sTmp = lngPack.i18n ("Text~Comp~Sentry");
		else
			sTmp = lngPack.i18n ("Text~Comp~Waits");

		// extra info only for infiltrators
		// TODO should it be original behavior (as it is now) or
		// don't display CommandRank for enemy (could also be a bug in original...?)
		if ((vehicle.getStaticData().canCapture || vehicle.getStaticData().canDisable) /* && vehicle.owner == gameGUI.getClient()->getActivePlayer()*/)
		{
			sTmp += "\n";
			sTmp += getRankString (vehicle.getCommandoData());
		}

		return sTmp;
	}

	return lngPack.i18n ("Text~Comp~Waits");
}

//------------------------------------------------------------------------------
std::string getStatusStr (const cUnit& unit, const cPlayer* whoWantsToKnow, const cUnitsData& unitData)
{
	if (const auto* vehicle = dynamic_cast<const cVehicle*> (&unit)) return getStatusStr (*vehicle, whoWantsToKnow, unitData);
	if (const auto* building = dynamic_cast<const cBuilding*> (&unit)) return getStatusStr (*building, whoWantsToKnow, unitData);
	return {};
}

namespace
{
	//--------------------------------------------------------------------------
	std::string getText (const cUnit& unit, const cSavedReportDetected&)
	{
		const auto playerName = unit.getOwner() ? unit.getOwner()->getName() : "";
		return getDisplayName (unit) + " (" + playerName + ") " + lngPack.i18n ("Text~Comp~Detected");
	}

	//--------------------------------------------------------------------------
	std::string getText (const cUnit& unit, const cSavedReportUnit& report)
	{
		switch (report.getType())
		{
			case eSavedReportType::Attacked:
				return lngPack.i18n ("Text~Comp~Attacked", getDisplayName (unit));
			case eSavedReportType::AttackingEnemy:
				return lngPack.i18n ("Text~Comp~AttackingEnemy", getDisplayName (unit));
			case eSavedReportType::CapturedByEnemy:
				return lngPack.i18n ("Text~Comp~CapturedByEnemy", getDisplayName (unit));
			case eSavedReportType::Destroyed:
				return lngPack.i18n ("Text~Comp~Destroyed", getDisplayName (unit));
			case eSavedReportType::Detected:
				return getText (unit, static_cast<const cSavedReportDetected&> (report));
			case eSavedReportType::Disabled:
				return getDisplayName (unit) + " " + lngPack.i18n ("Text~Comp~Disabled");
			case eSavedReportType::PathInterrupted:
				return lngPack.i18n ("Text~Comp~Path_interrupted");
			case eSavedReportType::SurveyorAiConfused:
				return "Surveyor AI: I'm totally confused. Don't know what to do...";
		}
		return "";
	}

	//--------------------------------------------------------------------------
	std::string getMessage (const cSavedReportChat& report)
	{
		return report.getPlayerName() + lngPack.i18n ("Text~Punctuation~Colon") + report.getText();
	}

	//------------------------------------------------------------------------------
	std::string getMessage (const cSavedReportHostCommand& report)
	{
		return lngPack.i18n ("Text~Multiplayer~Host_command", report.getCommand());
	}

	//------------------------------------------------------------------------------
	std::string getMessage (const cModel& model, const cSavedReportLostConnection& report)
	{
		auto player = model.getPlayer (report.getPlayerId());
		assert (player != nullptr);
		return lngPack.i18n ("Text~Multiplayer~Lost_Connection", player->getName());
	}

	//------------------------------------------------------------------------------
	std::string getMessage (const cModel& model, const cSavedReportPlayerDefeated& report)
	{
		auto player = model.getPlayer (report.getPlayerId());
		assert (player != nullptr);
		return lngPack.i18n ("Text~Comp~Defeated", player->getName());
	}

	//------------------------------------------------------------------------------
	std::string getMessage (const cModel& model, const cSavedReportPlayerLeft& report)
	{
		auto player = model.getPlayer (report.getPlayerId());
		assert (player != nullptr);
		return lngPack.i18n ("Text~Multiplayer~Player_Left", player->getName());
	}

	//------------------------------------------------------------------------------
	std::string getMessage (const cModel& model, const cSavedReportPlayerEndedTurn& report)
	{
		auto player = model.getPlayer (report.getPlayerId());
		assert (player != nullptr);
		return lngPack.i18n ("Text~Multiplayer~Player_Turn_End", player->getName());
	}

	//------------------------------------------------------------------------------
	std::string getMessage (const cModel& model, const cSavedReportPlayerWins& report)
	{
		auto player = model.getPlayer (report.getPlayerId());
		assert (player != nullptr);
		return lngPack.i18n ("Text~Comp~Wins", player->getName());
	}

	//------------------------------------------------------------------------------
	std::string getMessage (const cSavedReportResourceChanged& report)
	{
		int amount = report.getAmount();
		if (report.isIncrease())
		{
			switch (report.getResourceType())
			{
				case eResourceType::Gold: return lngPack.i18n ("Text~Comp~Adjustments_Gold_Increased", std::to_string (amount));
				case eResourceType::Oil: return lngPack.i18n ("Text~Comp~Adjustments_Fuel_Increased", std::to_string (amount));
				case eResourceType::Metal: return lngPack.i18n ("Text~Comp~Adjustments_Metal_Increased", std::to_string (amount));
			}
		}
		else
		{
			switch (report.getResourceType())
			{
				case eResourceType::Gold: return lngPack.i18n ("Text~Comp~Adjustments_Gold_Decreased", std::to_string (amount));
				case eResourceType::Oil: return lngPack.i18n ("Text~Comp~Adjustments_Fuel_Decreased", std::to_string (amount));
				case eResourceType::Metal: return lngPack.i18n ("Text~Comp~Adjustments_Metal_Decreased", std::to_string (amount));
			}
		}
		throw std::runtime_error ("Unknown resourceType " + std::to_string (static_cast<int> (report.getResourceType())));
	}

	//------------------------------------------------------------------------------
	std::string getMessage (const cModel& model, const cSavedReportUpgraded& report)
	{
		const auto& unitName = getStaticUnitName (model.getUnitsData()->getStaticUnitData (report.getUnitId()));
		return lngPack.i18n ("Text~Comp~Upgrades_Done") + " " + std::to_string (report.getUnitsCount()) + " " + lngPack.i18n ("Text~Comp~Upgrades_Done2", unitName) + " (" + lngPack.i18n ("Text~Others~Costs") + lngPack.i18n ("Text~Punctuation~Colon") + std::to_string (report.getCosts()) + ")";
	}

	//------------------------------------------------------------------------------
	std::string getMessage (const cModel& model, const cSavedReportTurnStart& report)
	{
		std::string message = lngPack.i18n ("Text~Comp~Turn_Start", std::to_string (report.turn));

		if (!report.unitReports.empty())
		{
			int totalUnitsCount = 0;
			message += "\n";
			const char* sep = "";
			for (const auto& entry : report.unitReports)
			{
				message += sep;
				sep = ", ";
				totalUnitsCount += entry.count;
				auto name = getStaticUnitName (model.getUnitsData()->getStaticUnitData (entry.type));
				message += entry.count > 1 ? std::to_string (entry.count) + " " + name : name;
			}
			// TODO: Plural rules are language dependant
			// - Russian has 3 forms, Chinese 1 form, ...
			//          | eng  | fre  | ...
			// singular | == 1 | <= 1 |
			// plural   | != 1 | 1 <  |
			// we should have `i18n (key, n)`
			if (totalUnitsCount == 1)
				message += " " + lngPack.i18n ("Text~Comp~Finished") + ".";
			else if (totalUnitsCount > 1)
				message += " " + lngPack.i18n ("Text~Comp~Finished2") + ".";
		}

		if (!report.researchAreas.empty())
		{
			message += "\n";
			message += lngPack.i18n ("Text~Others~Research") + " " + lngPack.i18n ("Text~Comp~Finished") + lngPack.i18n ("Text~Punctuation~Colon");

			const std::string themeNames[8] =
				{
					lngPack.i18n ("Text~Others~Attack"),
					lngPack.i18n ("Text~Others~Shots"),
					lngPack.i18n ("Text~Others~Range"),
					lngPack.i18n ("Text~Others~Armor"),
					lngPack.i18n ("Text~Others~Hitpoints"),
					lngPack.i18n ("Text~Others~Speed"),
					lngPack.i18n ("Text~Others~Scan"),
					lngPack.i18n ("Text~Others~Costs")};

			const char* sep = "";
			for (const auto researchArea : report.researchAreas)
			{
				message += sep;
				sep = ", ";
				message += themeNames[static_cast<int> (researchArea)];
			}
		}

		return message;
	}

} // namespace

//------------------------------------------------------------------------------
std::string getMessage (const cSavedReport& report, const cModel& model)
{
	if (auto* savedReportUnit = dynamic_cast<const cSavedReportUnit*> (&report))
	{
		auto pos = *savedReportUnit->getPosition();
		const auto* unit = model.getUnitFromID (savedReportUnit->getUnitId());
		assert(unit != nullptr);

		return "[" + std::to_string (pos.x()) + ", " + std::to_string (pos.y()) + "] " + getText (*unit, *savedReportUnit);
	}

	switch (report.getType())
	{
		case eSavedReportType::MetalInsufficient:
			return lngPack.i18n ("Text~Comp~Metal_Insufficient");
		case eSavedReportType::FuelInsufficient:
			return lngPack.i18n ("Text~Comp~Fuel_Insufficient");
		case eSavedReportType::GoldInsufficient:
			return lngPack.i18n ("Text~Comp~Gold_Insufficient");
		case eSavedReportType::EnergyInsufficient:
			return lngPack.i18n ("Text~Comp~Energy_Insufficient");
		case eSavedReportType::TeamInsufficient:
			return lngPack.i18n ("Text~Comp~Team_Insufficient");
		case eSavedReportType::MetalLow:
			return lngPack.i18n ("Text~Comp~Metal_Low");
		case eSavedReportType::FuelLow:
			return lngPack.i18n ("Text~Comp~Fuel_Low");
		case eSavedReportType::GoldLow:
			return lngPack.i18n ("Text~Comp~Gold_Low");
		case eSavedReportType::EnergyLow:
			return lngPack.i18n ("Text~Comp~Energy_Low");
		case eSavedReportType::TeamLow:
			return lngPack.i18n ("Text~Comp~Team_Low");
		case eSavedReportType::EnergyToLow:
			return lngPack.i18n ("Text~Comp~Energy_ToLow");
		case eSavedReportType::EnergyIsNeeded:
			return lngPack.i18n ("Text~Comp~Energy_IsNeeded");
		case eSavedReportType::BuildingDisabled:
			return lngPack.i18n ("Text~Comp~Building_Disabled");
		case eSavedReportType::Producing_InsufficientMaterial:
			return lngPack.i18n ("Text~Comp~Producing_InsufficientMaterial");
		case eSavedReportType::Producing_PositionBlocked:
			return lngPack.i18n ("Text~Comp~Producing_PositionBlocked");
		case eSavedReportType::TurnWait:
			return lngPack.i18n ("Text~Comp~Turn_Wait");
		case eSavedReportType::TurnAutoMove:
			return lngPack.i18n ("Text~Comp~Turn_Automove");
		case eSavedReportType::SuddenDeath:
			return lngPack.i18n ("Text~Comp~SuddenDeath");

		case eSavedReportType::Chat:
			return getMessage (static_cast<const cSavedReportChat&> (report));
		case eSavedReportType::TurnStart:
			return getMessage (model, static_cast<const cSavedReportTurnStart&> (report));
		case eSavedReportType::HostCommand:
			return getMessage (static_cast<const cSavedReportHostCommand&> (report));
		case eSavedReportType::ResourceChanged:
			return getMessage (static_cast<const cSavedReportResourceChanged&> (report));
		case eSavedReportType::PlayerEndedTurn:
			return getMessage (model, static_cast<const cSavedReportPlayerEndedTurn&> (report));
		case eSavedReportType::LostConnection:
			return getMessage (model, static_cast<const cSavedReportLostConnection&> (report));
		case eSavedReportType::PlayerDefeated:
			return getMessage (model, static_cast<const cSavedReportPlayerDefeated&> (report));
		case eSavedReportType::PlayerWins:
			return getMessage (model, static_cast<const cSavedReportPlayerWins&> (report));
		case eSavedReportType::PlayerLeft:
			return getMessage (model, static_cast<const cSavedReportPlayerLeft&> (report));
		case eSavedReportType::Upgraded:
			return getMessage (model, static_cast<const cSavedReportUpgraded&> (report));
	}
	return "";
}
