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

#include "toString.h"

#include <string>

std::string iToHex (unsigned int x)
{
	std::stringstream strStream;
	strStream << std::hex << x;
	return strStream.str();
}

std::string pToStr (const void* x)
{
	std::stringstream strStream;
	strStream << x;
	return "0x" + strStream.str();
}

std::string bToStr (bool x)
{
	return x ? "true" : "false";
}

//--------------------------------------------------------------------------
std::string getHexValue(unsigned char byte)
{
	std::string str = "";
	const char hexChars[] = "0123456789ABCDEF";
	const unsigned char high = (byte >> 4) & 0x0F;
	const unsigned char low = byte & 0x0F;

	str += hexChars[high];
	str += hexChars[low];
	return str;
}
//--------------------------------------------------------------------------
unsigned char getByteValue(const std::string& str, int index)
{
	unsigned char first = str[index + 0] - '0';
	unsigned char second = str[index + 1] - '0';

	if (first >= 'A' - '0') first -= 'A' - '0' - 10;
	if (second >= 'A' - '0') second -= 'A' - '0' - 10;
	return (first * 16 + second);
}
