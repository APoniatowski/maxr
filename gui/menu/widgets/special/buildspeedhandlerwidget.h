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

#ifndef gui_menu_widgets_special_buildspeedhandlerwidgetH
#define gui_menu_widgets_special_buildspeedhandlerwidgetH

#include <array>

#include "maxrconfig.h"
#include "gui/widget.h"

class cLabel;
class cCheckBox;

class cBuildSpeedHandlerWidget : public cWidget
{
	static const size_t elementsCount = 3;
public:
	cBuildSpeedHandlerWidget (const cPosition& position);

	void setValues (const std::array<int, elementsCount>& turns, const std::array<int, elementsCount>& costs);

	void setBuildSpeedIndex (size_t speedIndex);
	size_t getBuildSpeedIndex ();
private:
	std::array<cLabel*, elementsCount> turnLabels;
	std::array<cLabel*, elementsCount> costLabels;
	std::array<cCheckBox*, elementsCount> buttons;
};

#endif // gui_menu_widgets_special_buildspeedhandlerwidgetH
