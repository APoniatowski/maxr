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

#include "action.h"
#include "utility/log.h"
#include "utility/string/toString.h"
#include "actioninitnewgame.h"
#include "actionstartwork.h"
#include "actionstop.h"
#include "actiontransfer.h"
#include "actionstartmove.h"
#include "actionresumemove.h"
#include "actionstartturn.h"
#include "actionendturn.h"
#include "actionselfdestroy.h"
#include "actionattack.h"
#include "actionchangesentry.h"
#include "actionchangemanualfire.h"
#include "actionminelayerstatus.h"
#include "actionstartbuild.h"
#include "actionfinishbuild.h"
#include "actionchangebuildlist.h"
#include "actionload.h"
#include "actionactivate.h"
#include "actionrepairreload.h"
#include "actionresourcedistribution.h"
#include "actionclear.h"
#include "actionstealdisable.h"
#include "actionchangeresearch.h"
#include "actionchangeunitname.h"
#include "actionbuyupgrades.h"
#include "actionupgradevehicle.h"
#include "actionupgradebuilding.h"
#include "actionsetautomove.h"

std::unique_ptr<cAction> cAction::createFromBuffer(cBinaryArchiveOut& archive)
{
	eActiontype type;
	archive >> type;

	switch (type)
	{
	case eActiontype::ACTION_INIT_NEW_GAME:
		return std::make_unique<cActionInitNewGame>(archive);
	case eActiontype::ACTION_START_WORK:
		return std::make_unique<cActionStartWork>(archive);
	case eActiontype::ACTION_STOP:
		return std::make_unique<cActionStop>(archive);
	case eActiontype::ACTION_TRANSFER:
		return std::make_unique<cActionTransfer>(archive);
	case eActiontype::ACTION_START_MOVE:
		return std::make_unique<cActionStartMove>(archive);
	case eActiontype::ACTION_RESUME_MOVE:
		return std::make_unique<cActionResumeMove>(archive);
	case eActiontype::ACTION_START_TURN:
		return std::make_unique<cActionStartTurn>(archive);
	case eActiontype::ACTION_END_TURN:
		return std::make_unique<cActionEndTurn>(archive);
	case eActiontype::ACTION_SELF_DESTROY:
		return std::make_unique<cActionSelfDestroy>(archive);
	case eActiontype::ACTION_ATTACK:
		return std::make_unique<cActionAttack>(archive);
	case eActiontype::ACTION_CHANGE_SENTRY:
		return std::make_unique<cActionChangeSentry>(archive);
	case eActiontype::ACTION_CHANGE_MANUAL_FIRE:
		return std::make_unique<cActionChangeManualFire>(archive);
	case eActiontype::ACTION_MINELAYER_STATUS:
		return std::make_unique<cActionMinelayerStatus>(archive);
	case eActiontype::ACTION_START_BUILD:
		return std::make_unique<cActionStartBuild>(archive);
	case eActiontype::ACTION_FINISH_BUILD:
		return std::make_unique<cActionFinishBuild>(archive);
	case eActiontype::ACTION_CHANGE_BUILDLIST:
		return std::make_unique<cActionChangeBuildList>(archive);
	case eActiontype::ACTION_LOAD:
		return std::make_unique<cActionLoad>(archive);
	case eActiontype::ACTION_ACTIVATE:
		return std::make_unique<cActionActivate>(archive);
	case eActiontype::ACTION_REPAIR_RELOAD:
		return std::make_unique<cActionRepairReload>(archive);
	case eActiontype::ACTION_RESOURCE_DISTRIBUTION:
		return std::make_unique<cActionResourceDistribution>(archive);
	case eActiontype::ACTION_CLEAR:
		return std::make_unique<cActionClear>(archive);
	case eActiontype::ACTION_STEAL_DISABLE:
		return std::make_unique<cActionStealDisable>(archive);
	case eActiontype::ACTION_CHANGE_RESEARCH:
		return std::make_unique<cActionChangeResearch>(archive);
	case eActiontype::ACTION_CHANGE_UNIT_NAME:
		return std::make_unique<cActionChangeUnitName>(archive);
	case eActiontype::ACTION_BUY_UPGRADES:
		return std::make_unique<cActionBuyUpgrades>(archive);
	case eActiontype::ACTION_UPGRADE_VEHICLE:
		return std::make_unique<cActionUpgradeVehicle>(archive);
	case eActiontype::ACTION_UPGRADE_BUILDING:
		return std::make_unique<cActionUpgradeBuilding>(archive);
	case eActiontype::ACTION_SET_AUTO_MOVE:
		return std::make_unique<cActionSetAutoMove>(archive);
	default:
		throw std::runtime_error("Unknown action type " + std::to_string(static_cast<int>(type)));
		return nullptr;
	}
}

