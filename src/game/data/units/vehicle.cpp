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

#include "game/data/units/vehicle.h"

#include "game/data/map/map.h"
#include "game/data/map/mapfieldview.h"
#include "game/data/map/mapview.h"
#include "game/data/player/player.h"
#include "game/data/units/building.h"
#include "game/logic/attackjob.h"
#include "game/logic/client.h"
#include "game/logic/fxeffects.h"
#include "game/logic/jobs/planetakeoffjob.h"
#include "game/logic/jobs/startbuildjob.h"
#include "input/mouse/mouse.h"
#include "output/sound/sounddevice.h"
#include "output/sound/soundchannel.h"
#include "output/video/unifonts.h"
#include "output/video/video.h"
#include "resources/sound.h"
#include "settings.h"
#include "ui/graphical/application.h"
#include "ui/graphical/menu/windows/windowbuildbuildings/windowbuildbuildings.h"
#include "utility/crc.h"
#include "utility/files.h"
#include "utility/listhelpers.h"
#include "utility/log.h"
#include "utility/mathtools.h"
#include "utility/random.h"
#include "utility/string/toString.h"

#include <cmath>

using namespace std;

//-----------------------------------------------------------------------------
// cVehicle Class Implementation
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
cVehicle::cVehicle (const cStaticUnitData& staticData, const cDynamicUnitData& dynamicData, cPlayer* owner, unsigned int ID) :
	cUnit (&dynamicData, &staticData, owner, ID),
	bandPosition(0, 0),
	buildBigSavedPosition(0, 0),
	tileMovementOffset(0, 0),
	moveJob(nullptr),
	loaded (false),
	isBuilding (false),
	buildingTyp(),
	buildCosts (0),
	buildTurns (0),
	buildTurnsStart (0),
	buildCostsStart (0),
	isClearing (false),
	clearingTurns (0),
	layMines (false),
	clearMines (false)
{
	uiData = UnitsUiData.getVehicleUI (staticData.ID);
	ditherX = 0;
	ditherY = 0;
	flightHeight = 0;
	WalkFrame = 0;
	surveyorAutoMoveActive = false;
	moving = false;
	BuildPath = false;
	bigBetonAlpha = 254;

	DamageFXPointX = random (7) + 26 - 3;
	DamageFXPointY = random (7) + 26 - 3;
	refreshData();

	clearingTurnsChanged.connect ([&]() { statusChanged(); });
	buildingTurnsChanged.connect ([&]() { statusChanged(); });
	buildingTypeChanged.connect ([&]() { statusChanged(); });
	moveJobChanged.connect ([&]() { statusChanged(); });
	autoMoveJobChanged.connect ([&]() { statusChanged(); });
}

//-----------------------------------------------------------------------------
cVehicle::~cVehicle()
{
}

void cVehicle::drawOverlayAnimation (SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor, int frameNr, int alpha) const
{
	drawOverlayAnimation (surface, dest, zoomFactor, *uiData, frameNr, alpha);
}

/*static*/ void cVehicle::drawOverlayAnimation (SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor, const sVehicleUIData& uiData, int frameNr, int alpha)
{
	if (uiData.hasOverlay == false || cSettings::getInstance().isAnimations() == false) return;

	const Uint16 size = (Uint16) (uiData.overlay_org->h * zoomFactor);
	const Uint16 srcX = Round((uiData.overlay_org->h * frameNr) * zoomFactor);
	SDL_Rect src = {srcX, 0, size, size};

	SDL_Rect tmp = dest;
	const int offset = Round (64.0f * zoomFactor) / 2 - src.h / 2;
	tmp.x += offset;
	tmp.y += offset;

	SDL_SetSurfaceAlphaMod (uiData.overlay.get(), alpha);
	blitWithPreScale (uiData.overlay_org.get(), uiData.overlay.get(), &src, surface, &tmp, zoomFactor);
}

void cVehicle::drawOverlayAnimation (unsigned long long animationTime, SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor) const
{
	if (uiData->hasOverlay == false || cSettings::getInstance().isAnimations() == false) return;
	int frameNr = 0;
	if (isDisabled() == false)
	{
		frameNr = animationTime % (uiData->overlay_org->w / uiData->overlay_org->h);
	}

	drawOverlayAnimation(surface, dest, zoomFactor, frameNr, alphaEffectValue && cSettings::getInstance().isAlphaEffects() ? alphaEffectValue : 254);
}

void cVehicle::render_BuildingOrBigClearing (const cMapView& map, unsigned long long animationTime, SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor, bool drawShadow) const
{
	assert ((isUnitBuildingABuilding() || (isUnitClearing() && isBig)) && !jobActive);

	// draw beton if necessary
	SDL_Rect tmp = dest;
	if (isUnitBuildingABuilding() && getIsBig() && (!map.isWaterOrCoast (getPosition()) || map.getField (getPosition()).getBaseBuilding()))
	{
		SDL_SetSurfaceAlphaMod (GraphicsData.gfx_big_beton.get(), bigBetonAlpha);
		CHECK_SCALING (*GraphicsData.gfx_big_beton, *GraphicsData.gfx_big_beton_org, zoomFactor);
		SDL_BlitSurface (GraphicsData.gfx_big_beton.get(), nullptr, surface, &tmp);
	}

	// draw shadow
	tmp = dest;
	if (drawShadow) blitWithPreScale (uiData->build_shw_org.get(), uiData->build_shw.get(), nullptr, surface, &tmp, zoomFactor);

	// draw player color
	SDL_Rect src;
	src.y = 0;
	src.h = src.w = (int) (uiData->build_org->h * zoomFactor);
	src.x = (animationTime % 4) * src.w;
	SDL_BlitSurface (getOwner()->getColor().getTexture(), nullptr, GraphicsData.gfx_tmp.get(), nullptr);
	blitWithPreScale (uiData->build_org.get(), uiData->build.get(), &src, GraphicsData.gfx_tmp.get(), nullptr, zoomFactor, 4);

	// draw vehicle
	src.x = 0;
	src.y = 0;
	tmp = dest;
	SDL_SetSurfaceAlphaMod (GraphicsData.gfx_tmp.get(), 254);
	SDL_BlitSurface (GraphicsData.gfx_tmp.get(), &src, surface, &tmp);
}

