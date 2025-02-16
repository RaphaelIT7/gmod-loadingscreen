#include "netmessages.h"
#include "bitbuf.h"
#include "const.h"
#include "net_chan.h"
#include "mathlib/mathlib.h"
#include "tier0/vprof.h"
#include "net.h"
#include <common.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static char s_text[1024];

const char *CLC_ClientInfo::ToString(void) const
{
	Q_snprintf(s_text, sizeof(s_text), "%s: SendTableCRC %i", GetName(), 
		m_nSendTableCRC );
	return s_text;
}

bool CLC_ClientInfo::WriteToBuffer( bf_write &buffer )
{
	buffer.WriteUBitLong( GetType(), NETMSG_TYPE_BITS );

	buffer.WriteLong( m_nServerCount );
	buffer.WriteLong( m_nSendTableCRC );
	buffer.WriteOneBit( m_bIsHLTV?1:0 );
	buffer.WriteLong( m_nFriendsID );
	buffer.WriteString( m_FriendsName );
	
	for ( int i=0; i<MAX_CUSTOM_FILES; i++ )
	{
		if ( m_nCustomFiles[i] != 0 )
		{
			buffer.WriteOneBit( 1 );
			buffer.WriteUBitLong( m_nCustomFiles[i], 32 );
		}
		else
		{
			buffer.WriteOneBit( 0 );
		}
	}
		
#if defined( REPLAY_ENABLED )
	buffer.WriteOneBit( m_bIsReplay?1:0 );
#endif

	return !buffer.IsOverflowed();
}

bool CLC_ClientInfo::ReadFromBuffer( bf_read &buffer )
{
	VPROF( "CLC_ClientInfo::ReadFromBuffer" );

	m_nServerCount = buffer.ReadLong();
	m_nSendTableCRC = buffer.ReadLong();
	m_bIsHLTV = buffer.ReadOneBit()!=0;
	m_nFriendsID = buffer.ReadLong();
	buffer.ReadString( m_FriendsName, sizeof(m_FriendsName) );
	
	for ( int i=0; i<MAX_CUSTOM_FILES; i++ )
	{
		if ( buffer.ReadOneBit() != 0 )
		{
			m_nCustomFiles[i] = buffer.ReadUBitLong( 32 );
		}
		else
		{
			m_nCustomFiles[i] = 0;
		}
	}

#if defined( REPLAY_ENABLED )
	m_bIsReplay = buffer.ReadOneBit()!=0;
#endif

	return !buffer.IsOverflowed();
}

bool NET_SignonState::WriteToBuffer( bf_write &buffer )
{
	buffer.WriteUBitLong( GetType(), NETMSG_TYPE_BITS );
	buffer.WriteByte( m_nSignonState );
	buffer.WriteLong( m_nSpawnCount );

	return !buffer.IsOverflowed();
}

bool NET_SignonState::ReadFromBuffer( bf_read &buffer )
{
	VPROF( "NET_SignonState::ReadFromBuffer" );

	m_nSignonState = buffer.ReadByte();
	m_nSpawnCount = buffer.ReadLong();

	return !buffer.IsOverflowed();
}

const char *NET_SignonState::ToString(void) const
{
	Q_snprintf(s_text, sizeof(s_text), "%s: state %i, count %i", GetName(), m_nSignonState, m_nSpawnCount );
	return s_text;
}