////////////////////////////////////////////////////////////////////////////////
//
//  File:   ExtendedTinyXml.cpp
//  Date:   07-10-01
//  Author: JCK
//
////////////////////////////////////////////////////////////////////////////////
//  Description:
//  Improves the TinyXML family by adding ExTiXmlNode. This class is a bid more
//  user-friendly.
//
//	Example for usage is added in "ExtendedTinyXml.h"
// 
////////////////////////////////////////////////////////////////////////////////


#include "ExtendedTinyXml.h"

ExTiXmlNode* ExTiXmlNode::XmlGetFirstNode( TiXmlDocument &rTiXmlDoc, const char * pszCurrent, ... )
{
	va_list pvaArg;
	va_start(pvaArg, pszCurrent);

	TiXmlNode * pXmlNode;

	if( rTiXmlDoc.Value() == NULL )
	{
		va_end( pvaArg );
		return NULL;
	}

	pXmlNode = rTiXmlDoc.RootElement();
	if( pXmlNode == NULL )
	{
		va_end( pvaArg );
		return NULL;
	}

	if( strcmp(pXmlNode->Value(), pszCurrent) != 0 )
	{
		va_end( pvaArg );
		return NULL;
	}

	do
	{
		pszCurrent = va_arg(pvaArg, char * );
		if( pszCurrent != "" )
		{
			pXmlNode = pXmlNode->FirstChild( pszCurrent );
			if( pXmlNode == NULL )
			{
				va_end( pvaArg );
				return NULL;
			}
		}
	}while( pszCurrent != "" );


	return (ExTiXmlNode *)pXmlNode;
}

ExTiXmlNode* ExTiXmlNode::XmlGetNextNodeSibling()
{
	TiXmlNode * pXmlNode;
	if( this == NULL )
	{
		return NULL;
	}
	pXmlNode = this;

	pXmlNode = pXmlNode->NextSibling();

	return (ExTiXmlNode *)pXmlNode;
}


ExTiXmlNode * ExTiXmlNode::XmlReadNodeData( std::string &rstrData, XML_NODE_TYPE eType )
{
	bool bDataFoundPossible = true;
	TiXmlNode * pXmlNode = (TiXmlNode *)this;
	rstrData = "";

	if( this == NULL )
	{
		return NULL;
	}
	switch( eType )
	{
	case ExTiXmlNode::eXML_ATTRIBUTE :
			return NULL;
			break;
		case ExTiXmlNode::eXML_COMMENT :
			if ( pXmlNode->Type() == TiXmlNode::ELEMENT )
			{
				pXmlNode = pXmlNode->FirstChild();
			}
			if( pXmlNode == NULL )
			{
				return NULL;
			}
			do
			{
				if( pXmlNode->Type() == TiXmlNode::COMMENT )
				{
					rstrData =  pXmlNode->ToComment()->Value();
					bDataFoundPossible = false; // Get only the first data !
				}
				pXmlNode = pXmlNode->NextSibling();
				if( pXmlNode == NULL ) 
				{
					bDataFoundPossible = false; // Last sibling already checked
				}
			}while( bDataFoundPossible );
			break;
		case ExTiXmlNode::eXML_TEXT :
			if ( pXmlNode->Type() == TiXmlNode::ELEMENT )
			{
				pXmlNode = pXmlNode->FirstChild();
			}
			if( pXmlNode == NULL )
			{
				return NULL;
			}
			do
			{
				if( pXmlNode->Type() == TiXmlNode::TEXT )
				{
					rstrData =  pXmlNode->ToText()->Value();
					bDataFoundPossible = false; // Get only the first data !
				}
				pXmlNode = pXmlNode->NextSibling();
				if( pXmlNode == NULL ) 
				{
					bDataFoundPossible = false; // Last sibling already checked
				}
			}while( bDataFoundPossible );
			break;
		default :
		rstrData = "";
		return NULL;
	}
	return (ExTiXmlNode *) pXmlNode;
}


ExTiXmlNode * ExTiXmlNode::XmlReadNodeData( std::string &rstrData, XML_NODE_TYPE eType,  const char * pszAttributeName )
{
	TiXmlNode * pXmlNode = (TiXmlNode *)this;
	TiXmlElement * pXmlElement;
	const char * pszTemp;

	rstrData = "";

	if( this == NULL )
	{
		return NULL;
	}
	switch( eType )
	{
		case ExTiXmlNode::eXML_ATTRIBUTE :
			if( pXmlNode->Type() != TiXmlNode::ELEMENT ) return NULL;
			pXmlElement = pXmlNode->ToElement();// FirstChildElement();
			if( pXmlElement == NULL )
			{
				return NULL;
			}
			pszTemp =  pXmlElement->Attribute( pszAttributeName );
			if( pszTemp == 0 )
			{
				return NULL;
			}else
			{
				rstrData = pszTemp;
			}
			break;
		case ExTiXmlNode::eXML_COMMENT :
			return NULL;
			break;
		case ExTiXmlNode::eXML_TEXT :
			return NULL;
			 break;
		default :
			return NULL;
	}
	return (ExTiXmlNode *)pXmlNode;
}

bool ExTiXmlNode::IsTimeStamp( std::string &rstrData )
{
	return true;
}