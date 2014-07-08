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

#ifndef gui_game_widgets_gamemessagelistviewitemH
#define gui_game_widgets_gamemessagelistviewitemH

#include <chrono>

#include "gui/menu/widgets/abstractlistviewitem.h"
#include "autosurface.h"

class cLabel;

enum class eGameMessageListViewItemBackgroundColor
{
	DarkGray,
	LightGray,
	Red
};

class cGameMessageListViewItem : public cAbstractListViewItem
{
public:
	cGameMessageListViewItem (const std::string& message, eGameMessageListViewItemBackgroundColor backgroundColor);

	std::chrono::steady_clock::time_point getCreationTime () const;

	virtual void draw () MAXR_OVERRIDE_FUNCTION;

	virtual void handleResized (const cPosition& oldSize) MAXR_OVERRIDE_FUNCTION;
private:
	cLabel* messageLabel;
	
	eGameMessageListViewItemBackgroundColor backgroundColor;
	AutoSurface background;

	const cPosition beginMargin;
	const cPosition endMargin;

	std::chrono::steady_clock::time_point creationTime;

	void createBackground ();
};

#endif // gui_game_widgets_gamemessagelistviewitemH
