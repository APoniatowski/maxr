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

#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "fonts.h"
#include "main.h"

#define DEBUGFONTS true


// Funktionen der Font-Klasse ////////////////////////////////////////////////
cFonts::cFonts ( void )
{
	int i,last,anz;
	// Die Zeichenbreiten auslesen:
	chars=NULL;
	anz=0;
	last=0;
	for ( i=0;i<FontsData.font->w;i++ )
	{
		if ( ( ( unsigned int* ) FontsData.font->pixels ) [ ( FontsData.font->h-1 ) *FontsData.font->w+i]==0xFF00FF )
		{
			anz++;
			chars= ( SDL_Rect* ) realloc ( chars,sizeof ( SDL_Rect ) *anz );
			chars[anz-1].x=last;
			chars[anz-1].y=0;
			chars[anz-1].w=i-last+1;
			chars[anz-1].h=FontsData.font->h-1;
			last=i+1;
		}
	}
	chars_big=NULL;
	anz=0;
	last=0;
	for ( i=0;i<FontsData.font_big->w;i++ )
	{
		if ( ( ( unsigned int* ) FontsData.font_big->pixels ) [ ( FontsData.font_big->h-1 ) *FontsData.font_big->w+i]==0xFF00FF )
		{
			anz++;
			chars_big= ( SDL_Rect* ) realloc ( chars_big,sizeof ( SDL_Rect ) *anz );
			chars_big[anz-1].x=last;
			chars_big[anz-1].y=0;
			chars_big[anz-1].w=i-last+1;
			chars_big[anz-1].h=FontsData.font_big->h-1;
			last=i+1;
		}
	}
	chars_small=NULL;
	anz=0;
	last=0;
	for ( i=0;i<FontsData.font_small_white->w;i++ )
	{
		if ( ( ( unsigned int* ) FontsData.font_small_white->pixels ) [ ( FontsData.font_small_white->h-1 ) *FontsData.font_small_white->w+i]==0xFF00FF )
		{
			anz++;
			chars_small= ( SDL_Rect* ) realloc ( chars_small,sizeof ( SDL_Rect ) *anz );
			chars_small[anz-1].x=last;
			chars_small[anz-1].y=0;
			chars_small[anz-1].w=i-last+1;
			chars_small[anz-1].h=FontsData.font_small_white->h-1;
			last=i+1;
		}
	}
}

cFonts::~cFonts ( void )
{
	free ( chars );
	free ( chars_big );
	free ( chars_small );
}

// Konvertiert den Zeichensatz:
int cFonts::Charset ( char c )
{
	if ( c>='a'&&c<='z' ) return c-'a';
	if ( c>='0'&&c<='9' ) return 26+c-'0';
	if ( c>='A'&&c<='Z' ) return 36+c-'A';
	switch ( c )
	{
		case '�': return 62;
		case '�': return 63;
		case '�': return 64;
		case '�': return 65;
		case '�': return 66;
		case '�': return 67;
		case '�': return 68;
		case ',': return 69;
		case '.': return 70;
		case ';': return 71;
		case ':': return 72;
		case '-': return 73;
		case '_': return 74;
		case '#': return 75;
		case '\'': return 76;
		case '?': return 76;
		case '+': return 77;
		case '*': return 78;
		case '~': return 79;
		case '|': return 80;
		case '<': return 81;
		case '>': return 82;
		case '^': return 83;
		case '�': return 84;
		case '!': return 85;
		case '"': return 86;
		case '�': return 87;
		case '$': return 88;
		case '%': return 89;
		case '&': return 90;
		case '(': return 91;
		case ')': return 92;
		case '=': return 93;
		//case '?': return 94; //see case 76!
		case '`': return 95;
		case '{': return 96;
		case '[': return 97;
		case ']': return 98;
		case '}': return 99;
		case '\\': return 100;
		case '/': return 101;
		case '�': return 102;
		case '�': return 103;
	}
	return -1;
}

void cFonts::OutText (string str,int x,int y,SDL_Surface *sf )
{
	OutText((char *)str.c_str(),x,y,sf);
}

