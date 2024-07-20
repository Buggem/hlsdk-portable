/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "player.h"
#include "game.h"

extern cvar_t bm_mp5_mod;

//===================grenade


LINK_ENTITY_TO_CLASS( grenade, CGrenade )

// Grenades flagged with this will be triggered when the owner calls detonateSatchelCharges
#define SF_DETONATE		0x0001

//
// Grenade Explode
//
void CGrenade::Explode( Vector vecSrc, Vector vecAim )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -32 ), ignore_monsters, ENT( pev ), & tr );

	Explode( &tr, DMG_BLAST );
}

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CGrenade::Explode( TraceResult *pTrace, int bitsDamageType )
{
	// float flRndSound;// sound randomizer

	// BMOD Begin - Grenade Rune
	CBasePlayer *pPlayer = UTIL_CastPlayer( pev->owner );

	if( !strcmp( STRING( pev->classname ), "hand_grenade" ) && pPlayer && ( pPlayer->m_RuneFlags == RUNE_GRENADE ) )
	{
		pev->nextthink = gpGlobals->time + 0.1f;
		pev->dmg = 150;
		m_iMegaSmokeFrame = 0;
		SetThink( &CGrenade::MegaSmoke );
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.3f;
		SetThink( &CGrenade::Smoke );
	}
	// BMOD End - Grenade Rune

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if( pTrace->flFraction != 1.0f )
	{
		if (explosionfix.value)
			pev->origin = pTrace->vecEndPos + ( pTrace->vecPlaneNormal * 0.6f );
		else
			pev->origin = pTrace->vecEndPos + ( pTrace->vecPlaneNormal * ( pev->dmg - 24 ) * 0.6f );
	}

	int iContents = UTIL_PointContents( pev->origin );

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		if( iContents != CONTENTS_WATER )
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( ( pev->dmg - 50 ) * 0.6f ); // scale * 10
		WRITE_BYTE( 15 ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );
	entvars_t *pevOwner;
	if( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamage( pev, pevOwner, pev->dmg, CLASS_NONE, bitsDamageType );

	if( RANDOM_FLOAT( 0, 1 ) < 0.5f )
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	}

	//flRndSound = RANDOM_FLOAT( 0, 1 );

	switch( RANDOM_LONG( 0, 2 ) )
	{
		case 0:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM );
			break;
		case 2:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM );
			break;
	}

	pev->effects |= EF_NODRAW;
	pev->velocity = g_vecZero;

	if( iContents != CONTENTS_WATER )
	{
		int sparkCount = RANDOM_LONG( 0, 3 );
		for( int i = 0; i < sparkCount; i++ )
			Create( "spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL );
	}
}

void CGrenade::MegaSmoke( void )
{
	m_iMegaSmokeFrame++;

	if( m_iMegaSmokeFrame == 1 )
	{
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMTORUS );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z - 50 );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z - 50 + ( pev->dmg * 2.5f ) );
			WRITE_SHORT( g_sModelIndexLaser );
			WRITE_BYTE( 0 );	// start frame?	
			WRITE_BYTE( 10 );	// rame rate?
			WRITE_BYTE( 5 );	// life
			WRITE_BYTE( 40 );	// width
			WRITE_BYTE( 0 );	// noise
			WRITE_BYTE( 250 ); //Color
			WRITE_BYTE( 120 );
			WRITE_BYTE( 20 );
			WRITE_BYTE( 255 );	// brightness
			WRITE_BYTE( 0 );	// scroll speed
		MESSAGE_END();
	}
// coord coord coord (center position) 
// coord coord coord (axis and radius) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)
	if( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( ( pev->dmg - 50 ) * 0.8f ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();
	}

	pev->dmg *= 0.8f;
	pev->origin.z += pev->dmg / 2;

	if( m_iMegaSmokeFrame < 4 )
	{

		int iContents = UTIL_PointContents( pev->origin );
		
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
			WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			if( iContents != CONTENTS_WATER )
			{
				WRITE_SHORT( g_sModelIndexFireball );
			}
			else
			{
				WRITE_SHORT( g_sModelIndexWExplosion );
			}
			WRITE_BYTE( ( pev->dmg - 50 ) * 0.6f ); // scale * 10
			WRITE_BYTE( 15 ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();

		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

		switch( RANDOM_LONG( 0, 2 ) )
		{
			case 0:
				EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM );
				break;
			case 1:
				EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM );
				break;
			case 2:
				EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM );
				break;
		}

		if( iContents != CONTENTS_WATER )
		{
			int sparkCount = RANDOM_LONG( 1, 4 );
			for( int i = 0; i < sparkCount; i++ )
				Create( "spark_shower", pev->origin, Vector( 0, 0, 1 ), NULL );
		}
	}
	pev->nextthink = gpGlobals->time + 0.1f;

	if( m_iMegaSmokeFrame == 4 )
		UTIL_Remove( this );
}