void cVehicle::render_smallClearing (unsigned long long animationTime, SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor, bool drawShadow) const
{
	assert (isUnitClearing() && !isBig && !jobActive);

	// draw shadow
	SDL_Rect tmp = dest;
	if (drawShadow)
		blitWithPreScale (uiData->clear_small_shw_org.get(), uiData->clear_small_shw.get(), nullptr, surface, &tmp, zoomFactor);

	// draw player color
	SDL_Rect src;
	src.y = 0;
	src.h = src.w = (int) (uiData->clear_small_org->h * zoomFactor);
	src.x = (animationTime % 4) * src.w;
	SDL_BlitSurface (getOwner()->getColor().getTexture(), nullptr, GraphicsData.gfx_tmp.get(), nullptr);
	blitWithPreScale (uiData->clear_small_org.get(), uiData->clear_small.get(), &src, GraphicsData.gfx_tmp.get(), nullptr, zoomFactor, 4);

	// draw vehicle
	src.x = 0;
	src.y = 0;
	tmp = dest;
	SDL_SetSurfaceAlphaMod (GraphicsData.gfx_tmp.get(), 254);
	SDL_BlitSurface (GraphicsData.gfx_tmp.get(), &src, surface, &tmp);
}

void cVehicle::render_shadow (const cMapView& map, SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor) const
{
	if (map.isWater (getPosition()) && (staticData->isStealthOn & TERRAIN_SEA)) return;

	if (alphaEffectValue && cSettings::getInstance().isAlphaEffects())
	{
		SDL_SetSurfaceAlphaMod(uiData->shw[dir].get(), alphaEffectValue / 5);
	}
	else
	{
		SDL_SetSurfaceAlphaMod(uiData->shw[dir].get(), 50);
	}
	SDL_Rect tmp = dest;

	// draw shadow
	if (getFlightHeight() > 0)
	{
		int high = ((int) (Round (uiData->shw_org[dir]->w * zoomFactor) * (getFlightHeight() / 64.0f)));
		tmp.x += high;
		tmp.y += high;

		blitWithPreScale (uiData->shw_org[dir].get(), uiData->shw[dir].get(), nullptr, surface, &tmp, zoomFactor);
	}
	else if (uiData->animationMovement)
	{
		const Uint16 size = (int) (uiData->img_org[dir]->h * zoomFactor);
		SDL_Rect r = {Sint16 (WalkFrame * size), 0, size, size};
		blitWithPreScale (uiData->shw_org[dir].get(), uiData->shw[dir].get(), &r, surface, &tmp, zoomFactor);
	}
	else
		blitWithPreScale (uiData->shw_org[dir].get(), uiData->shw[dir].get(), nullptr, surface, &tmp, zoomFactor);
}

void cVehicle::render_simple (SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor, int alpha) const
{
	render_simple (surface, dest, zoomFactor, *uiData, getOwner(), dir, WalkFrame, alpha);
}

/*static*/ void cVehicle::render_simple (SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor, const sVehicleUIData& uiData, const cPlayer* owner, int dir, int walkFrame, int alpha)
{
	// draw player color
	if (owner)
	{
		SDL_Surface* src = owner->getColor().getTexture();
		SDL_Surface* dst = GraphicsData.gfx_tmp.get();
		SDL_BlitSurface(src, nullptr, dst, nullptr);
	}

	// read the size:
	SDL_Rect src;
	src.w = (int) (uiData.img_org[dir]->w * zoomFactor);
	src.h = (int) (uiData.img_org[dir]->h * zoomFactor);

	if (uiData.animationMovement)
	{
		SDL_Rect tmp;
		src.w = src.h = tmp.h = tmp.w = (int) (uiData.img_org[dir]->h * zoomFactor);
		tmp.x = walkFrame * tmp.w;
		tmp.y = 0;
		blitWithPreScale (uiData.img_org[dir].get(), uiData.img[dir].get(), &tmp, GraphicsData.gfx_tmp.get(), nullptr, zoomFactor);
	}
	else
		blitWithPreScale (uiData.img_org[dir].get(), uiData.img[dir].get(), nullptr, GraphicsData.gfx_tmp.get(), nullptr, zoomFactor);

	// draw the vehicle
	src.x = 0;
	src.y = 0;
	SDL_Rect tmp = dest;

	SDL_SetSurfaceAlphaMod (GraphicsData.gfx_tmp.get(), alpha);
	blittAlphaSurface (GraphicsData.gfx_tmp.get(), &src, surface, &tmp);
}

void cVehicle::render (const cMapView* map, unsigned long long animationTime, const cPlayer* activePlayer, SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor, bool drawShadow) const
{
	// Note: when changing something in this function,
	// make sure to update the caching rules!

	// draw working engineers and bulldozers:
	if (map && !jobActive)
	{
		if (isUnitBuildingABuilding() || (isUnitClearing() && isBig))
		{
			render_BuildingOrBigClearing (*map, animationTime, surface, dest, zoomFactor, drawShadow);
			return;
		}
		if (isUnitClearing() && !isBig)
		{
			render_smallClearing (animationTime, surface, dest, zoomFactor, drawShadow);
			return;
		}
	}

	// draw all other vehicles:

	// draw shadow
	if (drawShadow && map)
	{
		render_shadow (*map, surface, dest, zoomFactor);
	}

	int alpha = 254;
	if (map)
	{
		if (alphaEffectValue && cSettings::getInstance().isAlphaEffects())
		{
			alpha = alphaEffectValue;
		}

		bool water = map->isWater (getPosition());
		// if the vehicle can also drive on land, we have to check,
		// whether there is a brige, platform, etc.
		// because the vehicle will drive on the bridge
		cBuilding* building = map->getField (getPosition()).getBaseBuilding();
		if (building && staticData->factorGround > 0 && (building->getStaticUnitData().surfacePosition == cStaticUnitData::SURFACE_POS_BASE || building->getStaticUnitData().surfacePosition == cStaticUnitData::SURFACE_POS_ABOVE_SEA || building->getStaticUnitData().surfacePosition == cStaticUnitData::SURFACE_POS_ABOVE_BASE)) water = false;

		if (water && (staticData->isStealthOn & TERRAIN_SEA) && detectedByPlayerList.empty() && getOwner() == activePlayer) alpha = std::min (alpha, 100);
	}
	render_simple (surface, dest, zoomFactor, alpha);
}