// Gibt einen Text mit dem normalen Font aus:
void cFonts::OutText (const char *str,int x,int y,SDL_Surface *sf )
{
	SDL_Rect dest;
	int i=0,index;

	dest.x=x;
	dest.y=y;

	while ( str[i]!=0 )
	{
		index=Charset ( str[i++] );
		if ( index<0 )
		{
			dest.x+=5;
		}
		else
		{
			dest.w=chars[index].w;
			dest.h=chars[index].h;
			SDL_BlitSurface ( FontsData.font,chars+index,sf,&dest );
			dest.x+=dest.w;
		}
	}
}

// Konvertiert den Zeichensatz:
int cFonts::CharsetBig ( char c )
{
	if ( c>='A'&&c<='Z' ) return c-'A';
	if ( c>='a'&&c<='z' ) return 26+c-'a';
	if ( c>='0'&&c<='9' ) return 64+c-'0';
	switch ( c )
	{
		case '�': return 52;
		case '�': return 53;
		case '�': return 54;
		case '�': return 55;
		case '�': return 56;
		case '�': return 57;
		case '?': return 58;
		case '(': return 59;
		case ')': return 60;
		case '/': return 61;
		case '+': return 62;
		case '-': return 63;
		case '&': return 74;
		case '.': return 75;
	}
	return -1;
}

void cFonts::OutTextBig (string str,int x,int y,SDL_Surface *sf )
{
	OutTextBig((char *)str.c_str(), x, y, sf);
}

// Gibt einen Text mit dem Big Font aus:
void cFonts::OutTextBig (char *str,int x,int y,SDL_Surface *sf )
{
	SDL_Rect dest;
	int i=0,index;

	dest.x=x;
	dest.y=y;

	while ( str[i]!=0 )
	{
		index=CharsetBig ( str[i++] );
		if ( index<0 )
		{
			dest.x+=8;
		}
		else
		{
			dest.w=chars_big[index].w;
			dest.h=chars_big[index].h;
			SDL_BlitSurface ( FontsData.font_big,chars_big+index,sf,&dest );
			dest.x+=dest.w;
		}
	}
}
void cFonts::OutTextBigCenter (string str,int x,int y,SDL_Surface *sf )
{
	OutTextBigCenter((char *)str.c_str(), x, y, sf);
}

void cFonts::OutTextBigCenter (char *str,int x,int y,SDL_Surface *sf )
{
	SDL_Rect dest,tmp;
	int i=0,index,width=0;

	while ( str[i]!=0 )
	{
		index=CharsetBig ( str[i++] );
		if ( index<0 )
		{
			width+=8;
		}
		else
		{
			width+=chars_big[index].w;
		}
	}
	i=0;
	dest.x=x-width/2;
	dest.y=y;
	while ( str[i]!=0 )
	{
		index=CharsetBig ( str[i++] );
		if ( index<0 )
		{
			dest.x+=8;
		}
		else
		{
			dest.w=chars_big[index].w;
			dest.h=chars_big[index].h;
			tmp=dest;
			SDL_BlitSurface ( FontsData.font_big,chars_big+index,sf,&tmp );
			dest.x+=dest.w;
		}
	}
}

void cFonts::OutTextBigCenterGold ( string str,int x,int y,SDL_Surface *sf )
{
	OutTextBigCenterGold( (char *)str.c_str(), x, y, sf);
}

void cFonts::OutTextBigCenterGold ( char *str,int x,int y,SDL_Surface *sf )
{
	SDL_Rect dest,tmp;
	int i=0,index,width=0;

	while ( str[i]!=0 )
	{
		index=CharsetBig ( str[i++] );
		if ( index<0 )
		{
			width+=8;
		}
		else
		{
			width+=chars_big[index].w;
		}
	}
	i=0;
	dest.x=x-width/2;
	dest.y=y;
	while ( str[i]!=0 )
	{
		index=CharsetBig ( str[i++] );
		if ( index<0 )
		{
			dest.x+=8;
		}
		else
		{
			dest.w=chars_big[index].w;
			dest.h=chars_big[index].h;
			tmp=dest;
			SDL_BlitSurface ( FontsData.font_big_gold,chars_big+index,sf,&tmp );
			dest.x+=dest.w;
		}
	}
}

