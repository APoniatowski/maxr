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

#include "windowmultiplayer.h"

#include "ui/graphical/menu/control/local/hotseat/localhotseatgamesaved.h"
#include "ui/graphical/menu/control/menucontrollermultiplayerclient.h"
#include "ui/graphical/menu/control/menucontrollermultiplayerhost.h"
#include "ui/graphical/menu/control/menucontrollermultiplayerhotseat.h"
#include "ui/graphical/menu/widgets/pushbutton.h"
#include "ui/graphical/menu/windows/windowload/windowload.h"
#include "ui/widgets/application.h"
#include "utility/language.h"

#include <functional>

//------------------------------------------------------------------------------
cWindowMultiPlayer::cWindowMultiPlayer() :
	cWindowMain (lngPack.i18n ("Text~Others~Multi_Player"))
{
	auto hastButton = addChild (std::make_unique<cPushButton> (getPosition() + cPosition (390, 190), ePushButtonType::StandardBig, lngPack.i18n ("Text~Others~TCPIP_Host")));
	signalConnectionManager.connect (hastButton->clicked, [this]() { tcpHostClicked(); });

	auto clientButton = addChild (std::make_unique<cPushButton> (getPosition() + cPosition (390, 190 + buttonSpace), ePushButtonType::StandardBig, lngPack.i18n ("Text~Others~TCPIP_Client")));
	signalConnectionManager.connect (clientButton->clicked, [this]() { tcpClientClicked(); });

	auto newHotSeatButton = addChild (std::make_unique<cPushButton> (getPosition() + cPosition (390, 190 + buttonSpace * 2), ePushButtonType::StandardBig, lngPack.i18n ("Text~Others~HotSeat_New")));
	signalConnectionManager.connect (newHotSeatButton->clicked, [this]() { newHotSeatClicked(); });

	auto loadHotSeatButton = addChild (std::make_unique<cPushButton> (getPosition() + cPosition (390, 190 + buttonSpace * 3), ePushButtonType::StandardBig, lngPack.i18n ("Text~Others~HotSeat_Load")));
	signalConnectionManager.connect (loadHotSeatButton->clicked, [this]() { loadHotSeatClicked(); });

	auto backButton = addChild (std::make_unique<cPushButton> (getPosition() + cPosition (415, 190 + buttonSpace * 6), ePushButtonType::StandardSmall, lngPack.i18n ("Text~Others~Back")));
	signalConnectionManager.connect (backButton->clicked, [this]() { backClicked(); });
}

//------------------------------------------------------------------------------
void cWindowMultiPlayer::tcpHostClicked()
{
	auto application = getActiveApplication();

	if (!application) return;

	auto multiplayerHostController = std::make_shared<cMenuControllerMultiplayerHost> (*application);
	multiplayerHostController->start();
}

//------------------------------------------------------------------------------
void cWindowMultiPlayer::tcpClientClicked()
{
	auto application = getActiveApplication();

	if (!application) return;

	auto multiplayerClientController = std::make_shared<cMenuControllerMultiplayerClient> (*application);
	multiplayerClientController->start();
}

//------------------------------------------------------------------------------
void cWindowMultiPlayer::newHotSeatClicked()
{
	auto application = getActiveApplication();

	if (!application) return;

	auto multiplayerHotSeatController = std::make_shared<cMenuControllerMultiplayerHotSeat> (*application);
	multiplayerHotSeatController->start();
}

//------------------------------------------------------------------------------
void cWindowMultiPlayer::loadHotSeatClicked()
{
	if (!getActiveApplication()) return;

	auto application = getActiveApplication();

	auto windowLoad = getActiveApplication()->show (std::make_shared<cWindowLoad>());
	windowLoad->load.connect ([=] (const cSaveGameInfo& saveInfo) {
		auto game = std::make_shared<cLocalHotSeatGameSaved>();
		game->setSaveGameNumber (saveInfo.number);
		game->start (*application);

		windowLoad->close();
	});
}

//------------------------------------------------------------------------------
void cWindowMultiPlayer::backClicked()
{
	close();
}
