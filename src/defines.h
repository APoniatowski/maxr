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
#ifndef definesH
#define definesH

#include "settings.h"

#ifndef PATH_DELIMITER
# ifdef WIN32
#  define PATH_DELIMITER "\\"
# else
#  define PATH_DELIMITER "/"
# endif
#endif

#define DEFAULTPORT 58600
#define MAX_XML "maxr.xml"
#define CLANS_XML (cSettings::getInstance().getDataDir() + "clans.xml").c_str()
#define KEYS_XMLGame (cSettings::getInstance().getDataDir() + "keys.xml").c_str()
#define KEYS_XMLUsers (cSettings::getInstance().getHomeDir() + "keys.xml").c_str()

#define MAXR_ICON (cSettings::getInstance().getDataDir() + "maxr.bmp").c_str()

//#define DEDICATED_SERVER_APPLICATION 1
#if DEDICATED_SERVER_APPLICATION
# define DEDICATED_SERVER true
#else
# define DEDICATED_SERVER false
#endif

#endif