// Konvertiert den Zeichensatz:
int cFonts::CharsetSmall ( char c )
{
	if ( c>='0'&&c<='9' ) return c-'0';
	if ( c>='A'&&c<='Z' ) return c-'A'+10;
	if ( c>='a'&&c<='z' ) return c-'a'+10;
	switch ( c )
	{
		case '/': return 36;
		case '�': return 37;
		case '�': return 37;
		case '�': return 38;
		case '�': return 38;
		case '�': return 39;
		case '�': return 39;
		case '_': return 40;
		case '+': return 41;
		case '*': return 42;
		case '-': return 43;
		case '!': return 44;
		case '"': return 45;
		case '%': return 46;
		case '\\': return 47;
		case '(': return 48;
		case ')': return 49;
		case '[': return 50;
		case ']': return 51;
		case '=': return 52;
		case '\'': return 53;
		case '?': return 53;
		case '`': return 53;
		case '~': return 54;
		case '#': return 55;
		case '.': return 56;
		case ',': return 57;
		case ':': return 58;
		case '<': return 59;
		case '>': return 60;
		case '^': return 61;
		case '�': return 62;
		case '�': return 63;
		case '|': return 1;
		case '\n': return -2;
	}
	return -1;
}

void cFonts::OutTextSmall (string str,int x,int y,eFontSmallColor color,SDL_Surface *sf )
{
	OutTextSmall ( (char *)str.c_str(), x, y, color, sf );
}

// Gibt einen Text mit dem Small Font aus:
void cFonts::OutTextSmall (const char *str,int x,int y,eFontSmallColor color,SDL_Surface *sf )
{
	SDL_Rect dest;
	int i=0,index;

	dest.x=x;
	dest.y=y;

	while ( str[i]!=0 )
	{
		index=CharsetSmall ( str[i++] );
		if ( index<0 )
		{
			if ( index==-2 )
			{
				dest.x=x;
				dest.y+=8;
			}
			else
			{
				dest.x+=4;
			}
		}
		else
		{
			dest.w=chars_small[index].w;
			dest.h=chars_small[index].h;
			switch ( color )
			{
				case ClWhite:
					SDL_BlitSurface ( FontsData.font_small_white,chars_small+index,sf,&dest );
					break;
				case ClRed:
					SDL_BlitSurface ( FontsData.font_small_red,chars_small+index,sf,&dest );
					break;
				case ClGreen:
					SDL_BlitSurface ( FontsData.font_small_green,chars_small+index,sf,&dest );
					break;
				case ClYellow:
					SDL_BlitSurface ( FontsData.font_small_yellow,chars_small+index,sf,&dest );
					break;
			}
			dest.x+=dest.w;
		}
	}
}

void cFonts::OutTextCenter (string str,int x,int y,SDL_Surface *sf )
{
	OutTextCenter((char *)str.c_str(),x,y,sf);
}

// Gibt den Text mit dem normalen Font zentriert aus:
void cFonts::OutTextCenter (const char *str,int x,int y,SDL_Surface *sf )
{
	SDL_Rect dest,tmp;
	int i=0,index,width=0;

	while ( str[i]!=0 )
	{
		index=Charset ( str[i++] );
		if ( index<0 )
		{
			width+=5;
		}
		else
		{
			width+=chars[index].w;
		}
	}
	i=0;
	dest.x=x-width/2;
	dest.y=y;
	while ( str[i]!=0 )
	{
		index=Charset ( str[i++] );
		if ( index<0 )
		{
			dest.x+=5;
		}
		else
		{
			dest.w=chars[index].w;
			dest.h=chars[index].h;
			tmp=dest;
			SDL_BlitSurface ( FontsData.font,chars+index,sf,&tmp );
			dest.x+=dest.w;
		}
	}
}

// Ermittelt die Text-L�nge:
int cFonts::GetTextLen (const char *str )
{
	int i=0,index,width=0;
	while ( str[i]!=0 )
	{
		index=Charset ( str[i++] );
		if ( index<0 )
		{
			width+=5;
		}
		else
		{
			width+=chars[index].w;
		}
	}
	return width;
}

