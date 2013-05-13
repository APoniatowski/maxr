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
#include "clientevents.h"
#include "network.h"
#include "events.h"
#include "client.h"
#include "hud.h"
#include "netmessage.h"
#include "buildings.h"
#include "vehicles.h"
#include "player.h"

using namespace std;

void sendChatMessageToServer (const cClient &client, const string& sMsg)
{
	cNetMessage* message = new cNetMessage (GAME_EV_CHAT_CLIENT);
	message->pushString (sMsg);
	client.sendNetMessage (message);
}

void sendWantToEndTurn(const cClient &client)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_TO_END_TURN);
	client.sendNetMessage (message);
}

void sendWantStartWork (const cClient &client, const cUnit* building)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_START_WORK);
	message->pushInt32 (building->iID);
	client.sendNetMessage (message);
}

void sendWantStopWork (const cClient &client, const cUnit* building)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_STOP_WORK);
	message->pushInt32 (building->iID);
	client.sendNetMessage (message);
}

void sendMoveJob (const cClient &client, sWaypoint* path, int vehicleID)
{
	cNetMessage* message = new cNetMessage (GAME_EV_MOVE_JOB_CLIENT);

	const sWaypoint* waypoint = path;
	int iCount = 0;
	while (waypoint)
	{
		message->pushInt16 (waypoint->Costs);
		message->pushInt16 (waypoint->X);
		message->pushInt16 (waypoint->Y);

		if (message->iLength > PACKAGE_LENGTH - 19)
		{
			Log.write (" Client: Error sending movejob: message too long", cLog::eLOG_TYPE_NET_ERROR);
			delete message;
			return; // don't send movejobs that are to long
		}

		waypoint = waypoint->next;
		iCount++;
	}

	message->pushInt16 (iCount);
	message->pushInt32 (vehicleID);

	client.sendNetMessage (message);

	while (path)
	{
		waypoint = path;
		path = path->next;
		delete waypoint;
	}
}

void sendWantStopMove (const cClient &client, int iVehicleID)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_STOP_MOVE);
	message->pushInt16 (iVehicleID);
	client.sendNetMessage (message);
}

void sendMoveJobResume (const cClient &client, int unitId)
{
	cNetMessage* message = new cNetMessage (GAME_EV_MOVEJOB_RESUME);
	message->pushInt32 (unitId);
	client.sendNetMessage (message);
}

void sendWantAttack (const cClient &client, int targetID, int targetOffset, int agressor, bool isVehicle)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_ATTACK);
	message->pushInt32 (targetID);
	message->pushInt32 (targetOffset);
	message->pushInt32 (agressor);
	message->pushBool (isVehicle);
	client.sendNetMessage (message);
}

void sendMineLayerStatus (const cClient &client, const cVehicle* Vehicle)
{
	cNetMessage* message = new cNetMessage (GAME_EV_MINELAYERSTATUS);
	message->pushBool (Vehicle->LayMines);
	message->pushBool (Vehicle->ClearMines);
	message->pushInt16 (Vehicle->iID);
	client.sendNetMessage (message);
}

void sendWantBuild (const cClient &client, int iVehicleID, sID BuildingTypeID, int iBuildSpeed, int iBuildOff, bool bBuildPath, int iPathOff)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_BUILD);
	message->pushInt32 (iPathOff);
	message->pushBool (bBuildPath);
	message->pushInt32 (iBuildOff);
	message->pushInt16 (iBuildSpeed);
	message->pushInt16 (BuildingTypeID.iSecondPart);
	message->pushInt16 (BuildingTypeID.iFirstPart);
	message->pushInt16 (iVehicleID);
	client.sendNetMessage (message);
}

void sendWantEndBuilding (const cClient &client, const cVehicle* Vehicle, int EscapeX, int EscapeY)
{
	cNetMessage* message = new cNetMessage (GAME_EV_END_BUILDING);
	message->pushInt16 (EscapeY);
	message->pushInt16 (EscapeX);
	message->pushInt16 (Vehicle->iID);
	client.sendNetMessage (message);
}

void sendWantStopBuilding (const cClient &client, int iVehicleID)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_STOP_BUILDING);
	message->pushInt16 (iVehicleID);
	client.sendNetMessage (message);
}

