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
#include <math.h>
#include <sstream>
#include "client.h"
#include "server.h"
#include "events.h"
#include "serverevents.h"
#include "pcx.h"
#include "mouse.h"
#include "keys.h"
#include "unifonts.h"
#include "netmessage.h"
#include "main.h"
#include "attackJobs.h"
#include "buttons.h"
#include "menu.h"

sMessage::sMessage(std::string const& s, unsigned int const age_)
{
	chars = (int)s.length();
	msg = new char[chars + 1];
	strcpy(msg, s.c_str());
	if (chars > 500) msg[500] = '\0';
	len = font->getTextWide(s);
	age = age_;
}


sMessage::~sMessage()
{
	delete [] msg;
}

sFX::sFX( eFXTyps typ, int x, int y )
{
	this->typ = typ;
	PosX = x;
	PosY = y;
	StartFrame = Client->iFrame;
	param = 0;
	rocketInfo = NULL;
	smokeInfo = NULL;
	trackInfo = NULL;
	param = 0;

	switch ( typ )
	{
	case fxRocket:
	case fxTorpedo:
		rocketInfo = new sFXRocketInfos();
		rocketInfo->ScrX = 0;
		rocketInfo->ScrY = 0;
		rocketInfo->DestX = 0;
		rocketInfo->DestY = 0;
		rocketInfo->dir = 0;
		rocketInfo->fpx = 0;
		rocketInfo->fpy = 0;
		rocketInfo->mx = 0;
		rocketInfo->my = 0;
		rocketInfo->aj = NULL;
		break;
	case fxDarkSmoke:
		smokeInfo = new sFXDarkSmoke();
		smokeInfo->alpha = 0;
		smokeInfo->fx = 0;
		smokeInfo->fy = 0;
		smokeInfo->dx = 0;
		smokeInfo->dy = 0;
		break;
	case fxTracks:
		trackInfo = new sFXTracks();
		trackInfo->alpha = 0;
		trackInfo->dir = 0;
		break;
	}
}

sFX::~sFX()
{
	if ( rocketInfo ) delete rocketInfo;
	if ( smokeInfo ) delete smokeInfo;
	if ( trackInfo ) delete trackInfo;
}

Uint32 TimerCallback(Uint32 interval, void *arg)
{
	((cClient *)arg)->Timer();
	return interval;
}

cClient::cClient(cMap* const Map, cList<cPlayer*>* const PlayerList)
{
	this->Map = Map;
	bExit = false;

	this->PlayerList = PlayerList;

	TimerID = SDL_AddTimer ( 50, TimerCallback, this );
	iTimerTime = 0;
	iFrame = 0;
	fFPS = 0;
	fCPS = 0;
	iLoad = 0;
	SelectedVehicle = NULL;
	SelectedBuilding = NULL;
	neutralBuildings = NULL;
	iObjectStream = -1;
	OverUnitField = NULL;
	iBlinkColor = 0xFFFFFF;
	FLC = NULL;
	sFLCname = "";
	video = NULL;
	bHelpActive = false;
	bChangeObjectName = false;
	bChatInput = false;
	bDefeated = false;
	iMsgCoordsX = -1;
	iMsgCoordsY = -1;
	iTurn = 1;
	bWantToEnd = false;
	bUpShowTank = true;
	bUpShowPlane = true;
	bUpShowShip = true;
	bUpShowBuild = true;
	bUpShowTNT = false;
	bAlienTech = false;
	bDebugAjobs = false;
	bDebugBaseServer = false;
	bDebugBaseClient = false;
	bDebugSentry = false;
	bDebugFX = false;
	bDebugTraceServer = false;
	bDebugTraceClient = false;
	bDebugPlayers = false;
	bShowFPS = false;
	bWaitForOthers = false;
	iTurnTime = 0;
	isInMenu = false;

	SDL_Rect rSrc = {0,0,170,224};
	SDL_Surface *SfTmp = LoadPCX((SettingsData.sGfxPath + PATH_DELIMITER + "hud_left.pcx").c_str());
	SDL_BlitSurface( SfTmp, &rSrc, GraphicsData.gfx_hud, NULL );
	SDL_FreeSurface( SfTmp );

	for ( int i = 0; i < 4; i++ )
	{
		SavedPositions[i].offsetX = -1;
		SavedPositions[i].offsetY = -1;
	}

	setWind(random(360));
}

cClient::~cClient()
{
	Hud.Zoom = 64;
	Hud.ScaleSurfaces();
	SDL_RemoveTimer ( TimerID );
	StopFXLoop ( iObjectStream );
	while (messages.Size())
	{
		delete messages[0];
		messages.Delete ( 0 );
	}
	if ( FLC ) FLI_Close ( FLC );
	while (FXList.Size())
	{
		delete FXList[0];
		FXList.Delete ( 0 );
	}
	while (FXListBottom.Size())
	{
		delete FXListBottom[0];
		FXListBottom.Delete ( 0 );
	}

	for (unsigned int i = 0; i < attackJobs.Size(); i++)
	{
		delete attackJobs[i];
	}

	while ( neutralBuildings )
	{
		cBuilding* nextBuilding = neutralBuildings->next;
		delete neutralBuildings;
		neutralBuildings = nextBuilding;
	}
}

void cClient::sendNetMessage(cNetMessage *message)
{
	message->iPlayerNr = ActivePlayer->Nr;

	Log.write("Client: <-- " + message->getTypeAsString() + ", Hexdump: " + message->getHexDump(), cLog::eLOG_TYPE_NET_DEBUG );

	if (!network || network->isHost() )
	{
		//push an event to the lokal server in singleplayer, HotSeat or if this machine is the host
		Server->pushEvent(message->getGameEvent() );
		delete message;
		//Server->pushNetMessage( message );
	}
	else // else send it over the net
	{
		//the client is only connected to one socket
		//so netwwork->send() only sends to the server
		network->send( message->iLength, message->serialize() );
		delete message;
	}
}

void cClient::Timer()
{
	iTimerTime++;
}

void cClient::initPlayer( cPlayer *Player )
{
	bFlagDrawHud = true;
	bFlagDrawMap = true;
	bFlagDraw = true;
	bFlagDrawMMap = true;

	ActivePlayer = Player;

	// generate subbase for enemy players
	for ( unsigned int i = 0; i < PlayerList->Size(); i++ )
	{
		if ( (*PlayerList)[i] == ActivePlayer ) continue;
		(*PlayerList)[i]->base.SubBases.Add ( new sSubBase ( -(int)(i+1) ) );
	}
}

void cClient::run()
{
	int iLastMouseX = 0, iLastMouseY = 0;
	bool bStartup = true;

	mouse->Show();
	mouse->SetCursor ( CHand );
	mouse->MoveCallback = true;
	Hud.reset(); //to clean old garbage on hud from previous games

	waitForOtherPlayer( 0, true );

	while ( 1 )
	{
		// check defeat
		if ( bDefeated ) break;
		// get events
		EventHandler->HandleEvents();
		// check mouse moves 
		mouse->GetPos();
		Hud.CheckMouseOver();
		Hud.CheckScroll();
		CHECK_MEMORY;
		// check minimap
		if ( clientMouseState.leftButtonPressed && !bHelpActive ) Hud.CheckOneClick();
		// check length of input strings
		if ( bChangeObjectName && InputHandler->checkHasBeenInput () ) InputHandler->cutToLength ( 128 );
		else if ( bChatInput && InputHandler->checkHasBeenInput () ) InputHandler->cutToLength ( PACKAGE_LENGTH-20 );
		// end truth save/load menu
		if ( bExit )
		{
			drawMap();
			SHOW_SCREEN
			makePanel ( false );
			break;
		}
		// Die Map malen:
		if ( bFlagDrawMap )
		{
			drawMap();
			displayFX();
		}
		CHECK_MEMORY;

		// Ggf das Objekt deselektieren:
		if ( SelectedVehicle&&SelectedVehicle->owner!=ActivePlayer&&!ActivePlayer->ScanMap[SelectedVehicle->PosX+SelectedVehicle->PosY*Map->size] )
		{
			SelectedVehicle->Deselct();
			SelectedVehicle=NULL;
		}
		if ( SelectedBuilding&&SelectedBuilding->owner!=ActivePlayer&&!ActivePlayer->ScanMap[SelectedBuilding->PosX+SelectedBuilding->PosY*Map->size] )
		{
			SelectedBuilding->Deselct();
			SelectedBuilding=NULL;
		}

		// draw the unit circles
		drawUnitCircles();
		CHECK_MEMORY;
		// draw the minimap:
		if ( bFlagDrawMMap )
		{
			bFlagDrawMMap = false;
			drawMiniMap();
			bFlagDrawHud = true;
		}
		CHECK_MEMORY;
		// draw the debug information
		displayDebugOutput();

		// check whether the hud has to be drawn:
		if ( bFlagDrawHud || bFlagDrawMap )
		{
			SDL_BlitSurface ( GraphicsData.gfx_hud, NULL, buffer, NULL );
			mouse->GetBack ( buffer );
			bFlagDraw = true;
		}
		CHECK_MEMORY;
		// draw the video:
		if ( bFlagDraw || bFlagDrawHud )
		{
			drawFLC();
		}
		CHECK_MEMORY;
		// display the object menu:
		if ( bFlagDrawMap && SelectedVehicle && SelectedVehicle->MenuActive )
		{
			SelectedVehicle->DrawMenu();
		}
		if ( bFlagDrawMap && SelectedBuilding && SelectedBuilding->MenuActive )
		{
			SelectedBuilding->DrawMenu();
		}
		CHECK_MEMORY;
		// display the chatinput:
		if ( bChatInput && bFlagDrawMap )
		{
			displayChatInput();
		}
		// display the messages:
		if ( bFlagDrawMap )
		{
			handleMessages();
		}

		CHECK_MEMORY;
		// draw the mouse:
		if ( bFlagDraw )
		{
			iLastMouseX = mouse->x;
			iLastMouseY = mouse->y;
			mouse->draw ( false, buffer );
		}
		else if ( mouse->x != iLastMouseX || mouse->y != iLastMouseY )
		{
			iLastMouseX = mouse->x;
			iLastMouseY = mouse->y;
			mouse->draw ( true, screen );
		}

		// display the buffer:
		if ( bFlagDraw )
		{
			if ( bStartup )
			{
				int hudzoom = Hud.Zoom;
				Hud.SetZoom(1);
				drawMap();
				SHOW_SCREEN
				makePanel ( true );

				Hud.SetZoom( hudzoom );
				if ( bStartupHud )
				{
					if ( ActivePlayer->BuildingList) ActivePlayer->BuildingList->Center();
					else if (ActivePlayer->VehicleList) ActivePlayer->VehicleList->Center();
					bStartupHud = false;
				}
				drawMap();
				SHOW_SCREEN

				bStartup = false;
			}
			CHECK_MEMORY;
			SHOW_SCREEN
			bFlagDraw = false;
			bFlagDrawHud = false;
			bFlagDrawMap = false;
		}
		else if ( !SettingsData.bFastMode )
		{
			SDL_Delay ( 10 ); // theres northing to do.
		}
		// handle the timers and do game actions:
		doGameActions();
		if ( iTimer1 )
		{
			iFrame++;
			bFlagDrawMap = true;
			rotateBlinkColor();
			if ( FLC != NULL && Hud.PlayFLC )
			{
				FLI_NextFrame ( FLC );
			}
		}

		handleTurnTime();
		CHECK_MEMORY;
		// change the wind direction:
		if ( iTimer2 && SettingsData.bDamageEffects )
		{
			static int iNextChange = 25, iNextDirChange = 25, iDir = 90, iChange = 3;
			if ( iNextChange == 0 )
			{
				iNextChange = 10 + random(20);
				iDir += iChange;
				setWind ( iDir );
				if ( iDir >= 360 ) iDir -= 360;
				else if ( iDir < 0 ) iDir += 360;

				if ( iNextDirChange==0 )
				{
					iNextDirChange = random(25) + 10;
					iChange        = random(11) -  5;
				}
				else iNextDirChange--;

			}
			else iNextChange--;
		}
		//handle frames/s, cycles/s and load
		if ( bShowFPS )
		{
			static Uint32 iLastTicks = 0;
			static Uint32 iLastFrame = 0;
			static Uint32 iCycles = 0;
			static Uint32 iInverseLoad = 0; //this is 10*(100% - load) 
			static Uint32 iLastTickLoad = 0;

			iCycles++;
			Uint32 iTicks = SDL_GetTicks();
			
			if ( iTicks != iLastTickLoad ) iInverseLoad++;
			iLastTickLoad = iTicks;

			if ( iTicks > iLastTicks + 1000 )
			{
				float a = 0.5f;	//low pass filter coefficient

				fFPS = (1-a)*(iFrame - iLastFrame)*1000/(float)(iTicks - iLastTicks) + a*fFPS;
				iLastFrame = iFrame;

				fCPS = (1-a)*iCycles*1000 / (float) (iTicks - iLastTicks) + a*fCPS;
				iCycles = 0;

				iLoad = Round((1-a)*(1000 - iInverseLoad) + a*iLoad);
				iInverseLoad = 0;

				iLastTicks = iTicks;
			}
		}

		CHECK_MEMORY;
	}
	mouse->MoveCallback = false;
}

void cClient::handleMouseInput( sMouseState mouseState  )
{
	bool bChange = !bWaitForOthers;

	mouse->GetPos();
	clientMouseState = mouseState;

	if ( isInMenu ) return;
	cVehicle* overVehicle = NULL;
	cVehicle* overPlane = NULL;
	cBuilding* overBuilding = NULL;
	cBuilding* overBaseBuilding = NULL;
	if ( OverUnitField )
	{
		overVehicle  = OverUnitField->getVehicles();
		overPlane    = OverUnitField->getPlanes();
		overBuilding = OverUnitField->getTopBuilding();
		overBaseBuilding = OverUnitField->getBaseBuilding();
	}

	// give the mouse input to the unit menu if one is active
	if ( SelectedVehicle && SelectedVehicle->MenuActive && SelectedVehicle->MouseOverMenu ( mouse->x, mouse->y ) )
	{
		SelectedVehicle->DrawMenu ( &mouseState );
		return;
	}
	if ( SelectedBuilding && SelectedBuilding->MenuActive && SelectedBuilding->MouseOverMenu ( mouse->x, mouse->y ) )
	{
		SelectedBuilding->DrawMenu ( &mouseState );
		return;
	}
	// handle input on the map
	if ( MouseStyle == OldSchool && mouseState.rightButtonPressed && !mouseState.leftButtonHold && OverUnitField )
	{
		if ( overVehicle && overVehicle == SelectedVehicle ) SelectedVehicle->ShowHelp();
		else if ( overPlane && overPlane == SelectedVehicle ) SelectedVehicle->ShowHelp();
		else if ( overBuilding && overBuilding == SelectedBuilding ) SelectedBuilding->ShowHelp();
		else if ( overBaseBuilding && overBaseBuilding == SelectedBuilding ) SelectedBuilding->ShowHelp();
		else if ( OverUnitField ) selectUnit ( OverUnitField, true );
	}
	else if ( ( mouseState.rightButtonPressed && !mouseState.leftButtonHold ) || ( MouseStyle == OldSchool && mouseState.leftButtonHold && mouseState.rightButtonPressed ) )
	{
		if ( bHelpActive )
		{
			bHelpActive = false;
		}
		else
		{
			if ( OverUnitField && (
			            ( SelectedVehicle && ( overVehicle == SelectedVehicle || overPlane == SelectedVehicle ) ) ||
			            ( SelectedBuilding && ( overBaseBuilding == SelectedBuilding || overBuilding == SelectedBuilding ) ) ) )
			{
				int next = -1;

				if ( SelectedVehicle )
				{
					if ( overPlane == SelectedVehicle )
					{
						if ( overVehicle ) next = 'v';
						else if ( overBuilding ) next = 't';
						else if ( overBaseBuilding ) next = 'b';
					}
					else
					{
						if ( overBuilding ) next = 't';
						else if ( overBaseBuilding ) next = 'b';
						else if ( overPlane ) next = 'p';
					}

					SelectedVehicle->Deselct();
					SelectedVehicle = NULL;
					bChangeObjectName = false;
					if ( !bChatInput ) InputHandler->setInputState ( false );
					StopFXLoop ( iObjectStream );
				}
				else if ( SelectedBuilding )
				{
					if ( overBuilding == SelectedBuilding )
					{
						if ( overBaseBuilding ) next = 'b';
						else if ( overPlane ) next = 'p';
						else if ( OverUnitField->getVehicles() ) next = 'v';
					}
					else
					{
						if ( overPlane ) next = 'p';
						else if ( OverUnitField->getVehicles() ) next = 'v';
						else if ( overBuilding ) next = 't';
					}

					SelectedBuilding->Deselct();
					SelectedBuilding = NULL;
					bChangeObjectName = false;
					if ( !bChatInput ) InputHandler->setInputState ( false );
					StopFXLoop ( iObjectStream );
				}
				switch ( next )
				{
					case 't':
						SelectedBuilding = overBuilding ;
						SelectedBuilding->Select();
						iObjectStream=SelectedBuilding->PlayStram();
						break;
					case 'b':
						SelectedBuilding = overBaseBuilding;
						SelectedBuilding->Select();
						iObjectStream=SelectedBuilding->PlayStram();
						break;
					case 'v':
						SelectedVehicle = overVehicle;
						SelectedVehicle->Select();
						iObjectStream = SelectedVehicle->PlayStram();
						break;
					case 'p':
						SelectedVehicle = overPlane;
						SelectedVehicle->Select();
						iObjectStream = SelectedVehicle->PlayStram();
						break;
				}
			}
			else if ( SelectedVehicle != NULL )
			{
				SelectedVehicle->Deselct();
				SelectedVehicle = NULL;
				bChangeObjectName = false;
				if ( !bChatInput ) InputHandler->setInputState ( false );
				StopFXLoop ( iObjectStream );
			}
			else if ( SelectedBuilding!=NULL )
			{
				SelectedBuilding->Deselct();
				SelectedBuilding = NULL;
				bChangeObjectName = false;
				if ( !bChatInput ) InputHandler->setInputState ( false );
				StopFXLoop ( iObjectStream );
			}
		}
	}
	if ( mouseState.leftButtonPressed && !mouseState.rightButtonHold )
	{
		if ( OverUnitField && Hud.Lock ) ActivePlayer->ToggelLock ( OverUnitField );
		if ( bChange && SelectedVehicle && mouse->cur == GraphicsData.gfx_Ctransf )
		{
			if ( overVehicle ) showTransfer ( NULL, SelectedVehicle, NULL, overVehicle );
			else if ( overBuilding ) showTransfer ( NULL, SelectedVehicle, overBuilding, NULL );
		}
		else if ( bChange && SelectedBuilding && mouse->cur == GraphicsData.gfx_Ctransf )
		{
			if ( overVehicle ) showTransfer ( SelectedBuilding, NULL, NULL, overVehicle );
		}
		else if ( bChange && SelectedVehicle && SelectedVehicle->PlaceBand && mouse->cur == GraphicsData.gfx_Cband )
		{
			SelectedVehicle->PlaceBand = false;
			if ( SelectedVehicle->BuildingTyp.getUnitData()->is_big )
			{
				sendWantBuild ( SelectedVehicle->iID, SelectedVehicle->BuildingTyp, SelectedVehicle->BuildRounds, SelectedVehicle->BandX+SelectedVehicle->BandY*Map->size, false, 0 );
			}
			else
			{
				sendWantBuild ( SelectedVehicle->iID, SelectedVehicle->BuildingTyp, SelectedVehicle->BuildRounds, SelectedVehicle->PosX+SelectedVehicle->PosY*Map->size, true, SelectedVehicle->BandX+SelectedVehicle->BandY*Map->size );
			}
		}
		else if ( bChange && mouse->cur == GraphicsData.gfx_Cactivate && SelectedBuilding && SelectedBuilding->ActivatingVehicle )
		{
			sendWantActivate ( SelectedBuilding->iID, false, SelectedBuilding->StoredVehicles[SelectedBuilding->VehicleToActivate]->iID, mouse->GetKachelOff()%Map->size, mouse->GetKachelOff()/Map->size );
			mouseMoveCallback ( true );
		}
		else if ( bChange && mouse->cur == GraphicsData.gfx_Cactivate && SelectedVehicle && SelectedVehicle->ActivatingVehicle )
		{
			sendWantActivate ( SelectedVehicle->iID, true, SelectedVehicle->StoredVehicles[SelectedVehicle->VehicleToActivate]->iID, mouse->GetKachelOff()%Map->size, mouse->GetKachelOff()/Map->size );
			mouseMoveCallback ( true );
		}
		else if ( bChange && mouse->cur == GraphicsData.gfx_Cactivate && SelectedBuilding && SelectedBuilding->BuildList && SelectedBuilding->BuildList->Size())
		{
			int iX, iY;
			mouse->GetKachel ( &iX, &iY );
			sendWantExitFinishedVehicle ( SelectedBuilding, iX, iY );
		}
		else if ( bChange && mouse->cur == GraphicsData.gfx_Cload && SelectedBuilding && SelectedBuilding->LoadActive )
		{
			if ( SelectedBuilding->data.can_load != TRANS_AIR && overVehicle ) sendWantLoad ( SelectedBuilding->iID, false, overVehicle->iID );
			else if ( SelectedBuilding->data.can_load == TRANS_AIR && overPlane ) sendWantLoad ( SelectedBuilding->iID, false, overPlane->iID );
		}
		else if ( bChange && mouse->cur == GraphicsData.gfx_Cload && SelectedVehicle && SelectedVehicle->LoadActive )
		{
			if ( overVehicle )
			{
				sendWantLoad ( SelectedVehicle->iID, true, overVehicle->iID );
			}
		}
		else if ( bChange && mouse->cur == GraphicsData.gfx_Cmuni && SelectedVehicle && SelectedVehicle->MuniActive )
		{
			if ( overVehicle ) sendWantSupply ( overVehicle->iID, true, SelectedVehicle->iID, true, SUPPLY_TYPE_REARM);
			else if ( overPlane && overPlane->FlightHigh == 0 ) sendWantSupply ( overPlane->iID, true, SelectedVehicle->iID, true, SUPPLY_TYPE_REARM);
			else if ( overBuilding ) sendWantSupply ( overBuilding->iID, false, SelectedVehicle->iID, true, SUPPLY_TYPE_REARM);
		}
		else if ( bChange && mouse->cur == GraphicsData.gfx_Crepair && SelectedVehicle && SelectedVehicle->RepairActive )
		{
			if ( overVehicle ) sendWantSupply ( overVehicle->iID, true, SelectedVehicle->iID, true, SUPPLY_TYPE_REPAIR);
			else if ( overPlane && overPlane->FlightHigh == 0 ) sendWantSupply ( overPlane->iID, true, SelectedVehicle->iID, true, SUPPLY_TYPE_REPAIR);
			else if ( overBuilding ) sendWantSupply ( overBuilding->iID, false, SelectedVehicle->iID, true, SUPPLY_TYPE_REPAIR);
		}
		else if ( !bHelpActive )
		{
			Hud.CheckButtons();
			// check whether the mouse is over an unit menu:
			if ( ( SelectedVehicle&&SelectedVehicle->MenuActive&&SelectedVehicle->MouseOverMenu ( mouse->x,mouse->y ) ) ||
			        ( SelectedBuilding&&SelectedBuilding->MenuActive&&SelectedBuilding->MouseOverMenu ( mouse->x,mouse->y ) ) )
			{
			}
			else
				// check, if the player wants to attack:
				if ( bChange && mouse->cur==GraphicsData.gfx_Cattack&&SelectedVehicle&&!SelectedVehicle->Attacking&&!SelectedVehicle->MoveJobActive )
				{
					//find target ID
					int targetId = 0;
					cVehicle* vehicle;
					cBuilding* building;
					selectTarget( vehicle, building, mouse->GetKachelOff(), SelectedVehicle->data.can_attack, Client->Map);
					if ( vehicle ) targetId = vehicle->iID;

					Log.write(" Client: want to attack offset " + iToStr(mouse->GetKachelOff()) + ", Vehicle ID: " + iToStr(targetId), cLog::eLOG_TYPE_NET_DEBUG );
					sendWantAttack( targetId, mouse->GetKachelOff(), SelectedVehicle->iID, true );
				}
				else if ( bChange && mouse->cur == GraphicsData.gfx_Cattack && SelectedBuilding && !SelectedBuilding->Attacking )
				{
					//find target ID
					int targetId = 0;
					cVehicle* vehicle;
					cBuilding* building;
					selectTarget( vehicle, building, mouse->GetKachelOff(), SelectedBuilding->data.can_attack, Client->Map);
					if ( vehicle ) targetId = vehicle->iID;

					int offset = SelectedBuilding->PosX + SelectedBuilding->PosY * Map->size;
					sendWantAttack( targetId, mouse->GetKachelOff(), offset, false );
				}
				else if ( bChange && mouse->cur == GraphicsData.gfx_Csteal && SelectedVehicle )
				{
					// TODO: add commando steal
					//addMessage ( lngPack.i18n ( "Text~Error_Messages~INFO_Not_Implemented" ) );
					if ( overVehicle ) sendWantComAction ( SelectedVehicle->iID, overVehicle->iID, true, true );
					else if ( overPlane && overPlane->FlightHigh == 0 ) sendWantComAction ( SelectedVehicle->iID, overVehicle->iID, true, true );
				}
				else if ( bChange && mouse->cur == GraphicsData.gfx_Cdisable && SelectedVehicle )
				{
					// TODO: add commando disable
					//addMessage ( lngPack.i18n ( "Text~Error_Messages~INFO_Not_Implemented" ) );
					if ( overVehicle ) sendWantComAction ( SelectedVehicle->iID, overVehicle->iID, true, false );
					else if ( overPlane && overPlane->FlightHigh == 0 ) sendWantComAction ( SelectedVehicle->iID, overPlane->iID, true, false );
					else if ( overBuilding ) sendWantComAction ( SelectedVehicle->iID, overBuilding->iID, false, false );
				}
				else if ( MouseStyle == OldSchool && OverUnitField && selectUnit ( OverUnitField, false ) )
				{}
				else if ( bChange && mouse->cur == GraphicsData.gfx_Cmove && SelectedVehicle && !SelectedVehicle->moving && !SelectedVehicle->Attacking )
				{
					if ( SelectedVehicle->IsBuilding )
					{
						int iX, iY;
						mouse->GetKachel ( &iX, &iY );
						sendWantEndBuilding ( SelectedVehicle, iX, iY );
					}
					else
					{
						addMoveJob( SelectedVehicle, mouse->GetKachelOff() );
					}
				}
				else if ( OverUnitField )
				{
					// open unit menu
					if ( bChange && SelectedVehicle && ( overPlane == SelectedVehicle || overVehicle == SelectedVehicle ) )
					{
						if ( !SelectedVehicle->moving && SelectedVehicle->owner == ActivePlayer )
						{
							SelectedVehicle->MenuActive = true;
							PlayFX ( SoundData.SNDHudButton );
						}
					}
					else if ( bChange && SelectedBuilding&& ( overBaseBuilding == SelectedBuilding || overBuilding == SelectedBuilding ) )
					{
						if ( SelectedBuilding->owner == ActivePlayer )
						{
							SelectedBuilding->MenuActive = true;
							PlayFX ( SoundData.SNDHudButton );
						}
					}
					// select unit when using modern style
					else if ( MouseStyle == Modern ) selectUnit ( OverUnitField, true );
				}
			// check whether the name of a unit has to be changed:
			if ( bChange && SelectedVehicle&&SelectedVehicle->owner==ActivePlayer&&mouse->x>=10&&mouse->y>=29&&mouse->x<10+128&&mouse->y<29+10 )
			{
				bChangeObjectName = true;
				InputHandler->setInputStr ( SelectedVehicle->name );
				InputHandler->setInputState ( true );
			}
			else if ( bChange && SelectedBuilding&&SelectedBuilding->owner==ActivePlayer&&mouse->x>=10&&mouse->y>=29&&mouse->x<10+128&&mouse->y<29+10 )
			{
				bChangeObjectName = true;
				InputHandler->setInputStr ( SelectedVehicle->name );
				InputHandler->setInputState ( true );
			}
		}
		else if ( OverUnitField )
		{
			if ( overPlane ) overPlane->ShowHelp();
			else if ( overVehicle )overVehicle->ShowHelp();
			else if ( overBuilding ) overBuilding->ShowHelp();
			else if ( overBaseBuilding ) overBaseBuilding->ShowHelp();
			bHelpActive = false;
		}
	}
	// check zoom via mousewheel
	if ( mouseState.wheelUp ) Hud.SetZoom ( Hud.Zoom+3 );
	else if ( mouseState.wheelDown ) Hud.SetZoom ( Hud.Zoom-3 );
}

sMouseState cClient::getMouseState()
{
	return clientMouseState;
}