// Ermittelt die Text-L�nge:
int cFonts::GetTextLenSmall ( char *str )
{
	int i=0,index,width=0;
	while ( str[i]!=0 )
	{
		index=CharsetSmall ( str[i++] );
		if ( index<0 )
		{
			width+=4;
		}
		else
		{
			width+=chars_small[index].w;
		}
	}
	return width;
}

// Gibt den Text mit dem small Font zentriert aus:
void cFonts::OutTextSmallCenter ( char *str,int x,int y,eFontSmallColor color,SDL_Surface *sf )
{
	SDL_Rect dest;
	int i=0,index,width=0;

	while ( str[i]!=0 )
	{
		index=CharsetSmall ( str[i++] );
		if ( index<0 )
		{
			width+=4;
		}
		else
		{
			width+=chars_small[index].w;
		}
	}
	i=0;
	dest.x=x-width/2;
	dest.y=y;
	while ( str[i]!=0 )
	{
		index=CharsetSmall ( str[i++] );
		if ( index<0 )
		{
			dest.x+=4;
		}
		else
		{
			dest.w=chars_small[index].w;
			dest.h=chars_small[index].h;
			switch ( color )
			{
				case ClWhite:
					SDL_BlitSurface ( FontsData.font_small_white,chars_small+index,sf,&dest );
					break;
				case ClRed:
					SDL_BlitSurface ( FontsData.font_small_red,chars_small+index,sf,&dest );
					break;
				case ClGreen:
					SDL_BlitSurface ( FontsData.font_small_green,chars_small+index,sf,&dest );
					break;
				case ClYellow:
					SDL_BlitSurface ( FontsData.font_small_yellow,chars_small+index,sf,&dest );
					break;
			}
			dest.x+=dest.w;
		}
	}
}

// Gibt den Text mit automatischem Zeilenumbruch aus:
void cFonts::OutTextBlock ( char *str, SDL_Rect block, SDL_Surface *sf )
{
	char word[50], *ptr, *p, *p2;
	int len, x;
	ptr = str;
	x = block.x;
	// Wort f�r Wort durch den String gehen:

	while ( ptr )
	{
		p = strchr ( ptr, ' ' );
		p2 = strchr ( ptr, '\n' );

		if ( ( p2 && p && p2 < p ) || ( p2 && !p ) )
		{
			p = p2;
		}

		if ( p )
		{
			len = p - ptr;

			if ( len > 49 )
				len = 49;

			strncpy ( word, ptr, len );

			word[len] = 0;

			ptr = p + 1;
		}
		else
		{
			if ( strlen ( ptr ) >= 50 )
			{
				strncpy ( word, ptr, 49 );
				word[49] = 0;
			}
			else
			{
				strcpy ( word, ptr );
			}

			ptr = 0;
		}

		len = GetTextLen ( word );

		if ( block.x + len > x + block.w )
		{
			block.x = x;
			block.y += 11;
		}

		OutText ( word, block.x, block.y, sf );

		block.x += len + 5;

		if ( p && *p == '\n' )
		{
			block.x = x;
			block.y += 11;
		}
	}
}

cBitmapFont::cBitmapFont()
{
	iLoadedCharset = -1;
	sfTmp= NULL;
	sfLatinNormal= SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,256,192,8,0,0,0,0 );
	sfLatinBig= SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,256,256,8,0,0,0,0 );
	sfLatinBigGold= SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,256,256,8,0,0,0,0 );
	sfLatinSmallWhite= SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,128,128,8,0,0,0,0 );
	sfLatinSmallRed= SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,128,128,8,0,0,0,0 );
	sfLatinSmallGreen= SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,128,128,8,0,0,0,0 );
	sfLatinSmallYellow= SDL_CreateRGBSurface ( SDL_HWSURFACE|SDL_SRCCOLORKEY,128,128,8,0,0,0,0 );

		
	string sTmp = SettingsData.sFontPath + PATH_DELIMITER;
	
	sfLatinNormal = LoadPCX((char*)(sTmp + "latin_normal.pcx").c_str());
	buildFont(sfLatinNormal);
	copyArray(chars, LatinNormal);
	
	sfLatinSmallGreen = LoadPCX((char*)(sTmp + "latin_small_green.pcx").c_str());
	buildFont(sfLatinSmallGreen);
	copyArray(chars, LatinSmallGreen);
	
	sfLatinSmallRed = LoadPCX((char*)(sTmp + "latin_small_red.pcx").c_str());
	buildFont(sfLatinSmallRed);
	copyArray(chars, LatinSmallRed);
	
	sfLatinSmallYellow= LoadPCX((char*)(sTmp + "latin_small_yellow.pcx").c_str());
	buildFont(sfLatinSmallYellow);
	copyArray(chars, LatinSmallYellow);
	
	sfLatinSmallWhite = LoadPCX((char*)(sTmp + "latin_small_white.pcx").c_str());
	buildFont(sfLatinSmallWhite);
	copyArray(chars, LatinSmallWhite);
	
	sfLatinBig = LoadPCX((char*)(sTmp + "latin_big.pcx").c_str());
	buildFont(sfLatinBig);
	copyArray(chars, LatinBig);

	sfLatinBigGold = LoadPCX((char*)(sTmp + "latin_big_gold.pcx").c_str());
	buildFont(sfLatinBigGold);
	copyArray(chars, LatinBigGold);
	//TODO: add support for cryllian characters
}

