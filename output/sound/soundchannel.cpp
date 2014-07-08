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

#include <SDL_mixer.h>

#include "output/sound/soundchannel.h"
#include "output/sound/soundchunk.h"
#include "log.h"

//--------------------------------------------------------------------------
cSoundChannel::cSoundChannel (int sdlChannelId_) :
	sdlChannelId (sdlChannelId_)
{}

//--------------------------------------------------------------------------
void cSoundChannel::play (const cSoundChunk& chunk, bool loop)
{
	if (chunk.getSdlSound () == nullptr) return;

	if (Mix_PlayChannel (sdlChannelId, chunk.getSdlSound (), loop ? -1 : 0) < 0)
	{
		Log.write ("Could not play sound:", cLog::eLOG_TYPE_WARNING);
		Log.write (Mix_GetError (), cLog::eLOG_TYPE_WARNING);
	}
}

//--------------------------------------------------------------------------
void cSoundChannel::pause ()
{
	Mix_Pause (sdlChannelId);
}

//--------------------------------------------------------------------------
void cSoundChannel::resume ()
{
	Mix_Resume (sdlChannelId);
}

//--------------------------------------------------------------------------
void cSoundChannel::stop ()
{
	Mix_HaltChannel (sdlChannelId);
}

//--------------------------------------------------------------------------
bool cSoundChannel::isPlaying () const
{
	return Mix_Playing (sdlChannelId) != 0;
}

//--------------------------------------------------------------------------
bool cSoundChannel::isPlaying (const cSoundChunk& chunk) const
{
	return isPlaying () && Mix_GetChunk (sdlChannelId) == chunk.getSdlSound ();
}

//--------------------------------------------------------------------------
bool cSoundChannel::isPaused () const
{
	return Mix_Paused (sdlChannelId) != 0;
}

//--------------------------------------------------------------------------
void cSoundChannel::setVolume (int volume)
{
	Mix_Volume (sdlChannelId, volume);
}

//--------------------------------------------------------------------------
void cSoundChannel::setPanning (unsigned char left, unsigned char right)
{
	Mix_SetPanning (sdlChannelId, left, right);
}

//--------------------------------------------------------------------------
void cSoundChannel::setDistance (unsigned char distance)
{
	Mix_SetDistance (sdlChannelId, distance);
}

//--------------------------------------------------------------------------
void cSoundChannel::setPosition (short angle, unsigned char distance)
{
	Mix_SetPosition (sdlChannelId, angle, distance);
}

//--------------------------------------------------------------------------
int cSoundChannel::getSdlChannelId () const
{
	return sdlChannelId;
}