void cClient::handleHotKey ( SDL_keysym &keysym )
{
	if ( isInMenu ) return;
	if ( keysym.sym == KeysList.KeyExit )
	{
		isInMenu = true;
		if ( ShowYesNo ( lngPack.i18n( "Text~Comp~End_Game") ) )
		{
			drawMap ( false );
			SDL_BlitSurface ( GraphicsData.gfx_hud, NULL, buffer, NULL );
			bExit = true;
		}
		isInMenu = false;
	}
	else if ( keysym.sym == SDLK_RETURN && ( bChatInput || bChangeObjectName ) )
	{
		if ( bChatInput )
		{
			bChatInput = false;
			string s = InputHandler->getInputStr( CURSOR_DISABLED );
			if ( !s.empty() )
			{
				if ( s[0] == '/' ) 
				{
					doCommand( s );
				}
				else 
				{
					sendChatMessageToServer( ActivePlayer->name+": " + s );
				}
			}
			InputHandler->setInputState ( false );
		}
		else if ( bChangeObjectName && !bWaitForOthers )
		{
			bChangeObjectName = false;
			addMessage ( lngPack.i18n ( "Text~Error_Messages~INFO_Not_Implemented" ) );
			// TODO: implement changing names
			InputHandler->setInputState ( false );
		}
	}
	else if ( keysym.sym == KeysList.KeyEndTurn && !bWaitForOthers )
	{
		if ( !bWantToEnd )
		{
			Hud.EndeButton ( true );
			handleEnd();
		}
	}
	else if ( keysym.sym == KeysList.KeyJumpToAction )
	{
		if ( iMsgCoordsX!=-1 )
		{
			Hud.OffX = iMsgCoordsX * 64 - ( ( int ) ( ( ( float ) (SettingsData.iScreenW - 192) / (2 * Client->Hud.Zoom) ) * 64 ) ) + 32;
			Hud.OffY = iMsgCoordsY * 64 - ( ( int ) ( ( ( float ) (SettingsData.iScreenH - 32 ) / (2 * Client->Hud.Zoom) ) * 64 ) ) + 32;
			bFlagDrawMap=true;
			Hud.DoScroll ( 0 );
			iMsgCoordsX=-1;
		}
	}
	else if ( keysym.sym == KeysList.KeyChat )
	{
		if ( !(keysym.mod & KMOD_ALT) )
		{
			InputHandler->setInputState ( true );
			InputHandler->setInputStr ( "" );
			bChatInput = true;
		}
	}
	// scroll and zoom hotkeys
	else if ( keysym.sym == KeysList.KeyScroll1 ) Hud.DoScroll ( 1 );
	else if ( keysym.sym == KeysList.KeyScroll3 ) Hud.DoScroll ( 3 );
	else if ( keysym.sym == KeysList.KeyScroll7 ) Hud.DoScroll ( 7 );
	else if ( keysym.sym == KeysList.KeyScroll9 ) Hud.DoScroll ( 9 );
	else if ( keysym.sym == KeysList.KeyScroll2a || keysym.sym == KeysList.KeyScroll2b ) Hud.DoScroll ( 2 );
	else if ( keysym.sym == KeysList.KeyScroll4a || keysym.sym == KeysList.KeyScroll4b ) Hud.DoScroll ( 4 );
	else if ( keysym.sym == KeysList.KeyScroll6a || keysym.sym == KeysList.KeyScroll6b ) Hud.DoScroll ( 6 );
	else if ( keysym.sym == KeysList.KeyScroll8a || keysym.sym == KeysList.KeyScroll8b ) Hud.DoScroll ( 8 );
	else if ( keysym.sym == KeysList.KeyZoomIna || keysym.sym == KeysList.KeyZoomInb ) Hud.SetZoom ( Hud.Zoom+1 );
	else if ( keysym.sym == KeysList.KeyZoomOuta || keysym.sym == KeysList.KeyZoomOutb ) Hud.SetZoom ( Hud.Zoom-1 );
	// position handling hotkeys
	else if ( keysym.sym == KeysList.KeyCenterUnit && SelectedVehicle ) SelectedVehicle->Center();
	else if ( keysym.sym == KeysList.KeyCenterUnit && SelectedBuilding ) SelectedBuilding->Center();
	else if ( keysym.sym == SDLK_F5 && keysym.mod & KMOD_ALT )
	{
		SavedPositions[0].offsetX = Hud.OffX;
		SavedPositions[0].offsetY = Hud.OffY;
	}
	else if ( keysym.sym == SDLK_F6 && keysym.mod & KMOD_ALT )
	{
		SavedPositions[1].offsetX = Hud.OffX;
		SavedPositions[1].offsetY = Hud.OffY;
	}
	else if ( keysym.sym == SDLK_F7 && keysym.mod & KMOD_ALT )
	{
		SavedPositions[2].offsetX = Hud.OffX;
		SavedPositions[2].offsetY = Hud.OffY;
	}
	else if ( keysym.sym == SDLK_F8 && keysym.mod & KMOD_ALT )
	{
		SavedPositions[3].offsetX = Hud.OffX;
		SavedPositions[3].offsetY = Hud.OffY;
	}
	else if ( keysym.sym == SDLK_F5 && SavedPositions[0].offsetX >= 0 && SavedPositions[0].offsetY >= 0 )
	{
		Hud.OffX = SavedPositions[0].offsetX;
		Hud.OffY = SavedPositions[0].offsetY;
		bFlagDrawMap = true;
		Hud.DoScroll ( 0 );
	}
	else if ( keysym.sym == SDLK_F6 && SavedPositions[1].offsetX >= 0 && SavedPositions[1].offsetY >= 0 )
	{
		Hud.OffX = SavedPositions[1].offsetX;
		Hud.OffY = SavedPositions[1].offsetY;
		bFlagDrawMap = true;
		Hud.DoScroll ( 0 );
	}
	else if ( keysym.sym == SDLK_F7 && SavedPositions[2].offsetX >= 0 && SavedPositions[2].offsetY >= 0 )
	{
		Hud.OffX = SavedPositions[2].offsetX;
		Hud.OffY = SavedPositions[2].offsetY;
		bFlagDrawMap = true;
		Hud.DoScroll ( 0 );
	}
	else if ( keysym.sym == SDLK_F8 && SavedPositions[3].offsetX >= 0 && SavedPositions[3].offsetY >= 0 )
	{
		Hud.OffX = SavedPositions[3].offsetX;
		Hud.OffY = SavedPositions[3].offsetY;
		bFlagDrawMap = true;
		Hud.DoScroll ( 0 );
	}
	// Hotkeys for the unit menues
	else if ( keysym.sym == KeysList.KeyUnitMenuAttack && SelectedVehicle && SelectedVehicle->data.can_attack && SelectedVehicle->data.shots && !bWaitForOthers )
	{
		SelectedVehicle->AttackMode = true;
		Hud.CheckScroll();
		mouseMoveCallback ( true );
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuAttack && SelectedBuilding && SelectedBuilding->data.can_attack && SelectedBuilding->data.shots && !bWaitForOthers )
	{
		SelectedBuilding->AttackMode = true;
		Hud.CheckScroll();
		mouseMoveCallback ( true );
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuBuild && SelectedVehicle && SelectedVehicle->data.can_build && !SelectedVehicle->IsBuilding && !bWaitForOthers )
	{
		if ( SelectedVehicle->ClientMoveJob ) SelectedVehicle->ClientMoveJob->release();
		SelectedVehicle->ShowBuildMenu();
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuBuild && SelectedBuilding && SelectedBuilding->data.can_build && !bWaitForOthers )
	{
		SelectedBuilding->ShowBuildMenu();
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuTransfer && SelectedVehicle && ( SelectedVehicle->data.can_transport == TRANS_METAL || SelectedVehicle->data.can_transport == TRANS_OIL || SelectedVehicle->data.can_transport == TRANS_GOLD ) && !SelectedVehicle->IsBuilding && !SelectedVehicle->IsClearing && !bWaitForOthers )
	{
		SelectedVehicle->Transfer = true;
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuTransfer && SelectedBuilding && ( SelectedBuilding->data.can_transport == TRANS_METAL || SelectedBuilding->data.can_transport == TRANS_OIL || SelectedBuilding->data.can_transport == TRANS_GOLD ) && !bWaitForOthers )
	{
		SelectedBuilding->Transfer = true;
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuAutomove && SelectedVehicle && SelectedVehicle->data.can_survey && !bWaitForOthers )
	{
		if ( SelectedVehicle->autoMJob == NULL ) SelectedVehicle->autoMJob = new cAutoMJob ( SelectedVehicle );
		else
		{
			delete SelectedVehicle->autoMJob;
			SelectedVehicle->autoMJob = NULL;
		}
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuStart && SelectedBuilding && SelectedBuilding->data.can_work && !SelectedBuilding->IsWorking && ( (SelectedBuilding->BuildList && SelectedBuilding->BuildList->Size()) || SelectedBuilding->data.can_build == BUILD_NONE ) && !bWaitForOthers )
	{
		sendWantStartWork( SelectedBuilding );
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuStop && SelectedVehicle && ( SelectedVehicle->ClientMoveJob || ( SelectedVehicle->IsBuilding && SelectedVehicle->BuildRounds ) || ( SelectedVehicle->IsClearing && SelectedVehicle->ClearingRounds ) ) && !bWaitForOthers )
	{
		if ( SelectedVehicle->ClientMoveJob ) sendWantStopMove ( SelectedVehicle->iID );
		else if ( SelectedVehicle->IsBuilding ) sendWantStopBuilding ( SelectedVehicle->iID );
		else if ( SelectedVehicle->IsClearing ) sendWantStopClear ( SelectedVehicle );
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuStop && SelectedBuilding && SelectedBuilding->IsWorking && !bWaitForOthers )
	{
		sendWantStopWork( SelectedBuilding );
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuClear && SelectedVehicle && SelectedVehicle->data.can_clear && Map->fields[SelectedVehicle->PosX+SelectedVehicle->PosY*Map->size].getRubble() && !SelectedVehicle->IsClearing && !bWaitForOthers )
	{
		sendWantStartClear ( SelectedVehicle );
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuSentry && SelectedVehicle && ( SelectedVehicle->bSentryStatus || SelectedVehicle->data.can_attack ) && !bWaitForOthers )
	{
		sendChangeSentry ( SelectedVehicle->iID, true );
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuSentry && SelectedBuilding && ( SelectedBuilding->bSentryStatus || SelectedBuilding->data.can_attack ) && !bWaitForOthers )
	{
		sendChangeSentry ( SelectedBuilding->iID, false );
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuActivate && SelectedVehicle && ( SelectedVehicle->data.can_transport == TRANS_VEHICLES || SelectedVehicle->data.can_transport == TRANS_MEN ) && !bWaitForOthers )
	{
		SelectedVehicle->showStorage();
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuActivate && SelectedBuilding && ( SelectedBuilding->data.can_load == TRANS_VEHICLES || SelectedBuilding->data.can_load == TRANS_MEN || SelectedBuilding->data.can_load == TRANS_AIR ) && !bWaitForOthers )
	{
		SelectedBuilding->ShowStorage();
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuLoad && SelectedVehicle && ( SelectedVehicle->data.can_transport == TRANS_VEHICLES || SelectedVehicle->data.can_transport == TRANS_MEN ) && !bWaitForOthers )
	{
		SelectedVehicle->LoadActive = !SelectedVehicle->LoadActive;
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuLoad && SelectedBuilding && ( SelectedBuilding->data.can_load == TRANS_VEHICLES || SelectedBuilding->data.can_load == TRANS_MEN || SelectedBuilding->data.can_load == TRANS_AIR ) && !bWaitForOthers )
	{
		SelectedBuilding->LoadActive = !SelectedBuilding->LoadActive;
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuReload && SelectedVehicle && SelectedVehicle->data.can_reload && SelectedVehicle->data.cargo >= 2 && !bWaitForOthers )
	{
		SelectedVehicle->MuniActive = true;
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuRepair && SelectedVehicle && SelectedVehicle->data.can_repair && SelectedVehicle->data.cargo >= 2 && !bWaitForOthers )
	{
		SelectedVehicle->RepairActive = true;
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuLayMine && SelectedVehicle && SelectedVehicle->data.can_lay_mines && SelectedVehicle->data.cargo > 0 && !bWaitForOthers )
	{
		SelectedVehicle->LayMines = !SelectedVehicle->LayMines;
		SelectedVehicle->ClearMines = false;
		sendMineLayerStatus( SelectedVehicle );
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuClearMine && SelectedVehicle && SelectedVehicle->data.can_lay_mines && SelectedVehicle->data.cargo < SelectedVehicle->data.max_cargo && !bWaitForOthers )
	{
		SelectedVehicle->ClearMines = !SelectedVehicle->ClearMines;
		SelectedVehicle->LayMines = false;
		sendMineLayerStatus ( SelectedVehicle );
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuDisable && SelectedVehicle && SelectedVehicle->data.is_commando && SelectedVehicle->data.shots && !bWaitForOthers )
	{
		SelectedVehicle->DisableActive = true;
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuSteal && SelectedVehicle && SelectedVehicle->data.is_commando && SelectedVehicle->data.shots && !bWaitForOthers)
	{
		SelectedVehicle->StealActive = true;
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuInfo && SelectedVehicle )
	{
		SelectedVehicle->ShowHelp();
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuInfo && SelectedBuilding )
	{
		SelectedBuilding->ShowHelp();
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuDistribute && SelectedBuilding && SelectedBuilding->data.is_mine && SelectedBuilding->IsWorking && !bWaitForOthers )
	{
		SelectedBuilding->showMineManager();
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuResearch && SelectedBuilding && SelectedBuilding->data.can_research && SelectedBuilding->IsWorking && !bWaitForOthers )
	{
		addMessage ( lngPack.i18n ( "Text~Error_Messages~INFO_Not_Implemented" ) );
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuUpgrade && SelectedBuilding && SelectedBuilding->data.gold_need && !bWaitForOthers )
	{
		addMessage ( lngPack.i18n ( "Text~Error_Messages~INFO_Not_Implemented" ) );
	}
	else if ( keysym.sym == KeysList.KeyUnitMenuDestroy && SelectedBuilding && !SelectedBuilding->data.is_road && !bWaitForOthers )
	{
		addMessage ( lngPack.i18n ( "Text~Error_Messages~INFO_Not_Implemented" ) );
	}
	// Hotkeys for the hud
	else if ( keysym.sym == KeysList.KeyFog ) Hud.SwitchNebel ( !Hud.Nebel );
	else if ( keysym.sym == KeysList.KeyGrid ) Hud.SwitchGitter ( !Hud.Gitter );
	else if ( keysym.sym == KeysList.KeyScan ) Hud.SwitchScan ( !Hud.Scan );
	else if ( keysym.sym == KeysList.KeyRange ) Hud.SwitchReichweite ( !Hud.Reichweite );
	else if ( keysym.sym == KeysList.KeyAmmo ) Hud.SwitchMunition ( !Hud.Munition );
	else if ( keysym.sym == KeysList.KeyHitpoints ) Hud.SwitchTreffer ( !Hud.Treffer );
	else if ( keysym.sym == KeysList.KeyColors ) Hud.SwitchFarben ( !Hud.Farben );
	else if ( keysym.sym == KeysList.KeyStatus ) Hud.SwitchStatus ( !Hud.Status );
	else if ( keysym.sym == KeysList.KeySurvey ) Hud.SwitchStudie ( !Hud.Studie );
}

bool cClient::selectUnit( cMapField *OverUnitField, bool base )
{
	if ( OverUnitField->getPlanes() && !OverUnitField->getPlanes()->moving )
	{
		bChangeObjectName = false;
		if ( !bChatInput ) InputHandler->setInputState ( false );
		if ( SelectedVehicle == OverUnitField->getPlanes() )
		{
			if ( SelectedVehicle->owner == ActivePlayer )
			{
				SelectedVehicle->MenuActive = true;
				PlayFX ( SoundData.SNDHudButton );
			}
		}
		else
		{
			if ( SelectedVehicle )
			{
				SelectedVehicle->Deselct();
				SelectedVehicle = NULL;
				StopFXLoop ( iObjectStream );
			}
			else if ( SelectedBuilding )
			{
				SelectedBuilding->Deselct();
				SelectedBuilding = NULL;
				StopFXLoop ( iObjectStream );
			}
			SelectedVehicle = OverUnitField->getPlanes();
			SelectedVehicle->Select();
			iObjectStream = SelectedVehicle->PlayStram();
		}
		return true;
	}
	else if ( OverUnitField->getVehicles() && !OverUnitField->getVehicles()->moving && !( OverUnitField->getPlanes() && ( OverUnitField->getVehicles()->MenuActive || OverUnitField->getVehicles()->owner != ActivePlayer ) ) )
	{
		bChangeObjectName = false;
		if ( !bChatInput ) InputHandler->setInputState ( false );
		if ( SelectedVehicle == OverUnitField->getVehicles() )
		{
			if ( SelectedVehicle->owner == ActivePlayer )
			{
				SelectedVehicle->MenuActive = true;
				PlayFX ( SoundData.SNDHudButton );
			}
		}
		else
		{
			if ( SelectedVehicle )
			{
				SelectedVehicle->Deselct();
				SelectedVehicle = NULL;
				StopFXLoop ( iObjectStream );
			}
			else if ( SelectedBuilding )
			{
				SelectedBuilding->Deselct();
				SelectedBuilding = NULL;
				StopFXLoop ( iObjectStream );
			}
			SelectedVehicle = OverUnitField->getVehicles();
			SelectedVehicle->Select();
			iObjectStream = SelectedVehicle->PlayStram();
		}
		return true;
	}
	else if ( OverUnitField->getTopBuilding() && ( base || ( ( !OverUnitField->getTopBuilding()->data.is_connector || !SelectedVehicle ) && ( !OverUnitField->getTopBuilding()->data.is_pad || ( !SelectedVehicle || SelectedVehicle->data.can_drive != DRIVE_AIR ) ) ) ) )
	{
		bChangeObjectName = false;
		if ( !bChatInput ) InputHandler->setInputState ( false );
		if ( SelectedBuilding == OverUnitField->getTopBuilding() )
		{
			if ( SelectedBuilding->owner == ActivePlayer )
			{
				SelectedBuilding->MenuActive = true;
				PlayFX ( SoundData.SNDHudButton );
			}
		}
		else
		{
			if ( SelectedVehicle )
			{
				SelectedVehicle->Deselct();
				SelectedVehicle = NULL;
				StopFXLoop ( iObjectStream );
			}
			else if ( SelectedBuilding )
			{
				SelectedBuilding->Deselct();
				SelectedBuilding = NULL;
				StopFXLoop ( iObjectStream );
			}
			SelectedBuilding = OverUnitField->getTopBuilding();
			SelectedBuilding->Select();
			iObjectStream = SelectedBuilding->PlayStram();
		}
		return true;
	}
	else if ( ( base || !SelectedVehicle )&& OverUnitField->getBaseBuilding() )
	{
		bChangeObjectName = false;
		if ( !bChatInput ) InputHandler->setInputState ( false );
		if ( SelectedBuilding == OverUnitField->getBaseBuilding() )
		{
			if ( SelectedBuilding->owner == ActivePlayer )
			{
				SelectedBuilding->MenuActive = true;
				PlayFX ( SoundData.SNDHudButton );
			}
		}
		else
		{
			if ( SelectedVehicle )
			{
				SelectedVehicle->Deselct();
				SelectedVehicle = NULL;
				StopFXLoop ( iObjectStream );
			}
			else if ( SelectedBuilding )
			{
				SelectedBuilding->Deselct();
				SelectedBuilding = NULL;
				StopFXLoop ( iObjectStream );
			}
			SelectedBuilding = OverUnitField->getBaseBuilding();
			SelectedBuilding->Select();
			iObjectStream = SelectedBuilding->PlayStram();
		}
		return true;
	}
	return false;
}

void cClient::addMoveJob(cVehicle* vehicle, int iDestOffset)
{
	if ( vehicle->bIsBeeingAttacked ) return;

	cClientMoveJob *MoveJob = new cClientMoveJob ( vehicle->PosX+vehicle->PosY*Map->size, iDestOffset, vehicle->data.can_drive == DRIVE_AIR, vehicle );
	if ( MoveJob->calcPath() )
	{
		sendMoveJob ( MoveJob );
		Log.write(" Client: Added new movejob: VehicleID: " + iToStr ( vehicle->iID ) + ", SrcX: " + iToStr ( vehicle->PosX ) + ", SrcY: " + iToStr ( vehicle->PosY ) + ", DestX: " + iToStr ( MoveJob->DestX ) + ", DestY: " + iToStr ( MoveJob->DestY ), cLog::eLOG_TYPE_NET_DEBUG);
	}
	else
	{
		if ( !vehicle || !vehicle->autoMJob ) //automoving suveyors must not tell this
		{
			if ( random(2) ) PlayVoice(VoiceData.VOINoPath1);
			else PlayVoice ( VoiceData.VOINoPath2 );
		}

		if ( MoveJob->Vehicle )
		{
			MoveJob->Vehicle->ClientMoveJob = NULL;
		}
		delete MoveJob;
	}
}

void cClient::handleTimer()
{
	//iTimer0: 50ms
	//iTimer1: 100ms
	//iTimer2: 400ms

	static unsigned int iLast = 0;
	iTimer0 = 0;
	iTimer1 = 0;
	iTimer2 = 0;
	if ( iTimerTime != iLast )
	{
		iLast = iTimerTime;
		iTimer0 = 1;
		if (   iTimerTime & 0x1 ) iTimer1 = 1;
		if ( ( iTimerTime & 0x3 ) == 3 ) iTimer2 = 1;
	}
}