cBitmapFont::~cBitmapFont()
{
	/*FIXME: dunno why but app crashes when I want to free my surfaces
	SDL_FreeSurface(sfLatinNormal);
	SDL_FreeSurface(sfLatinBig);
	SDL_FreeSurface(sfLatinBigGold);
	SDL_FreeSurface(sfLatinSmallWhite);
	SDL_FreeSurface(sfLatinSmallRed);
	SDL_FreeSurface(sfLatinSmallGreen);
	SDL_FreeSurface(sfLatinSmallYellow);
	SDL_FreeSurface(sfTmp); */
}

void cBitmapFont::copyArray(SDL_Rect source[256],SDL_Rect dest[256])
{	
	for(int i=0; i < 256; i++)
	{
		dest[i].x = source[i].x;
		dest[i].y = source[i].y;
		dest[i].w = source[i].w;
		dest[i].h = source[i].h;
	}
}

void cBitmapFont::showTextAsBlock ( SDL_Rect rDest, string sText, int eBitmapFontType, SDL_Surface *surface )
{
	if ( DEBUGFONTS )
		cLog::write ( "Seeking through " + sText, cLog::eLOG_TYPE_DEBUG );

	string sTmp = sText;

	string sTextShortened;

	int k;

	int lastK = 0;

	do
	{
		//search and replace \n since we want a blocktext - no manual breaklines allowed
		k = sText.find ( "\n" );

		if ( k != string::npos )
		{
			if(DEBUGFONTS) cLog::write("Fount breakline at " + iToStr(k), cLog::eLOG_TYPE_DEBUG);
			sText.erase ( k, 1 );
			sText.insert ( k, " " );
			sTmp=sText;
		}
	}
	while ( k != string::npos );

	do
	{
		//erase all blanks > 2
		k = sText.find ( "  " ); //IMPORTANT: _two_ blanks! don't change this or this will become an endless loop

		if ( k != string::npos )
		{
			if(DEBUGFONTS) cLog::write("Fount doubleblank at " + iToStr(k), cLog::eLOG_TYPE_DEBUG);
			sText.erase ( k, 1 );
			sTmp = sText;
		}
	}
	while ( k != string::npos );

	SDL_Rect rLenght = getTextSize ( sText, eBitmapFontType );

	if ( rLenght.w > rDest.w ) //text is longer than dest-width - let's snip it
	{
		do
		{
			k = sTmp.find ( " " ); //search spaces/blanks

			if ( k == string::npos ) //reached the end but leftovers might be to long
			{
				rLenght = getTextSize ( sText, eBitmapFontType ); //test new string lenght
				if ( rLenght.w > rDest.w ) //if leftover is to long snip it too
				{
					sTextShortened = sText; //get total leftover again

					if ( lastK > 0 )
					{
						sTextShortened.erase ( lastK, sTextShortened.size() ); //erase everything longer than line
						sText.erase ( 0, lastK + 1 ); //erase txt from original that we just copied to tmp including leading blank
					}
					else
					{
						sTextShortened.erase ( sText.size() / 2, sTextShortened.size() ); //erase everything longer than line
						sText.erase ( 0, sText.size() / 2 + 1 ); //erase txt from original that we just copied to tmp
						cLog::write ( "Textbox defined to small for text! Can not snip text correctly!", cLog::eLOG_TYPE_ERROR );
					}

					showText ( rDest, sTextShortened, eBitmapFontType, surface ); //blit part of text
					rDest.y += getFontHeight ( eBitmapFontType ); //and add a linebreak
				}

				showText ( rDest, sText, eBitmapFontType, surface ); //draw last part of text
			}

			if ( k != string::npos )
			{
				sTmp.erase ( k, 1 ); //replace spaces with # so we don't find them again next search
				sTmp.insert ( k, "#" );
				sTextShortened = sText; //copy clean text to tmp string
				sTextShortened.erase ( k, sTextShortened.size() ); //erase everything longer than line
				
				rLenght = getTextSize ( sTextShortened, eBitmapFontType ); //test new string lenght
				if ( rLenght.w > rDest.w )
				{
					//found important lastK to snip text since text is now to long
					sTextShortened = sText; //copy text to tmp string

					if ( lastK > 0 )
					{
						sTextShortened.erase ( lastK, sTextShortened.size() ); //erase everything longer than line
						sText.erase ( 0, lastK + 1 ); //erase txt from original that we just copied to tmp including leading blank
					}
					else
					{
						sTextShortened.erase ( sText.size() / 2, sTextShortened.size() ); //erase everything longer than line
						sText.erase ( 0, sText.size() / 2 + 1 ); //erase txt from original that we just copied to tmp
						cLog::write ( "Textbox defined to small for text! Can not snip text correctly!", cLog::eLOG_TYPE_ERROR );

					}

					sTmp = sText; //copy snipped original sText to sTmp to start searching again
					showText ( rDest, sTextShortened, eBitmapFontType, surface ); //blit part of text
					rDest.y += getFontHeight ( eBitmapFontType ); //and add a linebreak
				}
				else
				{
					lastK = k; //seek more, couldn't find korrekt lastK
				}
			}
		}
		while ( k != string::npos );
	}
	else
	{
		showText ( rDest, sText, eBitmapFontType, surface ); //nothing to shorten, just blit text
	}
}