void CGrenade::Smoke( void )
{
	if( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( (int)( ( pev->dmg - 50 ) * 0.8f ) ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();
	}
	UTIL_Remove( this );
}

void CGrenade::Killed( entvars_t *pevAttacker, int iGib )
{
	Detonate();
}

// Timed grenade, this think is called when time runs out.
void CGrenade::DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time;
}

void CGrenade::PreDetonate( void )
{
	CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, 400, 0.3 );

	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time + 1;
}

void CGrenade::Detonate( void )
{
	TraceResult tr;
	Vector vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector( 0, 0, 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, ENT(pev), &tr );

	Explode( &tr, DMG_BLAST );
}


//
// Contact grenade, explode when it touches something
// 
void CGrenade::ExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT( pev ), &tr );

	Explode( &tr, DMG_BLAST );
}

void CGrenade::DangerSoundThink( void )
{
	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5f, (int)pev->velocity.Length(), 0.2 );
	pev->nextthink = gpGlobals->time + 0.2f;

	if( pev->waterlevel != 0 )
	{
		pev->velocity = pev->velocity * 0.5f;
	}
}

void CGrenade::BounceTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if( pOther->edict() == pev->owner )
		return;

	// only do damage if we're moving fairly fast
	if( m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100 )
	{
		entvars_t *pevOwner = VARS( pev->owner );
		if( pevOwner )
		{
			TraceResult tr = UTIL_GetGlobalTrace();
			ClearMultiDamage();
			pOther->TraceAttack( pevOwner, 1, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			ApplyMultiDamage( pev, pevOwner );
		}
		m_flNextAttack = gpGlobals->time + 1.0f; // debounce
	}

	Vector vecTestVelocity;
	// pev->avelocity = Vector( 300, 300, 300 );

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity; 
	vecTestVelocity.z *= 0.45f;

	if( !m_fRegisteredSound && vecTestVelocity.Length() <= 60 )
	{
		//ALERT( at_console, "Grenade Registered!: %f\n", vecTestVelocity.Length() );

		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving. 
		// go ahead and emit the danger sound.

		// register a radius louder than the explosion, so we make sure everyone gets out of the way
		CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, (int)( pev->dmg / 0.4f ), 0.3f );
		m_fRegisteredSound = TRUE;
	}

	if( pev->flags & FL_ONGROUND )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8f;

		pev->sequence = RANDOM_LONG( 1, 1 );
		ResetSequenceInfo();
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0f;
	if( pev->framerate > 1.0f )
		pev->framerate = 1.0f;
	else if( pev->framerate < 0.5f )
	{
		pev->framerate = 0.0f;
		pev->frame = 0.0f;
	}
}

void CGrenade::SlideTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if( pOther->edict() == pev->owner )
		return;

	// pev->avelocity = Vector( 300, 300, 300 );
	if( pev->flags & FL_ONGROUND )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95f;

		if( pev->velocity.x != 0 || pev->velocity.y != 0 )
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CGrenade::BounceSound( void )
{
	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit1.wav", 0.25, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit2.wav", 0.25, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit3.wav", 0.25, ATTN_NORM );
		break;
	}
}

void CGrenade::TumbleThink( void )
{
	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	if( pev->dmgtime - 1 < gpGlobals->time )
	{
		CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin + pev->velocity * ( pev->dmgtime - gpGlobals->time ), 400, 0.1 );
	}

	if( pev->dmgtime <= gpGlobals->time )
	{
		SetThink( &CGrenade::Detonate );
	}
	if( pev->waterlevel != 0 )
	{
		pev->velocity = pev->velocity * 0.5f;
		pev->framerate = 0.2f;
	}
}