void cClient::drawMap( bool bPure )
{
	int iX, iY, iPos, iZoom, iOffX, iOffY, iStartX, iStartY, iEndX, iEndY;
	struct sTerrain *terr;
	SDL_Rect dest, tmp, scr;
	iZoom = Hud.Zoom;
	float f = 64.0;
	iOffX = ( int ) ( Hud.OffX/ ( f/iZoom ) );
	iOffY = ( int ) ( Hud.OffY/ ( f/iZoom ) );
	scr.y = 0;
	scr.h = scr.w = iZoom;
	dest.y = 18-iOffY;
	float factor = (float)(Hud.Zoom/64.0);

	if ( iTimer2 ) Map->generateNextAnimationFrame();

	// draw the terrain
	for ( iY=0;iY<Map->size;iY++ )
	{
		dest.x=180-iOffX;
		if ( dest.y>=18-iZoom )
		{
			iPos=iY*Map->size;
			for ( iX=0;iX<Map->size;iX++ )
			{
				if ( dest.x>=180-iZoom )
				{
					tmp=dest;
					terr=Map->terrain+Map->Kacheln[iPos];

					// draw the fog:
					if ( Hud.Nebel&&!ActivePlayer->ScanMap[iPos] )
					{
						if ( !SettingsData.bPreScale && ( terr->shw->w != Hud.Zoom || terr->shw->h != Hud.Zoom ) ) scaleSurface ( terr->shw_org, terr->shw, Hud.Zoom, Hud.Zoom );
						SDL_BlitSurface ( terr->shw,NULL,buffer,&tmp );
					}
					else
					{
						if ( !SettingsData.bPreScale && ( terr->sf->w != Hud.Zoom || terr->sf->h != Hud.Zoom ) ) scaleSurface ( terr->sf_org, terr->sf, Hud.Zoom, Hud.Zoom );
						SDL_BlitSurface ( terr->sf,NULL,buffer,&tmp );
					}
				}
				iPos++;
				dest.x+=iZoom;
				if ( dest.x>SettingsData.iScreenW-13 ) break;
			}
		}
		dest.y+=iZoom;
		if ( dest.y>SettingsData.iScreenH-15 ) break;
	}
	// draw the grid:
	if ( Hud.Gitter )
	{
		dest.x=180;
		dest.y=18+iZoom- ( iOffY%iZoom );
		dest.w=SettingsData.iScreenW-192;
		dest.h=1;
		for ( iY=0;iY< ( SettingsData.iScreenH-32 ) /iZoom+1;iY++ )
		{
			SDL_FillRect ( buffer,&dest,GRID_COLOR );
			dest.y+=iZoom;
		}
		dest.x=180+iZoom- ( iOffX%iZoom );
		dest.y=18;
		dest.w=1;
		dest.h=SettingsData.iScreenH-32;
		for ( iX = 0; iX < ( SettingsData.iScreenW-192 ) /iZoom+1;iX++ )
		{
			SDL_FillRect ( buffer,&dest,GRID_COLOR );
			dest.x+=iZoom;
		}
	}
	if ( bPure ) return;

	// display the FX-Bottom-Effects:
	displayFXBottom();

	//draw rubble and all base buildings
	iStartX= ( Hud.OffX-1 ) /64;if ( iStartX<0 ) iStartX=0;
	iStartY= ( Hud.OffY-1 ) /64;if ( iStartY<0 ) iStartY=0;
	iStartX-=1;if ( iStartX<0 ) iStartX=0;
	iStartY-=1;if ( iStartY<0 ) iStartY=0;
	iEndX=Hud.OffX/64+ ( SettingsData.iScreenW-192 ) /Hud.Zoom+1;
	if ( iEndX>=Map->size ) iEndX=Map->size-1;
	iEndY=Hud.OffY/64+ ( SettingsData.iScreenH-32 ) /Hud.Zoom+1;
	if ( iEndY>=Map->size ) iEndY=Map->size-1;
	dest.y=18-iOffY+iZoom*iStartY;
	for ( iY=iStartY;iY<=iEndY;iY++ )
	{
		dest.x=180-iOffX+iZoom*iStartX;
		iPos=iY*Map->size+iStartX;
		for ( iX=iStartX;iX<=iEndX;iX++ )
		{
			cBuildingIterator bi = Map->fields[iPos].getBuildings();
			while ( !bi.end ) bi++;
			bi--;

			while ( !bi.rend && ( bi->data.is_base || !bi->owner ) )
			{
				if ( ActivePlayer->ScanMap[iPos]||
					( bi->data.is_big && ( ( iX < iEndX && ActivePlayer->ScanMap[iPos+1] ) || ( iY < iEndY && ActivePlayer->ScanMap[iPos+Map->size] ) || ( iX < iEndX && iY < iEndY&&ActivePlayer->ScanMap[iPos+Map->size+1] ) ) ) )
				{
					if ( bi->PosX == iX && bi->PosY == iY )
					{
						bi->Draw ( &dest );
					}
				}
				bi--;
			}
			iPos++;
			dest.x+=iZoom;
		}
		dest.y+=iZoom;
	}

	//draw top buildings (except connectors), and working vehicles
	dest.y=18-iOffY+iZoom*iStartY;
	for ( iY=iStartY;iY<=iEndY;iY++ )
	{
		dest.x=180-iOffX+iZoom*iStartX;
		iPos=iY*Map->size+iStartX;
		for ( iX=iStartX;iX<=iEndX;iX++ )
		{
			cBuilding* building = Map->fields[iPos].getBuildings();
			if ( building && !building->data.is_base && building->owner && !building->data.is_connector )
			{

				if ( ActivePlayer->ScanMap[iPos]||
						( building->data.is_big && ( ( iX < iEndX && ActivePlayer->ScanMap[iPos+1] ) || ( iY < iEndY && ActivePlayer->ScanMap[iPos+Map->size] ) || ( iX < iEndX && iY < iEndY&&ActivePlayer->ScanMap[iPos+Map->size+1] ) ) ) )
				{
					if ( building->PosX == iX && building->PosY == iY )	//make sure a big building is drawn only once
					{
						building->Draw ( &dest );

						if ( bDebugBaseClient )
						{
							sSubBase *sb;
							tmp=dest;
							if ( tmp.h>8 ) tmp.h=8;
							tmp.w = Hud.Zoom;
							if ( building->data.is_big ) tmp.w*=2;
							sb = building->SubBase;
							// the VS compiler gives a warning on casting a pointer to long.
							// therfore we will first cast to long long and then cut this to Unit32 again.
							SDL_FillRect ( buffer,&tmp, (Uint32)(long long)(sb));
							font->showText(dest.x+1,dest.y+1, iToStr( sb->iID ), FONT_LATIN_SMALL_WHITE);
							string sTmp = "m "+iToStr(sb->Metal)+"/"+iToStr(sb->MaxMetal)+" +"+iToStr(sb->MetalProd-sb->MetalNeed);
							font->showText(dest.x+1,dest.y+1+8, sTmp, FONT_LATIN_SMALL_WHITE);

							sTmp = "o "+iToStr(sb->Oil)+"/"+iToStr(sb->MaxOil)+" +"+iToStr(sb->OilProd-sb->OilNeed);
							font->showText(dest.x+1,dest.y+1+16, sTmp, FONT_LATIN_SMALL_WHITE);

							sTmp = "g "+iToStr(sb->Gold)+"/"+iToStr(sb->MaxGold)+" +"+iToStr(sb->GoldProd-sb->GoldNeed);
							font->showText(dest.x+1,dest.y+1+24, sTmp, FONT_LATIN_SMALL_WHITE);
						}
						if ( bDebugBaseServer )
						{
							sSubBase *sb;
							tmp=dest;
							if ( tmp.h>8 ) tmp.h=8;
							tmp.w = Hud.Zoom;
							if ( building->data.is_big ) tmp.w*=2;
							sb = Server->Map->fields[iPos].getBuildings()->SubBase;
							// the VS compiler gives a warning on casting a pointer to long.
							// therfore we will first cast to long long and then cut this to Unit32 again.
							SDL_FillRect ( buffer,&tmp, (Uint32)(long long)(sb) );
							font->showText(dest.x+1,dest.y+1, iToStr( sb->iID ), FONT_LATIN_SMALL_WHITE);
							string sTmp = "m "+iToStr(sb->Metal)+"/"+iToStr(sb->MaxMetal)+" +"+iToStr(sb->MetalProd-sb->MetalNeed);
							font->showText(dest.x+1,dest.y+1+8, sTmp, FONT_LATIN_SMALL_WHITE);

							sTmp = "o "+iToStr(sb->Oil)+"/"+iToStr(sb->MaxOil)+" +"+iToStr(sb->OilProd-sb->OilNeed);
							font->showText(dest.x+1,dest.y+1+16, sTmp, FONT_LATIN_SMALL_WHITE);

							sTmp = "g "+iToStr(sb->Gold)+"/"+iToStr(sb->MaxGold)+" +"+iToStr(sb->GoldProd-sb->GoldNeed);
							font->showText(dest.x+1,dest.y+1+24, sTmp, FONT_LATIN_SMALL_WHITE);
						}
					}
				}
			}
			cVehicle* vehicle = Map->fields[iPos].getVehicles();
			if ( vehicle && (vehicle->IsClearing || vehicle->IsBuilding) && ( ActivePlayer->ScanMap[iPos] || ( iX < iEndX && ActivePlayer->ScanMap[iPos+1] ) || ( iY < iEndY && ActivePlayer->ScanMap[iPos+Map->size] ) || ( iX < iEndX && iY < iEndY&&ActivePlayer->ScanMap[iPos+Map->size+1] ) ) )
			{
				if ( vehicle->PosX == iX && vehicle->PosY == iY )	//make sure a big vehicle is drawn only once
				{
					vehicle->Draw ( &dest );
				}
			}
			iPos++;
			dest.x+=iZoom;
		}
		dest.y+=iZoom;
	}

	//draw ships and bridges
	//the bridges are drawn by the vehicle->Draw() funktion
	dest.y=18-iOffY+iZoom*iStartY;
	for ( iY=iStartY;iY<=iEndY;iY++ )
	{
		dest.x=180-iOffX+iZoom*iStartX;
		iPos=iY*Map->size+iStartX;
		for ( iX=iStartX;iX<=iEndX;iX++ )
		{
			if ( ActivePlayer->ScanMap[iPos] )
			{
				cVehicle* vehicle = Map->fields[iPos].getVehicles();
				if ( vehicle && vehicle->data.can_drive == DRIVE_SEA )
				{
					vehicle->Draw ( &dest );
				}

			}
			iPos++;
			dest.x+=iZoom;
		}
		dest.y+=iZoom;
	}

	//draw vehicles
	dest.y=18-iOffY+iZoom*iStartY;
	for ( iY=iStartY;iY<=iEndY;iY++ )
	{
		dest.x=180-iOffX+iZoom*iStartX;
		iPos=iY*Map->size+iStartX;
		for ( iX=iStartX;iX<=iEndX;iX++ )
		{
			if ( ActivePlayer->ScanMap[iPos] )
			{
				cVehicle* vehicle = Map->fields[iPos].getVehicles();
				if ( vehicle && vehicle->data.can_drive != DRIVE_SEA && !vehicle->IsBuilding && !vehicle->IsClearing )
				{
					vehicle->Draw ( &dest );
				}

			}
			iPos++;
			dest.x+=iZoom;
		}
		dest.y+=iZoom;
	}

	//draw connectors
	dest.y=18-iOffY+iZoom*iStartY;
	for ( iY=iStartY;iY<=iEndY;iY++ )
	{
		dest.x=180-iOffX+iZoom*iStartX;
		iPos=iY*Map->size+iStartX;
		for ( iX=iStartX;iX<=iEndX;iX++ )
		{
			if ( ActivePlayer->ScanMap[iPos] )
			{
				cBuilding* building = Map->fields[iPos].getBuildings();
				if ( building && building->data.is_connector )
				{
					building->Draw ( &dest );
				}

			}
			iPos++;
			dest.x+=iZoom;
		}
		dest.y+=iZoom;
	}

	// draw the planes:
	dest.y=18-iOffY+iZoom*iStartY;
	scr.x=0;scr.y=0;
	scr.h=scr.w=iZoom;

	for ( iY=iStartY;iY<=iEndY;iY++ )
	{
		dest.x=180-iOffX+iZoom*iStartX;
		iPos=iY*Map->size+iStartX;
		for ( iX=iStartX;iX<=iEndX;iX++ )
		{
			if ( ActivePlayer->ScanMap[iPos] )
			{
				cVehicle* plane = Map->fields[iPos].getPlanes();
				if ( plane )
				{
					plane->Draw ( &dest );
				}
			}
			iPos++;
			dest.x+=iZoom;
		}
		dest.y+=iZoom;
	}
	// draw the resources:
	if ( Hud.Studie|| ( SelectedVehicle&&SelectedVehicle->owner==ActivePlayer&&SelectedVehicle->data.can_survey ) )
	{
		scr.y=0;
		scr.h=scr.w=iZoom;
		dest.y=18-iOffY+iZoom*iStartY;
		for ( iY=iStartY;iY<=iEndY;iY++ )
		{
			dest.x=180-iOffX+iZoom*iStartX;
			iPos=iY*Map->size+iStartX;
			for ( iX=iStartX;iX<=iEndX;iX++ )
			{
				if ( ActivePlayer->ResourceMap[iPos] )
				{
					if ( Map->Resources[iPos].typ==RES_NONE )
					{
						scr.x=0;
						tmp=dest;
						if ( !SettingsData.bPreScale && ( ResourceData.res_metal->w != ResourceData.res_metal_org->w/64*Hud.Zoom || ResourceData.res_metal->h != Hud.Zoom ) ) scaleSurface ( ResourceData.res_metal_org, ResourceData.res_metal, ResourceData.res_metal_org->w/64*Hud.Zoom, Hud.Zoom );
						SDL_BlitSurface ( ResourceData.res_metal,&scr,buffer,&tmp );
					}
					else
					{
						scr.x=Map->Resources[iPos].value*iZoom;
						tmp=dest;
						if ( Map->Resources[iPos].typ==RES_METAL )
						{
							if ( !SettingsData.bPreScale && ( ResourceData.res_metal->w != ResourceData.res_metal_org->w/64*Hud.Zoom || ResourceData.res_metal->h != Hud.Zoom ) ) scaleSurface ( ResourceData.res_metal_org, ResourceData.res_metal, ResourceData.res_metal_org->w/64*Hud.Zoom, Hud.Zoom );
							SDL_BlitSurface ( ResourceData.res_metal,&scr,buffer,&tmp );
						}
						else if ( Map->Resources[iPos].typ==RES_OIL )
						{
							if ( !SettingsData.bPreScale && ( ResourceData.res_oil->w != ResourceData.res_oil_org->w/64*Hud.Zoom || ResourceData.res_oil->h != Hud.Zoom ) ) scaleSurface ( ResourceData.res_oil_org, ResourceData.res_oil, ResourceData.res_oil_org->w/64*Hud.Zoom, Hud.Zoom );
							SDL_BlitSurface ( ResourceData.res_oil,&scr,buffer,&tmp );
						}
						else
						{
							if ( !SettingsData.bPreScale && ( ResourceData.res_gold->w != ResourceData.res_gold_org->w/64*Hud.Zoom || ResourceData.res_gold->h != Hud.Zoom ) ) scaleSurface ( ResourceData.res_gold_org, ResourceData.res_gold, ResourceData.res_gold_org->w/64*Hud.Zoom, Hud.Zoom );
							SDL_BlitSurface ( ResourceData.res_gold,&scr,buffer,&tmp );
						}
					}
				}
				iPos++;
				dest.x+=iZoom;
			}
			dest.y+=iZoom;
		}
	}
	// draw the path:
	if ( SelectedVehicle&& ( ( SelectedVehicle->ClientMoveJob&&SelectedVehicle->ClientMoveJob->bSuspended ) || SelectedVehicle->BuildPath ) )
	{
		SelectedVehicle->DrawPath();
	}
	// debug sentry:
	if ( bDebugSentry )
	{
		for ( unsigned int i = 0; i < Server->PlayerList->Size(); i++ )
		{
			cPlayer *Player = (*Server->PlayerList)[i];

			scr.y = 0;
			scr.h = scr.w = iZoom;
			dest.y = 18-iOffY+iZoom*iStartY;
			for ( iY = iStartY; iY <= iEndY; iY++ )
			{
				dest.x = 180-iOffX+iZoom*iStartX;
				iPos = iY*Map->size+iStartX;
				for ( iX = iStartX; iX <= iEndX; iX++ )
				{
					int offset = (Player->Nr - 1) * font->getFontHeight( FONT_LATIN_SMALL_YELLOW );
					if ( Player->SentriesMapAir[iPos] )
					{
						if ( Player->ScanMap[iPos] )
						{
							font->showText(dest.x+1,dest.y+1+offset, iToStr ( Player->Nr ) + " A+", FONT_LATIN_SMALL_YELLOW);
						}
						else
						{
							font->showText(dest.x+1,dest.y+1+offset, iToStr ( Player->Nr ) + " A-", FONT_LATIN_SMALL_YELLOW);
						}
					}
					if ( Player->SentriesMapGround[iPos] )
					{
						if ( Player->ScanMap[iPos] )
						{
							font->showText(dest.x+10,dest.y+1+offset, iToStr ( Player->Nr ) + " G+", FONT_LATIN_SMALL_YELLOW);
						}
						else
						{
							font->showText(dest.x+10,dest.y+1+offset, iToStr ( Player->Nr ) + " G-", FONT_LATIN_SMALL_YELLOW);
						}
					}
					iPos++;
					dest.x += iZoom;
				}
				dest.y += iZoom;
			}
		}
	}

	/*if ( 1 )
	{
		scr.y = 0;
		scr.h = scr.w = iZoom;
		dest.y = 18-iOffY+iZoom*iStartY;
		for ( iY = iStartY; iY <= iEndY; iY++ )
		{
			dest.x = 180-iOffX+iZoom*iStartX;
			iPos = iY*Map->size+iStartX;
			for ( iX = iStartX; iX <= iEndX; iX++ )
			{
				if ( Map->fields[iPos].getVehicles() )
				{
					font->showText(dest.x+1,dest.y+1, "C", FONT_LATIN_SMALL_YELLOW);
				
				}
				if ( Server->Map->fields[iPos].getVehicles() )
				{
					font->showText(dest.x+1,dest.y+10, "S", FONT_LATIN_SMALL_YELLOW);
				}
				iPos++;
				dest.x += iZoom;
			}
			dest.y += iZoom;
		}
	}*/
}

void cClient::drawMiniMap()
{

	SDL_Surface* minimapSurface = SDL_CreateRGBSurface( SDL_SWSURFACE, MINIMAP_SIZE, MINIMAP_SIZE, 32, 0, 0, 0, 0);
	Uint32* minimap = ( (Uint32*) minimapSurface->pixels );	
		
	//set zoom factor
	int zoomFactor = 1;
	if ( Hud.MinimapZoom )
		zoomFactor = MINIMAP_ZOOM_FACTOR;
	Hud.minimapZoomFactor = zoomFactor;

	//set drawing offset, to center the minimap on the screen position
	int minimapOffsetX = 0, minimapOffsetY = 0;	//position of the upper left edge on the map
	if ( zoomFactor > 1 )
	{
		int centerPosX = (int) (Hud.OffX / 64.0 + (SettingsData.iScreenW - 192.0) / (Hud.Zoom * 2));
		int centerPosY = (int) (Hud.OffY / 64.0 + (SettingsData.iScreenH -  32.0) / (Hud.Zoom * 2));
		minimapOffsetX = centerPosX - (Map->size / (zoomFactor * 2));
		minimapOffsetY = centerPosY - (Map->size / (zoomFactor * 2));

		if ( minimapOffsetX < 0 ) minimapOffsetX = 0;
		if ( minimapOffsetY < 0 ) minimapOffsetY = 0;
		if ( minimapOffsetX > Map->size - (Map->size / zoomFactor) ) minimapOffsetX = Map->size - (Map->size / zoomFactor);
		if ( minimapOffsetY > Map->size - (Map->size / zoomFactor) ) minimapOffsetY = Map->size - (Map->size / zoomFactor);
	}
	Hud.minimapOffsetX = minimapOffsetX;
	Hud.minimapOffsetY = minimapOffsetY;

	//draw the landscape
	for ( int miniMapX = 0; miniMapX < MINIMAP_SIZE; miniMapX++ )
	{
		//calculate the field on the map	
		int terrainx = (miniMapX * Map->size) / (MINIMAP_SIZE * zoomFactor) + minimapOffsetX;
		if ( terrainx >= Map->size ) terrainx = Map->size - 1;

		//calculate the position within the terrain graphic (for better rendering of maps < 112)
		int offsetx  = ((miniMapX * Map->size ) % (MINIMAP_SIZE * zoomFactor)) * 64 / (MINIMAP_SIZE * zoomFactor);

		for ( int miniMapY = 0; miniMapY < MINIMAP_SIZE; miniMapY++ )
		{
			int terrainy =  (miniMapY * Map->size) / (MINIMAP_SIZE * zoomFactor) + minimapOffsetY;
			if ( terrainy >= Map->size ) terrainy = Map->size - 1;
			int offsety  = ((miniMapY * Map->size ) % (MINIMAP_SIZE * zoomFactor)) * 64 / (MINIMAP_SIZE * zoomFactor);

			SDL_Color sdlcolor;
			Uint8* terrainPixels = (Uint8*) Map->terrain[Map->Kacheln[terrainx+terrainy*Map->size]].sf_org->pixels;
			Uint8 index = terrainPixels[offsetx + offsety*64];
			sdlcolor = Map->terrain[Map->Kacheln[terrainx+terrainy*Map->size]].sf_org->format->palette->colors[index];
			Uint32 color = (sdlcolor.r << 16) + (sdlcolor.g << 8) + sdlcolor.b;

			minimap[miniMapX+miniMapY*MINIMAP_SIZE] = color;
		}
	}

	//draw the fog
	for ( int miniMapX = 0; miniMapX < MINIMAP_SIZE; miniMapX++ )
	{
		int terrainx = (miniMapX * Map->size) / (MINIMAP_SIZE * zoomFactor) + minimapOffsetX;
		for ( int miniMapY = 0; miniMapY < MINIMAP_SIZE; miniMapY++ )
		{
			int terrainy = (miniMapY * Map->size) / (MINIMAP_SIZE * zoomFactor) + minimapOffsetY;

			if ( !ActivePlayer->ScanMap[terrainx + terrainy * Map->size] )
			{
				Uint8* color = (Uint8*) &minimap[miniMapX+miniMapY*MINIMAP_SIZE];
				color[0] = (Uint8) ( color[0] * 0.6 );
				color[1] = (Uint8) ( color[1] * 0.6 );
				color[2] = (Uint8) ( color[2] * 0.6 );
			}
		}
	}

	//draw the units
	//here we go through each map field instead of through each minimap pixel,
	//to make sure, that every unit is diplayed and has the same size on the minimap.
	
	//the size of the rect, that is drawn for each unit
	int size = (int) ceil((float) MINIMAP_SIZE * zoomFactor / Map->size);
	if ( size < 2 ) size = 2;
	SDL_Rect rect;	
	rect.h = size;
	rect.w = size;
			
	for ( int mapx = 0; mapx < Map->size; mapx++ )
	{
		rect.x = ( (mapx - minimapOffsetX) * MINIMAP_SIZE * zoomFactor ) / Map->size;
		if ( rect.x < 0 || rect.x >= MINIMAP_SIZE ) continue; 
		for ( int mapy = 0; mapy < Map->size; mapy++ )
		{
			rect.y = ( (mapy - minimapOffsetY) * MINIMAP_SIZE * zoomFactor ) / Map->size;
			if ( rect.y < 0 || rect.y >= MINIMAP_SIZE ) continue;

			if ( !ActivePlayer->ScanMap[mapx + mapy * Map->size] ) continue;

			cMapField& field = (*Map)[mapx + mapy * Map->size];

			//draw building
			cBuilding* building = field.getBuildings();
			if ( building && building->owner )
			{
				if ( !Hud.TNT || building->data.can_attack )
				{
					unsigned int color = *( (unsigned int*) building->owner->color->pixels );
					SDL_FillRect( minimapSurface, &rect, color);
				}
			}

			//draw vehicle
			cVehicle* vehicle = field.getVehicles();
			if ( vehicle )
			{
				if ( !Hud.TNT || vehicle->data.can_attack )
				{
					unsigned int color = *( (unsigned int*) vehicle->owner->color->pixels );
					SDL_FillRect( minimapSurface, &rect, color);
				}
			}

			//draw plane
			vehicle = field.getPlanes();
			if ( vehicle )
			{
				if ( !Hud.TNT || vehicle->data.can_attack )
				{
					unsigned int color = *( (unsigned int*) vehicle->owner->color->pixels );
					SDL_FillRect( minimapSurface, &rect, color);
				}
			}
		}
	}


	//draw the screen borders
	int startx, starty, endx, endy;
	startx = (int) ((((Hud.OffX / 64.0) - minimapOffsetX) * MINIMAP_SIZE * zoomFactor) / Map->size);
	starty = (int) ((((Hud.OffY / 64.0) - minimapOffsetY) * MINIMAP_SIZE * zoomFactor) / Map->size);
	endx = (int) ( startx + ((SettingsData.iScreenW - 192.0) * MINIMAP_SIZE * zoomFactor) / (Map->size * Hud.Zoom));
	endy = (int) ( starty + ((SettingsData.iScreenH -  32.0) * MINIMAP_SIZE * zoomFactor) / (Map->size * Hud.Zoom));

	if ( endx == MINIMAP_SIZE ) endx = MINIMAP_SIZE - 1; //workaround
	if ( endy == MINIMAP_SIZE ) endy = MINIMAP_SIZE - 1;

	for ( int y = starty; y <= endy; y++ )
	{
		if ( y < 0 || y >= MINIMAP_SIZE ) continue;
		if ( startx >= 0 && startx < MINIMAP_SIZE )
		{
			minimap[y*MINIMAP_SIZE + startx] = MINIMAP_COLOR;
		}
		if ( endx >= 0 && endx < MINIMAP_SIZE )
		{
			minimap[y*MINIMAP_SIZE + endx] = MINIMAP_COLOR;
		}
	}
	for ( int x = startx; x <= endx; x++ )
	{
		if ( x < 0 || x >= MINIMAP_SIZE ) continue;
		if ( starty >= 0 && starty < MINIMAP_SIZE )
		{
			minimap[starty * MINIMAP_SIZE + x] = MINIMAP_COLOR;
		}
		if ( endy >= 0 && endy < MINIMAP_SIZE )
		{
			minimap[endy * MINIMAP_SIZE + x] = MINIMAP_COLOR;
		}
	}
	
	//ready. blitt the map to screen
	SDL_Rect minimapPos = { MINIMAP_POS_X, MINIMAP_POS_Y, 0, 0 };
	SDL_BlitSurface(minimapSurface, NULL, GraphicsData.gfx_hud, &minimapPos );
	SDL_FreeSurface( minimapSurface );	
}

void cClient::drawFLC()
{
	SDL_Rect dest;
	string stmp;
	if ( SelectedVehicle == NULL && SelectedBuilding == NULL ) return;
	// draw the video:
	dest.x=10;
	dest.y=29;
	dest.w=128;
	dest.h=128;
	if ( FLC )
	{
		SDL_BlitSurface ( FLC->surface, NULL, buffer, &dest );
	}
	else if ( video )
	{
		SDL_BlitSurface ( video, NULL, buffer, &dest );
	}
	// display the name of the unit:
	if ( SelectedVehicle )
	{
		if ( bChangeObjectName )
		{
			dest.y+=2;
			dest.h=6;
			SDL_FillRect ( buffer,&dest,0x404040 );
			font->showText(10, 32, InputHandler->getInputStr(), FONT_LATIN_SMALL_GREEN);
		}
		else
		{
			font->showText(10, 32, SelectedVehicle->name, FONT_LATIN_SMALL_GREEN);
		}
		font->showText(10, 40, SelectedVehicle->GetStatusStr(), FONT_LATIN_SMALL_WHITE);
	}
	else if ( SelectedBuilding )
	{
		if ( bChangeObjectName )
		{
			dest.y+=2;
			dest.h=6;
			SDL_FillRect ( buffer,&dest,0x404040 );
			font->showText(10, 32, InputHandler->getInputStr(), FONT_LATIN_SMALL_GREEN);
		}
		else
		{
			font->showText(10, 32, SelectedBuilding->name, FONT_LATIN_SMALL_GREEN);
		}
		font->showText(10, 40, SelectedBuilding->GetStatusStr(), FONT_LATIN_SMALL_WHITE);
	}
}

void cClient::displayFX()
{
	if (!FXList.Size()) return;

	for (int i = (int)FXList.Size() - 1; i >= 0; i--)
	{
		drawFX ( i );
	}
}

void cClient::displayFXBottom()
{
	if (!FXListBottom.Size()) return;

	for (int i = (int)FXListBottom.Size() - 1; i >= 0; i--)
	{
		drawFXBottom ( i );
	}
}

void cClient::runFX()
{
	for ( unsigned int i = 0; i < FXList.Size(); i++ )
	{
		sFX* fx = FXList[i];
		switch ( fx->typ )
		{
			case fxRocket:
				{
					sFXRocketInfos *ri= fx->rocketInfo;
					if ( abs ( fx->PosX - ri->DestX ) <64&&abs ( fx->PosY-ri->DestY ) <64 )
					{
						ri->aj->state = cClientAttackJob::FINISHED;
						delete fx;
						FXList.Delete ( i );
						return;
					}
				
					for ( int k=0; k < 64; k += 8 )
					{
						if ( SettingsData.bAlphaEffects )
						{
							addFX ( fxSmoke, ( int ) ri->fpx, ( int ) ri->fpy,0 );
							if (!isInMenu) drawFX((int)FXList.Size() - 1);
						}
						ri->fpx+=ri->mx*8;
						ri->fpy-=ri->my*8;
					}

					fx->PosX= ( int ) ri->fpx;
					fx->PosY= ( int ) ri->fpy;
				}
				break;
			default:
				break;
		}
	}

	for ( unsigned int i = 0; i < FXListBottom.Size(); i++ )
	{
		sFX* fx = FXListBottom[i];
		switch ( fx->typ )
		{
			case fxTorpedo:
			{
				sFXRocketInfos *ri= fx->rocketInfo;
				if ( abs ( fx->PosX-ri->DestX ) <64&&abs ( fx->PosY-ri->DestY ) <64 )
				{
					ri->aj->state = cClientAttackJob::FINISHED;
					delete fx;
					FXListBottom.Delete ( i );
					return;
				}

				for ( int k=0; k < 64; k += 8 )
				{
					if ( SettingsData.bAlphaEffects )
					{
						addFX ( fxBubbles, ( int ) ri->fpx, ( int ) ri->fpy,0 );
						if (!isInMenu) drawFXBottom((int)FXListBottom.Size() - 1);
					}
					ri->fpx+=ri->mx*8;
					ri->fpy-=ri->my*8;
				}

				fx->PosX= ( int ) ( ri->fpx );
				fx->PosY= ( int ) ( ri->fpy );
			}
			default:
				break;
		}
			
	}
}