int cBitmapFont::getFontHeight(int eBitmapFontType)
{
	getCharset(eBitmapFontType);
	return sfTmp->h / 16;
}

void cBitmapFont::buildFont(SDL_Surface *surface)
{
	if(surface == NULL)
	{
		cLog::write("Got NULL surface for font", cLog::eLOG_TYPE_ERROR);
		return;
	}
	sfTmp = surface;
	
	int iFormat = sfTmp->format->BitsPerPixel;
	
	Uint32 bgColor32;
	Uint16 bgColor16;
	Uint8 bgColor8;
	switch(iFormat)
	{
		case 32:
		case 24:
			bgColor32 = SDL_MapRGB(sfTmp->format, 0xFF, 0, 0xFF);
			break;
		case 16:
			bgColor16 = SDL_MapRGB(sfTmp->format, 0xFF, 0, 0xFF);
			break;	
		case 8:
		default:
			bgColor8 = SDL_MapRGB(sfTmp->format, 0xFF, 0, 0xFF);
			break;
	}
	

	int cellW = sfTmp->w / 16;
	int cellH = sfTmp->h / 16;
	int currentChar = 0;
	register int pX = 0;
	register int pY = 0;
			
	//go through the rows
	for( int rows = 0; rows < 16; rows ++)
	{
		//go through the cols
		for( int cols = 0; cols < 16; cols ++)
		{
			chars[currentChar].x = cellW * cols; //write each cell position and size into array
			chars[currentChar].y = cellH * rows;
			chars[currentChar].h = cellH;
			chars[currentChar].w = cellW;
			
			//go through pixels to find offset x
			for( int pCol = 0; pCol < cellH; pCol++)
			{
				for( int pRow = 0; pRow < cellH; pRow++)
				{
					pX = (cellW * cols) + pCol;
					pY = (cellH * rows) + pRow;
					bool bFound = false;

					//search for pixel != background colour
					switch(iFormat)
					{
						case 32:
							if(getPixel32(pX, pY, sfTmp) != bgColor32)
							{
								bFound = true;
							}
							break;
						case 16:
							if(getPixel16(pX, pY, sfTmp) != bgColor16)
							{
								bFound = true;
							}
							break;
						case 8:
						default:
							if(getPixel8(pX, pY, sfTmp) != bgColor8)
							{
								bFound = true;
							}
							break;	
					}
					if(bFound)
					{
						//offset
						chars[currentChar].x = pX;
						pCol = cellW; //break loop
						pRow = cellH;	
						bFound = false;		
					}
				}
			}
			
			//go through pixel to find offset w
			for( int pCol_w = cellW - 1; pCol_w >= 0; pCol_w--)
			{
				for ( int pRow_w = 0; pRow_w < cellH; pRow_w++)
				{
					pX = (cellW * cols ) + pCol_w;
					pY = (cellH * rows ) + pRow_w;
					bool bFound = false;
					//search for pixel != background colour
					switch(iFormat)
					{
						case 32:
							if(getPixel32(pX, pY, sfTmp) != bgColor32)
							{
								bFound = true;
							}
							break;
						case 16:
							if(getPixel16(pX, pY, sfTmp) != bgColor16)
							{
								bFound = true;
							}
							break;
						case 8:
						default:
							if(getPixel8(pX, pY, sfTmp) != bgColor8)
							{
								bFound = true;
							}
							break;	
					}
					if(bFound)
					{
						chars[currentChar].w = (pX - chars[currentChar].x) +1;
						pCol_w = -1; //break loop
						pRow_w = cellH;	
						bFound = false;				
					}
				
				}
			}
			//goto next character		
			currentChar++;	
		}
	}
}

