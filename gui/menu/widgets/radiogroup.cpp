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

#include "gui/menu/widgets/radiogroup.h"
#include "gui/menu/widgets/checkbox.h"


//------------------------------------------------------------------------------
cRadioGroup::cRadioGroup (bool allowUncheckAll_) :
	currentlyCheckedButton (nullptr),
	allowUncheckAll (allowUncheckAll_),
	internalMoving (false)
{}

//------------------------------------------------------------------------------
cRadioGroup::~cRadioGroup ()
{}

//------------------------------------------------------------------------------
cCheckBox* cRadioGroup::addButton (std::unique_ptr<cCheckBox> button)
{
	const bool hadButtons = hasChildren();
	
	auto addedbutton = addChild (std::move (button));

	if (currentlyCheckedButton == nullptr && !allowUncheckAll && !addedbutton->isChecked ()) addedbutton->setChecked (true);

	signalConnectionManager.connect (addedbutton->toggled, std::bind (&cRadioGroup::buttonToggled, this, addedbutton));
	buttonToggled (addedbutton);

	// resize own area to include the new button
	internalMoving = true;
	if (hadButtons)
	{
		auto area = getArea ();
		area.add (addedbutton->getArea ());
		setArea (area);
	}
	else
	{
		setArea (addedbutton->getArea ());
	}
	internalMoving = false;

	return addedbutton;
}

//------------------------------------------------------------------------------
void cRadioGroup::handleMoved (const cPosition& offset)
{
	if (internalMoving) return;

	cWidget::handleMoved (offset);
}

//------------------------------------------------------------------------------
void cRadioGroup::buttonToggled (cCheckBox* button)
{
	if (button == currentlyCheckedButton)
	{
		if (!button->isChecked ())
		{
			if (!allowUncheckAll) button->setChecked (true);
			else currentlyCheckedButton = false;
		}
	}
	else
	{
		if (button->isChecked ())
		{
			auto oldCheckedButton = currentlyCheckedButton;
			currentlyCheckedButton = button;
			if (oldCheckedButton) oldCheckedButton->setChecked (false);
		}
	}
}