void cClient::drawFX( int iNum )
{
	SDL_Rect scr,dest;
	int width, height;
	sFX *fx;

	fx = FXList[iNum];
	if ( !ActivePlayer->ScanMap[fx->PosX/64+fx->PosY/64*Map->size] && fx->typ != fxRocket ) return;

	switch ( fx->typ )
	{
		case fxMuzzleBig:
			if ( !EffectsData.fx_muzzle_big ) break;
			width = (EffectsData.fx_muzzle_big[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_muzzle_big[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_muzzle_big[1]->w != width || EffectsData.fx_muzzle_big[1]->h != height ) ) scaleSurface ( EffectsData.fx_muzzle_big[0], EffectsData.fx_muzzle_big[1], width, height );
			if ( iFrame - fx->StartFrame > 2 )
			{
				delete fx;
				FXList.Delete ( iNum );
				return;
			}
			scr.x=Hud.Zoom*fx->param;
			scr.y=0;
			scr.w=Hud.Zoom;
			scr.h=Hud.Zoom;
			dest.x=180- ( ( int ) ( ( Hud.OffX-fx->PosX ) / ( 64.0/Hud.Zoom ) ) );
			dest.y=18- ( ( int ) ( ( Hud.OffY-fx->PosY ) / ( 64.0/Hud.Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_muzzle_big[1],&scr,buffer,&dest );
			break;
		case fxMuzzleSmall:
			if ( !EffectsData.fx_muzzle_small ) break;
			width = (EffectsData.fx_muzzle_small[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_muzzle_small[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_muzzle_small[1]->w != width || EffectsData.fx_muzzle_small[1]->h != height ) ) scaleSurface ( EffectsData.fx_muzzle_small[0], EffectsData.fx_muzzle_small[1], width, height );
			if ( iFrame - fx->StartFrame > 2 )
			{
				delete fx;
				FXList.Delete ( iNum );
				return;
			}
			scr.x=Hud.Zoom*fx->param;
			scr.y=0;
			scr.w=Hud.Zoom;
			scr.h=Hud.Zoom;
			dest.x=180- ( ( int ) ( ( Hud.OffX-fx->PosX ) / ( 64.0/Hud.Zoom ) ) );
			dest.y=18- ( ( int ) ( ( Hud.OffY-fx->PosY ) / ( 64.0/Hud.Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_muzzle_small[1],&scr,buffer,&dest );
			break;
		case fxMuzzleMed:
			if ( !EffectsData.fx_muzzle_med ) break;
			width = (EffectsData.fx_muzzle_med[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_muzzle_med[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_muzzle_med[1]->w != width || EffectsData.fx_muzzle_med[1]->h != height ) ) scaleSurface ( EffectsData.fx_muzzle_med[0], EffectsData.fx_muzzle_med[1], width, height );
			if ( iFrame - fx->StartFrame > 2 )
			{
				delete fx;
				FXList.Delete ( iNum );
				return;
			}
			scr.x=Hud.Zoom*fx->param;
			scr.y=0;
			scr.w=Hud.Zoom;
			scr.h=Hud.Zoom;
			dest.x=180- ( ( int ) ( ( Hud.OffX-fx->PosX ) / ( 64.0/Hud.Zoom ) ) );
			dest.y=18- ( ( int ) ( ( Hud.OffY-fx->PosY ) / ( 64.0/Hud.Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_muzzle_med[1],&scr,buffer,&dest );
			break;
		case fxMuzzleMedLong:
			if ( !EffectsData.fx_muzzle_med ) break;
			width = (EffectsData.fx_muzzle_med[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_muzzle_med[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_muzzle_med[1]->w != width || EffectsData.fx_muzzle_med[1]->h != height ) ) scaleSurface ( EffectsData.fx_muzzle_med[0], EffectsData.fx_muzzle_med[1], width, height );
			if ( iFrame - fx->StartFrame > 5 )
			{
				delete fx;
				FXList.Delete ( iNum );
				return;
			}
			scr.x=Hud.Zoom*fx->param;
			scr.y=0;
			scr.w=Hud.Zoom;
			scr.h=Hud.Zoom;
			dest.x=180- ( ( int ) ( ( Hud.OffX-fx->PosX ) / ( 64.0/Hud.Zoom ) ) );
			dest.y=18- ( ( int ) ( ( Hud.OffY-fx->PosY ) / ( 64.0/Hud.Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_muzzle_med[1],&scr,buffer,&dest );
			break;
		case fxHit:
			if ( !EffectsData.fx_hit ) break;
			width = (EffectsData.fx_hit[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_hit[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_hit[1]->w != width || EffectsData.fx_hit[1]->h != height ) ) scaleSurface ( EffectsData.fx_hit[0], EffectsData.fx_hit[1], width, height );
			if ( iFrame - fx->StartFrame > 5 )
			{
				delete fx;
				FXList.Delete ( iNum );
				return;
			}
			scr.x=Hud.Zoom* ( iFrame-fx->StartFrame );
			scr.y=0;
			scr.w=Hud.Zoom;
			scr.h=Hud.Zoom;
			dest.x=180- ( ( int ) ( ( Hud.OffX-fx->PosX ) / ( 64.0/Hud.Zoom ) ) );
			dest.y=18- ( ( int ) ( ( Hud.OffY-fx->PosY ) / ( 64.0/Hud.Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_hit[1],&scr,buffer,&dest );
			break;
		case fxExploSmall:
			if ( !EffectsData.fx_explo_small ) break;
			width = (EffectsData.fx_explo_small[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_explo_small[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_explo_small[1]->w != width || EffectsData.fx_explo_small[1]->h != height ) ) scaleSurface ( EffectsData.fx_explo_small[0], EffectsData.fx_explo_small[1], width, height );
			if ( iFrame - fx->StartFrame > 14 )
			{
				delete fx;
				FXList.Delete ( iNum );
				return;
			}
			scr.x = (int) (Hud.Zoom * 114 * ( iFrame - fx->StartFrame ) / 64.0);
			scr.y = 0;
			scr.w = (int) (Hud.Zoom * 114 / 64.0);
			scr.h = (int) (Hud.Zoom * 108 / 64.0);
			dest.x = 180 - ( (int) ( ( Hud.OffX- ( fx->PosX - 57 ) ) / ( 64.0/Hud.Zoom ) ) );
			dest.y = 18 -  ( (int) ( ( Hud.OffY- ( fx->PosY - 54 ) ) / ( 64.0/Hud.Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_explo_small[1], &scr, buffer, &dest );
			break;
		case fxExploBig:
			if ( !EffectsData.fx_explo_big ) break;
			width = (EffectsData.fx_explo_big[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_explo_big[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_explo_big[1]->w != width || EffectsData.fx_explo_big[1]->h != height ) ) scaleSurface ( EffectsData.fx_explo_big[0], EffectsData.fx_explo_big[1], width, height );
			if ( iFrame - fx->StartFrame > 28 )
			{
				delete fx;
				FXList.Delete ( iNum );
				return;
			}
			scr.x = (int) (Hud.Zoom * 307 * ( iFrame - fx->StartFrame ) / 64.0);
			scr.y = 0;
			scr.w = (int) (Hud.Zoom * 307 / 64.0);
			scr.h = (int) (Hud.Zoom * 194 / 64.0);
			dest.x = 180- ( (int) ( ( Hud.OffX- ( fx->PosX - 134 ) ) / ( 64.0/Hud.Zoom ) ) );
			dest.y = 18-  ( (int) ( ( Hud.OffY- ( fx->PosY - 85 ) ) / ( 64.0/Hud.Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_explo_big[1], &scr, buffer, &dest );
			break;
		case fxExploWater:
			if ( !EffectsData.fx_explo_water ) break;
			width = (EffectsData.fx_explo_water[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_explo_water[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_explo_water[1]->w != width || EffectsData.fx_explo_water[1]->h != height ) ) scaleSurface ( EffectsData.fx_explo_water[0], EffectsData.fx_explo_water[1], width, height );
			if ( iFrame - fx->StartFrame > 14 )
			{
				delete fx;
				FXList.Delete ( iNum );
				return;
			}
			scr.x = (int) (Hud.Zoom * 114 * ( iFrame - fx->StartFrame ) / 64.0);
			scr.y = 0;
			scr.w = (int) (Hud.Zoom * 114 / 64.0);
			scr.h = (int) (Hud.Zoom * 108 / 64.0);
			dest.x = 180- ( (int) ( ( Hud.OffX- ( fx->PosX - 57 ) ) / ( 64.0/Hud.Zoom ) ) );
			dest.y = 18-  ( (int) ( ( Hud.OffY- ( fx->PosY - 54 ) ) / ( 64.0/Hud.Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_explo_water[1],&scr,buffer,&dest );
			break;
		case fxExploAir:
			if ( !EffectsData.fx_explo_air ) break;
			width = (EffectsData.fx_explo_air[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_explo_air[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_explo_air[1]->w != width || EffectsData.fx_explo_air[1]->h != height ) ) scaleSurface ( EffectsData.fx_explo_air[0], EffectsData.fx_explo_air[1], width, height );
			if ( iFrame - fx->StartFrame > 14 )
			{
				delete fx;
				FXList.Delete ( iNum );
				return;
			}
			scr.x = (int) (Hud.Zoom * 137 * ( iFrame - fx->StartFrame ) / 64.0);
			scr.y = 0;
			scr.w = (int) (Hud.Zoom * 137 / 64.0);
			scr.h = (int) (Hud.Zoom * 121 / 64.0);
			dest.x = 180- ( ( int ) ( ( Hud.OffX- ( fx->PosX - 61 ) ) / ( 64.0/Hud.Zoom ) ) );
			dest.y = 18-  ( ( int ) ( ( Hud.OffY- ( fx->PosY - 68 ) ) / ( 64.0/Hud.Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_explo_air[1],&scr,buffer,&dest );
			break;
		case fxSmoke:
			if ( !EffectsData.fx_smoke ) break;
			width = (EffectsData.fx_smoke[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_smoke[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_smoke[1]->w != width || EffectsData.fx_smoke[1]->h != height ) ) scaleSurface ( EffectsData.fx_smoke[0], EffectsData.fx_smoke[1], width, height );
			if ( iFrame-fx->StartFrame>100/4 )
			{
				delete fx;
				FXList.Delete ( iNum );
				return;
			}
			SDL_SetAlpha ( EffectsData.fx_smoke[1],SDL_SRCALPHA,100- ( iFrame-fx->StartFrame ) *4 );
			scr.y=scr.x=0;
			scr.w=EffectsData.fx_smoke[1]->h;
			scr.h=EffectsData.fx_smoke[1]->h;
			dest.x=180- ( ( int ) ( ( Hud.OffX- ( fx->PosX-EffectsData.fx_smoke[0]->h/2+32 ) ) / ( 64.0/Hud.Zoom ) ) );
			dest.y=18- ( ( int ) ( ( Hud.OffY- ( fx->PosY-EffectsData.fx_smoke[0]->h/2+32 ) ) / ( 64.0/Hud.Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_smoke[1],&scr,buffer,&dest );
			break;
		case fxRocket:
		{
			if ( !EffectsData.fx_rocket ) break;
			width = (EffectsData.fx_rocket[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_rocket[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_rocket[1]->w != width || EffectsData.fx_rocket[1]->h != height ) ) scaleSurface ( EffectsData.fx_rocket[0], EffectsData.fx_rocket[1], width, height );
			sFXRocketInfos *ri;
			ri= fx->rocketInfo;
			
			scr.x=ri->dir*EffectsData.fx_rocket[1]->h;
			scr.y=0;
			scr.h=scr.w=EffectsData.fx_rocket[1]->h;
			dest.x=180- ( ( int ) ( ( Hud.OffX- ( fx->PosX-EffectsData.fx_rocket[0]->h/2+32 ) ) / ( 64.0/Hud.Zoom ) ) );
			dest.y=18- ( ( int ) ( ( Hud.OffY- ( fx->PosY-EffectsData.fx_rocket[0]->h/2+32 ) ) / ( 64.0/Hud.Zoom ) ) );
			
			if ( ActivePlayer->ScanMap[fx->PosX/64+fx->PosY/64*Map->size] )
				SDL_BlitSurface ( EffectsData.fx_rocket[1],&scr,buffer,&dest );

			break;
		}
		case fxDarkSmoke:
		{
			if ( !EffectsData.fx_dark_smoke ) break;
			width = (EffectsData.fx_dark_smoke[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_dark_smoke[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_dark_smoke[1]->w != width || EffectsData.fx_dark_smoke[1]->h != height ) ) scaleSurface ( EffectsData.fx_dark_smoke[0], EffectsData.fx_dark_smoke[1], width, height );
			sFXDarkSmoke *dsi;
			dsi = fx->smokeInfo;
			if ( iFrame-fx->StartFrame>50||dsi->alpha<=1 )
			{
				delete fx;
				FXList.Delete ( iNum );
				return;
			}
			scr.x= ( int ) ( 0.375*Hud.Zoom ) * ( iFrame-fx->StartFrame );
			scr.y=0;
			scr.w=EffectsData.fx_dark_smoke[1]->h;
			scr.h=EffectsData.fx_dark_smoke[1]->h;
			dest.x=180- ( ( int ) ( ( Hud.OffX- ( ( int ) dsi->fx ) ) / ( 64.0/Hud.Zoom ) ) );
			dest.y=18- ( ( int ) ( ( Hud.OffY- ( ( int ) dsi->fy ) ) / ( 64.0/Hud.Zoom ) ) );

			SDL_SetAlpha ( EffectsData.fx_dark_smoke[1],SDL_SRCALPHA,dsi->alpha );
			SDL_BlitSurface ( EffectsData.fx_dark_smoke[1],&scr,buffer,&dest );

			if ( iTimer0 )
			{
				dsi->fx+=dsi->dx;
				dsi->fy+=dsi->dy;
				dsi->alpha-=3;
				if ( dsi->alpha<=0 ) dsi->alpha=1;
			}
			break;
		}
		case fxAbsorb:
		{
			if ( !EffectsData.fx_absorb ) break;
			width = (EffectsData.fx_absorb[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_absorb[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_absorb[1]->w != width || EffectsData.fx_absorb[1]->h != height ) ) scaleSurface ( EffectsData.fx_absorb[0], EffectsData.fx_absorb[1], width, height );
			if ( iFrame-fx->StartFrame>10 )
			{
				delete fx;
				FXList.Delete ( iNum );
				return;
			}
			scr.x=Hud.Zoom* ( iFrame-fx->StartFrame );
			scr.y=0;
			scr.w=Hud.Zoom;
			scr.h=Hud.Zoom;
			dest.x=180- ( ( int ) ( ( Hud.OffX-fx->PosX ) / ( 64.0/Hud.Zoom ) ) );
			dest.y=18- ( ( int ) ( ( Hud.OffY-fx->PosY ) / ( 64.0/Hud.Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_absorb[1],&scr,buffer,&dest );
			break;
		}
	}
}

void cClient::drawFXBottom( int iNum )
{
	SDL_Rect scr,dest;
	int width, height;
	sFX *fx;

	fx = FXListBottom[iNum];
	if ( ( !ActivePlayer->ScanMap[fx->PosX/64+fx->PosY/64*Map->size] ) &&fx->typ!=fxTorpedo ) return;
	switch ( fx->typ )
	{
		case fxTorpedo:
		{
			width = (EffectsData.fx_rocket[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_rocket[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_rocket[1]->w != width || EffectsData.fx_rocket[1]->h != height ) ) scaleSurface ( EffectsData.fx_rocket[0], EffectsData.fx_rocket[1], width, height );
			sFXRocketInfos *ri;
			ri = fx->rocketInfo;
			
			scr.x=ri->dir*EffectsData.fx_rocket[1]->h;
			scr.y=0;
			scr.h=scr.w=EffectsData.fx_rocket[1]->h;
			dest.x=180- ( ( int ) ( ( Hud.OffX- ( fx->PosX-EffectsData.fx_rocket[0]->h/2+32 ) ) / ( 64.0/Hud.Zoom ) ) );
			dest.y=18- ( ( int ) ( ( Hud.OffY- ( fx->PosY-EffectsData.fx_rocket[0]->h/2+32 ) ) / ( 64.0/Hud.Zoom ) ) );
			
			if ( ActivePlayer->ScanMap[fx->PosX/64+fx->PosY/64*Map->size] )
			{
				SDL_BlitSurface ( EffectsData.fx_rocket[1],&scr,buffer,&dest );
			}
			break;
		}
		case fxTracks:
		{
			width = (EffectsData.fx_tracks[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_tracks[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_tracks[1]->w != width || EffectsData.fx_tracks[1]->h != height ) ) scaleSurface ( EffectsData.fx_tracks[0], EffectsData.fx_tracks[1], width, height );
			sFXTracks *tri;
			tri = fx->trackInfo;
			if ( tri->alpha<=1 )
			{
				delete fx;
				FXListBottom.Delete ( iNum );
				return;
			}
			scr.y=0;
			scr.w=scr.h=EffectsData.fx_tracks[1]->h;
			scr.x=tri->dir*scr.w;
			dest.x=180- ( ( int ) ( ( Hud.OffX- ( fx->PosX ) ) / ( 64.0/Hud.Zoom ) ) );
			dest.y=18- ( ( int ) ( ( Hud.OffY- ( fx->PosY ) ) / ( 64.0/Hud.Zoom ) ) );
			SDL_SetAlpha ( EffectsData.fx_tracks[1],SDL_SRCALPHA,tri->alpha );
			SDL_BlitSurface ( EffectsData.fx_tracks[1],&scr,buffer,&dest );

			if ( iTimer0 )
			{
				tri->alpha--;
			}
			break;
		}
		case fxBubbles:
			width = (EffectsData.fx_smoke[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_smoke[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_smoke[1]->w != width || EffectsData.fx_smoke[1]->h != height ) ) scaleSurface ( EffectsData.fx_smoke[0], EffectsData.fx_smoke[1], width, height );
			if ( iFrame-fx->StartFrame>100/4 )
			{
				delete fx;
				FXListBottom.Delete ( iNum );
				return;
			}
			SDL_SetAlpha ( EffectsData.fx_smoke[1],SDL_SRCALPHA,100- ( iFrame-fx->StartFrame ) *4 );
			scr.y=scr.x=0;
			scr.w=EffectsData.fx_smoke[1]->h;
			scr.h=EffectsData.fx_smoke[1]->h;
			dest.x=180- ( ( int ) ( ( Hud.OffX- ( fx->PosX-EffectsData.fx_smoke[0]->h/2+32 ) ) / ( 64.0/Hud.Zoom ) ) );
			dest.y=18- ( ( int ) ( ( Hud.OffY- ( fx->PosY-EffectsData.fx_smoke[0]->h/2+32 ) ) / ( 64.0/Hud.Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_smoke[1],&scr,buffer,&dest );
			break;
		case fxCorpse:
			width = (EffectsData.fx_corpse[0]->w*Hud.Zoom)/64;
			height = (EffectsData.fx_corpse[0]->h*Hud.Zoom)/64;
			if ( !SettingsData.bPreScale && ( EffectsData.fx_corpse[1]->w != width || EffectsData.fx_corpse[1]->h != height ) ) scaleSurface ( EffectsData.fx_corpse[0], EffectsData.fx_corpse[1], width, height );
			SDL_SetAlpha ( EffectsData.fx_corpse[1],SDL_SRCALPHA,fx->param-- );
			scr.y=scr.x=0;
			scr.w=EffectsData.fx_corpse[1]->h;
			scr.h=EffectsData.fx_corpse[1]->h;
			dest.x=180- ( ( int ) ( ( Hud.OffX-fx->PosX ) / ( 64.0/Hud.Zoom ) ) );
			dest.y=18- ( ( int ) ( ( Hud.OffY-fx->PosY ) / ( 64.0/Hud.Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_corpse[1],&scr,buffer,&dest );

			if ( fx->param<=0 )
			{
				delete fx;
				FXListBottom.Delete ( iNum );
				return;
			}
			break;
	}
}



void cClient::drawExitPoint( int iX, int iY )
{
	SDL_Rect dest, scr;
	int iNr;
	int iZoom;
	iNr = iFrame%5;
	iZoom = Hud.Zoom;
	scr.y = 0;
	scr.h = scr.w = iZoom;
	scr.x = iZoom*iNr;
	dest.y = iY;
	dest.x = iX;
	if ( !SettingsData.bPreScale && ( GraphicsData.gfx_exitpoints->w != GraphicsData.gfx_exitpoints_org->w/64*iZoom || GraphicsData.gfx_exitpoints->h != iZoom ) ) scaleSurface ( GraphicsData.gfx_exitpoints_org, GraphicsData.gfx_exitpoints, GraphicsData.gfx_exitpoints_org->w/64*iZoom, iZoom );
	SDL_BlitSurface ( GraphicsData.gfx_exitpoints, &scr, buffer, &dest );
}

void cClient::drawUnitCircles ()
{
	if ( SelectedVehicle )
	{
		cVehicle& v   = *SelectedVehicle; // XXX not const is suspicious
		int const spx = v.GetScreenPosX();
		int const spy = v.GetScreenPosY();
		if (Hud.Scan)
		{
			if ( v.data.is_big )
			{
				drawCircle ( spx+ Hud.Zoom, spy + Hud.Zoom, v.data.scan * Hud.Zoom, SCAN_COLOR, buffer );
			}
			else
			{
				drawCircle ( spx + Hud.Zoom/2, spy + Hud.Zoom/2, v.data.scan * Hud.Zoom, SCAN_COLOR, buffer );
			}
		}
		if (Hud.Reichweite)
		{
			switch (v.data.can_attack)
			{
				case ATTACK_LAND:
				case ATTACK_SUB_LAND:
				case ATTACK_AIRnLAND:
					drawCircle(spx + Hud.Zoom / 2, spy + Hud.Zoom / 2, v.data.range * Hud.Zoom + 1, RANGE_GROUND_COLOR, buffer);
					break;

				case ATTACK_AIR:
					drawCircle(spx + Hud.Zoom / 2, spy + Hud.Zoom / 2, v.data.range * Hud.Zoom + 2, RANGE_AIR_COLOR, buffer);
					break;
			}
		}
		if (v.owner == ActivePlayer &&
				(
					v.IsBuilding && v.BuildRounds    == 0 ||
					v.IsClearing && v.ClearingRounds == 0
					) && !v.BuildPath )
		{
			if ( v.data.is_big )
			{
				if ( Map->possiblePlace(&v, v.PosX - 1, v.PosY - 1)) drawExitPoint(spx - Hud.Zoom,     spy - Hud.Zoom);
				if ( Map->possiblePlace(&v, v.PosX    , v.PosY - 1)) drawExitPoint(spx,                spy - Hud.Zoom);
				if ( Map->possiblePlace(&v, v.PosX + 1, v.PosY - 1)) drawExitPoint(spx + Hud.Zoom,     spy - Hud.Zoom);
				if ( Map->possiblePlace(&v, v.PosX + 2, v.PosY - 1)) drawExitPoint(spx + Hud.Zoom * 2, spy - Hud.Zoom);
				if ( Map->possiblePlace(&v, v.PosX - 1, v.PosY    )) drawExitPoint(spx - Hud.Zoom,     spy);
				if ( Map->possiblePlace(&v, v.PosX + 2, v.PosY    )) drawExitPoint(spx + Hud.Zoom * 2, spy);
				if ( Map->possiblePlace(&v, v.PosX - 1, v.PosY + 1)) drawExitPoint(spx - Hud.Zoom,     spy + Hud.Zoom);
				if ( Map->possiblePlace(&v, v.PosX + 2, v.PosY + 1)) drawExitPoint(spx + Hud.Zoom * 2, spy + Hud.Zoom);
				if ( Map->possiblePlace(&v, v.PosX - 1, v.PosY + 2)) drawExitPoint(spx - Hud.Zoom,     spy + Hud.Zoom * 2);
				if ( Map->possiblePlace(&v, v.PosX    , v.PosY + 2)) drawExitPoint(spx,                spy + Hud.Zoom * 2);
				if ( Map->possiblePlace(&v, v.PosX + 1, v.PosY + 2)) drawExitPoint(spx + Hud.Zoom,     spy + Hud.Zoom * 2);
				if ( Map->possiblePlace(&v, v.PosX + 2, v.PosY + 2)) drawExitPoint(spx + Hud.Zoom * 2, spy + Hud.Zoom * 2);
			}
			else
			{
				if ( Map->possiblePlace(&v, v.PosX - 1, v.PosY - 1)) drawExitPoint(spx - Hud.Zoom, spy - Hud.Zoom);
				if ( Map->possiblePlace(&v, v.PosX    , v.PosY - 1)) drawExitPoint(spx,            spy - Hud.Zoom);
				if ( Map->possiblePlace(&v, v.PosX + 1, v.PosY - 1)) drawExitPoint(spx + Hud.Zoom, spy - Hud.Zoom);
				if ( Map->possiblePlace(&v, v.PosX - 1, v.PosY    )) drawExitPoint(spx - Hud.Zoom, spy			);
				if ( Map->possiblePlace(&v, v.PosX + 1, v.PosY    )) drawExitPoint(spx + Hud.Zoom, spy			);
				if ( Map->possiblePlace(&v, v.PosX - 1, v.PosY + 1)) drawExitPoint(spx - Hud.Zoom, spy + Hud.Zoom);
				if ( Map->possiblePlace(&v, v.PosX    , v.PosY + 1)) drawExitPoint(spx,            spy + Hud.Zoom);
				if ( Map->possiblePlace(&v, v.PosX + 1, v.PosY + 1)) drawExitPoint(spx + Hud.Zoom, spy + Hud.Zoom);
			}
		}
		if (v.PlaceBand)
		{
			if (v.data.can_build == BUILD_BIG)
			{
				SDL_Rect dest;
				dest.x = 180 - (int)(Hud.OffX / (64.0 / Hud.Zoom)) + Hud.Zoom * v.BandX;
				dest.y =  18 - (int)(Hud.OffY / (64.0 / Hud.Zoom)) + Hud.Zoom * v.BandY;
				if ( !SettingsData.bPreScale && ( GraphicsData.gfx_band_big->w != Hud.Zoom*2 || GraphicsData.gfx_band_big->h != Hud.Zoom*2 ) ) scaleSurface ( GraphicsData.gfx_band_big_org, GraphicsData.gfx_band_big, Hud.Zoom*2, Hud.Zoom*2 );
				SDL_BlitSurface(GraphicsData.gfx_band_big, NULL, buffer, &dest);
			}
			else
			{
				int x;
				int y;
				mouse->GetKachel(&x, &y);
				if (x == v.PosX || y == v.PosY)
				{
					SDL_Rect dest;
					dest.x = 180 - (int)(Hud.OffX / (64.0 / Hud.Zoom)) + Hud.Zoom * x;
					dest.y =  18 - (int)(Hud.OffY / (64.0 / Hud.Zoom)) + Hud.Zoom * y;
					if ( !SettingsData.bPreScale && ( GraphicsData.gfx_band_small->w != Hud.Zoom || GraphicsData.gfx_band_small->h != Hud.Zoom ) ) scaleSurface ( GraphicsData.gfx_band_small_org, GraphicsData.gfx_band_small, Hud.Zoom, Hud.Zoom );
					SDL_BlitSurface(GraphicsData.gfx_band_small, NULL, buffer, &dest);
					v.BandX     = x;
					v.BandY     = y;
				}
				else
				{
					v.BandX = v.PosX;
					v.BandY = v.PosY;
				}
			}
		}
		if (v.ActivatingVehicle && v.owner == ActivePlayer)
		{
			v.DrawExitPoints(v.StoredVehicles[v.VehicleToActivate]->typ);
		}
	}
	else if ( SelectedBuilding )
	{
		int spx,spy;
		spx=SelectedBuilding->GetScreenPosX();
		spy=SelectedBuilding->GetScreenPosY();
		if ( Hud.Scan )
		{
			if ( SelectedBuilding->data.is_big )
			{
				drawCircle ( spx+Hud.Zoom,
				             spy+Hud.Zoom,
				             SelectedBuilding->data.scan*Hud.Zoom,SCAN_COLOR,buffer );
			}
			else
			{
				drawCircle ( spx+Hud.Zoom/2,
				             spy+Hud.Zoom/2,
				             SelectedBuilding->data.scan*Hud.Zoom,SCAN_COLOR,buffer );
			}
		}
		if ( Hud.Reichweite&& ( SelectedBuilding->data.can_attack==ATTACK_LAND||SelectedBuilding->data.can_attack==ATTACK_SUB_LAND ) &&!SelectedBuilding->data.is_expl_mine )
		{
			drawCircle ( spx+Hud.Zoom/2,
			             spy+Hud.Zoom/2,
			             SelectedBuilding->data.range*Hud.Zoom+2,RANGE_GROUND_COLOR,buffer );
		}
		if ( Hud.Reichweite&&SelectedBuilding->data.can_attack==ATTACK_AIR )
		{
			drawCircle ( spx+Hud.Zoom/2,
			             spy+Hud.Zoom/2,
			             SelectedBuilding->data.range*Hud.Zoom+2,RANGE_AIR_COLOR,buffer );
		}
		
		if (SelectedBuilding->BuildList                              &&
				SelectedBuilding->BuildList->Size()                      &&
				!SelectedBuilding->IsWorking                             &&
				(*SelectedBuilding->BuildList)[0]->metall_remaining <= 0 &&
				SelectedBuilding->owner == ActivePlayer)
		{
			SelectedBuilding->DrawExitPoints((*SelectedBuilding->BuildList)[0]->typ);
		}
		if ( SelectedBuilding->ActivatingVehicle&&SelectedBuilding->owner==ActivePlayer )
		{
			SelectedBuilding->DrawExitPoints(SelectedBuilding->StoredVehicles[SelectedBuilding->VehicleToActivate]->typ);
		}
	}
	ActivePlayer->DrawLockList(Hud);
}

void cClient::displayDebugOutput()
{
	if ( !bFlagDrawMap ) return;
	#define DEBUGOUT_X_POS		(SettingsData.iScreenW-140)

	iDebugOff = 30;

	if ( bDebugPlayers )
	{
		font->showText(DEBUGOUT_X_POS, iDebugOff, "Players: " + iToStr( (int)PlayerList->Size() ), FONT_LATIN_SMALL_WHITE);
		iDebugOff += font->getFontHeight(FONT_LATIN_SMALL_WHITE);

		SDL_Rect rDest = { DEBUGOUT_X_POS, iDebugOff, 20, 10 };
		SDL_Rect rSrc = { 0, 0, 20, 10 };
		SDL_Rect rDotDest = {DEBUGOUT_X_POS-10, iDebugOff, 10, 10 };
		SDL_Rect rBlackOut = {DEBUGOUT_X_POS+20, iDebugOff, 0, 10 };
		for ( unsigned int i = 0; i < PlayerList->Size(); i++ )
		{
			//HACK SHOWFINISHEDPLAYERS
			SDL_Rect rDot = { 10 , 0, 10, 10 }; //for green dot

			if( (*PlayerList)[i]->bFinishedTurn && (*PlayerList)[i] != ActivePlayer)
			{
				SDL_BlitSurface( GraphicsData.gfx_player_ready, &rDot, buffer, &rDotDest );
			}
			else if(  (*PlayerList)[i] == ActivePlayer && bWantToEnd )
			{
				SDL_BlitSurface( GraphicsData.gfx_player_ready, &rDot, buffer, &rDotDest );
			}
			else
			{
				rDot.x = 0; //for red dot
				SDL_BlitSurface( GraphicsData.gfx_player_ready, &rDot, buffer, &rDotDest );
			}

			SDL_BlitSurface ( (*PlayerList)[i]->color, &rSrc, buffer, &rDest );
			if ( (*PlayerList)[i] == ActivePlayer ) 
			{
				string sTmpLine = " " + (*PlayerList)[i]->name + ", nr: " + iToStr ( (*PlayerList)[i]->Nr ) + " << you! ";
				rBlackOut.w = font->getTextWide(sTmpLine, FONT_LATIN_SMALL_WHITE); //black out background for better recognizing
				SDL_FillRect(buffer, &rBlackOut, 0x000000);
				font->showText(rBlackOut.x, iDebugOff+1, sTmpLine , FONT_LATIN_SMALL_WHITE);
			}
			else
			{
				string sTmpLine = " " + (*PlayerList)[i]->name + ", nr: " + iToStr ( (*PlayerList)[i]->Nr ) + " ";
				rBlackOut.w = font->getTextWide(sTmpLine, FONT_LATIN_SMALL_WHITE); //black out background for better recognizing
				SDL_FillRect(buffer, &rBlackOut, 0x000000);
				font->showText(rBlackOut.x, iDebugOff+1, sTmpLine , FONT_LATIN_SMALL_WHITE);
			}
			iDebugOff += 10; //use 10 for pixel high of dots instead of text high
			rDest.y = rDotDest.y = rBlackOut.y = iDebugOff;

		}
	}

	if ( bShowFPS )
	{
		font->showText(DEBUGOUT_X_POS, iDebugOff, "FPS: " + iToStr(Round(fFPS)), FONT_LATIN_SMALL_WHITE );
		iDebugOff += font->getFontHeight(FONT_LATIN_SMALL_WHITE);
		font->showText(DEBUGOUT_X_POS, iDebugOff, "Cycles/s: " + iToStr(Round(fCPS)), FONT_LATIN_SMALL_WHITE );
		iDebugOff += font->getFontHeight(FONT_LATIN_SMALL_WHITE);
		font->showText(DEBUGOUT_X_POS, iDebugOff, "Load: " + iToStr(iLoad/10) + "." + iToStr(iLoad%10) + "%", FONT_LATIN_SMALL_WHITE );
		iDebugOff += font->getFontHeight(FONT_LATIN_SMALL_WHITE);
	}

	if ( bDebugAjobs )
	{
		font->showText(DEBUGOUT_X_POS, iDebugOff, "ClientAttackJobs: " + iToStr((int)Client->attackJobs.Size()), FONT_LATIN_SMALL_WHITE);
		iDebugOff += font->getFontHeight(FONT_LATIN_SMALL_WHITE);
		if ( Server )
		{
			font->showText(DEBUGOUT_X_POS, iDebugOff, "ServerAttackJobs: " + iToStr((int)Server->AJobs.Size()), FONT_LATIN_SMALL_WHITE);
			iDebugOff += font->getFontHeight(FONT_LATIN_SMALL_WHITE);
		}
	}

	if ( bDebugBaseClient )
	{
		font->showText(DEBUGOUT_X_POS, iDebugOff, "subbases: " + iToStr((int)ActivePlayer->base.SubBases.Size()), FONT_LATIN_SMALL_WHITE);
		iDebugOff += font->getFontHeight ( FONT_LATIN_SMALL_WHITE );
	}

	if ( bDebugBaseServer )
	{
		cPlayer* serverPlayer = Server->getPlayerFromNumber(ActivePlayer->Nr);
		font->showText(DEBUGOUT_X_POS, iDebugOff, "subbases: " + iToStr((int)serverPlayer->base.SubBases.Size()), FONT_LATIN_SMALL_WHITE);
		iDebugOff += font->getFontHeight ( FONT_LATIN_SMALL_WHITE );
	}

	if ( bDebugSentry )
	{
		for ( unsigned int i = 0; i < Server->PlayerList->Size(); i++ )
		{
			cPlayer *Player = (*Server->PlayerList)[i];
			font->showText(DEBUGOUT_X_POS, iDebugOff, Player->name + " (" + iToStr ( Player->Nr ) + ") s-air: " + iToStr((int)Player->SentriesAir.Size()), FONT_LATIN_SMALL_WHITE);
			iDebugOff += font->getFontHeight(FONT_LATIN_SMALL_WHITE);
			font->showText(DEBUGOUT_X_POS, iDebugOff, Player->name + " (" + iToStr ( Player->Nr ) + ") s-ground: " + iToStr((int)Player->SentriesGround.Size()), FONT_LATIN_SMALL_WHITE);
			iDebugOff += font->getFontHeight(FONT_LATIN_SMALL_WHITE);
		}
	}

	if ( bDebugFX )
	{
		font->showText(DEBUGOUT_X_POS, iDebugOff, "fx-count: " + iToStr((int)FXList.Size() + (int)FXListBottom.Size()), FONT_LATIN_SMALL_WHITE);
		iDebugOff += font->getFontHeight(FONT_LATIN_SMALL_WHITE);
		font->showText(DEBUGOUT_X_POS, iDebugOff, "wind-dir: " + iToStr(( int ) ( fWindDir*57.29577 )), FONT_LATIN_SMALL_WHITE);
		iDebugOff += font->getFontHeight(FONT_LATIN_SMALL_WHITE);
	}
	if ( bDebugTraceServer || bDebugTraceClient )
	{
		trace();
	}
}

void cClient::displayChatInput()
{
	int y, iParts;
	string OutTxt = ">";
	OutTxt += InputHandler->getInputStr();

	iParts = font->getTextWide( OutTxt ) / (SettingsData.iScreenW-210)+1;
	y = SettingsData.iScreenH-40-iParts*font->getFontHeight ();
	int iStartPos = 0, iLength = 1;
	for ( int i = 0; i < iParts; i++ )
	{
		string sTmpStr = OutTxt.substr( iStartPos, iLength );
		while ( sTmpStr.length() < OutTxt.length()-iStartPos && font->getTextWide( sTmpStr ) < SettingsData.iScreenW-210 )
		{
			iLength++;
			sTmpStr = OutTxt.substr( iStartPos, iLength );
		}
		font->showText( 185, y, sTmpStr );
		y += font->getFontHeight ();
		iStartPos += iLength;
		iLength = 1;
	}
}

void cClient::setWind( int iDir )
{
	fWindDir = (float)(iDir/57.29577);
}

void cClient::makePanel( bool bOpen )
{
	SDL_Rect top,bottom,tmp;
	if ( bOpen )
	{
		PlayFX ( SoundData.SNDPanelOpen );
		top.x=0;top.y= ( SettingsData.iScreenH/2 )-479;
		top.w=bottom.w=171;
		top.h=479;bottom.h=481;
		bottom.x=0;bottom.y= ( SettingsData.iScreenH/2 );
		tmp=top;
		SDL_BlitSurface ( GraphicsData.gfx_panel_top,NULL,buffer,&tmp );
		tmp=bottom;
		SDL_BlitSurface ( GraphicsData.gfx_panel_bottom,NULL,buffer,&tmp );
		while ( top.y>-479 )
		{
			SHOW_SCREEN
			SDL_Delay ( 10 );
			top.y-=10;
			bottom.y+=10;
			SDL_BlitSurface ( GraphicsData.gfx_hud,NULL,buffer,NULL );
			tmp=top;
			SDL_BlitSurface ( GraphicsData.gfx_panel_top,NULL,buffer,&tmp );
			SDL_BlitSurface ( GraphicsData.gfx_panel_bottom,NULL,buffer,&bottom );
		}
	}
	else
	{
		PlayFX ( SoundData.SNDPanelClose );
		top.x=0;top.y=-480;
		top.w=bottom.w=171;
		top.h=479;bottom.h=481;
		bottom.x=0;bottom.y=SettingsData.iScreenH;
		while ( bottom.y>SettingsData.iScreenH/2 )
		{
			SHOW_SCREEN
			SDL_Delay ( 10 );
			top.y+=10;
			if ( top.y> ( SettingsData.iScreenH/2 )-479-9 ) top.y= ( SettingsData.iScreenH/2 )-479;
			bottom.y-=10;
			if ( bottom.y<SettingsData.iScreenH/2+9 ) bottom.y=SettingsData.iScreenH/2;
			SDL_BlitSurface ( GraphicsData.gfx_hud,NULL,buffer,NULL );
			tmp=top;
			SDL_BlitSurface ( GraphicsData.gfx_panel_top,NULL,buffer,&tmp );
			tmp=bottom;
			SDL_BlitSurface ( GraphicsData.gfx_panel_bottom,NULL,buffer,&tmp );
		}
		SHOW_SCREEN
		SDL_Delay ( 100 );
	}
}

void cClient::rotateBlinkColor()
{
	static bool bDec = true;
	if ( bDec )
	{
		iBlinkColor -= 0x0A0A0A;
		if ( iBlinkColor <= 0xA0A0A0 ) bDec = false;
	}
	else
	{
		iBlinkColor += 0x0A0A0A;
		if ( iBlinkColor >= 0xFFFFFF ) bDec = true;
	}
}

// F¸gt einen FX-Effekt ein:
void cClient::addFX ( eFXTyps typ,int x,int y, cClientAttackJob* aj, int iDestOff, int iFireDir )
{
	sFX* n = new sFX(typ, x, y);
	sFXRocketInfos* ri = n->rocketInfo;
	ri->ScrX = x;
	ri->ScrY = y;
	ri->DestX = (iDestOff % Client->Map->size) * 64;
	ri->DestY = (iDestOff / Client->Map->size) * 64;
	ri->aj = aj;
	ri->dir = iFireDir;
	addFX( n );
}

// F¸gt einen FX-Effekt ein:
void cClient::addFX ( eFXTyps typ,int x,int y,int param )
{
	sFX* n = new sFX(typ, x, y);
	n->param = param;
	addFX( n );
}

// F¸gt einen FX-Effekt ein:
void cClient::addFX ( sFX* n )
{

	if ( n->typ==fxTracks||n->typ==fxTorpedo||n->typ==fxBubbles||n->typ==fxCorpse )
	{
		FXListBottom.Add ( n );
	}
	else
	{
		FXList.Add ( n );
	}
	switch ( n->typ )
	{
		case fxExploAir:
			int nr;
			nr = random(3);
			if ( nr==0 )
			{
				PlayFX ( SoundData.EXPSmall0 );
			}
			else if ( nr==1 )
			{
				PlayFX ( SoundData.EXPSmall1 );
			}
			else
			{
				PlayFX ( SoundData.EXPSmall2 );
			}
			break;
		case fxExploSmall:
		case fxExploWater:
			if ( Map->IsWater ( n->PosX/64+ ( n->PosY/64 ) *Map->size ) )
			{
				int nr;
				nr = random(3);
				if ( nr==0 )
				{
					PlayFX ( SoundData.EXPSmallWet0 );
				}
				else if ( nr==1 )
				{
					PlayFX ( SoundData.EXPSmallWet1 );
				}
				else
				{
					PlayFX ( SoundData.EXPSmallWet2 );
				}
			}
			else
			{
				int nr;
				nr =  random(3);
				if ( nr==0 )
				{
					PlayFX ( SoundData.EXPSmall0 );
				}
				else if ( nr==1 )
				{
					PlayFX ( SoundData.EXPSmall1 );
				}
				else
				{
					PlayFX ( SoundData.EXPSmall2 );
				}
			}
			break;
		case fxExploBig:
			if ( Map->IsWater ( n->PosX/64+ ( n->PosY/64 ) *Map->size ) )
			{
				if (random(2))
				{
					PlayFX ( SoundData.EXPBigWet0 );
				}
				else
				{
					PlayFX ( SoundData.EXPBigWet1 );
				}
			}
			else
			{
				int nr;
				nr = random(4);
				if ( nr==0 )
				{
					PlayFX ( SoundData.EXPBig0 );
				}
				else if ( nr==1 )
				{
					PlayFX ( SoundData.EXPBig1 );
				}
				else if ( nr==2 )
				{
					PlayFX ( SoundData.EXPBig2 );
				}
				else
				{
					PlayFX ( SoundData.EXPBig3 );
				}
			}
			break;
		case fxRocket:
		case fxTorpedo:
		{
			sFXRocketInfos *ri;
			int dx,dy;
			ri= n->rocketInfo;
			ri->fpx=(float)n->PosX;
			ri->fpy=(float)n->PosY;
			dx=ri->ScrX-ri->DestX;
			dy=ri->ScrY-ri->DestY;
			if ( abs ( dx ) >abs ( dy ) )
			{
				if ( ri->ScrX>ri->DestX ) ri->mx=-1;
				else ri->mx=1;
				ri->my=dy/ ( float ) dx* ( -ri->mx );
			}
			else
			{
				if ( ri->ScrY<ri->DestY ) ri->my=-1;
				else ri->my=1;
				ri->mx=dx/ ( float ) dy* ( -ri->my );
			}
			break;
		}
		case fxDarkSmoke:
		{
			float x,y,ax,ay;
			sFXDarkSmoke *dsi = n->smokeInfo;
			dsi->alpha=n->param;
			if ( dsi->alpha>150 ) dsi->alpha=150;
			dsi->fx=(float)n->PosX;
			dsi->fy=(float)n->PosY;

			ax=x=sin ( fWindDir );
			ay=y=cos ( fWindDir );
			if ( ax<0 ) ax=-ax;
			if ( ay<0 ) ay=-ay;
			if ( ax>ay )
			{
				dsi->dx = (float)(x * 2 + random(5)        / 10.0);
				dsi->dy = (float)(y * 2 + (random(15) - 7) / 14.0);
			}
			else
			{
				dsi->dx = (float)(x * 2 + (random(15) - 7) / 14.0);
				dsi->dy = (float)(y * 2 + random(5)        / 10.0);
			}
			break;
		}
		case fxTracks:
		{
			sFXTracks *tri = n->trackInfo;
			tri->alpha = 100;
			tri->dir = n->param;
			break;
		}
		case fxCorpse:
			n->param=255;
			break;
		case fxAbsorb:
			PlayFX ( SoundData.SNDAbsorb );
			break;
		default:
			break;
	}
}


void cClient::doCommand ( string sCmd )
{
	if ( sCmd.compare( "/fps on" ) == 0 ) { bShowFPS = true; return;}
	if ( sCmd.compare( "/fps off" ) == 0 ) { bShowFPS = false; return;}
	if ( sCmd.compare( "/base client" ) == 0 ) { bDebugBaseClient = true; bDebugBaseServer = false; return; }
	if ( sCmd.compare( "/base server" ) == 0 ) { if (Server) bDebugBaseServer = true; bDebugBaseClient = false; return; }
	if ( sCmd.compare( "/base off" ) == 0 ) { bDebugBaseServer = false; bDebugBaseClient = false; return; }
	if ( sCmd.compare( "/sentry server" ) == 0 ) { if (Server) bDebugSentry = true; return; }
	if ( sCmd.compare( "/sentry off" ) == 0 ) { bDebugSentry = false; return; }
	if ( sCmd.compare( "/fx on" ) == 0 ) { bDebugFX = true; return; }
	if ( sCmd.compare( "/fx off" ) == 0 ) { bDebugFX = false; return; }
	if ( sCmd.compare( "/trace server" ) == 0 ) { if ( Server ) bDebugTraceServer = true; bDebugTraceClient = false; return; }
	if ( sCmd.compare( "/trace client" ) == 0 ) { bDebugTraceClient = true; bDebugTraceServer = false; return; }
	if ( sCmd.compare( "/trace off" ) == 0 ) { bDebugTraceServer = false; bDebugTraceClient = false; return; }
	if ( sCmd.compare( "/ajobs on" ) == 0 ) { bDebugAjobs = true; return; }
	if ( sCmd.compare( "/ajobs off" ) == 0 ) { bDebugAjobs = false; return; }
	if ( sCmd.compare( "/checkpos on" ) == 0 && Server ) { Server->bDebugCheckPos = true; return; }
	if ( sCmd.compare( "/checkpos off") == 0 && Server ) { Server->bDebugCheckPos = false; return; }
	if ( sCmd.compare( "/checkpos" ) == 0 && Server ) { sendCheckVehiclePositions(); return; }
	if ( sCmd.compare( "/players on" ) == 0 ) { bDebugPlayers = true; return; }
	if ( sCmd.compare( "/players off" ) == 0 ) { bDebugPlayers = false; return; }

	if ( sCmd.substr( 0, 6 ).compare( "/kick " ) == 0 )
	{
		if ( sCmd.length() > 6 && Server )
		{
			int playerNum = -1;
			//first try to find player by name
			for ( unsigned int i = 0; i < Server->PlayerList->Size(); i++ )
			{
				if ( (*Server->PlayerList)[i]->name.compare( sCmd.substr ( 6, sCmd.length() )) == 0 )
				{
					playerNum = (*Server->PlayerList)[i]->Nr;
				}
			}
			//then by number
			if ( playerNum == -1 )
			{
				playerNum = atoi ( sCmd.substr ( 6, sCmd.length() ).c_str() );
			}

			//since atoi is too stupid to report an error, do an extra check, when the number is 0
			if ( playerNum == 0 && sCmd[6] != '0' ) return;
			
			cPlayer *Player = Server->getPlayerFromNumber ( playerNum );
			if ( !Player ) return;
			
			// close the socket
			if ( network ) network->close ( Player->iSocketNum );
			for ( unsigned int i = 0; i < Server->PlayerList->Size(); i++ )
			{
				if ( (*Server->PlayerList)[i]->iSocketNum > Player->iSocketNum && (*Server->PlayerList)[i]->iSocketNum < MAX_CLIENTS ) (*Server->PlayerList)[i]->iSocketNum--;
			}
			// delete the player
			Server->deletePlayer ( Player );
		}
	}
	if ( sCmd.substr( 0, 12 ).compare( "/disconnect " ) == 0 )
	{
		if ( sCmd.length() > 12 && Server )
		{
			int playerNum = -1;
			//first try to find player by name
			for ( unsigned int i = 0; i < Server->PlayerList->Size(); i++ )
			{
				if ( (*Server->PlayerList)[i]->name.compare( sCmd.substr ( 12, sCmd.length() )) == 0 )
				{
					playerNum = (*Server->PlayerList)[i]->Nr;
				}
			}
			//then by number
			if ( playerNum == -1 )
			{
				playerNum = atoi ( sCmd.substr ( 12, sCmd.length() ).c_str() );
			}

			//since atoi is too stupid to report an error, do an extra check, when the number is 0
			if ( playerNum == 0 && sCmd[12] != '0' ) return;
			
			cPlayer *Player = Server->getPlayerFromNumber ( playerNum );
			if ( !Player ) return;

			//can not disconnect local players
			if ( Player->iSocketNum == MAX_CLIENTS ) return;

			SDL_Event* event = new SDL_Event;
			event->type = NETWORK_EVENT;
			event->user.code = TCP_CLOSEEVENT;
			event->user.data1 = new Sint16[1];
			((Sint16*)event->user.data1)[0] = Player->iSocketNum;
			Server->pushEvent ( event );
		}
	}
	if ( sCmd.substr( 0, 9 ).compare( "/deadline"  ) == 0 )
	{
		if(sCmd.length() > 9  && Server)
		{
			int i = 90;
			i = atoi ( sCmd.substr ( 9, sCmd.length() ).c_str());
			Server->setDeadline(i);
			Log.write("Deadline changed to "  + iToStr( i ) , cLog::eLOG_TYPE_INFO);
		}
		return;
	}
	if ( sCmd.substr( 0, 7 ).compare( "/resync" ) == 0 )
	{
		if ( sCmd.length() > 7 && Server )
		{
			unsigned int playernum = atoi ( sCmd.substr ( 7, 8 ).c_str() );
			sendRequestResync( playernum );
		}
		else
		{
			if ( Server )
			{
				for ( unsigned int i = 0; i < Server->PlayerList->Size(); i++ )
				{
					sendRequestResync( 	(*Server->PlayerList)[i]->Nr );
				}
			}
			else
			{
				sendRequestResync( Client->ActivePlayer->Nr );
			}
		}
		return;
	}
	if ( sCmd.substr( 0, 5 ).compare( "/mark"  ) == 0 )
	{
		sCmd.erase(0, 5 );
		cNetMessage* message = new cNetMessage( GAME_EV_WANT_MARK_LOG );
		message->pushString( sCmd );
		Client->sendNetMessage( message );
		return;
	}
	if ( sCmd.substr( 0, 7 ).compare( "/color " ) == 0 ) {
		int cl=0;sscanf ( sCmd.c_str(),"color %d",&cl );cl%=8;ActivePlayer->color=OtherData.colors[cl];return;}
	if ( sCmd.compare( "/fog off" ) == 0 && Server )
	{
		memset ( Server->getPlayerFromNumber(ActivePlayer->Nr)->ScanMap,1,Map->size*Map->size );
		memset ( ActivePlayer->ScanMap,1,Map->size*Map->size );
		sPlayerCheat = ActivePlayer->name + " " + lngPack.i18n( "Text~Comp~Cheat");
		sPlayerCheat+=" \"Fog Off\"";
		return;
	}

	if ( sCmd.compare( "/survey" ) == 0 )
	{
		if ( network && !network->isHost() ) return;
		memcpy ( Map->Resources , Server->Map->Resources, Map->size*Map->size*sizeof ( sResources ) );
		memset ( ActivePlayer->ResourceMap,1,Map->size*Map->size );
		sPlayerCheat=ActivePlayer->name + " " + lngPack.i18n( "Text~Comp~Cheat");
		sPlayerCheat+=" \"Survey\"";
		return;
	}

	if ( sCmd.compare( "/credits" ) == 0 )
	{
		//ActivePlayer->Credits+=1000;
		sPlayerCheat = ActivePlayer->name + " " + lngPack.i18n( "Text~Comp~Cheat");
		sPlayerCheat+=" \"Credits\"";
		return;
	}
	if ( sCmd.substr( 0, 5 ).compare( "/kill " ) == 0 )
	{
		int x,y;
		sscanf ( sCmd.c_str(),"kill %d,%d",&x,&y );
		/*engine->DestroyObject ( x+y*Map->size,false );
		engine->DestroyObject ( x+y*Map->size,true );*/
		sPlayerCheat=ActivePlayer->name + " " + lngPack.i18n( "Text~Comp~Cheat");
		sPlayerCheat+=" \"Kill\"";
		return;
	}
	if ( sCmd.compare( "/load" ) == 0 )
	{
		sPlayerCheat = ActivePlayer->name + " " + lngPack.i18n( "Text~Comp~Cheat");
		sPlayerCheat += " \"Load\"";

		/*if ( SelectedVehicle ) {SelectedVehicle->data.cargo=SelectedVehicle->data.max_cargo;SelectedVehicle->data.ammo=SelectedVehicle->data.max_ammo;SelectedVehicle->ShowDetails();}
		else if ( SelectedBuilding )
		{
			if ( SelectedBuilding->data.can_load==TRANS_METAL )
			{
				SelectedBuilding->SubBase->Metal-=SelectedBuilding->data.cargo;
				SelectedBuilding->data.cargo=SelectedBuilding->data.max_cargo;
				SelectedBuilding->SubBase->Metal+=SelectedBuilding->data.cargo;
			}
			else if ( SelectedBuilding->data.can_load==TRANS_OIL )
			{
				SelectedBuilding->SubBase->Oil-=SelectedBuilding->data.cargo;
				SelectedBuilding->data.cargo=SelectedBuilding->data.max_cargo;
				SelectedBuilding->SubBase->Oil+=SelectedBuilding->data.cargo;
			}
			else if ( SelectedBuilding->data.can_load==TRANS_GOLD )
			{
				SelectedBuilding->SubBase->Gold-=SelectedBuilding->data.cargo;
				SelectedBuilding->data.cargo=SelectedBuilding->data.max_cargo;
				SelectedBuilding->SubBase->Gold+=SelectedBuilding->data.cargo;
			}
			SelectedBuilding->data.ammo=SelectedBuilding->data.max_ammo;SelectedBuilding->ShowDetails();
		}*/
		return;
	}
}

void cClient::mouseMoveCallback ( bool bForce )
{
	static int iLastX = -1, iLastY = -1;
	SDL_Rect scr, dest;

	int iX, iY;
	mouse->GetKachel ( &iX, &iY );
	if ( iX == iLastX && iY == iLastY && !bForce ) return;
	iLastX = iX;
	iLastY = iY;
	// re-establish the hud:
	scr.x=262;
	scr.y=25;
	scr.w=64;
	scr.h=16;
	dest.x=265;
	dest.y=SettingsData.iScreenH-21;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	scr.x=64;
	scr.y=198;
	scr.w=215;
	dest.x=342;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	if ( iX == -1 )
	{
		return;
	}
	// draw the coordinates:
	/*array to get map coords in sceme XXX-YYY\0 = 8 characters
	a case where I accept an array since I don't know a better
	method to format x and y easily with leading 0 -- beko */
	char str[8];
	sprintf ( str, "%0.3d-%0.3d", iX, iY );
	font->showTextCentered(265+32, ( SettingsData.iScreenH-21 ) +4, str, FONT_LATIN_NORMAL, GraphicsData.gfx_hud);

	if ( !ActivePlayer->ScanMap[iX+iY*Map->size] )
	{
		OverUnitField = NULL;
		if ( mouse->cur==GraphicsData.gfx_Cattack )
		{
			SDL_Rect r;
			r.x=1;r.y=29;
			r.h=3;r.w=35;
			SDL_FillRect ( GraphicsData.gfx_Cattack,&r,0 );
		}
		return;
	}
	// check wether there is a unit under the mouse:
	OverUnitField = Map->fields + (iX + iY * Map->size);
	if ( mouse->cur == GraphicsData.gfx_Csteal && SelectedVehicle )
	{
		SelectedVehicle->drawCommandoCursor ( Map->size*iY+iX, true );
	}
	else if ( mouse->cur == GraphicsData.gfx_Cdisable && SelectedVehicle )
	{
		SelectedVehicle->drawCommandoCursor ( Map->size*iY+iX, false );
	}
	if ( OverUnitField->getVehicles() != NULL )
	{
		font->showTextCentered(343+106, ( SettingsData.iScreenH-21 ) +4, OverUnitField->getVehicles()->name, FONT_LATIN_NORMAL, GraphicsData.gfx_hud);
		if ( mouse->cur==GraphicsData.gfx_Cattack )
		{
			if ( SelectedVehicle )
			{
				SelectedVehicle->DrawAttackCursor ( Map->size*iY+iX );
			}
			else if ( SelectedBuilding )
			{
				SelectedBuilding->DrawAttackCursor ( Map->size*iY+iX );
			}
		}
	}
	else if ( OverUnitField->getPlanes() != NULL )
	{
		font->showTextCentered(343+106, ( SettingsData.iScreenH-21 ) +4, OverUnitField->getPlanes()->name, FONT_LATIN_NORMAL, GraphicsData.gfx_hud);
		if ( mouse->cur==GraphicsData.gfx_Cattack )
		{
			if ( SelectedVehicle )
			{
				SelectedVehicle->DrawAttackCursor ( Map->size*iY+iX );
			}
			else if ( SelectedBuilding )
			{
				SelectedBuilding->DrawAttackCursor ( Map->size*iY+iX );
			}
		}
	}
	else if ( OverUnitField->getTopBuilding() != NULL )
	{
		font->showTextCentered(343+106, ( SettingsData.iScreenH-21 ) +4, OverUnitField->getTopBuilding()->name, FONT_LATIN_NORMAL, GraphicsData.gfx_hud);
		if ( mouse->cur==GraphicsData.gfx_Cattack )
		{
			if ( SelectedVehicle )
			{
				SelectedVehicle->DrawAttackCursor ( Map->size*iY+iX );
			}
			else if ( SelectedBuilding )
			{
				SelectedBuilding->DrawAttackCursor ( Map->size*iY+iX );
			}
		}
	}
	else if ( OverUnitField->getBaseBuilding() != NULL )
	{
		font->showTextCentered(343+106, ( SettingsData.iScreenH-21 ) +4, OverUnitField->getBaseBuilding()->name, FONT_LATIN_NORMAL, GraphicsData.gfx_hud);
		if ( mouse->cur==GraphicsData.gfx_Cattack )
		{
			if ( SelectedVehicle )
			{
				SelectedVehicle->DrawAttackCursor ( Map->size*iY+iX );
			}
			else if ( SelectedBuilding )
			{
				SelectedBuilding->DrawAttackCursor ( Map->size*iY+iX );
			}
		}
	}
	else
	{
		if ( mouse->cur == GraphicsData.gfx_Cattack )
		{
			SDL_Rect r;
			r.x=1;r.y=29;
			r.h=3;r.w=35;
			SDL_FillRect ( GraphicsData.gfx_Cattack,&r,0 );
		}
		OverUnitField = NULL;
	}
	// place band:
	if ( SelectedVehicle && SelectedVehicle->PlaceBand )
	{
		SelectedVehicle->FindNextband();
	}
}

// Adds an message to be displayed in the game
void cClient::addMessage ( string sMsg )
{
	sMessage* const Message = new sMessage(sMsg, iFrame);
	messages.Add(Message);
	if(SettingsData.bDebug) Log.write(Message->msg, cLog::eLOG_TYPE_DEBUG);
}

// displays a message with 'goto' coordinates
void cClient::addCoords (const string msg,int x,int y )
{
 	stringstream strStream;
 	//e.g. [85,22] missel MK I is under attack (F1)
 	strStream << "[" << x << "," << y << "] " << msg << " (" << GetKeyString ( KeysList.KeyJumpToAction ) << ")";
	Client->addMessage ( strStream.str() );
	iMsgCoordsX=x;
	iMsgCoordsY=y;
}

void cClient::handleMessages()
{
	SDL_Rect scr, dest;
	int iHeight;
	sMessage *message;
	if (messages.Size() == 0) return;
	iHeight = 0;
	// Alle alten Nachrichten lˆschen:
	for (int i = (int)messages.Size() - 1; i >= 0; i--)
	{
		message = messages[i];
		if ( message->age+MSG_FRAMES < iFrame || iHeight > 200 )
		{
			delete message;
			messages.Delete ( i );
			continue;
		}
		iHeight += 17 + font->getFontHeight() * ( message->len  / (SettingsData.iScreenW - 300) );
	}
	if (messages.Size() == 0) return;
	
	scr.x = 0; scr.y = 0;
	dest.x = 180; dest.y = 30;
	scr.w = SettingsData.iScreenW - 200;
	scr.h = iHeight+6;

	if ( SettingsData.bAlphaEffects )
	{
		SDL_BlitSurface ( GraphicsData.gfx_shadow, &scr, buffer, &dest );
	}
	dest.x = 180+2; dest.y = 34;
	dest.w = SettingsData.iScreenW - 204;
	dest.h = iHeight;
	
	for (unsigned int i = 0; i < messages.Size(); i++)
	{
		message = messages[i];
		string sMsg = message->msg;
		//HACK TO SHOW PLAYERCOLOR IN CHAT
		int iColor = -1;
		for(unsigned int i=0; i < sMsg.length(); i++)
		{
			if(sMsg[i] == ':') //scan for chatmessages from _players_
			{
				string sTmp = sMsg.substr( 0, i );
				for ( unsigned int i = 0; i < Client->PlayerList->Size(); i++ )
				{
					cPlayer* const Player = (*Client->PlayerList)[i];
					if (Player)
					{
						if(sTmp.compare( Player->name ) == 0)
						{
							iColor = GetColorNr(Player->color);
							break;
						}
					}
				}
				break;
			}
		}
		if(iColor != -1)
		{
			#define CELLSPACE 3
			SDL_Rect rColorSrc = { 0, 0, 10, font->getFontHeight() };
			SDL_Rect rDest = dest;
			rDest.w = rColorSrc.w;
			rDest.h = rColorSrc.h;
			SDL_BlitSurface(OtherData.colors[iColor], &rColorSrc, buffer, &rDest ); //blit color
			dest.x += rColorSrc.w + CELLSPACE; //add border for color
			dest.w -= rColorSrc.w + CELLSPACE;
			dest.y = font->showTextAsBlock( dest, sMsg );
			dest.x -= rColorSrc.w + CELLSPACE; //reset border from color
			dest.w += rColorSrc.w + CELLSPACE;
		}
		else
		{
			dest.y = font->showTextAsBlock( dest, sMsg );
		}
		
		dest.y += 5;
	}
}

int cClient::HandleNetMessage( cNetMessage* message )
{	
	if ( message->iType != DEBUG_CHECK_VEHICLE_POSITIONS )		//do not pollute log file with debug events
		Log.write("Client: --> " + message->getTypeAsString() + ", Hexdump: " + message->getHexDump(), cLog::eLOG_TYPE_NET_DEBUG );

	switch ( message->iType )
	{
	case DEBUG_CHECK_VEHICLE_POSITIONS:
		checkVehiclePositions( message );
		break;
	case GAME_EV_LOST_CONNECTION:
		addMessage( lngPack.i18n ( "Text~Multiplayer~Lost_Connection", "server" ) );
		break;
	case GAME_EV_CHAT_SERVER:
		switch (message->popChar())
		{
		case USER_MESSAGE:
			PlayFX ( SoundData.SNDChat );
			addMessage( message->popString() );
			break;
		case SERVER_ERROR_MESSAGE:
			PlayFX ( SoundData.SNDQuitsch );
			addMessage( lngPack.i18n( message->popString() ) );
			break;
		case SERVER_INFO_MESSAGE:
			{
				string translationpath = message->popString();
				string inserttext = message->popString();
				if ( !inserttext.compare ( "" ) ) addMessage( lngPack.i18n( translationpath ) );
				else addMessage( lngPack.i18n( translationpath, inserttext ) );
			}
			break;
		}
		break;
	case GAME_EV_ADD_BUILDING:
		{
			cBuilding *AddedBuilding;
			sID UnitID;
			bool Init = message->popBool();
			cPlayer *Player = getPlayerFromNumber ( message->popInt16() );
			UnitID.iFirstPart = message->popInt16();
			UnitID.iSecondPart = message->popInt16();
			int PosY = message->popInt16();
			int PosX = message->popInt16();

			AddedBuilding = Player->addBuilding( PosX, PosY, UnitID.getBuilding() );
			AddedBuilding->iID = message->popInt16();

			addUnit ( PosX, PosY, AddedBuilding, Init );

			// play placesound if it is a mine
			if ( UnitID.getBuilding()->nr == BNrLandMine && Player == ActivePlayer ) PlayFX ( SoundData.SNDLandMinePlace );
			else if ( UnitID.getBuilding()->nr == BNrSeaMine && Player == ActivePlayer ) PlayFX ( SoundData.SNDSeaMinePlace );
		}
		break;
	case GAME_EV_ADD_VEHICLE:
		{
			cVehicle *AddedVehicle;
			sID UnitID;
			bool Init = message->popBool();
			cPlayer *Player = getPlayerFromNumber ( message->popInt16() );
			UnitID.iFirstPart = message->popInt16();
			UnitID.iSecondPart = message->popInt16();
			int PosY = message->popInt16();
			int PosX = message->popInt16();

			AddedVehicle = Player->AddVehicle(PosX, PosY, UnitID.getVehicle());
			AddedVehicle->iID = message->popInt16();
			bool bAddToMap = message->popBool();

			addUnit ( PosX, PosY, AddedVehicle, Init, bAddToMap );
		}
		break;
	case GAME_EV_DEL_BUILDING:
		{
			cBuilding *Building;

			Building = getBuildingFromID ( message->popInt16() );

			if ( Building )
			{
				// play clearsound if it is a mine
				if ( Building->owner && Building->typ->nr == BNrLandMine && Building->owner == ActivePlayer ) PlayFX ( SoundData.SNDLandMineClear );
				else if ( Building->owner && Building->typ->nr == BNrSeaMine && Building->owner == ActivePlayer ) PlayFX ( SoundData.SNDSeaMineClear );

				deleteUnit ( Building );
			}
		}
		break;
	case GAME_EV_DEL_VEHICLE:
		{
			cVehicle *Vehicle;

			Vehicle = getVehicleFromID ( message->popInt16() );

			if ( Vehicle )
			{
				deleteUnit ( Vehicle );
			}
		}
		break;
	case GAME_EV_ADD_ENEM_VEHICLE:
		{
			cVehicle *AddedVehicle;
			cPlayer *Player = getPlayerFromNumber ( message->popInt16() );
			sID UnitID;

			UnitID.iFirstPart = message->popInt16();
			UnitID.iSecondPart = message->popInt16();
			int iPosY = message->popInt16();
			int iPosX = message->popInt16();
			AddedVehicle = Player->AddVehicle(iPosX, iPosY, UnitID.getVehicle() );

			AddedVehicle->dir = message->popInt16();
			AddedVehicle->iID = message->popInt16();

			addUnit ( iPosX, iPosY, AddedVehicle, false );

			// make report
			addCoords ( AddedVehicle->name + " " + lngPack.i18n ( "Text~Comp~Detected" ), iPosX, iPosY );
			if ( random( 2 ) == 0 ) PlayVoice ( VoiceData.VOIDetected1 );
			else PlayVoice ( VoiceData.VOIDetected2 );
		}
		break;
	case GAME_EV_ADD_ENEM_BUILDING:
		{
			cBuilding *AddedBuilding;
			cPlayer *Player = getPlayerFromNumber ( message->popInt16() );
			sID UnitID;

			UnitID.iFirstPart = message->popInt16();
			UnitID.iSecondPart = message->popInt16();
			int iPosY = message->popInt16();
			int iPosX = message->popInt16();

			AddedBuilding = Player->addBuilding( iPosX, iPosY, UnitID.getBuilding() );
			AddedBuilding->iID = message->popInt16();
			addUnit ( iPosX, iPosY, AddedBuilding, false );

			Player->base.SubBases[0]->buildings.Add ( AddedBuilding );
			AddedBuilding->SubBase = Player->base.SubBases[0];

			AddedBuilding->updateNeighbours( Map );
		}
		break;
	case GAME_EV_MAKE_TURNEND:
		{
			int iNextPlayerNum = message->popInt16();
			bool bWaitForNextPlayer = message->popBool();
			bool bEndTurn = message->popBool();

			if ( bEndTurn )
			{
				iTurn++;
				iTurnTime = 0;
				Hud.ShowRunde();
				if (!bWaitForNextPlayer ) Hud.EndeButton ( false );
				bWantToEnd = false;
				Hud.showTurnTime ( -1 );
				bFlagDrawHud = true;
				Log.write("######### Round " + iToStr( iTurn ) + " ###########", cLog::eLOG_TYPE_NET_DEBUG );
			}

			if ( bWaitForNextPlayer )
			{
				if ( iNextPlayerNum != ActivePlayer->Nr )
				{
					if (bWaitForOthers)
					{
						drawMap();
						SDL_BlitSurface ( GraphicsData.gfx_hud, NULL, buffer, NULL );
						drawMiniMap();
						drawFLC();
					}
					else bWaitForOthers = true;
					waitForOtherPlayer( iNextPlayerNum );
				}
				else
				{
					bWaitForOthers = false;
					Hud.EndeButton ( false );
					bFlagDrawHud = true;
				}
			}
			else if ( iNextPlayerNum != -1 )
			{
				makeHotSeatEnd( iNextPlayerNum );
			}
		}
		break;
	case GAME_EV_FINISHED_TURN:
		{
			int iPlayerNum = message->popInt16();
			int iTimeDelay = message->popInt16();

			cPlayer *Player = getPlayerFromNumber( iPlayerNum );
			if ( Player == NULL && iPlayerNum != -1 )
			{
				Log.write(" Client: Player with nr " + iToStr(iPlayerNum) + " has finished turn, but can't find him", cLog::eLOG_TYPE_NET_WARNING );
				break;
			}

			//HACK SHOWFINISHEDPLAYERS player finished his turn
			if ( Player ) 
			{
				Player->bFinishedTurn=true;
				for ( unsigned int i = 0; i < PlayerList->Size(); i++ )
				{
					if ( Player == (*PlayerList)[i] )
					{
						Hud.ExtraPlayers(Player->name, GetColorNr(Player->color), i, Player->bFinishedTurn);
						break;
					}
				}
			}


			if ( iTimeDelay != -1 )
			{
				if ( iPlayerNum != ActivePlayer->Nr && iPlayerNum != -1  ) addMessage( Player->name + " " + lngPack.i18n( "Text~Multiplayer~Player_Turn_End") + ". " + lngPack.i18n( "Text~Multiplayer~Deadline", iToStr( iTimeDelay ) ) );
				iTurnTime = iTimeDelay;
				iStartTurnTime = SDL_GetTicks();
			}
			else if ( iPlayerNum != ActivePlayer->Nr && iPlayerNum != -1  ) addMessage( Player->name + " " + lngPack.i18n( "Text~Multiplayer~Player_Turn_End") );
		}
		break;
	case GAME_EV_UNIT_DATA:
		{
			cPlayer *Player = getPlayerFromNumber ( message->popInt16() );
			sUnitData *Data;

			bool bWasBuilding = false;
			int iID = message->popInt16();
			bool bVehicle = message->popBool();
			int iPosY = message->popInt16();
			int iPosX = message->popInt16();

			cVehicle *Vehicle = NULL;
			cBuilding *Building = NULL;

			Log.write(" Client: Received Unit Data: Vehicle: " + iToStr ( (int)bVehicle ) + ", ID: " + iToStr ( iID ) + ", XPos: " + iToStr ( iPosX ) + ", YPos: " +iToStr ( iPosY ), cLog::eLOG_TYPE_NET_DEBUG);
			// unit is a vehicle
			if ( bVehicle )
			{
				bool bBig = message->popBool();
				Vehicle = getVehicleFromID ( iID );

				if ( !Vehicle )
				{
					Log.write(" Client: Unknown vehicle with ID: "  + iToStr( iID ) , cLog::eLOG_TYPE_NET_WARNING);
					// TODO: Request sync of vehicle
					break;
				}
				if ( Vehicle->PosX != iPosX || Vehicle->PosY != iPosY || Vehicle->data.is_big != bBig )
				{
					// if the vehicle is moving it is normal that the positions are not the same,
					// when the vehicle was building it is also normal that the position should be changed
					// so the log message will just be an debug one
					int iLogType = cLog::eLOG_TYPE_NET_WARNING;
					if ( Vehicle->IsBuilding || Vehicle->IsClearing || Vehicle->moving ) iLogType = cLog::eLOG_TYPE_NET_DEBUG;
					Log.write(" Client: Vehicle identificated by ID (" + iToStr( iID ) + ") but has wrong position [IS: X" + iToStr( Vehicle->PosX ) + " Y" + iToStr( Vehicle->PosY ) + "; SHOULD: X" + iToStr( iPosX ) + " Y" + iToStr( iPosY ) + "]", iLogType );

					// set to server position if vehicle is not moving
					if ( !Vehicle->moving )
					{
						Map->moveVehicle( Vehicle, iPosX, iPosY );
						if ( bBig ) Map->moveVehicleBig( Vehicle, iPosX, iPosY );
						Vehicle->owner->DoScan();
					}
				}

				Vehicle->name = message->popString();
				Vehicle->bIsBeeingAttacked = message->popBool();
				Vehicle->Disabled = message->popInt16();
				Vehicle->CommandoRank = message->popInt16();
				Vehicle->IsClearing = message->popBool();
				bWasBuilding = Vehicle->IsBuilding;
				Vehicle->IsBuilding = message->popBool();
				Vehicle->BuildRounds = message->popInt16();
				Vehicle->ClearingRounds = message->popInt16();
				Vehicle->bSentryStatus = message->popBool();

				Data = &Vehicle->data;
			}
			else
			{
				Building = getBuildingFromID ( iID );
				if ( !Building )
				{
					Log.write(" Client: Unknown building with ID: "  + iToStr( iID ) , cLog::eLOG_TYPE_NET_WARNING);
					// TODO: Request sync of building
					break;
				}

				Building->name = message->popString();
				Building->Disabled = message->popInt16();
				Building->IsWorking = message->popBool();
				Building->bSentryStatus = message->popBool();

				Data = &Building->data;
			}

			Data->costs = message->popInt16();
			Data->ammo = message->popInt16();
			Data->max_ammo = message->popInt16();
			Data->cargo = message->popInt16();
			Data->max_cargo = message->popInt16();
			Data->damage = message->popInt16();
			Data->shots = message->popInt16();
			Data->max_shots = message->popInt16();
			Data->range = message->popInt16();
			Data->scan = message->popInt16();
			Data->armor = message->popInt16();
			Data->hit_points = message->popInt16();
			Data->max_hit_points = message->popInt16();
			Data->version = message->popInt16();

			if ( bVehicle )
			{
				if ( Data->can_lay_mines )
				{
					if ( Data->cargo <= 0 ) Vehicle->LayMines = false;
					if ( Data->cargo >= Data->max_cargo ) Vehicle->ClearMines = false;
				}
				Data->speed = message->popInt16();
				Data->max_speed = message->popInt16();

				if ( Vehicle == SelectedVehicle ) Vehicle->ShowDetails();
				if ( bWasBuilding && !Vehicle->IsBuilding ) StopFXLoop ( iObjectStream );
			}
			else if ( Building == SelectedBuilding ) Building->ShowDetails();
		}
		break;
	case GAME_EV_SPECIFIC_UNIT_DATA:
		{
			cVehicle *Vehicle = getVehicleFromID ( message->popInt16() );
			if ( !Vehicle ) break;
			Vehicle->dir = message->popInt16();
			Vehicle->BuildingTyp.iFirstPart = message->popInt16();
			Vehicle->BuildingTyp.iSecondPart = message->popInt16();
			Vehicle->BuildPath = message->popBool();
			Vehicle->BandX = message->popInt16();
			Vehicle->BandY = message->popInt16();
		}
		break;
	case GAME_EV_DO_START_WORK:
		{
			int iID = message->popInt32();

			cBuilding* building = getBuildingFromID( iID);
			if ( building == NULL )
			{
				Log.write(" Client: Can't start work of building: Unknown building with id: "  + iToStr( iID ) , cLog::eLOG_TYPE_NET_ERROR);
				// TODO: Request sync of building
				break;
			}

			building->ClientStartWork();
		}
		break;
	case GAME_EV_DO_STOP_WORK:
		{
			int iID = message->popInt32();
			cBuilding* building = getBuildingFromID(iID);
			if ( building == NULL )
			{
				Log.write(" Client: Can't stop work of building: Unknown building with id: "  + iToStr( iID ) , cLog::eLOG_TYPE_NET_WARNING);
				// TODO: Request sync of building
				break;
			}

			building->ClientStopWork();
		}
		break;
	case GAME_EV_MOVE_JOB_SERVER:
		{
			int iVehicleID = message->popInt16();
			int iSrcOff = message->popInt32();
			int iDestOff = message->popInt32();
			bool bPlane = message->popBool();

			cVehicle *Vehicle = getVehicleFromID ( iVehicleID );
			if ( Vehicle == NULL )
			{
				Log.write(" Client: Can't find vehicle with id " + iToStr ( iVehicleID ) + " for movejob from " +  iToStr (iSrcOff%Map->size) + "x" + iToStr (iSrcOff/Map->size) + " to " + iToStr (iDestOff%Map->size) + "x" + iToStr (iDestOff/Map->size), cLog::eLOG_TYPE_NET_WARNING);
				// TODO: request sync of vehicle
				break;
			}

			cClientMoveJob *MoveJob = new cClientMoveJob ( iSrcOff, iDestOff, bPlane, Vehicle );
			if ( !MoveJob->generateFromMessage ( message ) ) break;
			Log.write(" Client: Added received movejob", cLog::eLOG_TYPE_NET_DEBUG);
		}
		break;
	case GAME_EV_NEXT_MOVE:
		{
			int iID = message->popInt16();
			int iDestOff = message->popInt16();
			int iType = message->popInt16();

			Log.write(" Client: Received information for next move: ID: " + iToStr ( iID ) + ", SrcX: " + iToStr( iDestOff%Map->size ) + ", SrcY: " + iToStr( iDestOff/Map->size ) + ", Type: " + iToStr ( iType ), cLog::eLOG_TYPE_NET_DEBUG);

			cVehicle *Vehicle = getVehicleFromID ( iID );
			if ( Vehicle && Vehicle->ClientMoveJob )
			{
				Vehicle->ClientMoveJob->handleNextMove( iDestOff%Map->size, iDestOff/Map->size, iType );
			}
			else
			{
				if ( Vehicle == NULL ) Log.write(" Client: Can't find vehicle with ID " + iToStr(iID), cLog::eLOG_TYPE_NET_WARNING);
				else Log.write(" Client: Vehicle with ID " + iToStr(iID) + "has no movejob", cLog::eLOG_TYPE_NET_WARNING);
				// TODO: request sync of vehicle
			}
		}
		break;
	case GAME_EV_ATTACKJOB_LOCK_TARGET:
		{
			cClientAttackJob::lockTarget( message );
		}
		break;
	case GAME_EV_ATTACKJOB_FIRE:
		{
			cClientAttackJob* job = new cClientAttackJob( message );
			Client->attackJobs.Add( job );
		}
		break;
	case GAME_EV_ATTACKJOB_IMPACT:
		{
			int attackMode = message->popInt16();
			int remainingHP = message->popInt16();
			int offset = message->popInt32();
			cClientAttackJob::makeImpact( offset, remainingHP, attackMode );
			break;
		}
	case GAME_EV_RESOURCES:
		{
			int iCount = message->popInt16();
			for ( int i = 0; i < iCount; i++ )
			{
				int iOff = message->popInt32();
				ActivePlayer->ResourceMap[iOff] = 1;

				Map->Resources[iOff].typ = (unsigned char)message->popInt16();
				Map->Resources[iOff].value = (unsigned char)message->popInt16();
			}
		}
		break;
	case GAME_EV_BUILD_ANSWER:
		{
			cVehicle *Vehicle;
			bool bOK = message->popBool();
			int iID = message->popInt16();

			Vehicle = getVehicleFromID ( iID );
			if ( Vehicle == NULL )
			{
				Log.write(" Client: Vehicle can't start building: Unknown vehicle with ID: "  + iToStr( iID ) , cLog::eLOG_TYPE_NET_WARNING);
				// TODO: Request sync of vehicle
				break;
			}

			if ( !bOK )
			{
				// TODO: translate
				if ( !Vehicle->BuildPath && Vehicle->owner == ActivePlayer ) addMessage ( "Buildposition is blocked" );
				Vehicle->BuildRounds = 0;
				Vehicle->BuildingTyp.iFirstPart = 0;
				Vehicle->BuildingTyp.iSecondPart = 0;
				Vehicle->BuildPath = false;
				Vehicle->BandX = 0;
				Vehicle->BandY = 0;
				break;
			}

			if ( Vehicle->IsBuilding ) Log.write(" Client: Vehicle is already building", cLog::eLOG_TYPE_NET_ERROR );

			int iBuildX = message->popInt16();
			int iBuildY = message->popInt16();
			bool bBuildBig = message->popBool();
			
			if ( bBuildBig )
			{
				Map->moveVehicleBig(Vehicle, iBuildX, iBuildY );
				Vehicle->owner->DoScan();

				Vehicle->BigBetonAlpha = 10;
			}
			else
			{
				Map->moveVehicle(Vehicle, iBuildX, iBuildY );
				Vehicle->owner->DoScan();
			}

			if ( Vehicle->owner == ActivePlayer )
			{
				Vehicle->BuildingTyp.iFirstPart = message->popInt16();
				Vehicle->BuildingTyp.iSecondPart = message->popInt16();
				Vehicle->BuildRounds = message->popInt16();
				Vehicle->BuildPath = message->popBool();
				Vehicle->BandX = message->popInt16();
				Vehicle->BandY = message->popInt16();	
			}

			Vehicle->IsBuilding = true;

			if ( Vehicle == SelectedVehicle )
			{
				StopFXLoop ( iObjectStream );
				iObjectStream = Vehicle->PlayStram();
			}

			if ( Vehicle->ClientMoveJob ) Vehicle->ClientMoveJob->release();
		}
		break;
	case GAME_EV_STOP_BUILD:
		{
			int iID = message->popInt16();
			cVehicle *Vehicle = getVehicleFromID ( iID );
			if ( Vehicle == NULL )
			{
				Log.write(" Client: Can't stop building: Unknown vehicle with ID: "  + iToStr( iID ) , cLog::eLOG_TYPE_NET_WARNING);
				// TODO: Request sync of vehicle
				break;
			}

			int iNewPos = message->popInt32();

			if ( Vehicle->data.can_build == BUILD_BIG )
			{
				Map->moveVehicle(Vehicle, iNewPos );
				Vehicle->owner->DoScan();
			}

			Vehicle->IsBuilding = false;
			Vehicle->BuildPath = false;

			if ( SelectedVehicle == Vehicle )
			{
				StopFXLoop ( iObjectStream );
				iObjectStream = Vehicle->PlayStram();
			}
		}
		break;
	case GAME_EV_NEW_SUBBASE:
		{
			sSubBase *NewSubBase;
			// generate new subbase
			NewSubBase = new sSubBase ( message->popInt16() );
			ActivePlayer->base.SubBases.Add( NewSubBase );
		}
		break;
	case GAME_EV_DELETE_SUBBASE:
		{
			int iID = message->popInt16();
			sSubBase *SubBase = NULL;
			for (unsigned int i = 0; i < ActivePlayer->base.SubBases.Size(); i++)
			{
				if (ActivePlayer->base.SubBases[i]->iID == iID)
				{
					SubBase = ActivePlayer->base.SubBases[i];
					ActivePlayer->base.SubBases.Delete ( i );
					break;
				}
			}
			if ( SubBase == NULL )
			{
				Log.write(" Client: Can't delete subbase: Unknown subbase with ID: "  + iToStr( iID ) , cLog::eLOG_TYPE_NET_WARNING);
				// TODO: Request sync of subbases
				break;
			}
			for (unsigned int i = 0; i < SubBase->buildings.Size(); i++)
			{
				SubBase->buildings[i]->SubBase = NULL;
			}
			delete SubBase;
		}
		break;
	case GAME_EV_SUBBASE_BUILDINGS:
		{
			int iID = message->popInt16();
			sSubBase *SubBase = getSubBaseFromID ( iID );
			if ( SubBase == NULL )
			{
				Log.write(" Client: Can't add buildings to subbase: Unknown subbase with ID: "  + iToStr( iID ) , cLog::eLOG_TYPE_NET_WARNING);
				// TODO: Request sync of subbases
				break;
			}
			int iCount = message->popInt16();
			for ( int i = 0; i < iCount; i++ )
			{
				int iBuildingID =  message->popInt16();
				cBuilding *Building = getBuildingFromID ( iBuildingID );
				if ( Building != NULL )
				{
					SubBase->buildings.Add ( Building );
					Building->SubBase = SubBase;

					Building->updateNeighbours( Map );
				}
			}
		}
		break;
	case GAME_EV_SUBBASE_VALUES:
		{
			int iID = message->popInt16();
			sSubBase *SubBase = getSubBaseFromID ( iID );
			if ( SubBase == NULL )
			{
				Log.write(" Client: Can't add subbase values: Unknown subbase with ID: "  + iToStr( iID ) , cLog::eLOG_TYPE_NET_WARNING);
				// TODO: Request sync of subbases
				break;
			}

			SubBase->HumanProd = message->popInt16();
			SubBase->MaxHumanNeed = message->popInt16();
			SubBase->HumanNeed = message->popInt16();
			SubBase->OilProd = message->popInt16();
			SubBase->MaxOilNeed = message->popInt16();
			SubBase->OilNeed = message->popInt16();
			SubBase->MaxOil = message->popInt16();
			SubBase->Oil = message->popInt16();
			SubBase->GoldProd = message->popInt16();
			SubBase->MaxGoldNeed = message->popInt16();
			SubBase->GoldNeed = message->popInt16();
			SubBase->MaxGold = message->popInt16();
			SubBase->Gold = message->popInt16();
			SubBase->MetalProd = message->popInt16();
			SubBase->MaxMetalNeed = message->popInt16();
			SubBase->MetalNeed = message->popInt16();
			SubBase->MaxMetal = message->popInt16();
			SubBase->Metal = message->popInt16();
			SubBase->MaxEnergyNeed  = message->popInt16();
			SubBase->MaxEnergyProd = message->popInt16();
			SubBase->EnergyNeed = message->popInt16();
			SubBase->EnergyProd = message->popInt16();

			if ( SelectedBuilding ) SelectedBuilding->ShowDetails();
		}
		break;
	case GAME_EV_BUILDLIST:
		{
			int iID = message->popInt16();
			cBuilding *Building = getBuildingFromID ( iID );
			if ( Building == NULL )
			{
				Log.write(" Client: Can't set buildlist: Unknown building with ID: "  + iToStr( iID ) , cLog::eLOG_TYPE_NET_WARNING);
				// TODO: Request sync of building
				break;
			}

			while (Building->BuildList->Size())
			{
				delete (*Building->BuildList)[0];
				Building->BuildList->Delete( 0 );
			}
			int iCount = message->popInt16();
			for ( int i = 0; i < iCount; i++ )
			{
				sBuildList *BuildListItem = new sBuildList;
				sID UnitID;
				UnitID.iFirstPart = message->popInt16();
				UnitID.iSecondPart = message->popInt16();
				BuildListItem->typ = UnitID.getVehicle();
				BuildListItem->metall_remaining = message->popInt16();
				Building->BuildList->Add ( BuildListItem );
			}

			Building->MetalPerRound = message->popInt16();
			Building->BuildSpeed = message->popInt16();
			Building->RepeatBuild = message->popBool();
		}
		break;
	case GAME_EV_PRODUCE_VALUES:
		{
			int iID = message->popInt16();
			cBuilding *Building = getBuildingFromID ( iID );
			if ( Building == NULL )
			{
				Log.write(" Client: Can't set produce values of building: Unknown building with ID: "  + iToStr( iID ) , cLog::eLOG_TYPE_NET_WARNING);
				// TODO: Request sync of building
				break;
			}

			Building->MetalProd = message->popInt16();
			Building->MaxMetalProd = message->popInt16();
			Building->OilProd = message->popInt16();
			Building->MaxOilProd = message->popInt16();
			Building->GoldProd = message->popInt16();
			Building->MaxGoldProd = message->popInt16();
		}
		break;
	case GAME_EV_TURN_REPORT:
		{
			string sReportMsg = "";
			string sTmp;
			int iCount = 0;

			int iReportAnz = message->popInt16();
			while ( iReportAnz )
			{
				sID Type;
				Type.iFirstPart = message->popInt16();
				Type.iSecondPart = message->popInt16();
				int iAnz = message->popInt16();
				if ( iCount ) sReportMsg += ", ";
				iCount += iAnz;
				sTmp = iToStr( iAnz ) + " " + Type.getUnitData()->name;
				sReportMsg += iAnz > 1 ? sTmp : Type.getUnitData()->name;
				iReportAnz--;
			}

			bool bFinishedResearch = message->popBool();
			if ( iCount == 0 )
			{
				if ( !bFinishedResearch ) PlayVoice ( VoiceData.VOIStartNone );
			}
			else if ( iCount == 1 )
			{
				sReportMsg += " " + lngPack.i18n( "Text~Comp~Finished") + ".";
				if ( !bFinishedResearch ) PlayVoice ( VoiceData.VOIStartOne );
			}
			else
			{
				sReportMsg += " " + lngPack.i18n( "Text~Comp~Finished2") + ".";
				if ( !bFinishedResearch ) PlayVoice ( VoiceData.VOIStartMore );
			}
			if ( bFinishedResearch ) PlayVoice ( VoiceData.VOIResearchComplete );

			addMessage( lngPack.i18n( "Text~Comp~Turn_Start") + " " + iToStr( iTurn ) );
			if ( sReportMsg.length() > 0 ) addMessage( sReportMsg.c_str() );

			//HACK SHOWFINISHEDPLAYERS reset finished turn for all players since a new turn started right now
			for ( unsigned int i = 0; i < PlayerList->Size(); i++ )
			{
				cPlayer *Player = (*Client->PlayerList)[i];
				if(Player)
				{
					Player->bFinishedTurn=false;
					Hud.ExtraPlayers(Player->name, GetColorNr(Player->color), i, Player->bFinishedTurn);
				}

			}
		}
		break;
	case GAME_EV_MARK_LOG:
		{
			Log.write("=============================================================================================", cLog::eLOG_TYPE_NET_DEBUG);
			Log.write( message->popString(), cLog::eLOG_TYPE_NET_DEBUG );
			Log.write("=============================================================================================", cLog::eLOG_TYPE_NET_DEBUG);
		}
		break;
	case GAME_EV_SUPPLY:
		{
			int iType = message->popChar ();
			if ( message->popBool () ) 
			{
				int iID = message->popInt16();
				cVehicle *DestVehicle = getVehicleFromID ( iID );
				if ( !DestVehicle )
				{
					Log.write(" Client: Can't supply vehicle: Unknown vehicle with ID: "  + iToStr( iID ) , cLog::eLOG_TYPE_NET_WARNING);
					// TODO: Request sync of vehicle
					break;
				}
				if ( iType == SUPPLY_TYPE_REARM ) DestVehicle->data.ammo = message->popInt16();
				else DestVehicle->data.hit_points = message->popInt16();
				if ( DestVehicle->Loaded )
				{
					// get the building which has loaded the unit
					cBuilding *Building = DestVehicle->owner->BuildingList;
					while ( Building )
					{
						bool found = false;
						for ( unsigned int i = 0; i < Building->StoredVehicles.Size(); i++ )
						{
							if ( Building->StoredVehicles[i] == DestVehicle )
							{
								found = true;
								break;
							}
						}
						if ( found ) break;
						Building = Building->next;
					}
					if ( Building != NULL )
					{
						Building->DrawStored ( Building->wantRedrawedStoredOffset );
						Building->ShowStorageMetalBar();
						SHOW_SCREEN
					}
				}
			}
			else
			{
				int iID = message->popInt16();
				cBuilding *DestBuilding = getBuildingFromID ( iID );
				if ( !DestBuilding )
				{
					Log.write(" Client: Can't supply building: Unknown building with ID: "  + iToStr( iID ) , cLog::eLOG_TYPE_NET_WARNING);
					// TODO: Request sync of building
					break;
				}
				if ( iType == SUPPLY_TYPE_REARM ) DestBuilding->data.ammo = message->popInt16();
				else DestBuilding->data.hit_points = message->popInt16();
			}
			if ( iType == SUPPLY_TYPE_REARM )
			{
				PlayVoice ( VoiceData.VOILoaded );
				PlayFX ( SoundData.SNDReload );
			}
			else
			{
				PlayVoice ( VoiceData.VOIRepaired );
				PlayFX ( SoundData.SNDRepair );
			}
		}
		break;
	case GAME_EV_ADD_RUBBLE:
		{
			cBuilding* rubble = new cBuilding( NULL, NULL, NULL );
			rubble->next = neutralBuildings;
			if ( neutralBuildings ) neutralBuildings->prev = rubble;
			neutralBuildings = rubble;
			rubble->prev = NULL;

			rubble->data.is_big = message->popBool();
			rubble->RubbleTyp = message->popInt16();
			rubble->RubbleValue = message->popInt16();
			rubble->iID = message->popInt16();
			rubble->PosY = message->popInt16();
			rubble->PosX = message->popInt16();
			
			Map->addBuilding( rubble, rubble->PosX, rubble->PosY);
		}
		break;
	case GAME_EV_DETECTION_STATE:
		{
			int id = message->popInt32();
			cVehicle* vehicle = getVehicleFromID( id );
			if ( vehicle == NULL )
			{
				Log.write(" Client: Vehicle (ID: " + iToStr(id) + ") not found", cLog::eLOG_TYPE_NET_ERROR);
				break;
			}
			bool detected = message->popBool();
			if ( detected )
			{
				//mark vehicle as detected with size of DetectedByPlayerList > 0
				vehicle->DetectedByPlayerList.Add(NULL);
			}
			else
			{
				while ( vehicle->DetectedByPlayerList.Size() > 0 ) vehicle->DetectedByPlayerList.Delete(0);
			}
		}
		break;
	case GAME_EV_CLEAR_ANSWER:
		{
			switch ( message->popInt16() )
			{
			case 0:
				{
					int id = message->popInt16();
					cVehicle *Vehicle = getVehicleFromID ( id );
					if ( Vehicle == NULL )
					{
						Log.write("Client: Can not find vehicle with id " + iToStr ( id ) + " for clearing", LOG_TYPE_NET_WARNING);
						break;
					}

					Vehicle->ClearingRounds = message->popInt16();
					int bigoffset = message->popInt16();
					if ( bigoffset >= 0 ) Map->moveVehicleBig ( Vehicle, bigoffset );
					Vehicle->IsClearing = true;

					if ( SelectedVehicle == Vehicle )
					{
						StopFXLoop( Client->iObjectStream );
						Client->iObjectStream = Vehicle->PlayStram();
					}
				}
				break;
			case 1:
				// TODO: add blocked message
				// addMessage ( "blocked" );
				break;
			case 2:
				Log.write("Client: warning on start of clearing", LOG_TYPE_NET_WARNING);
				break;
			default:
				break;
			}
		}
		break;
	case GAME_EV_STOP_CLEARING:
		{
			int id = message->popInt16();
			cVehicle *Vehicle = getVehicleFromID ( id );
			if ( Vehicle == NULL )
			{
				Log.write("Client: Can not find vehicle with id " + iToStr ( id ) + " for stop clearing", LOG_TYPE_NET_WARNING);
				break;
			}

			int bigoffset = message->popInt16();
			if ( bigoffset >= 0 ) Map->moveVehicle ( Vehicle, bigoffset );
			Vehicle->IsClearing = false;
			Vehicle->ClearingRounds = 0;

			if ( SelectedVehicle == Vehicle )
			{
				StopFXLoop( Client->iObjectStream );
				Client->iObjectStream = Vehicle->PlayStram();
			}
		}
		break;
	case GAME_EV_NOFOG:
		memset ( ActivePlayer->ScanMap, 1, Map->size*Map->size );
		break;
	case GAME_EV_DEFEATED:
		{
			int iTmp = message->popInt16();
			cPlayer *Player = getPlayerFromNumber ( iTmp );
			if ( Player == NULL )
			{
				Log.write ( "Client: Cannot find defeated player!", LOG_TYPE_NET_WARNING );
				break;
			}
			addMessage ( lngPack.i18n( "Text~Multiplayer~Player") + " " + Player->name + " " + lngPack.i18n( "Text~Comp~Defeated") );
			for ( unsigned int i = 0; i < PlayerList->Size(); i++ )
			{
				if ( Player == (*PlayerList)[i] )
				{
					Hud.ExtraPlayers(Player->name + " (d)", GetColorNr(Player->color), i, Player->bFinishedTurn, false);
					break;
				}
			}
		}
		break;
	case GAME_EV_FREEZE:
		bWaitForOthers = true;
		waitForOtherPlayer ( -1 );
		break;
	case GAME_EV_DEFREEZE:
		bWaitForOthers = false;
		break;
	case GAME_EV_DEL_PLAYER:
		{
			cPlayer *Player = getPlayerFromNumber ( message->popInt16() );
			if ( Player == ActivePlayer )
			{
				Log.write ( "Client: Cannot delete own player!", LOG_TYPE_NET_WARNING );
				break;
			}
			if ( Player->VehicleList || Player->BuildingList )
			{
				Log.write ( "Client: Player to be deleted has some units left !", LOG_TYPE_NET_ERROR );
			}
			addMessage ( Player->name + " has left the game." );
			for ( unsigned int i = 0; i < PlayerList->Size(); i++ )
			{
				if ( Player == (*PlayerList)[i] )
				{
					Hud.ExtraPlayers(Player->name + " (-)", GetColorNr(Player->color), i, false, false);
					delete (*PlayerList)[i];
					PlayerList->Delete ( i );
				}
			}
		}
		break;
	case GAME_EV_TURN:
		iTurn = message->popInt16();
		Hud.ShowRunde();
		break;
	case GAME_EV_HUD_SETTINGS:
		{
			int unitID = message->popInt16();
			cBuilding *building = NULL;
			cVehicle *vehicle = getVehicleFromID ( unitID );
			if ( !vehicle ) building = getBuildingFromID ( unitID );
			if ( vehicle || building )
			{
				if ( SelectedVehicle ) SelectedVehicle->Deselct();
				if ( SelectedBuilding ) SelectedBuilding->Deselct();
				if ( vehicle )
				{
					SelectedVehicle = vehicle;
					vehicle->Select();
					iObjectStream = SelectedVehicle->PlayStram();
				}
				if ( building )
				{
					SelectedBuilding = building;
					building->Select();
					iObjectStream = SelectedBuilding->PlayStram();
				}
			}
			Hud.OffX = message->popInt16();
			Hud.OffY = message->popInt16();
			Hud.SetZoom ( message->popInt16() );
			Hud.Farben = message->popBool();
			Hud.Gitter = message->popBool();
			Hud.Munition = message->popBool();
			Hud.Nebel = message->popBool();
			Hud.MinimapZoom = message->popBool();
			Hud.Reichweite = message->popBool();
			Hud.Scan = message->popBool();
			Hud.Status = message->popBool();
			Hud.Studie = message->popBool();
			Hud.Lock = message->popBool();
			Hud.Treffer = message->popBool();
			Hud.TNT = message->popBool();
			Hud.DoAllHud();
			bStartupHud = false;
		}
		break;
	case GAME_EV_STORE_UNIT:
		{
			cVehicle *StoredVehicle = getVehicleFromID ( message->popInt16() );
			if ( !StoredVehicle ) break;

			if ( message->popBool() )
			{
				cVehicle *StoringVehicle = getVehicleFromID ( message->popInt16() );
				if ( !StoringVehicle ) break;
				StoringVehicle->storeVehicle ( StoredVehicle, Map );

				if ( SelectedVehicle == StoringVehicle ) StoringVehicle->ShowDetails();

				if ( StoredVehicle == SelectedVehicle )
				{
					StoredVehicle->Deselct();
					SelectedVehicle = NULL;
				}
			}
			else
			{
				cBuilding *StoringBuilding = getBuildingFromID ( message->popInt16() );
				if ( !StoringBuilding ) break;
				StoringBuilding->storeVehicle ( StoredVehicle, Map );

				if ( SelectedBuilding == StoringBuilding ) StoringBuilding->ShowDetails();

				if ( StoredVehicle == SelectedVehicle )
				{
					StoredVehicle->Deselct();
					SelectedVehicle = NULL;
				}
			}
			PlayFX ( SoundData.SNDLoad );
		}
		break;
	case GAME_EV_EXIT_UNIT:
		{
			cVehicle *StoredVehicle = getVehicleFromID ( message->popInt16() );
			if ( !StoredVehicle ) break;

			if ( message->popBool() )
			{
				cVehicle *StoringVehicle = getVehicleFromID ( message->popInt16() );
				if ( !StoringVehicle ) break;

				int x = message->popInt16 ();
				int y = message->popInt16 ();
				StoringVehicle->exitVehicleTo ( StoredVehicle, x+y*Map->size, Map );
				if ( StoringVehicle == SelectedVehicle ) StoringVehicle->ShowDetails();
			}
			else
			{
				cBuilding *StoringBuilding = getBuildingFromID ( message->popInt16() );
				if ( !StoringBuilding ) break;

				int x = message->popInt16 ();
				int y = message->popInt16 ();
				StoringBuilding->exitVehicleTo ( StoredVehicle, x+y*Map->size, Map );
				if ( StoringBuilding == SelectedBuilding ) StoringBuilding->ShowDetails();
			}
			PlayFX ( SoundData.SNDActivate );
			mouseMoveCallback ( true );
		}
		break;
	case GAME_EV_DELETE_EVERYTHING:
		{
			SelectedBuilding = NULL;
			SelectedVehicle = NULL;
			for ( unsigned int i = 0; i < PlayerList->Size(); i++ )
			{
				cPlayer *const Player = (*PlayerList)[i];
				
				cVehicle *vehicle = Player->VehicleList;
				while ( vehicle )
				{
					if ( vehicle->StoredVehicles.Size() ) vehicle->DeleteStored();
					vehicle = vehicle->next;
				}

				while ( Player->VehicleList )
				{
					vehicle = Player->VehicleList->next;
					Player->VehicleList->bSentryStatus = false;
					Map->deleteVehicle ( Player->VehicleList );
					delete Player->VehicleList;
					Player->VehicleList = vehicle;
				}
				while ( Player->BuildingList )
				{
					cBuilding *building;
					building = Player->BuildingList->next;
					Player->BuildingList->bSentryStatus = false;


					while( Player->BuildingList->StoredVehicles.Size() > 0 )
					{
						Player->BuildingList->StoredVehicles.Delete( 0 );
					}

					Map->deleteBuilding ( Player->BuildingList );
					delete Player->BuildingList;
					Player->BuildingList = building;
				}
			}
			while ( neutralBuildings )
			{
				cBuilding* nextBuilding = neutralBuildings->next;
				Map->deleteBuilding( neutralBuildings );
				delete neutralBuildings;
				neutralBuildings = nextBuilding;
			}
		}
		break;
	case GAME_EV_UNIT_UPGRADE_VALUES:
		{
			sID ID;
			sUnitData *Data;
			ID.iFirstPart = message->popInt16();
			ID.iSecondPart = message->popInt16();
			Data = ID.getUnitData ( ActivePlayer );
			if ( Data != NULL )
			{
				Data->version = message->popInt16();
				Data->scan = message->popInt16();
				Data->range = message->popInt16();
				Data->damage = message->popInt16();
				Data->costs = message->popInt16();
				Data->armor = message->popInt16();
				Data->max_speed = message->popInt16();
				Data->max_shots = message->popInt16();
				Data->max_ammo = message->popInt16();
				Data->max_hit_points = message->popInt16();
			}
		}
		break;
	case GAME_EV_CREDITS_CHANGED:
		{
			ActivePlayer->Credits = message->popInt16();
		}
		break;
	case GAME_EV_UPGRADED_BUILDINGS:
		{
			int buildingsInMsg = message->popInt16();
			int totalCosts = message->popInt16();
			if (buildingsInMsg > 0)
			{
				string buildingName;
				bool scanNecessary = false;
				for (int i = 0; i < buildingsInMsg; i++)
				{
					int buildingID = message->popInt32();
					cBuilding* building = getBuildingFromID(buildingID);
					if (!scanNecessary && building->data.scan < ActivePlayer->BuildingData[building->typ->nr].scan)
						scanNecessary = true; // Scan range was upgraded. So trigger a scan.
					building->upgradeToCurrentVersion();
					if (i == 0)
					{
						buildingName = building->data.szName;
						if (buildingsInMsg > 1)
							buildingName.append(1,'s');
					}
				}
				ostringstream os;
				os << "Upgraded " << buildingsInMsg << " " << buildingName << " for " << totalCosts << " raw materials"; // TODO: translated? check original 
				string printStr(os.str());
				addMessage (printStr);
				if (scanNecessary)
					ActivePlayer->DoScan(); 
			}
		}
		break;
	case GAME_EV_UPGRADED_VEHICLES:
		{
			int vehiclesInMsg = message->popInt16();
			int totalCosts = message->popInt16();
			unsigned int storingBuildingID = message->popInt32();
			if (vehiclesInMsg > 0)
			{
				string vehicleName;
				for (int i = 0; i < vehiclesInMsg; i++)
				{
					int vehicleID = message->popInt32();
					cVehicle* vehicle = getVehicleFromID(vehicleID);
					vehicle->upgradeToCurrentVersion();
					if (i == 0)
					{
						vehicleName = vehicle->data.szName;
						if (vehiclesInMsg > 1)
							vehicleName = "units"; // TODO: translated
					}
				}
				cBuilding* storingBuilding = getBuildingFromID(storingBuildingID);
				if (storingBuilding && Client->isInMenu)
				{
					storingBuilding->DrawStored(0);
					storingBuilding->ShowStorageMetalBar();
					SHOW_SCREEN
				}
				ostringstream os;
				os << "Upgraded " << vehiclesInMsg << " " << vehicleName << " for " << totalCosts << " raw materials"; // TODO: translated? check original 
				string printStr(os.str());
				addMessage (printStr);
			}
		}
		break;
	case GAME_EV_SET_AUTOMOVE:
		{
			cVehicle *Vehicle = getVehicleFromID ( message->popInt16() );
			if ( Vehicle )
			{
				if ( Vehicle->autoMJob )
				{
					delete Vehicle->autoMJob;
					Vehicle->autoMJob = NULL;
				}
				Vehicle->autoMJob = new cAutoMJob ( Vehicle );
			}
		}
		break;
	case GAME_EV_COMMANDO_ANSWER:
		{
			if ( message->popBool() )
			{
				if ( message->popBool() ) PlayVoice ( VoiceData.VOIUnitStolen );
				else PlayVoice ( VoiceData.VOIUnitDisabled );
			}
			else PlayVoice ( VoiceData.VOICommandoDetected );
			cVehicle *srcUnit = getVehicleFromID ( message->popInt16() );
			if ( srcUnit && !srcUnit->data.shots && srcUnit == SelectedVehicle )
			{
				srcUnit->DisableActive = false;
				srcUnit->StealActive = false;
			}
		}
		break;
	default:
		Log.write("Client: Can not handle message type " + message->getTypeAsString(), cLog::eLOG_TYPE_NET_ERROR);
		break;
	}

	CHECK_MEMORY;
	return 0;
}

void cClient::addUnit( int iPosX, int iPosY, cVehicle *AddedVehicle, bool bInit, bool bAddToMap )
{
	// place the vehicle
	if ( bAddToMap ) Map->addVehicle( AddedVehicle, iPosX, iPosY );

	if ( !bInit ) AddedVehicle->StartUp = 10;

	mouseMoveCallback(true);
}

void cClient::addUnit( int iPosX, int iPosY, cBuilding *AddedBuilding, bool bInit )
{
	// place the building
	Map->addBuilding( AddedBuilding, iPosX, iPosY);

	if ( !bInit ) AddedBuilding->StartUp = 10;

	mouseMoveCallback(true);
}

cPlayer *cClient::getPlayerFromNumber ( int iNum )
{
	for ( unsigned int i = 0; i < PlayerList->Size(); i++)
	{
		cPlayer* const p = (*PlayerList)[i];
		if (p->Nr == iNum) return p;
	}
	return NULL;
}

void cClient::deleteUnit( cBuilding *Building )
{
	if( !Building ) return;
	bFlagDrawMMap = true;

	Map->deleteBuilding( Building );

	if ( !Building->owner )
	{
		if ( !Building->prev )
		{
			neutralBuildings = Building->next;
			if ( Building->next )
				Building->next->prev = NULL;
		}
		else
		{
			Building->prev->next = Building->next;
			if ( Building->next )
				Building->next->prev = Building->prev;
		}
		delete Building;
		return;
	}

	for ( unsigned int i = 0; i < attackJobs.Size(); i++)
	{
		if ( attackJobs[i]->building == Building )
		{
			attackJobs[i]->state = cClientAttackJob::FINISHED;
		}
	}

	if( Building->prev )
	{
		Building->prev->next = Building->next;
		if( Building->next )
		{
			Building->next->prev = Building->prev;
		}
	}
	else
	{
		Building->owner->BuildingList = Building->next;
		if( Building->next )
		{
			Building->next->prev = NULL;
		}
	}

	if ( SelectedBuilding == Building )
	{
		Building->Deselct();
		SelectedBuilding = NULL;
	}

	cPlayer* owner = Building->owner;
	delete Building;

	owner->DoScan();

}

void cClient::deleteUnit( cVehicle *Vehicle )
{
	if( !Vehicle ) return;

	Map->deleteVehicle( Vehicle );

	for ( unsigned int i = 0; i < attackJobs.Size(); i++)
	{
		if ( attackJobs[i]->vehicle == Vehicle )
		{
			attackJobs[i]->state = cClientAttackJob::FINISHED;
		}
	}

	bFlagDrawMMap = true;

	if( Vehicle->prev )
	{
		Vehicle->prev->next = Vehicle->next;
		if( Vehicle->next )
		{
			Vehicle->next->prev = Vehicle->prev;
		}
	}
	else
	{
		Vehicle->owner->VehicleList = Vehicle->next;
		if( Vehicle->next )
		{
			Vehicle->next->prev = NULL;
		}
	}

	if ( SelectedVehicle == Vehicle )
	{
		Vehicle->Deselct();
		SelectedVehicle = NULL;
	}

	cPlayer* owner = Vehicle->owner;
	delete Vehicle;

	if ( owner ) owner->DoScan();
}

void cClient::handleEnd()
{
	if ( bWaitForOthers ) return;
	bWantToEnd = true;
	sendWantToEndTurn();	
}

void cClient::makeHotSeatEnd( int iNextPlayerNum )
{
	// clear the messages
	sMessage *Message;
	while (messages.Size())
	{
		Message = messages[0];
		delete Message;
		messages.Delete ( 0 );
	}

	// save information and set next player
	int iZoom, iX, iY;
	ActivePlayer->HotHud = Hud;
	iZoom = Hud.LastZoom;
	ActivePlayer = getPlayerFromNumber( iNextPlayerNum );	// TODO: maybe here must be done more than just set the next player!
	Hud = ActivePlayer->HotHud;
	iX = Hud.OffX;
	iY = Hud.OffY;
	if ( Hud.LastZoom != iZoom )
	{
		Hud.LastZoom = -1;
		Hud.ScaleSurfaces();
	}
	Hud.DoAllHud();
	Hud.EndeButton ( false );
	Hud.OffX = iX;
	Hud.OffY = iY;

	// reset the screen
	if ( SelectedBuilding ) { SelectedBuilding->Deselct(); SelectedBuilding = NULL; }
	if ( SelectedVehicle ) { SelectedVehicle->Deselct(); SelectedVehicle = NULL; }
	SDL_Surface *sf;
	SDL_Rect scr;
	sf=SDL_CreateRGBSurface ( SDL_SRCCOLORKEY,SettingsData.iScreenW,SettingsData.iScreenH,32,0,0,0,0 );
	scr.x=15;
	scr.y=356;
	scr.w=scr.h=112;
	SDL_BlitSurface ( sf,NULL,buffer,NULL );
	SDL_BlitSurface ( GraphicsData.gfx_hud,NULL,buffer,NULL );
	SDL_BlitSurface ( sf,&scr,buffer,&scr );

	isInMenu = true;
	ShowOK ( ActivePlayer->name + lngPack.i18n( "Text~Multiplayer~Player_Turn"), true );
	isInMenu = false;
}

void cClient::waitForOtherPlayer( int iPlayerNum, bool bStartup )
{
	if ( !bWaitForOthers ) return;
	int iLastX = -1, iLastY = -1;
	bool aborted = false;
	isInMenu = false;

	while ( bWaitForOthers )
	{
		EventHandler->HandleEvents();

		// check mouse moves 
		mouse->GetPos();
		Hud.CheckMouseOver();
		Hud.CheckScroll();
		// check minimap
		if ( clientMouseState.leftButtonPressed && !bHelpActive ) Hud.CheckOneClick();
		// check length of chat string
		if ( bChatInput && InputHandler->checkHasBeenInput () ) InputHandler->cutToLength ( PACKAGE_LENGTH-20 );

		// check exit
		if ( bExit )
		{
			bExit = true;
			bWaitForOthers = false;
			break;
		}

		// draw the map:
		if ( bFlagDrawMap )
		{
			drawMap();
			displayFX();
		}
		drawUnitCircles ();
		displayDebugOutput();

		// draw the minimap:
		if ( bFlagDrawMMap )
		{
			bFlagDrawMMap = false;
			drawMiniMap();
			bFlagDrawHud = true;
		}
		// check whether the hud has to be drawn:
		if ( bFlagDrawHud || bFlagDrawMap )
		{
			SDL_BlitSurface ( GraphicsData.gfx_hud, NULL, buffer, NULL );
			// draw closed panel on startup
			if ( bStartup )
			{
				SDL_Rect Top, Bottom;
				Top.x = 0;
				Top.y = ( SettingsData.iScreenH/2 ) - 479;
				Top.h = 479;
				Top.w = Bottom.w = 171;
				Bottom.h = 481;
				Bottom.x = 0;
				Bottom.y = ( SettingsData.iScreenH/2 );
				SDL_BlitSurface ( GraphicsData.gfx_panel_top ,NULL, buffer, &Top );
				SDL_BlitSurface ( GraphicsData.gfx_panel_bottom, NULL, buffer, &Bottom );
			}
			mouse->GetBack ( buffer );
			bFlagDraw = true;
		}
		// draw the video:
		if ( bFlagDraw || bFlagDrawHud )
		{
			drawFLC();
		}
		// display the chatinput:
		if ( bChatInput && bFlagDrawMap )
		{
			displayChatInput();
		}
		// display the messages:
		if ( bFlagDrawMap )
		{
			handleMessages();
		}
		// display waiting text
		if ( iPlayerNum != -1 ) font->showTextCentered( 320, 235, lngPack.i18n ( "Text~Multiplayer~Wait_Until", getPlayerFromNumber( iPlayerNum )->name ), FONT_LATIN_BIG );
		else
		{
			font->showTextCentered( 320, 235, lngPack.i18n ( "Text~Multiplayer~Wait_Reconnect" ), FONT_LATIN_BIG );
			font->showTextCentered( 320, 235+font->getFontHeight(FONT_LATIN_BIG), lngPack.i18n ( "Text~Multiplayer~Abort_Waiting" ), FONT_LATIN_NORMAL );

			Uint8 *keystate;
			keystate = SDL_GetKeyState( NULL );
			if ( keystate[SDLK_F2] && !aborted )
			{
				sendAbortWaiting ();
				aborted = true;
			}
		}
		// draw the mouse
		if ( mouse->x != iLastX || mouse->y != iLastY || bFlagDraw )
		{
			if ( bFlagDraw ) mouse->draw ( false, buffer );
			else mouse->draw ( true, screen );
			iLastX = mouse->x;
			iLastY = mouse->y;
		}
		// display the buffer:
		if ( bFlagDraw )
		{
			SHOW_SCREEN
			bFlagDraw = false;
			bFlagDrawHud = false;
			bFlagDrawMap = false;
		}
		else if ( !SettingsData.bFastMode )
		{
			SDL_Delay ( 10 ); // theres northing to do.
		}

		doGameActions();
		if ( iTimer1 )
		{
			iFrame++;
			bFlagDrawMap = true;
			rotateBlinkColor();
			if ( FLC != NULL && Hud.PlayFLC )
			{
				FLI_NextFrame ( FLC );
			}
		}
		handleTurnTime();
	}
}

void cClient::handleTurnTime()
{
	static int lastCheckTime = SDL_GetTicks();
	if ( !iTimer0 ) return;
	// stop time when waiting for reconnection
	if ( bWaitForOthers  )
	{
		iStartTurnTime += SDL_GetTicks()-lastCheckTime;
	}
	if ( iTurnTime > 0 )
	{
		int iRestTime = iTurnTime - Round( ( SDL_GetTicks() - iStartTurnTime )/1000 );
		if ( iRestTime < 0 ) iRestTime = 0;
		Hud.showTurnTime ( iRestTime );
	}
	lastCheckTime = SDL_GetTicks();
}

void cClient::addActiveMoveJob ( cClientMoveJob *MoveJob )
{
	ActiveMJobs.Add ( MoveJob );
	MoveJob->bSuspended = false;
}

void cClient::handleMoveJobs ()
{
	for (unsigned int i = 0; i < ActiveMJobs.Size(); i++)
	{
		cClientMoveJob *MoveJob;
		cVehicle *Vehicle;

		MoveJob = ActiveMJobs[i];
		Vehicle = MoveJob->Vehicle;
		
		//suspend movejobs of attacked vehicles 
		if ( Vehicle && Vehicle->bIsBeeingAttacked ) continue; 

		if ( MoveJob->bFinished || MoveJob->bEndForNow )
		{
			// Stop the soundstream
			if ( Vehicle && Vehicle == SelectedVehicle && Vehicle->MoveJobActive )
			{
				StopFXLoop ( iObjectStream );
				if ( Map->IsWater ( Vehicle->PosX+Vehicle->PosY*Map->size ) && Vehicle->data.can_drive != DRIVE_AIR ) PlayFX ( Vehicle->typ->StopWater );
				else PlayFX ( Vehicle->typ->Stop );
				iObjectStream = Vehicle->PlayStram();
			}
		}

		if ( MoveJob->bFinished )
		{
			if ( Vehicle && Vehicle->ClientMoveJob == MoveJob )
			{
				Log.write(" Client: Movejob is finished and will be deleted now", cLog::eLOG_TYPE_NET_DEBUG);
				Vehicle->ClientMoveJob = NULL;
				Vehicle->moving = false;
				Vehicle->MoveJobActive = false;

			}
			else Log.write(" Client: Delete movejob with nonactive vehicle (released one)", cLog::eLOG_TYPE_NET_DEBUG);
			ActiveMJobs.Delete ( i );
			delete MoveJob;
			continue;
		}
		if ( MoveJob->bEndForNow )
		{
			Log.write(" Client: Movejob has end for now and will be stoped (delete from active ones)", cLog::eLOG_TYPE_NET_DEBUG);
			if ( Vehicle )
			{
				Vehicle->MoveJobActive = false;
				Vehicle->moving = false;
				// save speed
				if ( MoveJob->Waypoints && MoveJob->Waypoints->next )
				{
					if ( Vehicle->data.speed < MoveJob->Waypoints->next->Costs )
					{
						MoveJob->iSavedSpeed += Vehicle->data.speed;
						Vehicle->data.speed = 0;
						if ( Vehicle == SelectedVehicle ) Vehicle->ShowDetails();
					}
				}
				else
				{
					Log.write(" Client: movejob has end for now but has no waypoints anymore", cLog::eLOG_TYPE_NET_WARNING);
				}
			}
			ActiveMJobs.Delete ( i );
			continue;
		}

		if ( Vehicle == NULL ) continue;

		// rotate vehicle
		if ( MoveJob->iNextDir != Vehicle->dir && Vehicle->data.speed )
		{
			if ( iTimer1 )
			{
				Vehicle->RotateTo ( MoveJob->iNextDir );
			}
			continue;
		}

		if ( Vehicle->MoveJobActive )
		{
			MoveJob->moveVehicle();
		}
	}
}

cVehicle *cClient::getVehicleFromID ( int iID )
{
	cVehicle *Vehicle;
	for (unsigned int i = 0; i < PlayerList->Size(); i++)
	{
		Vehicle = (*PlayerList)[i]->VehicleList;
		while ( Vehicle )
		{
			if ( Vehicle->iID == iID ) return Vehicle;
			Vehicle = Vehicle->next;
		}
	}
	return NULL;
}

cBuilding *cClient::getBuildingFromID ( int iID )
{
	cBuilding *Building;
	for (unsigned int i = 0; i < PlayerList->Size(); i++)
	{
		Building = (*PlayerList)[i]->BuildingList;
		while ( Building )
		{
			if ( Building->iID == iID ) return Building;
			Building = Building->next;
		}
	}

	Building = neutralBuildings;
	while ( Building )
	{
		if ( Building->iID == iID ) return Building;
		Building = Building->next;
	}

	return NULL;
}

void cClient::trace ()
{
	int iY, iX;
	cMapField* field;

	mouse->GetKachel ( &iX, &iY );
	if ( iX < 0 || iY < 0 ) return;
	
	if ( bDebugTraceServer ) field = Server->Map->fields + ( Server->Map->size*iY+iX );
	else field = Map->fields + ( Map->size*iY+iX );

	iY = 18+5+8;
	iX = 180+5;

	if ( field->getVehicles() ) { traceVehicle ( field->getVehicles(), &iY, iX ); iY += 20; }
	if ( field->getPlanes() ) { traceVehicle ( field->getPlanes(), &iY, iX ); iY += 20; }
	cBuildingIterator bi = field->getBuildings();
	while ( !bi.end ) { traceBuilding ( bi, &iY, iX ); iY += 20; bi++;}
}

void cClient::traceVehicle ( cVehicle *Vehicle, int *iY, int iX )
{
	string sTmp;

	sTmp = "name: \"" + Vehicle->name + "\" id: \"" + iToStr ( Vehicle->iID ) + "\" owner: \"" + Vehicle->owner->name + "\" posX: +" + iToStr ( Vehicle->PosX ) + " posY: " + iToStr ( Vehicle->PosY ) + " offX: " + iToStr ( Vehicle->OffX ) + " offY: " + iToStr ( Vehicle->OffY );
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	sTmp = "dir: " + iToStr ( Vehicle->dir ) + " selected: " + iToStr ( Vehicle->selected ) + " moving: +" + iToStr ( Vehicle->moving ) + " mjob: "  + pToStr ( Vehicle->ClientMoveJob ) + " speed: " + iToStr ( Vehicle->data.speed ) + " mj_active: " + iToStr ( Vehicle->MoveJobActive ) + " menu_active: " + iToStr ( Vehicle->MenuActive );
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	sTmp = "attack_mode: " + iToStr ( Vehicle->AttackMode ) + " attacking: " + iToStr ( Vehicle->Attacking ) + " on sentry: +" + iToStr ( Vehicle->bSentryStatus ) + " transfer: " + iToStr ( Vehicle->Transfer ) + " ditherx: " + iToStr (Vehicle->ditherX ) + " dithery: " + iToStr ( Vehicle->ditherY );
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	sTmp = "is_building: " + iToStr ( Vehicle->IsBuilding ) + " building_typ: " + Vehicle->BuildingTyp.getText() + " build_costs: +" + iToStr ( Vehicle->BuildCosts ) + " build_rounds: " + iToStr ( Vehicle->BuildRounds ) + " build_round_start: " + iToStr (Vehicle->BuildRoundsStart );
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	sTmp = "place_band: " + iToStr ( Vehicle->PlaceBand ) + " bandx: " + iToStr ( Vehicle->BandX ) + " bandy: +" + iToStr ( Vehicle->BandY ) + " build_big_saved_pos: " + iToStr ( Vehicle->BuildBigSavedPos ) + " build_path: " + iToStr (Vehicle->BuildPath );
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	sTmp = " is_clearing: " + iToStr ( Vehicle->IsClearing ) + " clearing_rounds: +" + iToStr ( Vehicle->ClearingRounds ) + " clear_big: " + iToStr ( Vehicle->data.is_big ) + " loaded: " + iToStr (Vehicle->Loaded );
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	sTmp = "commando_rank: " + dToStr ( Round ( Vehicle->CommandoRank, 2 ) ) + " steal_active: " + iToStr ( Vehicle->StealActive ) + " disable_active: +" + iToStr ( Vehicle->DisableActive ) + " disabled: " + iToStr ( Vehicle->Disabled ) /*+ " detection_override: " + iToStr (Vehicle->detection_override )*/;
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	sTmp = "is_locked: " + iToStr ( Vehicle->IsLocked ) + /*" detected: " + iToStr ( Vehicle->detected ) +*/ " clear_mines: +" + iToStr ( Vehicle->ClearMines ) + " lay_mines: " + iToStr ( Vehicle->LayMines ) + " repair_active: " + iToStr (Vehicle->RepairActive ) + " muni_active: " + iToStr (Vehicle->MuniActive );
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	sTmp =
		"load_active: "            + iToStr(Vehicle->LoadActive) +
		" activating_vehicle: "    + iToStr(Vehicle->ActivatingVehicle) +
		" vehicle_to_activate: +"  + iToStr(Vehicle->VehicleToActivate) +
		" stored_vehicles_count: " + iToStr((int)Vehicle->StoredVehicles.Size());
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	if ( Vehicle->StoredVehicles.Size() )
	{
		cVehicle *StoredVehicle;
		for (unsigned int i = 0; i < Vehicle->StoredVehicles.Size(); i++)
		{
			StoredVehicle = Vehicle->StoredVehicles[i];
			font->showText(iX, *iY, " store " + iToStr(i)+": \""+StoredVehicle->name+"\"", FONT_LATIN_SMALL_WHITE);
			*iY += 8;
		}
	}

	if ( bDebugTraceServer )
	{
		sTmp = "seen by players: owner";
		for (unsigned int i = 0; i < Vehicle->SeenByPlayerList.Size(); i++)
		{
			sTmp += ", \"" + Vehicle->SeenByPlayerList[i]->name + "\"";
		}
		font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
		*iY+=8;
	}
}

void cClient::traceBuilding ( cBuilding *Building, int *iY, int iX )
{
	string sTmp;

	sTmp = "name: \"" + Building->name + "\" id: \"" + iToStr ( Building->iID ) + "\" owner: \"" + ( Building->owner?Building->owner->name:"<null>" ) + "\" posX: +" + iToStr ( Building->PosX ) + " posY: " + iToStr ( Building->PosY ) + " selected: " + iToStr ( Building->selected );
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	sTmp = "dir: " + iToStr ( Building->dir ) + " menu_active: " + iToStr ( Building->MenuActive ) + " on sentry: +" + iToStr ( Building->bSentryStatus ) + " attacking_mode: +" + iToStr ( Building->AttackMode ) + " base: " + pToStr ( Building->base ) + " sub_base: " + pToStr (Building->SubBase );
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	sTmp = "attacking: " + iToStr ( Building->Attacking ) + " UnitsData.dirt_typ: " + iToStr ( Building->RubbleTyp ) + " UnitsData.dirt_value: +" + iToStr ( Building->RubbleValue ) + " big_dirt: " + iToStr ( Building->data.is_big ) + " is_working: " + iToStr (Building->IsWorking ) + " transfer: " + iToStr (Building->Transfer );
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	sTmp = "metal_prod: " + iToStr ( Building->MetalProd ) + " oil_prod: " + iToStr ( Building->OilProd ) + " gold_prod: +" + iToStr ( Building->GoldProd ) + " max_metal_p: " + iToStr ( Building->MaxMetalProd ) + " max_oil_p: " + iToStr (Building->MaxOilProd ) + " max_gold_p: " + iToStr (Building->MaxGoldProd );
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	sTmp = "is_locked: " + iToStr ( Building->IsLocked ) + " disabled: " + iToStr ( Building->Disabled ) /*+ " detected: +" + iToStr ( Building->detected )*/ + " activating_vehicle: " + iToStr ( Building->ActivatingVehicle ) + " vehicle_to_activate: " + iToStr (Building->VehicleToActivate );
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	sTmp =
		"load_active: "            + iToStr(Building->LoadActive) +
		" stored_vehicles_count: " + iToStr((int)Building->StoredVehicles.Size());
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	if (Building->StoredVehicles.Size())
	{
		cVehicle *StoredVehicle;
		for (unsigned int i = 0; i < Building->StoredVehicles.Size(); i++)
		{
			StoredVehicle = Building->StoredVehicles[i];
			font->showText(iX, *iY, " store " + iToStr(i)+": \""+StoredVehicle->name+"\"", FONT_LATIN_SMALL_WHITE);
			*iY+=8;
		}
	}

	sTmp =
		"build_speed: "        + iToStr(Building->BuildSpeed)  +
		" repeat_build: "      + iToStr(Building->RepeatBuild) +
		" build_list_count: +" + iToStr(Building->BuildList ? (int)Building->BuildList->Size() : 0);
	font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
	*iY+=8;

	if (Building->BuildList && Building->BuildList->Size())
	{
		sBuildList *BuildingList;
		for (unsigned int i = 0; i < Building->BuildList->Size(); i++)
		{
			BuildingList = (*Building->BuildList)[i];
			font->showText(iX, *iY, "  build "+iToStr(i)+": "+iToStr(BuildingList->typ->nr)+" \""+UnitsData.vehicle[BuildingList->typ->nr].data.name+"\"", FONT_LATIN_SMALL_WHITE);
			*iY+=8;
		}
	}

	if ( bDebugTraceServer )
	{
		sTmp = "seen by players: owner";
		for (unsigned int i = 0; i < Building->SeenByPlayerList.Size(); i++)
		{
			sTmp += ", \"" + Building->SeenByPlayerList[i]->name + "\"";
		}
		font->showText(iX,*iY, sTmp, FONT_LATIN_SMALL_WHITE);
		*iY+=8;
	}
}

void cClient::doGameActions()
{
	handleTimer();
	if ( !iTimer0 ) return;

	//run attackJobs
	cClientAttackJob::handleAttackJobs();
	//run moveJobs - this has to be called before handling the auto movejobs
	handleMoveJobs();
	//run surveyor ai
	cAutoMJob::handleAutoMoveJobs();
	//run effects
	runFX();
}

sSubBase *cClient::getSubBaseFromID ( int iID )
{
	sSubBase *SubBase = NULL;
	for (unsigned int i = 0; i < ActivePlayer->base.SubBases.Size(); i++)
	{
		if (ActivePlayer->base.SubBases[i]->iID == iID)
		{
			SubBase = ActivePlayer->base.SubBases[i];
			break;
		}
	}
	return SubBase;
}

void cClient::showTransfer( cBuilding *SrcBuilding, cVehicle *SrcVehicle, cBuilding *DestBuilding, cVehicle *DestVehicle )
{
	if ( ( SrcBuilding == NULL && SrcVehicle == NULL ) || ( DestBuilding == NULL && DestVehicle == NULL ) ) return;
	int LastMouseX = 0, LastMouseY = 0, LastB = 0, x, y, b;
	SDL_Rect scr, dest;
	bool IncPressed = false;
	bool DecPressed = false;
	bool MouseHot = false;
	int iMaxDestCargo, iDestCargo;
	int iTransf = 0;
	SDL_Surface *img;
	isInMenu = true;

	mouse->SetCursor ( CHand );
	mouse->draw ( false, buffer );
	Client->drawMap();
	SDL_BlitSurface ( GraphicsData.gfx_hud, NULL, buffer, NULL );

	if ( SettingsData.bAlphaEffects ) SDL_BlitSurface ( GraphicsData.gfx_shadow, NULL, buffer, NULL );

	dest.x = 166;
	dest.y = 159;
	SDL_BlitSurface ( GraphicsData.gfx_transfer, NULL, buffer, &dest );

	// create the images
	if ( SrcBuilding != NULL )
	{
		if ( SrcBuilding->data.is_big ) scaleSurface ( SrcBuilding->typ->img_org, SrcBuilding->typ->img, SrcBuilding->typ->img_org->w / 4, SrcBuilding->typ->img_org->h / 4 );
		else scaleSurface ( SrcBuilding->typ->img_org, SrcBuilding->typ->img, SrcBuilding->typ->img_org->w / 2, SrcBuilding->typ->img_org->h / 2 );

		img = SDL_CreateRGBSurface ( SDL_SRCCOLORKEY, SrcBuilding->typ->img->w, SrcBuilding->typ->img->h, 32, 0, 0, 0, 0 );
	}
	else
	{
		scaleSurface ( SrcVehicle->typ->img_org[0], SrcVehicle->typ->img[0], SrcVehicle->typ->img_org[0]->w / 2, SrcVehicle->typ->img_org[0]->h / 2 );
		img = SDL_CreateRGBSurface ( SDL_SRCCOLORKEY, SrcVehicle->typ->img[0]->w, SrcVehicle->typ->img[0]->h, 32, 0, 0, 0, 0 );
	}

	SDL_SetColorKey ( img, SDL_SRCCOLORKEY, 0xFF00FF );
	if ( SrcBuilding != NULL )
	{
		SDL_BlitSurface ( SrcBuilding->owner->color, NULL, img, NULL );
		SDL_BlitSurface ( SrcBuilding->typ->img, NULL, img, NULL );
	}
	else
	{
		SDL_BlitSurface ( SrcVehicle->owner->color, NULL, img, NULL );
		SDL_BlitSurface ( SrcVehicle->typ->img[0], NULL, img, NULL );
	}
	dest.x = 88 + 166;
	dest.y = 20 + 159;
	SDL_BlitSurface ( img, NULL, buffer, &dest );
	SDL_FreeSurface ( img );

	if ( DestBuilding )
	{
		if ( DestBuilding->data.is_big )
		{
			scaleSurface ( DestBuilding->typ->img_org, DestBuilding->typ->img, DestBuilding->typ->img_org->w / 4, DestBuilding->typ->img_org->h / 4 );
			img = SDL_CreateRGBSurface ( SDL_SRCCOLORKEY, DestBuilding->typ->img->w, DestBuilding->typ->img->h, 32, 0, 0, 0, 0 );
			SDL_SetColorKey ( img, SDL_SRCCOLORKEY, 0xFF00FF );
			SDL_BlitSurface ( DestBuilding->owner->color, NULL, img, NULL );
			SDL_BlitSurface ( DestBuilding->typ->img, NULL, img, NULL );
		}
		else
		{
			scaleSurface ( DestBuilding->typ->img_org, DestBuilding->typ->img, DestBuilding->typ->img_org->w / 2, DestBuilding->typ->img_org->h / 2 );

			if ( DestBuilding->data.has_frames || DestBuilding->data.is_connector )
			{
				DestBuilding->typ->img->h = DestBuilding->typ->img->w = 32;
			}

			img = SDL_CreateRGBSurface ( SDL_SRCCOLORKEY, DestBuilding->typ->img->w, DestBuilding->typ->img->h, 32, 0, 0, 0, 0 );
			SDL_SetColorKey ( img, SDL_SRCCOLORKEY, 0xFF00FF );

			if ( !DestBuilding->data.is_connector )
			{
				SDL_BlitSurface ( DestBuilding->owner->color, NULL, img, NULL );
			}
			SDL_BlitSurface ( DestBuilding->typ->img, NULL, img, NULL );
		}
	}
	else
	{
		scaleSurface ( DestVehicle->typ->img_org[0], DestVehicle->typ->img[0], DestVehicle->typ->img_org[0]->w / 2, DestVehicle->typ->img_org[0]->h / 2 );
		img = SDL_CreateRGBSurface ( SDL_SRCCOLORKEY, DestVehicle->typ->img[0]->w, DestVehicle->typ->img[0]->h, 32, 0, 0, 0, 0 );
		SDL_SetColorKey ( img, SDL_SRCCOLORKEY, 0xFF00FF );
		SDL_BlitSurface ( DestVehicle->owner->color, NULL, img, NULL );
		SDL_BlitSurface ( DestVehicle->typ->img[0], NULL, img, NULL );
	}

	dest.x = 192 + 166;
	dest.y = 20 + 159;
	SDL_BlitSurface ( img, NULL, buffer, &dest );
	SDL_FreeSurface ( img );

	// show texts:
	if ( SrcBuilding ) font->showTextCentered ( 102 + 166, 64 + 159, SrcBuilding->typ->data.name );
	else font->showTextCentered ( 102 + 166, 64 + 159, SrcVehicle->typ->data.name );


	if ( DestBuilding )
	{
		font->showTextCentered ( 208 + 166, 64 + 159, DestBuilding->typ->data.name );

		switch ( SrcVehicle->data.can_transport )
		{
			case TRANS_METAL:
				{
					iMaxDestCargo = DestBuilding->SubBase->MaxMetal;
					iDestCargo = DestBuilding->SubBase->Metal;
				}
				break;
			case TRANS_OIL:
				{
					iMaxDestCargo = DestBuilding->SubBase->MaxOil;
					iDestCargo = DestBuilding->SubBase->Oil;
				}
				break;
			case TRANS_GOLD:
				{
					iMaxDestCargo = DestBuilding->SubBase->MaxGold;
					iDestCargo = DestBuilding->SubBase->Gold;
				}
				break;
		}
		iTransf = iMaxDestCargo;

		makeTransBar ( &iTransf, iMaxDestCargo, iDestCargo, SrcVehicle->data.can_transport, NULL, SrcVehicle );
	}
	else
	{
		font->showTextCentered ( 208 + 166, 64 + 159, DestVehicle->typ->data.name );

		iMaxDestCargo = DestVehicle->data.max_cargo;
		iDestCargo = DestVehicle->data.cargo;
		iTransf = iMaxDestCargo;

		if ( SrcBuilding ) makeTransBar ( &iTransf, iMaxDestCargo, iDestCargo, SrcBuilding->data.can_load, SrcBuilding->SubBase, NULL );
		else makeTransBar ( &iTransf, iMaxDestCargo, iDestCargo, SrcVehicle->data.can_transport, NULL, SrcVehicle );
	}
	NormalButton btn_cancel( 74 + 166, 125 + 159, "Text~Button~Cancel");
	NormalButton btn_done(  165 + 166, 125 + 159, "Text~Button~Done");
	btn_cancel.Draw();
	btn_done.Draw();

	// Den Buffer anzeigen:
	SHOW_SCREEN
	mouse->GetBack ( buffer );

	while ( 1 )
	{
		if ( SrcBuilding && Client->SelectedBuilding != SrcBuilding ) break;
		if ( SrcVehicle && Client->SelectedVehicle != SrcVehicle ) break;
		if ( !isInMenu ) break;

		handleTimer();
		doGameActions();

		// Events holen:
		EventHandler->HandleEvents();

		// Die Maus machen:
		mouse->GetPos();

		b = (int)clientMouseState.leftButtonPressed;

		if ( !b ) MouseHot = true;

		if ( !MouseHot ) b = 0;

		x = mouse->x;

		y = mouse->y;

		if ( x != LastMouseX || y != LastMouseY )
		{
			mouse->draw ( true, screen );
		}

		bool const down = b > LastB;
		bool const up   = b < LastB;

		if ( btn_cancel.CheckClick( x, y, down, up ) ) break;

		if ( btn_done.CheckClick( x, y, down, up ) )
		{
			if ( !iTransf ) break;

			if ( SrcBuilding ) sendWantTransfer ( false, SrcBuilding->iID, true, DestVehicle->iID, iTransf, SrcBuilding->data.can_load );
			else
			{
				if ( DestBuilding ) sendWantTransfer ( true, SrcVehicle->iID, false, DestBuilding->iID, iTransf, SrcVehicle->data.can_transport );
				else sendWantTransfer ( true, SrcVehicle->iID, true, DestVehicle->iID, iTransf, SrcVehicle->data.can_transport );
			}

			/*if ( pv )
			{
				switch ( data.can_load )
				{

					case TRANS_METAL:
						owner->base.AddMetal ( SubBase, -Transf );
						break;

					case TRANS_OIL:
						owner->base.AddOil ( SubBase, -Transf );
						break;

					case TRANS_GOLD:
						owner->base.AddGold ( SubBase, -Transf );
						break;
				}

				pv->data.cargo += Transf;
			}
			else
			{
				if ( data.cargo > Transf )
				{
					data.cargo -= Transf;
				}
				else
				{
					Transf = data.cargo;
					data.cargo = 0;
				}

				pb->data.cargo += Transf;
			}

			ShowDetails();*/

			PlayVoice ( VoiceData.VOITransferDone );
			break;
		}

		// Inc-Button:
		if ( x >= 277 + 166 && x < 277 + 19 + 166 && y >= 88 + 159 && y < 88 + 18 + 159 && b && !IncPressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			scr.x = 257;
			scr.y = 177;
			scr.w = 19;
			scr.h = 18;
			dest.x = 277 + 166;
			dest.y = 88 + 159;
			iTransf++;
			if ( DestBuilding ) makeTransBar ( &iTransf, iMaxDestCargo, iDestCargo, SrcVehicle->data.can_transport, NULL, SrcVehicle );
			else
			{
				if ( SrcBuilding ) makeTransBar ( &iTransf, iMaxDestCargo, iDestCargo, SrcBuilding->data.can_load, SrcBuilding->SubBase, NULL );
				else makeTransBar ( &iTransf, iMaxDestCargo, iDestCargo, SrcVehicle->data.can_transport, NULL, SrcVehicle );
			}
			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &scr, buffer, &dest );
			SHOW_SCREEN
			mouse->draw ( false, screen );
			IncPressed = true;
		}
		else
			if ( !b && IncPressed )
			{
				scr.x = 277;
				scr.y = 88;
				scr.w = 19;
				scr.h = 18;
				dest.x = 277 + 166;
				dest.y = 88 + 159;
				SDL_BlitSurface ( GraphicsData.gfx_transfer, &scr, buffer, &dest );
				SHOW_SCREEN
				mouse->draw ( false, screen );
				IncPressed = false;
			}

		// Dec-Button:
		if ( x >= 16 + 166 && x < 16 + 19 + 166 && y >= 88 + 159 && y < 88 + 18 + 159 && b && !DecPressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			scr.x = 237;
			scr.y = 177;
			scr.w = 19;
			scr.h = 18;
			dest.x = 16 + 166;
			dest.y = 88 + 159;
			iTransf--;
			if ( DestBuilding ) makeTransBar ( &iTransf, iMaxDestCargo, iDestCargo, SrcVehicle->data.can_transport, NULL, SrcVehicle );
			else
			{
				if ( SrcBuilding ) makeTransBar ( &iTransf, iMaxDestCargo, iDestCargo, SrcBuilding->data.can_load, SrcBuilding->SubBase, NULL );
				else makeTransBar ( &iTransf, iMaxDestCargo, iDestCargo, SrcVehicle->data.can_transport, NULL, SrcVehicle );
			}
			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &scr, buffer, &dest );
			SHOW_SCREEN
			mouse->draw ( false, screen );
			DecPressed = true;
		}
		else
			if ( !b && DecPressed )
			{
				scr.x = 16;
				scr.y = 88;
				scr.w = 19;
				scr.h = 18;
				dest.x = 16 + 166;
				dest.y = 88 + 159;
				SDL_BlitSurface ( GraphicsData.gfx_transfer, &scr, buffer, &dest );
				SHOW_SCREEN
				mouse->draw ( false, screen );
				DecPressed = false;
			}

		// Klick auf den Bar:
		if ( x >= 44 + 166 && x < 44 + 223 + 166 && y >= 86 + 159 && y < 86 + 20 + 159 && b && !LastB )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			iTransf = Round ( ( x - ( 44 +  166 ) ) * ( iMaxDestCargo / 223.0 ) - iDestCargo );
			if ( DestBuilding ) makeTransBar ( &iTransf, iMaxDestCargo, iDestCargo, SrcVehicle->data.can_transport, NULL, SrcVehicle );
			else
			{
				if ( SrcBuilding ) makeTransBar ( &iTransf, iMaxDestCargo, iDestCargo, SrcBuilding->data.can_load, SrcBuilding->SubBase, NULL );
				else makeTransBar ( &iTransf, iMaxDestCargo, iDestCargo, SrcVehicle->data.can_transport, NULL, SrcVehicle );
			}
			SHOW_SCREEN
			mouse->draw ( false, screen );
		}

		LastMouseX = x;

		LastMouseY = y;
		LastB = b;
	}

	float fNewZoom = (float)(Hud.Zoom / 64.0);

	if ( SrcBuilding != NULL )
	{
		scaleSurface ( SrcBuilding->typ->img_org, SrcBuilding->typ->img, ( int ) ( SrcBuilding->typ->img_org->w* fNewZoom ) , ( int ) ( SrcBuilding->typ->img_org->h* fNewZoom ) );
		SrcBuilding->Transfer = false;
	}
	else
	{
		scaleSurface ( SrcVehicle->typ->img_org[0], SrcVehicle->typ->img[0], ( int ) ( SrcVehicle->typ->img_org[0]->w* fNewZoom ) , ( int ) ( SrcVehicle->typ->img_org[0]->h* fNewZoom ) );
		SrcVehicle->Transfer = false;
	}

	if ( DestBuilding ) scaleSurface ( DestBuilding->typ->img_org, DestBuilding->typ->img, ( int ) ( DestBuilding->typ->img_org->w* fNewZoom ), ( int ) ( DestBuilding->typ->img_org->h* fNewZoom ) );
	else scaleSurface ( DestVehicle->typ->img_org[0], DestVehicle->typ->img[0], ( int ) ( DestVehicle->typ->img_org[0]->w* fNewZoom ), ( int ) ( DestVehicle->typ->img_org[0]->h* fNewZoom ) );
	isInMenu = false;
}

void cClient::drawTransBar ( int iLenght, int iType )
{
	SDL_Rect scr, dest;

	if ( iLenght < 0 ) iLenght = 0;

	if ( iLenght > 223 ) iLenght = 223;

	scr.x = 44;
	scr.y = 90;
	scr.w = 223;
	scr.h = 16;
	dest.x = 44 + 166;
	dest.y = 90 + 159;
	SDL_BlitSurface ( GraphicsData.gfx_transfer, &scr, buffer, &dest );
	scr.x = 156 + ( 223 - iLenght );
	scr.w = 223 - ( 223 - iLenght );

	if ( iType == TRANS_METAL )
	{
		scr.y = 256;
	}
	else
		if ( iType == TRANS_OIL )
		{
			scr.y = 273;
		}
		else
		{
			scr.y = 290;
		}

	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &scr, buffer, &dest );
}

void cClient::makeTransBar( int *iTransfer, int iMaxDestCargo, int iDestCargo, int iType, sSubBase *SubBase, cVehicle *Vehicle )
{
	int iCargo, iMaxCargo;
	SDL_Rect scr, dest;
	string sText;

	if ( SubBase != NULL )
	{
		switch ( iType )
		{
			case TRANS_METAL:
				iCargo = SubBase->Metal;
				iMaxCargo = SubBase->MaxMetal;
				break;

			case TRANS_OIL:
				iCargo = SubBase->Oil;
				iMaxCargo = SubBase->MaxOil;
				break;

			case TRANS_GOLD:
				iCargo = SubBase->Gold;
				iMaxCargo = SubBase->MaxGold;
				break;
		}
	}
	else if ( Vehicle != NULL )
	{
		iCargo = Vehicle->data.cargo;
		iMaxCargo = Vehicle->data.max_cargo;
	}
	else return;

	if ( iCargo - *iTransfer < 0 ) *iTransfer += iCargo - *iTransfer;
	if ( iDestCargo + *iTransfer < 0 ) *iTransfer -= iDestCargo + *iTransfer;
	if ( iDestCargo + *iTransfer > iMaxDestCargo ) *iTransfer -= ( iDestCargo + *iTransfer ) - iMaxDestCargo;
	if ( iCargo - *iTransfer > iMaxCargo ) *iTransfer += ( iCargo - *iTransfer ) - iMaxCargo;

	// Die Nummern machen:
	scr.x = 4;
	scr.y = 30;
	dest.x = 4 + 166;
	dest.y = 30 + 159;
	scr.w = 78;
	scr.h = 14;
	SDL_BlitSurface ( GraphicsData.gfx_transfer, &scr, buffer, &dest );
	sText = iToStr ( iCargo - *iTransfer );

	font->showTextCentered ( 4 + 39 + 166, 30 + 159, sText );
	scr.x = 229;
	dest.x = 229 + 166;
	SDL_BlitSurface ( GraphicsData.gfx_transfer, &scr, buffer, &dest );
	sText = iToStr ( iDestCargo + *iTransfer );


	font->showTextCentered ( 229 + 39 + 166, 30 + 159, sText );
	scr.x = 141;
	scr.y = 15;
	dest.x = 141 + 166;
	dest.y = 15 + 159;
	scr.w = 29;
	scr.h = 21;

	SDL_BlitSurface ( GraphicsData.gfx_transfer, &scr, buffer, &dest );

	sText = iToStr ( abs ( *iTransfer ) );

	font->showTextCentered ( 155 + 166, 21 + 159, sText );

	// Den Pfeil malen:
	if ( *iTransfer < 0 )
	{
		scr.x = 122;
		scr.y = 263;
		dest.x = 143 + 166;
		dest.y = 44 + 159;
		scr.w = 30;
		scr.h = 16;
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &scr, buffer, &dest );
	}
	else
	{
		scr.x = 143;
		scr.y = 44;
		dest.x = 143 + 166;
		dest.y = 44 + 159;
		scr.w = 30;
		scr.h = 16;
		SDL_BlitSurface ( GraphicsData.gfx_transfer, &scr, buffer, &dest );
	}

	drawTransBar ( ( int ) ( 223 * ( float ) ( iDestCargo + *iTransfer ) / iMaxDestCargo ), iType );
}

void cClient::destroyUnit( cVehicle* vehicle )
{
	//play explosion
	if ( vehicle->data.is_big )
	{
		Client->addFX( fxExploBig, vehicle->PosX * 64 + 64, vehicle->PosY * 64 + 64, 0);
	}
	else if ( vehicle->data.can_drive == DRIVE_AIR && vehicle->FlightHigh == 0 )
	{
		Client->addFX( fxExploAir, vehicle->PosX*64 + vehicle->OffX + 32, vehicle->PosY*64 + vehicle->OffY + 32, 0);
	}
	else if ( Map->IsWater(vehicle->PosX + vehicle->PosY*Map->size) )
	{
		Client->addFX( fxExploWater, vehicle->PosX*64 + vehicle->OffX + 32, vehicle->PosY*64 + vehicle->OffY + 32, 0);
	}
	else
	{
		Client->addFX( fxExploSmall, vehicle->PosX*64 + vehicle->OffX + 32, vehicle->PosY*64 + vehicle->OffY + 32, 0);
	}

	if ( vehicle->data.is_human )
	{
		//add corpse
		Client->addFX( fxCorpse,  vehicle->PosX*64 + vehicle->OffX, vehicle->PosY*64 + vehicle->OffY, 0);
	}
}

void cClient::destroyUnit(cBuilding *building)
{
	//play explosion animation
	cBuilding* topBuilding = Map->fields[building->PosX + building->PosY*Map->size].getBuildings();
	if ( topBuilding && topBuilding->data.is_big )
	{
		Client->addFX( fxExploBig, topBuilding->PosX * 64 + 64, topBuilding->PosY * 64 + 64, 0);
	}
	else
	{
		Client->addFX( fxExploSmall, building->PosX * 64 + 32, building->PosY * 64 + 32, 0);
	}
	
}

void cClient::checkVehiclePositions(cNetMessage *message)
{
	
	static cList<cVehicle*>  vehicleList;
	bool lastMessagePart = message->popBool();

	if ( vehicleList.Size() == 0 && !lastMessagePart )
	{
		//generate list with all vehicles
		for ( unsigned int i = 0; i < Client->PlayerList->Size(); i++ )
		{
			cVehicle* vehicle = (*Client->PlayerList)[i]->VehicleList;
			while ( vehicle )
			{
				vehicleList.Add( vehicle );
				vehicle = vehicle->next;
			}
		}
	}

	//check all sent positions
	while ( message->iLength > 6 )
	{
		int id = message->popInt32();
		int PosY = message->popInt16();
		int PosX = message->popInt16();
		cVehicle* vehicle = getVehicleFromID(id);
		if ( vehicle == NULL )
		{
			Log.write("   --Vehicle not present, ID: " + iToStr(id), cLog::eLOG_TYPE_NET_ERROR );
			continue;
		}

		if ( PosX != -1 && PosY != -1 && ( vehicle->PosX != PosX || vehicle->PosY != PosY ) && !vehicle->ClientMoveJob )
		{
			Log.write("   --wrong position, ID: " + iToStr(id) + ", is: "+iToStr(vehicle->PosX)+":"+iToStr(vehicle->PosY)+", should: "+iToStr(PosX)+":"+iToStr(PosY) , cLog::eLOG_TYPE_NET_ERROR); 
		}

		//remove vehicle from list
		for ( unsigned int i = 0; i < vehicleList.Size(); i++)
		{
			if ( vehicleList[i] == vehicle )
			{
				vehicleList.Delete(i);
				break;
			}
		}
	}

	if ( lastMessagePart )
	{
		//check remaining vehicles
		while ( vehicleList.Size() > 0 )
		{
			Log.write("   --vehicle should not exist, ID: "+iToStr(vehicleList[0]->iID), cLog::eLOG_TYPE_NET_ERROR );
			vehicleList.Delete(0);
		}
	}
}