void cVehicle::proceedBuilding (cModel& model)
{
	if (isUnitBuildingABuilding() == false || getBuildTurns() == 0) return;

	setStoredResources (getStoredResources() - (getBuildCosts() / getBuildTurns()));
	setBuildCosts (getBuildCosts() - (getBuildCosts() / getBuildTurns()));

	setBuildTurns (getBuildTurns() - 1);
	if (getBuildTurns() != 0) return;

	const cMap& map = *model.getMap();
	getOwner()->addTurnReportUnit (getBuildingType());

	// handle pathbuilding
	// here the new building is added (if possible) and
	// the move job to the next field is generated
	// the new build event is generated in cMoveJob::endMove()
	if (BuildPath)
	{
		// Find a next position that either
		// a) is something we can't move to
		//  (in which case we cancel the path building)
		// or b) doesn't have a building type that we're trying to build.
		cPosition nextPosition (getPosition());
		bool found_next  = false;

		while (!found_next && (nextPosition != bandPosition))
		{
			// Calculate the next position in the path.
			if (getPosition().x() > bandPosition.x()) nextPosition.x()--;
			if (getPosition().x() < bandPosition.x()) nextPosition.x()++;
			if (getPosition().y() > bandPosition.y()) nextPosition.y()--;
			if (getPosition().y() < bandPosition.y()) nextPosition.y()++;
			// Can we move to this position?
			// If not, we need to kill the path building now.
			model.sideStepStealthUnit (nextPosition, *this);
			if (!map.possiblePlace (*this, nextPosition, false))
			{
				// We can't build along this path any more.
				break;
			}
			// Can we build at this next position?
			if (map.possiblePlaceBuilding (model.getUnitsData()->getStaticUnitData(getBuildingType()), nextPosition, nullptr))
			{
				// We can build here.
				found_next = true;
				break;
			}
		}

		//If we've found somewhere to move to, move there now.
		const cPosition oldPosition = getPosition();
		if (found_next && model.addMoveJob (*this, nextPosition))
		{
			model.addBuilding (oldPosition, getBuildingType(), getOwner());
			setBuildingABuilding (false);
		}
		else
		{
			if (model.getUnitsData()->getStaticUnitData(getBuildingType()).surfacePosition != cStaticUnitData::SURFACE_POS_GROUND)
			{
				// add building immediately
				// if it doesn't require the engineer to drive away
				setBuildingABuilding (false);
				model.addBuilding (getPosition(), getBuildingType(), getOwner());
			}
			BuildPath = false;
			getOwner()->buildPathInterrupted(*this);
 		}
	}
	else if (model.getUnitsData()->getStaticUnitData(getBuildingType()).surfacePosition != staticData->surfacePosition)
	{
		// add building immediately
		// if it doesn't require the engineer to drive away
		setBuildingABuilding (false);
		model.addBuilding (getPosition(), getBuildingType(), getOwner());
	}
}

void cVehicle::continuePathBuilding(cModel& model)
{
	if (!BuildPath) return;

	if (getStoredResources() >= getBuildCostsStart() && model.getMap()->possiblePlaceBuilding(model.getUnitsData()->getStaticUnitData(getBuildingType()), getPosition(), nullptr, this))
	{
		model.addJob(new cStartBuildJob(*this, getPosition(), getIsBig()));
		setBuildingABuilding(true);
		setBuildCosts(getBuildCostsStart());
		setBuildTurns(getBuildTurnsStart());
	}
	else
	{
		BuildPath = false;
		getOwner()->buildPathInterrupted(*this);
	}
}

void cVehicle::proceedClearing(cModel& model)
{
	if (isUnitClearing() == false || getClearingTurns() == 0) return;

	setClearingTurns (getClearingTurns() - 1);

	if (getClearingTurns() > 0) return;

	// clearing finished
	setClearing (false);

	cMap& map = *model.getMap();
	cBuilding* rubble = map.getField (getPosition()).getRubble();
	if (isBig)
	{
		getOwner()->updateScan(*this, buildBigSavedPosition);
		map.moveVehicle(*this, buildBigSavedPosition);
	}

	setStoredResources (getStoredResources() + rubble->getRubbleValue());
	model.deleteRubble(rubble);
}

//-----------------------------------------------------------------------------
/** Initializes all unit data to its maxiumum values */
//-----------------------------------------------------------------------------

