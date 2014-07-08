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

#ifndef gui_menu_widgets_special_reportdisadvantageslistviewitemH
#define gui_menu_widgets_special_reportdisadvantageslistviewitemH

#include "gui/menu/widgets/abstractlistviewitem.h"
#include "main.h" // sID

class cCasualtiesTracker;

class cReportDisadvantagesListViewItem : public cAbstractListViewItem
{
public:
	static const int unitImageWidth = 32;
	static const int unitImageHeight = 32;
	static const int unitNameWidth = 110;
	static const int casualityLabelWidth = 75;
	static const int maxItemsInRow = 4;

	cReportDisadvantagesListViewItem (const sID& unitId, std::vector<int> disadvantages);

protected:
	sID unitId;
	std::vector<int> disadvantages;
};

#endif // gui_menu_widgets_special_reportdisadvantageslistviewitemH