void cBitmapFont::showText(SDL_Rect rdest, string sText, int eBitmapFontType, SDL_Surface *surface)
{
	showText(rdest.x, rdest.y, sText, eBitmapFontType, surface);
}

void cBitmapFont::getCharset(int eBitmapFontType)
{
	if(iLoadedCharset != eBitmapFontType) //requested new font - load font surface and array of charlocations
	{
		iLoadedCharset = eBitmapFontType;
		if(DEBUGFONTS) cLog::write("Setting fonttype to " + iToStr(eBitmapFontType), cLog::eLOG_TYPE_DEBUG);
		switch(eBitmapFontType)
		{
			case LATIN_BIG_GOLD:
				sfTmp = sfLatinBigGold;
				copyArray(LatinBigGold, chars); //FIXME: to this with pointers to prevent high cpu load
				break;
			case LATIN_BIG:
				sfTmp = sfLatinBig;
				copyArray(LatinBig, chars);
				break;
			case LATIN_SMALL_GREEN:
				sfTmp = sfLatinSmallGreen;
				copyArray(LatinSmallGreen, chars);
				break;
			case LATIN_SMALL_RED:
				sfTmp = sfLatinSmallRed;
				copyArray(LatinSmallRed, chars);
				break;
			case LATIN_SMALL_WHITE:
				sfTmp = sfLatinSmallWhite;
				copyArray(LatinSmallWhite, chars);
				break;
			case LATIN_SMALL_YELLOW:
				sfTmp = sfLatinSmallYellow;
				copyArray(LatinSmallYellow, chars);
				break;
			case LATIN_NORMAL :
			default:
				sfTmp = sfLatinNormal;
				copyArray(LatinNormal, chars);
		}
	}
}

SDL_Rect cBitmapFont::getTextSize(string sText, int eBitmapFontType)
{
	//tmp offsets
	SDL_Rect rTmp = {0, 0, 0, sfTmp->h / 16};
	
	getCharset(eBitmapFontType);
	
	int ascii;
	int iSpace = 0;
	
	//make sure only upper characters are read for the small fonts
	// since we don't support lower chars on the small fonts
	switch(eBitmapFontType)
	{
		case LATIN_SMALL_GREEN:
		case LATIN_SMALL_RED:
		case LATIN_SMALL_WHITE:
		case LATIN_SMALL_YELLOW:
			for(int i=0; i < sText.size(); i++)
			{
				sText[i] = toupper(sText[i]);
			}
			iSpace = 1;
			break;
		default:
			break;
	}
	

	for(int i = 0; sText[i] != '\0'; i++)
	{
		ascii = (unsigned char) sText[i];
		//is space?
		if(sText[i] == ' ')
		{
			rTmp.w += sfTmp->w / 32;
		} //is new line?
		else if(sText[i] == '\n')
		{
			rTmp.h += sfTmp->h / 16; //high always the same per charset
		}
		else
		{
			//get ascii value
			rTmp.w += chars[ascii].w + iSpace;
		}
	}
	
	if(DEBUGFONTS)
	{	
		stringstream strStream;
		strStream << "Text: " << sText << " is " << rTmp.w << " width and " << rTmp.h << " hight\n";
		cLog::write(strStream.str(), cLog::eLOG_TYPE_DEBUG);
	}
	return rTmp;
}