void sendWantTransfer (const cClient &client, bool bSrcVehicle, int iSrcID, bool bDestVehicle, int iDestID, int iTransferValue, int iType)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_TRANSFER);
	message->pushInt16 (iType);
	message->pushInt16 (iTransferValue);
	message->pushInt16 (iDestID);
	message->pushBool (bDestVehicle);
	message->pushInt16 (iSrcID);
	message->pushBool (bSrcVehicle);
	client.sendNetMessage (message);
}

void sendWantBuildList (const cClient &client, const cBuilding* Building, const cList<sBuildList>& BuildList, bool bRepeat, int buildSpeed)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_BUILDLIST);
	message->pushBool (bRepeat);
	for (int i = (int) BuildList.size() - 1; i >= 0; i--)
	{
		message->pushInt16 (BuildList[i].type.iSecondPart);
		message->pushInt16 (BuildList[i].type.iFirstPart);
	}
	message->pushInt16 ( (int) BuildList.size());
	message->pushInt16 (buildSpeed);
	message->pushInt16 (Building->iID);
	client.sendNetMessage (message);
}

void sendWantExitFinishedVehicle (const cClient &client, const cBuilding* Building, int iX, int iY)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_EXIT_FIN_VEH);
	message->pushInt16 (iY);
	message->pushInt16 (iX);
	message->pushInt16 (Building->iID);
	client.sendNetMessage (message);
}

void sendChangeResources (const cClient &client, const cBuilding* Building, int iMetalProd, int iOilProd, int iGoldProd)
{
	cNetMessage* message = new cNetMessage (GAME_EV_CHANGE_RESOURCES);
	message->pushInt16 (iGoldProd);
	message->pushInt16 (iOilProd);
	message->pushInt16 (iMetalProd);
	message->pushInt16 (Building->iID);
	client.sendNetMessage (message);
}

void sendChangeManualFireStatus (const cClient &client, int iUnitID, bool bVehicle)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_CHANGE_MANUAL_FIRE);
	message->pushInt16 (iUnitID);
	message->pushBool (bVehicle);
	client.sendNetMessage (message);
}

void sendChangeSentry (const cClient &client, int iUnitID, bool bVehicle)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_CHANGE_SENTRY);
	message->pushInt16 (iUnitID);
	message->pushBool (bVehicle);
	client.sendNetMessage (message);
}

void sendWantSupply (const cClient &client, int iDestID, bool bDestVehicle, int iSrcID, bool bSrcVehicle, int iType)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_SUPPLY);
	message->pushInt16 (iDestID);
	message->pushBool (bDestVehicle);
	message->pushInt16 (iSrcID);
	message->pushBool (bSrcVehicle);
	message->pushChar (iType);
	client.sendNetMessage (message);
}

void sendWantStartClear (const cClient &client, const cUnit* unit)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_START_CLEAR);
	message->pushInt16 (unit->iID);
	client.sendNetMessage (message);
}

void sendWantStopClear (const cClient &client, const cUnit* unit)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_STOP_CLEAR);
	message->pushInt16 (unit->iID);
	client.sendNetMessage (message);
}

void sendAbortWaiting(const cClient &client)
{
	cNetMessage* message = new cNetMessage (GAME_EV_ABORT_WAITING);
	client.sendNetMessage (message);
}

void sendWantLoad (const cClient &client, int unitid, bool vehicle, int loadedunitid)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_LOAD);
	message->pushInt16 (unitid);
	message->pushBool (vehicle);
	message->pushInt16 (loadedunitid);
	client.sendNetMessage (message);
}

void sendWantActivate (const cClient &client, int unitid, bool vehicle, int activatunitid, int x, int y)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_EXIT);
	message->pushInt16 (y);
	message->pushInt16 (x);
	message->pushInt16 (unitid);
	message->pushBool (vehicle);
	message->pushInt16 (activatunitid);
	client.sendNetMessage (message);
}

void sendRequestResync (const cClient &client, char PlayerNr)
{
	cNetMessage* message = new cNetMessage (GAME_EV_REQUEST_RESYNC);
	message->pushChar (PlayerNr);
	client.sendNetMessage (message);
}

void sendSetAutoStatus (const cClient &client, int unitID, bool set)
{
	cNetMessage* message = new cNetMessage (GAME_EV_AUTOMOVE_STATUS);
	message->pushBool (set);
	message->pushInt16 (unitID);
	client.sendNetMessage (message);
}

