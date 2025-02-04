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

#include "input/keyboard/keycombination.h"
#include "input/keyboard/keysequence.h"

#include <3rd/doctest/doctest.h>

namespace
{

	//--------------------------------------------------------------------------
	TEST_CASE ("Parsing KeyCombination")
	{
		CHECK (cKeyCombination (SDLK_a) == cKeyCombination ("a"));
		CHECK (cKeyCombination (eKeyModifierType::Ctrl, SDLK_a) == cKeyCombination ("Ctrl+a"));
		CHECK (cKeyCombination (eKeyModifierType::Ctrl, SDLK_PLUS) == cKeyCombination ("Ctrl+PLUS"));
		CHECK (cKeyCombination (KeyModifierFlags(eKeyModifierType::Ctrl) | eKeyModifierType::Alt, SDLK_COMMA) == cKeyCombination ("Alt+Ctrl+Comma"));
	}

	//--------------------------------------------------------------------------
	TEST_CASE ("Parsing KeySequence")
	{
		CHECK (cKeySequence (cKeyCombination ("a"), cKeyCombination ("b")) == cKeySequence ("a,b"));
		CHECK (cKeySequence (cKeyCombination ("a"), cKeyCombination ("b"), cKeyCombination ("c")) == cKeySequence ("a,b,c"));
		CHECK (cKeySequence (cKeyCombination ("PLUS"), cKeyCombination ("COMMA")) == cKeySequence ("PLUS,COMMA"));
		CHECK (cKeySequence (cKeyCombination ("Ctrl+a"), cKeyCombination ("Shift+b")) == cKeySequence ("Ctrl+a,Shift+b"));
	}

} // namespace