bool cVehicle::refreshData()
{
	// NOTE: according to MAX 1.04 units get their shots/movepoints back even if they are disabled

	if (data.getSpeed() < data.getSpeedMax() || data.getShots() < data.getShotsMax())
	{
		data.setSpeed (data.getSpeedMax());
		data.setShots (std::min (data.getAmmo(), data.getShotsMax()));
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
/** Returns a string with the current state */
//-----------------------------------------------------------------------------
string cVehicle::getStatusStr(const cPlayer* player, const cUnitsData& unitsData) const
{
	auto font = cUnicodeFont::font.get();
	if (isDisabled())
	{
		string sText;
		sText = lngPack.i18n ("Text~Comp~Disabled") + " (";
		sText += iToStr (getDisabledTurns()) + ")";
		return sText;
	}
	else if (surveyorAutoMoveActive)
		return lngPack.i18n ("Text~Comp~Surveying");
	else if (isUnitBuildingABuilding())
	{
		if (getOwner() != player)
			return lngPack.i18n ("Text~Comp~Producing");
		else
		{
			string sText;
			if (getBuildTurns())
			{
				sText = lngPack.i18n ("Text~Comp~Producing");
				sText += lngPack.i18n ("Text~Punctuation~Colon");
				sText += unitsData.getStaticUnitData(getBuildingType()).getName() + " (";
				sText += iToStr (getBuildTurns());
				sText += ")";

				if (font->getTextWide (sText) > 126)
				{
					sText = lngPack.i18n ("Text~Comp~Producing") + lngPack.i18n("Text~Punctuation~Colon");
					sText += "\n";
					sText += unitsData.getStaticUnitData(getBuildingType()).getName() + " (";
					sText += iToStr (getBuildTurns());
					sText += ")";
				}
				return sText;
			}
			else //small building is rdy + activate after engineere moves away
			{
				sText = lngPack.i18n ("Text~Comp~Producing_Fin");
				sText += lngPack.i18n ("Text~Punctuation~Colon");
				sText += unitsData.getStaticUnitData(getBuildingType()).getName();

				if (font->getTextWide (sText) > 126)
				{
					sText = lngPack.i18n ("Text~Comp~Producing_Fin") + lngPack.i18n("Text~Punctuation~Colon");
					sText += "\n";
					sText += unitsData.getStaticUnitData(getBuildingType()).getName();
				}
				return sText;
			}
		}
	}
	else if (isUnitClearingMines())
		return lngPack.i18n ("Text~Comp~Clearing_Mine");
	else if (isUnitLayingMines())
		return lngPack.i18n ("Text~Comp~Laying");
	else if (isUnitClearing())
	{
		if (getClearingTurns())
		{
			string sText;
			sText = lngPack.i18n ("Text~Comp~Clearing") + " (";
			sText += iToStr (getClearingTurns()) + ")";
			return sText;
		}
		else
			return lngPack.i18n ("Text~Comp~Clearing_Fin");
	}

	// generate other infos for normal non-unit-related-events and infiltrators
	string sTmp;
	{
		if (moveJob && moveJob->getEndMoveAction().getType() == EMAT_ATTACK)
			sTmp = lngPack.i18n ("Text~Comp~MovingToAttack");
		else if (moveJob)
			sTmp = lngPack.i18n ("Text~Comp~Moving");
		else if (isAttacking())
			sTmp = lngPack.i18n ("Text~Comp~AttackingStatusStr");
		else if (isBeeingAttacked())
			sTmp = lngPack.i18n ("Text~Comp~IsBeeingAttacked");
		else if (isManualFireActive())
			sTmp = lngPack.i18n ("Text~Comp~ReactionFireOff");
		else if (isSentryActive())
			sTmp = lngPack.i18n ("Text~Comp~Sentry");
		else sTmp = lngPack.i18n ("Text~Comp~Waits");

		// extra info only for infiltrators
		// TODO should it be original behavior (as it is now) or
		// don't display CommandRank for enemy (could also be a bug in original...?)
		if ((staticData->canCapture || staticData->canDisable) /* && owner == gameGUI.getClient()->getActivePlayer()*/)
		{
			sTmp += "\n";
			sTmp += commandoData.getRankString();
		}

		return sTmp;
	}

	return lngPack.i18n ("Text~Comp~Waits");
}

//-----------------------------------------------------------------------------
/** Reduces the remaining speedCur and shotsCur during movement */
//-----------------------------------------------------------------------------
void cVehicle::DecSpeed (int value)
{
	data.setSpeed (data.getSpeed() - value);

	if (staticData->canAttack == false || staticData->canDriveAndFire) return;

	const int s = data.getSpeed() * data.getShotsMax() / data.getSpeedMax();
	data.setShots (std::min (data.getShots(), s));
}

//-----------------------------------------------------------------------------
void cVehicle::calcTurboBuild (std::array<int, 3>& turboBuildTurns, std::array<int, 3>& turboBuildCosts, int buildCosts) const
{
	turboBuildTurns[0] = 0;
	turboBuildTurns[1] = 0;
	turboBuildTurns[2] = 0;

	// step 1x
	if (getStoredResources() >= buildCosts)
	{
		turboBuildCosts[0] = buildCosts;
		// prevent division by zero
		const auto needsMetal = staticData->needsMetal == 0 ? 1 : staticData->needsMetal;
		turboBuildTurns[0] = (int)ceilf (turboBuildCosts[0] / (float) (needsMetal));
	}

	// step 2x
	// calculate building time and costs
	int a = turboBuildCosts[0];
	int rounds = turboBuildTurns[0];
	int costs = turboBuildCosts[0];

	while (a >= 4 && getStoredResources() >= costs + 4)
	{
		rounds--;
		costs += 4;
		a -= 4;
	}

	if (rounds < turboBuildTurns[0] && rounds > 0 && turboBuildTurns[0])
	{
		turboBuildCosts[1] = costs;
		turboBuildTurns[1] = rounds;
	}

	// step 4x
	a = turboBuildCosts[1];
	rounds = turboBuildTurns[1];
	costs = turboBuildCosts[1];

	while (a >= 10 && costs < staticData->storageResMax - 2)
	{
		int inc = 24 - min (16, a);
		if (costs + inc > getStoredResources()) break;

		rounds--;
		costs += inc;
		a -= 16;
	}

	if (rounds < turboBuildTurns[1] && rounds > 0 && turboBuildTurns[1])
	{
		turboBuildCosts[2] = costs;
		turboBuildTurns[2] = rounds;
	}
}

//-----------------------------------------------------------------------------
/** Scans for resources */
//-----------------------------------------------------------------------------
bool cVehicle::doSurvey(const cMap& map)
{
	const auto& owner = *getOwner();
	bool ressourceFound = false;

	const int minx = std::max (getPosition().x() - 1, 0);
	const int maxx = std::min (getPosition().x() + 1, owner.getMapSize().x() - 1);
	const int miny = std::max (getPosition().y() - 1, 0);
	const int maxy = std::min (getPosition().y() + 1, owner.getMapSize().y() - 1);

	for (int y = miny; y <= maxy; ++y)
	{
		for (int x = minx; x <= maxx; ++x)
		{
			const cPosition position (x, y);
			if (!owner.hasResourceExplored(position) && map.getResource(position).typ != eResourceType::None)
			{
				ressourceFound = true;
			}

			getOwner()->exploreResource(position);
		}
	}

	return ressourceFound;
}

//-----------------------------------------------------------------------------
/** checks, if resources can be transferred to the unit */
//-----------------------------------------------------------------------------
bool cVehicle::canTransferTo(const cPosition& position, const cMapView& map) const
{
	const auto& field = map.getField(position);

	const cUnit* unit = field.getVehicle();
	if (unit)
	{
		return canTransferTo(*unit);
	}

	unit = field.getTopBuilding();
	if (unit)
	{
		return canTransferTo(*unit);
	}

	return false;
}

//------------------------------------------------------------------------------
bool cVehicle::canTransferTo (const cUnit& unit) const
{
	if (!unit.isNextTo(getPosition()))
		return false;

	if (&unit == this)
		return false;

	if (unit.getOwner() != getOwner())
		return false;



	if (unit.isAVehicle())
	{
		const cVehicle* v = static_cast<const cVehicle*>(&unit);

		if (v->staticData->storeResType != staticData->storeResType)
			return false;

		if (v->isUnitBuildingABuilding() || v->isUnitClearing())
			return false;

		return true;
	}
	else if (unit.isABuilding())
	{
		const cBuilding* b = static_cast<const cBuilding*>(&unit);

		if (!b->subBase)
			return false;

		const sMiningResource& maxStored = b->subBase->getMaxResourcesStored();
		if (staticData->storeResType == eResourceType::Metal && maxStored.metal == 0)
			return false;

		if (staticData->storeResType == eResourceType::Oil && maxStored.oil == 0)
			return false;

		if (staticData->storeResType == eResourceType::Gold && maxStored.oil == 0)
			return false;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::makeAttackOnThis (cModel& model, cUnit* opponentUnit, const string& reasonForLog) const
{
	cMapView mapView(model.getMap(), nullptr);
	const cUnit* target = cAttackJob::selectTarget (getPosition(), opponentUnit->getStaticUnitData().canAttack, mapView, getOwner());
	if (target != this) return false;

	Log.write (" cVehicle: " + reasonForLog + ": attacking " + toString (getPosition()) + ", Aggressor ID: " + iToStr (opponentUnit->iID) + ", Target ID: " + iToStr(target->getId()), cLog::eLOG_TYPE_NET_DEBUG);

	model.addAttackJob (*opponentUnit, getPosition());

	return true;
}

//-----------------------------------------------------------------------------
bool cVehicle::makeSentryAttack (cModel& model, cUnit* sentryUnit) const
{
	cMapView mapView(model.getMap(), nullptr);
	if (sentryUnit != 0 && sentryUnit->isSentryActive() && sentryUnit->canAttackObjectAt (getPosition(), mapView, true))
	{
		if (makeAttackOnThis (model, sentryUnit, "sentry reaction"))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::inSentryRange (cModel& model)
{
	for (const auto& player : model.getPlayerList())
	{
		if (player.get() == getOwner()) continue;

		// Don't attack invisible units
		if (!player->canSeeUnit (*this, *model.getMap())) continue;

		// Check sentry type
		if (staticData->factorAir > 0 && player->hasSentriesAir (getPosition()) == 0) continue;
		// Check sentry type
		if (staticData->factorAir == 0 && player->hasSentriesGround (getPosition()) == 0) continue;

		const auto& vehicles = player->getVehicles();
		for (auto i = vehicles.begin(); i != vehicles.end(); ++i)
		{
			const auto& vehicle = *i;
			if (makeSentryAttack (model, vehicle.get()))
				return true;
		}
		const auto& buildings = player->getBuildings();
		for (auto i = buildings.begin(); i != buildings.end(); ++i)
		{
			const auto& building = *i;
			if (makeSentryAttack (model, building.get()))
				return true;
		}
	}

	return provokeReactionFire (model);
}

//-----------------------------------------------------------------------------
bool cVehicle::isOtherUnitOffendedByThis(const cModel& model, const cUnit& otherUnit) const
{
	// don't treat the cheap buildings
	// (connectors, roads, beton blocks) as offendable
	if (otherUnit.isABuilding() && model.getUnitsData()->getDynamicUnitData(otherUnit.data.getId()).getBuildCost() <= 2)
		return false;

	cMapView mapView(model.getMap(), nullptr);
	if (isInRange (otherUnit.getPosition()) && canAttackObjectAt (otherUnit.getPosition(), mapView, true, false))
	{
		// test, if this vehicle can really attack the opponentVehicle
		cUnit* target = cAttackJob::selectTarget (otherUnit.getPosition(), staticData->canAttack, mapView, getOwner());
		if (target == &otherUnit)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::doesPlayerWantToFireOnThisVehicleAsReactionFire(const cModel& model, const cPlayer* player) const
{
	if (model.getGameSettings()->getGameType() == eGameSettingsGameType::Turns)
	{
		// In the turn based game style,
		// the opponent always fires on the unit if he can,
		// regardless if the unit is offending or not.
		return true;
	}
	else
	{
		// check if there is a vehicle or building of player, that is offended

		const auto& vehicles = player->getVehicles();
		for (auto i = vehicles.begin(); i != vehicles.end(); ++i)
		{
			const auto& opponentVehicle = *i;
			if (isOtherUnitOffendedByThis (model, *opponentVehicle))
				return true;
		}
		const auto& buildings = player->getBuildings();
		for (auto i = buildings.begin(); i != buildings.end(); ++i)
		{
			const auto& opponentBuilding = *i;
			if (isOtherUnitOffendedByThis (model, *opponentBuilding))
				return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::doReactionFireForUnit (cModel& model, cUnit* opponentUnit) const
{
	cMapView mapView(model.getMap(), nullptr);
	if (opponentUnit->isSentryActive() == false && opponentUnit->isManualFireActive() == false
		&& opponentUnit->canAttackObjectAt (getPosition(), mapView, true)
		// Possible TODO: better handling of stealth units.
		// e.g. do reaction fire, if already detected ?
		&& (opponentUnit->isAVehicle() == false || opponentUnit->getStaticUnitData().isStealthOn == TERRAIN_NONE))
	{
		if (makeAttackOnThis (model, opponentUnit, "reaction fire"))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::doReactionFire (cModel& model, cPlayer* player) const
{
	// search a unit of the opponent, that could fire on this vehicle
	// first look for a building
	const auto& buildings = player->getBuildings();
	for (auto i = buildings.begin(); i != buildings.end(); ++i)
	{
		const auto& opponentBuilding = *i;
		if (doReactionFireForUnit (model, opponentBuilding.get()))
			return true;
	}
	const auto& vehicles = player->getVehicles();
	for (auto i = vehicles.begin(); i != vehicles.end(); ++i)
	{
		const auto& opponentVehicle = *i;
		if (doReactionFireForUnit (model, opponentVehicle.get()))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::provokeReactionFire (cModel& model)
{
	// unit can't fire, so it can't provoke a reaction fire
	if (staticData->canAttack == false || data.getShots() <= 0 || data.getAmmo() <= 0)
		return false;

	const auto& playerList = model.getPlayerList();
	for (size_t i = 0; i != playerList.size(); ++i)
	{
		cPlayer& player = *playerList[i];
		if (&player == getOwner())
			continue;

		// The vehicle can't be seen by the opposing player.
		// No possibility for reaction fire.
		if (!player.canSeeUnit (*this, *model.getMap()))
			continue;

		if (!doesPlayerWantToFireOnThisVehicleAsReactionFire (model, &player))
			continue;

		if (doReactionFire (model, &player))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::canExitTo(const cPosition& position, const cMapView& map, const cStaticUnitData& unitData) const
{
	if (!map.possiblePlaceVehicle(unitData, position)) return false;
	if (staticData->factorAir > 0 && (position != getPosition())) return false;
	if (!isNextTo(position)) return false;

	return true;
}

//-----------------------------------------------------------------------------
bool cVehicle::canExitTo(const cPosition& position, const cMap& map, const cStaticUnitData& unitData) const
{
	if (!map.possiblePlaceVehicle(unitData, position, getOwner())) return false;
	if (staticData->factorAir > 0 && (position != getPosition())) return false;
	if (!isNextTo(position)) return false;

	return true;
}

//-----------------------------------------------------------------------------
bool cVehicle::canLoad (const cPosition& position, const cMapView& map, bool checkPosition) const
{
	if (map.isValidPosition (position) == false) return false;

	return canLoad (map.getField (position).getVehicle(), checkPosition);
}

//-----------------------------------------------------------------------------
bool cVehicle::canLoad (const cVehicle* vehicle, bool checkPosition) const
{
	if (loaded) return false;

	if (!vehicle) return false;

	if (vehicle->isUnitLoaded()) return false;

	if (storedUnits.size() >= static_cast<unsigned> (staticData->storageUnitsMax)) return false;

	if (checkPosition && !isNextTo (vehicle->getPosition())) return false;

	if (checkPosition && staticData->factorAir > 0 && (vehicle->getPosition() != getPosition())) return false;

	if (!Contains (staticData->storeUnitsTypes, vehicle->getStaticUnitData().isStorageType)) return false;

	if (vehicle->moving || vehicle->isAttacking()) return false;

	if (vehicle->getOwner() != getOwner() || vehicle->isUnitBuildingABuilding() || vehicle->isUnitClearing()) return false;

	if (vehicle->isBeeingAttacked()) return false;

	return true;
}

//-----------------------------------------------------------------------------
/** Checks, if an object can get ammunition. */
//-----------------------------------------------------------------------------
bool cVehicle::canSupply (const cMapView& map, const cPosition& position, eSupplyType supplyType) const
{
	if (map.isValidPosition (position) == false) return false;

	const auto& field = map.getField (position);
	if (field.getVehicle()) return canSupply (field.getVehicle(), supplyType);
	else if (field.getPlane()) return canSupply (field.getPlane(), supplyType);
	else if (field.getTopBuilding()) return canSupply (field.getTopBuilding(), supplyType);

	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::canSupply (const cUnit* unit, eSupplyType supplyType) const
{
	if (unit == nullptr || unit == this)
		return false;

	if (getStoredResources() <= 0)
		return false;

	if (unit->isNextTo (getPosition()) == false)
		return false;

	if (unit->isAVehicle() && unit->getStaticUnitData().factorAir > 0 && static_cast<const cVehicle*> (unit)->getFlightHeight() > 0)
		return false;

	if (unit->isAVehicle() && static_cast<const cVehicle*> (unit)->isUnitMoving())
		return false;

	if (unit->isAttacking())
		return false;

	switch (supplyType)
	{
		case eSupplyType::REARM:
			if (unit->getStaticUnitData().canAttack == false || unit->data.getAmmo() >= unit->data.getAmmoMax())
				return false;
			if (!staticData->canRearm)
				return false;
			break;
		case eSupplyType::REPAIR:
			if (unit->data.getHitpoints() >= unit->data.getHitpointsMax())
				return false;
			if (!staticData->canRepair)
				return false;
			break;
		default:
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
void cVehicle::layMine (cModel& model)
{
	if (getStoredResources() <= 0) return;

	const cMap& map = *model.getMap();
	if (staticData->factorSea > 0 && staticData->factorGround == 0)
	{
		const auto& staticMineData = model.getUnitsData()->getSeaMineData();
		if (!map.possiblePlaceBuilding (staticMineData, getPosition(), nullptr, this)) return;
		model.addBuilding(getPosition(), staticMineData.ID, getOwner());
	}
	else
	{
		const auto& staticMineData = model.getUnitsData()->getLandMineData();
		if (!map.possiblePlaceBuilding(staticMineData, getPosition(), nullptr, this)) return;
		model.addBuilding(getPosition(), staticMineData.ID, getOwner());
	}
	setStoredResources (getStoredResources() - 1);

	if (getStoredResources() <= 0) setLayMines (false);

	return;
}

//-----------------------------------------------------------------------------
void cVehicle::clearMine (cModel& model)
{
	const cMap& map = *model.getMap();
	cBuilding* mine = map.getField (getPosition()).getMine();

	if (!mine || mine->getOwner() != getOwner() || getStoredResources() >= staticData->storageResMax) return;

	// sea minelayer can't collect land mines and vice versa
	if (mine->getStaticUnitData().factorGround > 0 && staticData->factorGround == 0) return;
	if (mine->getStaticUnitData().factorSea > 0 && staticData->factorSea == 0) return;

	model.deleteUnit(mine);
	setStoredResources (getStoredResources() + 1);

	if (getStoredResources() >= staticData->storageResMax) setClearMines (false);

	return;
}

//-----------------------------------------------------------------------------
void cVehicle::tryResetOfDetectionStateBeforeMove (const cMap& map, const std::vector<std::shared_ptr<cPlayer>>& playerList)
{
	for (const auto& player : playerList)
	{
		if (!Contains(detectedInThisTurnByPlayerList, player->getId()) && !checkDetectedByPlayer(*player, map))
		{
			resetDetectedByPlayer(player.get());
		}
	}
}

//-----------------------------------------------------------------------------
sVehicleUIData::sVehicleUIData() :
	hasCorpse(false),
	hasDamageEffect(false),
	hasPlayerColor(false),
	hasOverlay(false),
	buildUpGraphic(false),
	animationMovement(false),
	powerOnGraphic(false),
	isAnimated(false),
	makeTracks(false),
	hasFrames(0)
{}

//-----------------------------------------------------------------------------
sVehicleUIData::sVehicleUIData (sVehicleUIData&& other) :
	hasCorpse(other.hasCorpse),
	hasDamageEffect(other.hasDamageEffect),
	hasPlayerColor(other.hasPlayerColor),
	hasOverlay(other.hasOverlay),
	buildUpGraphic(other.buildUpGraphic),
	animationMovement(other.animationMovement),
	powerOnGraphic(other.powerOnGraphic),
	isAnimated(other.isAnimated),
	makeTracks(other.makeTracks),
	hasFrames(0),
	build (std::move (other.build)), build_org (std::move (other.build_org)),
	build_shw (std::move (other.build_shw)), build_shw_org (std::move (other.build_shw_org)),
	clear_small (std::move (other.clear_small)), clear_small_org (std::move (other.clear_small_org)),
	clear_small_shw (std::move (other.clear_small_shw)), clear_small_shw_org (std::move (other.clear_small_shw_org)),
	overlay (std::move (other.overlay)), overlay_org (std::move (other.overlay_org)),
	storage (std::move (other.storage)),
	FLCFile (std::move (other.FLCFile)),
	info (std::move (other.info)),
	Wait (std::move (other.Wait)),
	WaitWater (std::move (other.WaitWater)),
	Start (std::move (other.Start)),
	StartWater (std::move (other.StartWater)),
	Stop (std::move (other.Stop)),
	StopWater (std::move (other.StopWater)),
	Drive (std::move (other.Drive)),
	DriveWater (std::move (other.DriveWater)),
	Attack (std::move (other.Attack))
{
	for (size_t i = 0; i < img.size(); ++i) img[i] = std::move (other.img[i]);
	for (size_t i = 0; i < img_org.size(); ++i) img_org[i] = std::move (other.img_org[i]);
	for (size_t i = 0; i < shw.size(); ++i) shw[i] = std::move (other.shw[i]);
	for (size_t i = 0; i < shw_org.size(); ++i) shw_org[i] = std::move (other.shw_org[i]);
}

//-----------------------------------------------------------------------------
sVehicleUIData& sVehicleUIData::operator= (sVehicleUIData && other)
{
	for (size_t i = 0; i < img.size(); ++i) img[i] = std::move (other.img[i]);
	for (size_t i = 0; i < img_org.size(); ++i) img_org[i] = std::move (other.img_org[i]);
	for (size_t i = 0; i < shw.size(); ++i) shw[i] = std::move (other.shw[i]);
	for (size_t i = 0; i < shw_org.size(); ++i) shw_org[i] = std::move (other.shw_org[i]);

	build = std::move (other.build);
	build_org = std::move (other.build_org);
	build_shw = std::move (other.build_shw);
	build_shw_org = std::move (other.build_shw_org);
	clear_small = std::move (other.clear_small);
	clear_small_org = std::move (other.clear_small_org);
	clear_small_shw = std::move (other.clear_small_shw);
	clear_small_shw_org = std::move (other.clear_small_shw_org);
	overlay = std::move (other.overlay);
	overlay_org = std::move (other.overlay_org);

	Wait = std::move (other.Wait);
	WaitWater = std::move (other.WaitWater);
	Start = std::move (other.Start);
	StartWater = std::move (other.StartWater);
	Stop = std::move (other.Stop);
	StopWater = std::move (other.StopWater);
	Drive = std::move (other.Drive);
	DriveWater = std::move (other.DriveWater);
	Attack = std::move (other.Attack);

	hasCorpse = other.hasCorpse;
	hasDamageEffect = other.hasDamageEffect;
	hasPlayerColor = other.hasPlayerColor;
	hasOverlay = other.hasOverlay;
	buildUpGraphic = other.buildUpGraphic;
	animationMovement = other.animationMovement;
	powerOnGraphic = other.powerOnGraphic;
	isAnimated = other.isAnimated;
	makeTracks = other.makeTracks;
	hasFrames = 0;
	return *this;
}

//-----------------------------------------------------------------------------
void sVehicleUIData::scaleSurfaces (float factor)
{
	int width, height;
	for (int i = 0; i < 8; ++i)
	{
		width = (int) (img_org[i]->w * factor);
		height = (int) (img_org[i]->h * factor);
		scaleSurface (img_org[i].get(), img[i].get(), width, height);
		width = (int) (shw_org[i]->w * factor);
		height = (int) (shw_org[i]->h * factor);
		scaleSurface (shw_org[i].get(), shw[i].get(), width, height);
	}
	if (build_org)
	{
		height = (int) (build_org->h * factor);
		width = height * 4;
		scaleSurface (build_org.get(), build.get(), width, height);
		width = (int) (build_shw_org->w * factor);
		height = (int) (build_shw_org->h * factor);
		scaleSurface (build_shw_org.get(), build_shw.get(), width, height);
	}
	if (clear_small_org)
	{
		height = (int) (clear_small_org->h * factor);
		width = height * 4;
		scaleSurface (clear_small_org.get(), clear_small.get(), width, height);
		width = (int) (clear_small_shw_org->w * factor);
		height = (int) (clear_small_shw_org->h * factor);
		scaleSurface (clear_small_shw_org.get(), clear_small_shw.get(), width, height);
	}
	if (overlay_org)
	{
		height = (int) (overlay_org->h * factor);
		width = (int) (overlay_org->w * factor);
		scaleSurface (overlay_org.get(), overlay.get(), width, height);
	}
}

//-----------------------------------------------------------------------------
void cVehicle::blitWithPreScale (SDL_Surface* org_src, SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dest, SDL_Rect* destrect, float factor, int frames)
{
	if (!cSettings::getInstance().shouldDoPrescale())
	{
		int width, height;
		height = (int) (org_src->h * factor);
		if (frames > 1) width = height * frames;
		else width = (int) (org_src->w * factor);
		if (src->w != width || src->h != height)
		{
			scaleSurface (org_src, src, width, height);
		}
	}
	blittAlphaSurface (src, srcrect, dest, destrect);
}

cBuilding* cVehicle::getContainerBuilding()
{
	if (!isUnitLoaded()) return nullptr;

	const auto& buildings = getOwner()->getBuildings();
	for (auto i = buildings.begin(); i != buildings.end(); ++i)
	{
		const auto& building = *i;
		if (Contains (building->storedUnits, this)) return building.get();
	}

	return nullptr;
}

cVehicle* cVehicle::getContainerVehicle()
{
	if (!isUnitLoaded()) return nullptr;

	const auto& vehicles = getOwner()->getVehicles();
	for (auto i = vehicles.begin(); i != vehicles.end(); ++i)
	{
		const auto& vehicle = *i;
		if (Contains (vehicle->storedUnits, this)) return vehicle.get();
	}

	return nullptr;
}

uint32_t cVehicle::getChecksum(uint32_t crc) const
{
	crc = cUnit::getChecksum(crc);
	crc = calcCheckSum(surveyorAutoMoveActive, crc);
	crc = calcCheckSum(bandPosition, crc);
	crc = calcCheckSum(buildBigSavedPosition, crc);
	crc = calcCheckSum(BuildPath, crc);
	crc = calcCheckSum(WalkFrame, crc);
	crc = calcCheckSum(tileMovementOffset, crc);
	crc = calcCheckSum(loaded, crc);
	crc = calcCheckSum(moving, crc);
	crc = calcCheckSum(isBuilding, crc);
	crc = calcCheckSum(buildingTyp, crc);
	crc = calcCheckSum(buildCosts, crc);
	crc = calcCheckSum(buildTurns, crc);
	crc = calcCheckSum(buildTurnsStart, crc);
	crc = calcCheckSum(buildCostsStart, crc);
	crc = calcCheckSum(isClearing, crc);
	crc = calcCheckSum(clearingTurns, crc);
	crc = calcCheckSum(layMines, crc);
	crc = calcCheckSum(clearMines, crc);
	crc = calcCheckSum(flightHeight, crc);
	crc = commandoData.calcCheckSum(crc);

	return crc;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-- methods, that already have been extracted as part of cUnit refactoring
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool cVehicle::canBeStoppedViaUnitMenu() const
{
	return (moveJob != nullptr || (isUnitBuildingABuilding() && getBuildTurns() > 0) || (isUnitClearing() && getClearingTurns() > 0));
}

//-----------------------------------------------------------------------------
bool cVehicle::canLand (const cMap& map) const
{
	// normal vehicles are always "landed"
	if (staticData->factorAir == 0) return true;

	//vehicle busy?
	if ((moveJob && !moveJob->isFinished()) || isAttacking()) return false;
	if (isUnitMoving()) return false;

	// landing pad there?
	const std::vector<cBuilding*>& buildings = map.getField (getPosition()).getBuildings();
	std::vector<cBuilding*>::const_iterator b_it = buildings.begin();
	for (; b_it != buildings.end(); ++b_it)
	{
		if ((*b_it)->getStaticUnitData().canBeLandedOn)
			break;
	}
	if (b_it == buildings.end()) return false;

	// is the landing pad already occupied?
	const std::vector<cVehicle*>& v = map.getField (getPosition()).getPlanes();
	for (std::vector<cVehicle*>::const_iterator it = v.begin(); it != v.end(); ++it)
	{
		const cVehicle& vehicle = **it;
		if (vehicle.getFlightHeight() < 64 && vehicle.iID != iID)
			return false;
	}

	// returning true before checking owner, because a stolen vehicle
	// can stay on an enemy landing pad until it is moved
	if (getFlightHeight() == 0) return true;

	if ((*b_it)->getOwner() != getOwner()) return false;

	return true;
}

//-----------------------------------------------------------------------------
void cVehicle::setMoving (bool value)
{
	std::swap (moving, value);
	if (value != moving) movingChanged();
}

//-----------------------------------------------------------------------------
void cVehicle::setLoaded (bool value)
{
	std::swap (loaded, value);
	if (value != loaded)
	{
		if (loaded) stored();
		else activated();
	}
}

//-----------------------------------------------------------------------------
void cVehicle::setClearing (bool value)
{
	std::swap (isClearing, value);
	if (value != isClearing) clearingChanged();
}

//-----------------------------------------------------------------------------
void cVehicle::setBuildingABuilding (bool value)
{
	std::swap (isBuilding, value);
	if (value != isBuilding) buildingChanged();
}

//-----------------------------------------------------------------------------
void cVehicle::setLayMines (bool value)
{
	std::swap (layMines, value);
	if (value != layMines) layingMinesChanged();
}

//-----------------------------------------------------------------------------
void cVehicle::setClearMines (bool value)
{
	std::swap (clearMines, value);
	if (value != clearMines) clearingMinesChanged();
}

//-----------------------------------------------------------------------------
int cVehicle::getClearingTurns() const
{
	return clearingTurns;
}

//-----------------------------------------------------------------------------
void cVehicle::setClearingTurns (int value)
{
	std::swap (clearingTurns, value);
	if (value != clearingTurns) clearingTurnsChanged();
}

//-----------------------------------------------------------------------------
const sID& cVehicle::getBuildingType() const
{
	return buildingTyp;
}

//-----------------------------------------------------------------------------
void cVehicle::setBuildingType (const sID& id)
{
	auto oldId = id;
	buildingTyp = id;
	if (buildingTyp != oldId) buildingTypeChanged();
}

//-----------------------------------------------------------------------------
int cVehicle::getBuildCosts() const
{
	return buildCosts;
}

//-----------------------------------------------------------------------------
void cVehicle::setBuildCosts (int value)
{
	std::swap (buildCosts, value);
	if (value != buildCosts) buildingCostsChanged();
}

//-----------------------------------------------------------------------------
int cVehicle::getBuildTurns() const
{
	return buildTurns;
}

//-----------------------------------------------------------------------------
void cVehicle::setBuildTurns (int value)
{
	std::swap (buildTurns, value);
	if (value != buildTurns) buildingTurnsChanged();
}

//-----------------------------------------------------------------------------
int cVehicle::getBuildCostsStart() const
{
	return buildCostsStart;
}

//-----------------------------------------------------------------------------
void cVehicle::setBuildCostsStart (int value)
{
	std::swap (buildCostsStart, value);
	//if (value != buildCostsStart) event ();
}

//-----------------------------------------------------------------------------
int cVehicle::getBuildTurnsStart() const
{
	return buildTurnsStart;
}

//-----------------------------------------------------------------------------
void cVehicle::setBuildTurnsStart (int value)
{
	std::swap (buildTurnsStart, value);
	//if (value != buildTurnsStart) event ();
}

//------------------------------------------------------------------------------
void cVehicle::setSurveyorAutoMoveActive(bool value)
{
	std::swap(surveyorAutoMoveActive, value);

	if (value != surveyorAutoMoveActive) autoMoveJobChanged();
}

//-----------------------------------------------------------------------------
int cVehicle::getFlightHeight() const
{
	return flightHeight;
}

//-----------------------------------------------------------------------------
void cVehicle::setFlightHeight (int value)
{
	value = std::min (std::max (value, 0), MAX_FLIGHT_HEIGHT);
	std::swap (flightHeight, value);
	if (flightHeight != value) flightHeightChanged();
}

//-----------------------------------------------------------------------------
cMoveJob* cVehicle::getMoveJob()
{
	return moveJob;
}

//-----------------------------------------------------------------------------
const cMoveJob* cVehicle::getMoveJob() const
{
	return moveJob;
}

//-----------------------------------------------------------------------------
void cVehicle::setMoveJob (cMoveJob* moveJob_)
{
	std::swap (moveJob, moveJob_);
	if (moveJob != moveJob_) moveJobChanged();
}

//------------------------------------------------------------------------------
void cVehicle::triggerLandingTakeOff(cModel& model)
{
	if (canLand(*model.getMap()))
	{
		// height should be 0
		if (flightHeight > 0)
		{
			model.addJob(new cPlaneTakeoffJob(*this));
		}
	}
	else
	{
		// height should be MAX
		if (flightHeight < MAX_FLIGHT_HEIGHT)
		{
			model.addJob(new cPlaneTakeoffJob(*this));
		}
	}
}