//------------------------------------------------------------------------------
cAction::eActiontype cAction::getType() const
{
	return type;
}

//------------------------------------------------------------------------------
std::string enumToString(cAction::eActiontype value)
{
	switch (value)
	{
	case cAction::eActiontype::ACTION_INIT_NEW_GAME:
		return "ACTION_INIT_NEW_GAME";
	case cAction::eActiontype::ACTION_START_WORK:
		return "ACTION_START_WORK";
	case cAction::eActiontype::ACTION_STOP:
		return "ACTION_STOP";
	case cAction::eActiontype::ACTION_TRANSFER:
		return "ACTION_TRANSFER";
	case cAction::eActiontype::ACTION_START_MOVE:
		return "ACTION_START_MOVE";
	case cAction::eActiontype::ACTION_RESUME_MOVE:
		return "ACTION_RESUME_MOVE";
	case cAction::eActiontype::ACTION_START_TURN:
		return "ACTION_START_TURN";
	case cAction::eActiontype::ACTION_END_TURN:
		return "ACTION_END_TURN";
	case cAction::eActiontype::ACTION_SELF_DESTROY:
		return "ACTION_SELF_DESTROY";
	case cAction::eActiontype::ACTION_ATTACK:
		return "ACTION_ATTACK";
	case cAction::eActiontype::ACTION_CHANGE_SENTRY:
		return "ACTION_CHANGE_SENTRY";
	case cAction::eActiontype::ACTION_CHANGE_MANUAL_FIRE:
		return "ACTION_CHANGE_MANUAL_FIRE";
	case cAction::eActiontype::ACTION_MINELAYER_STATUS:
		return "ACTION_MINELAYER_STATUS";
	case cAction::eActiontype::ACTION_START_BUILD:
		return "ACTION_START_BUILD";
	case cAction::eActiontype::ACTION_FINISH_BUILD:
		return "ACTION_FINISH_BUILD";
	case cAction::eActiontype::ACTION_CHANGE_BUILDLIST:
		return "ACTION_CHANGE_BUILDLIST";
	case cAction::eActiontype::ACTION_LOAD:
		return "ACTION_LOAD";
	case cAction::eActiontype::ACTION_ACTIVATE:
		return "ACTION_ACTIVATE";
	case cAction::eActiontype::ACTION_REPAIR_RELOAD:
		return "ACTION_REPAIR_RELOAD";
	case cAction::eActiontype::ACTION_RESOURCE_DISTRIBUTION:
		return "ACTION_RESOURCE_DISTRIBUTION";
	case cAction::eActiontype::ACTION_CLEAR:
		return "ACTION_CLEAR";
	case cAction::eActiontype::ACTION_STEAL_DISABLE:
		return "ACTION_STEAL_DISABLE";
	case cAction::eActiontype::ACTION_CHANGE_RESEARCH:
		return "ACTION_CHANGE_RESEARCH";
	case cAction::eActiontype::ACTION_CHANGE_UNIT_NAME:
		return "ACTION_CHANGE_UNIT_NAME";
	case cAction::eActiontype::ACTION_BUY_UPGRADES:
		return "ACTION_BUY_UPGRADES";
	case cAction::eActiontype::ACTION_UPGRADE_VEHICLE:
		return "ACTION_UPGRADE_VEHICLE";
	case cAction::eActiontype::ACTION_UPGRADE_BUILDING:
		return "ACTION_UPGRADE_BUILDING";
	case cAction::eActiontype::ACTION_SET_AUTO_MOVE:
		return "ACTION_SET_AUTO_MOVE";
	default:
		assert(false);
		return toString(static_cast<int>(value));
		break;
	}
}
