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

#ifndef ui_graphical_menu_control_menucontrollermultiplayerhostH
#define ui_graphical_menu_control_menucontrollermultiplayerhostH

#include <memory>
#include <vector>

#include "game/connectionmanager.h"
#include "game/startup/lobbyclient.h"
#include "game/startup/lobbyserver.h"
#include "ui/graphical/menu/control/initgamepreparation.h"
#include "utility/runnable.h"
#include "utility/signal/signalconnectionmanager.h"

class cApplication;
class cNetworkHostGameNew;
class cWindowNetworkLobbyHost;

class cMenuControllerMultiplayerHost : public cRunnable, public std::enable_shared_from_this<cMenuControllerMultiplayerHost>
{
public:
	explicit cMenuControllerMultiplayerHost (cApplication& application);
	~cMenuControllerMultiplayerHost();

	void start();

	void run() override;
private:
	cSignalConnectionManager signalConnectionManager;

	std::shared_ptr<cConnectionManager> connectionManager = std::make_shared<cConnectionManager>();
	cLobbyServer lobbyServer;
	cLobbyClient lobbyClient;

	cApplication& application;

	std::shared_ptr<cWindowNetworkLobbyHost> windowNetworkLobby;
	std::shared_ptr<cInitGamePreparation> initGamePreparation;
	std::shared_ptr<cNetworkHostGameNew> newGame;

private:
	void reset();

	void handleSelectMap();
	void handleSelectSettings();
	void handleSelectSaveGame();

	void startHost();

	void checkGameStart();

	void startSavedGame (const cSaveGameInfo&, std::shared_ptr<cStaticMap>, std::shared_ptr<cConnectionManager>, cPlayerBasicData);
	void startGamePreparation (const sLobbyPreparationData&, const cPlayerBasicData& localPlayer, std::shared_ptr<cConnectionManager>);

	void startNewGame();

	void saveOptions();
};

#endif // ui_graphical_menu_control_menucontrollermultiplayerhostH