void CGrenade::Spawn( void )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "grenade" );

	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/grenade.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pev->dmg = 100;
	m_fRegisteredSound = FALSE;
}

CGrenade *CGrenade::ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->Spawn();
	// contact grenades arc lower
	pGrenade->pev->gravity = 0.5;// lower gravity since grenade is aerodynamic and engine doesn't know it.
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles( pGrenade->pev->velocity );
	pGrenade->pev->owner = ENT( pevOwner );

	// make monsters afaid of it while in the air
	pGrenade->SetThink( &CGrenade::DangerSoundThink );
	pGrenade->pev->nextthink = gpGlobals->time;

	// Tumble in air
	pGrenade->pev->avelocity.x = RANDOM_FLOAT( -100, -500 );

	// Explode on contact
	pGrenade->SetTouch( &CGrenade::ExplodeTouch );

	// BMOD Begin - mp5 mod
	// pGrenade->pev->dmg = gSkillData.plrDmgM203Grenade;
	if( bm_mp5_mod.value )
		pGrenade->pev->dmg = 80;
	else
		pGrenade->pev->dmg = gSkillData.plrDmgM203Grenade;
	// BMOD End - mp5 mod

	return pGrenade;
}

CGrenade *CGrenade::ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->Spawn();
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles( pGrenade->pev->velocity );
	pGrenade->pev->owner = ENT( pevOwner );

	pGrenade->SetTouch( &CGrenade::BounceTouch );	// Bounce if touched

	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that 
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed(). 

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink( &CGrenade::TumbleThink );
	pGrenade->pev->nextthink = gpGlobals->time + 0.1f;
	if( time < 0.1f )
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector( 0, 0, 0 );
	}

	SET_MODEL( ENT( pGrenade->pev ), "models/w_grenade.mdl" );
	pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->ResetSequenceInfo();
	pGrenade->pev->framerate = 1.0f;

	// Tumble through the air
	// pGrenade->pev->avelocity.x = -400;

	pGrenade->pev->gravity = 0.5f;
	pGrenade->pev->friction = 0.8f;

	pGrenade->pev->dmg = 100;

	return pGrenade;
}

CGrenade *CGrenade::ShootSatchelCharge( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->pev->movetype = MOVETYPE_BOUNCE;
	pGrenade->pev->classname = MAKE_STRING( "grenade" );

	pGrenade->pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pGrenade->pev ), "models/grenade.mdl" );	// Change this to satchel charge model

	UTIL_SetSize( pGrenade->pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pGrenade->pev->dmg = 200;
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = g_vecZero;
	pGrenade->pev->owner = ENT( pevOwner );

	// Detonate in "time" seconds
	pGrenade->SetThink( &CBaseEntity::SUB_DoNothing );
	pGrenade->SetUse( &CGrenade::DetonateUse );
	pGrenade->SetTouch( &CGrenade::SlideTouch );
	pGrenade->pev->spawnflags = SF_DETONATE;

	pGrenade->pev->friction = 0.9f;

	return pGrenade;
}

void CGrenade::UseSatchelCharges( entvars_t *pevOwner, SATCHELCODE code )
{
	edict_t *pentFind;
	edict_t *pentOwner;

	if( !pevOwner )
		return;

	CBaseEntity *pOwner = CBaseEntity::Instance( pevOwner );

	pentOwner = pOwner->edict();

	pentFind = FIND_ENTITY_BY_CLASSNAME( NULL, "grenade" );
	while( !FNullEnt( pentFind ) )
	{
		CBaseEntity *pEnt = Instance( pentFind );
		if( pEnt )
		{
			if( FBitSet( pEnt->pev->spawnflags, SF_DETONATE ) && pEnt->pev->owner == pentOwner )
			{
				if( code == SATCHEL_DETONATE )
					pEnt->Use( pOwner, pOwner, USE_ON, 0 );
				else // SATCHEL_RELEASE
					pEnt->pev->owner = NULL;
			}
		}
		pentFind = FIND_ENTITY_BY_CLASSNAME( pentFind, "grenade" );
	}
}

//======================end grenade