void sendWantComAction (const cClient &client, int srcUnitID, int destUnitID, bool destIsVehicle, bool steal)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_COM_ACTION);
	message->pushBool (steal);
	message->pushInt16 (destUnitID);
	message->pushBool (destIsVehicle);
	message->pushInt16 (srcUnitID);
	client.sendNetMessage (message);
}

void sendWantUpgrade (const cClient &client, int buildingID, int storageSlot, bool upgradeAll)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_VEHICLE_UPGRADE);
	message->pushInt32 (buildingID);
	if (!upgradeAll) message->pushInt16 (storageSlot);
	message->pushBool (upgradeAll);
	client.sendNetMessage (message);
}

void sendWantResearchChange (const cClient &client, int (&newResearchSettings)[cResearch::kNrResearchAreas], int ownerNr)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_RESEARCH_CHANGE);
	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
	{
		message->pushInt16 (newResearchSettings[i]);
	}
	message->pushInt16 (ownerNr);
	client.sendNetMessage (message);
}

void sendSaveHudInfo (const cClient &client, int selectedUnitID, int ownerNr, int savingID)
{
	cNetMessage* message = new cNetMessage (GAME_EV_SAVE_HUD_INFO);
	message->pushBool (client.gameGUI.tntChecked());
	message->pushBool (client.gameGUI.hitsChecked());
	message->pushBool (client.gameGUI.lockChecked());
	message->pushBool (client.gameGUI.surveyChecked());
	message->pushBool (client.gameGUI.statusChecked());
	message->pushBool (client.gameGUI.scanChecked());
	message->pushBool (client.gameGUI.rangeChecked());
	message->pushBool (client.gameGUI.twoXChecked());
	message->pushBool (client.gameGUI.fogChecked());
	message->pushBool (client.gameGUI.ammoChecked());
	message->pushBool (client.gameGUI.gridChecked());
	message->pushBool (client.gameGUI.colorChecked());
	message->pushFloat (client.gameGUI.getZoom());
	message->pushInt16 (client.gameGUI.getOffsetY());
	message->pushInt16 (client.gameGUI.getOffsetX());
	message->pushInt16 (selectedUnitID);
	message->pushInt16 (ownerNr);
	message->pushInt16 (savingID);
	client.sendNetMessage (message);
}

void sendSaveReportInfo (const cClient &client, const sSavedReportMessage* savedReport, int ownerNr, int savingID)
{
	cNetMessage* message = new cNetMessage (GAME_EV_SAVE_REPORT_INFO);
	message->pushInt16 (savedReport->colorNr);
	message->pushInt16 (savedReport->unitID.iSecondPart);
	message->pushInt16 (savedReport->unitID.iFirstPart);
	message->pushInt16 (savedReport->yPos);
	message->pushInt16 (savedReport->xPos);
	message->pushInt16 (savedReport->type);
	message->pushString (savedReport->message);
	message->pushInt16 (ownerNr);
	message->pushInt16 (savingID);
	client.sendNetMessage (message);
}

void sendFinishedSendSaveInfo (const cClient &client, int ownerNr, int savingID)
{
	cNetMessage* message = new cNetMessage (GAME_EV_FIN_SEND_SAVE_INFO);
	message->pushInt16 (ownerNr);
	message->pushInt16 (savingID);
	client.sendNetMessage (message);
}

void sendRequestCasualtiesReport(const cClient &client)
{
	cNetMessage* message = new cNetMessage (GAME_EV_REQUEST_CASUALTIES_REPORT);
	client.sendNetMessage (message);
}

void sendWantSelfDestroy (const cClient &client, const cUnit* building)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_SELFDESTROY);
	message->pushInt16 (building->iID);
	client.sendNetMessage (message);
}

void sendWantChangeUnitName (const cClient &client, const string& newName, int unitID)
{
	cNetMessage* message = new cNetMessage (GAME_EV_WANT_CHANGE_UNIT_NAME);
	message->pushString (newName);
	message->pushInt16 (unitID);
	client.sendNetMessage (message);
}

void sendEndMoveAction (const cClient &client, int vehicleID, int destID, eEndMoveActionType type)
{
	cNetMessage* message = new cNetMessage (GAME_EV_END_MOVE_ACTION);
	message->pushChar (type);
	message->pushInt32 (destID);
	message->pushInt32 (vehicleID);
	client.sendNetMessage (message);
}