void cBitmapFont::showTextCentered(SDL_Rect rDest, string sText, int eBitmapFontType, SDL_Surface *surface)
{
	showTextCentered(rDest.x, rDest.y, sText, eBitmapFontType, surface);
}

void cBitmapFont::showTextCentered(int x, int y, string sText, int eBitmapFontType, SDL_Surface *surface)
{
	SDL_Rect rTmp = getTextSize(sText, eBitmapFontType);
	showText( x - rTmp.w / 2, y, sText, eBitmapFontType, surface);	
}

void cBitmapFont::showText(int x, int y, string sText, int eBitmapFontType, SDL_Surface *surface)
{
	//tmp offsets
	int offX = x;
	int offY = y;
	int iSpace = 0;

	getCharset(eBitmapFontType);
	
	//make sure only upper characters are read for the small fonts
	// since we don't support lower chars on the small fonts
	switch(eBitmapFontType)
	{
		case LATIN_SMALL_GREEN:
		case LATIN_SMALL_RED:
		case LATIN_SMALL_WHITE:
		case LATIN_SMALL_YELLOW:
			for(int i=0; i < sText.size(); i++)
			{
				sText[i] = toupper(sText[i]);
			}
			iSpace = 1;
			break;
		default:
			break;
	}	
		
	if(sfTmp != NULL)
	{
		for(int i = 0; sText[i] != '\0'; i++)
		{
			//is space?
			if(sText[i] == ' ')
			{
				offX += sfTmp->w / 32;
			} //is new line?
			else if(sText[i] == '\n')
			{
				offY += sfTmp->h / 16;
				offX = x;
			}
			else
			{
				//get ascii value
				int ascii = (unsigned char) sText[i];
				
				//blit signs to surface
				SDL_Rect rTmp = {offX, offY, 16, 16};
				SDL_BlitSurface(sfTmp, &chars[ascii], surface, &rTmp);
				if(DEBUGFONTS)
				{	
				 	stringstream strStream;
 					strStream << "Sign: " << sText[i] << "(" << ascii << ")" << "  x: " << chars[ascii].x << "  y: " << chars[ascii].y << "  w: " << chars[ascii].w << "  h: " << chars[ascii].h;
					cLog::write(strStream.str(), cLog::eLOG_TYPE_DEBUG);
				}
				//move one px forward for space between signs
				offX += chars[ascii].w + iSpace;
			}
		}
	}

}

Uint32 cBitmapFont::getPixel32(int x, int y, SDL_Surface *surface)
{
	//converts the Pixel to 32 bit
	Uint32 *pixels = (Uint32 *) surface->pixels;
	
	//get the requested pixels
	return pixels[(y*surface->w)+x];
}

Uint16 cBitmapFont::getPixel16(int x, int y, SDL_Surface *surface)
{
	//converts the Pixel to 16 bit
	Uint16 *pixels = (Uint16 *) surface->pixels;
	
	//get the requested pixels
	return pixels[(y*surface->w)+x];
}

Uint8 cBitmapFont::getPixel8(int x, int y, SDL_Surface *surface)
{
	//converts the Pixel to 8 bit
	Uint8 *pixels = (Uint8 *) surface->pixels;
	
	//get the requested pixels
	return pixels[(y*surface->w)+x];
}

int cBitmapFont::getTextWide(string sText, int eBitmapFontType)
{
	SDL_Rect rTmp = getTextSize(sText, eBitmapFontType);
	return rTmp.w;
}

