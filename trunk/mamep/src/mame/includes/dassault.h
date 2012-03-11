/*************************************************************************

    Desert Assault

*************************************************************************/

#include "sound/okim6295.h"
#include "video/deco16ic.h"
#include "video/decocomn.h"
#include "video/bufsprite.h"

class dassault_state : public driver_device
{
public:
	dassault_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu"),
		  m_audiocpu(*this, "audiocpu"),
		  m_subcpu(*this, "sub"),
		  m_decocomn(*this, "deco_common"),
		  m_deco_tilegen1(*this, "tilegen1"),
		  m_deco_tilegen2(*this, "tilegen2"),
		  m_oki2(*this, "oki2"),
		  m_spriteram(*this, "spriteram"),
		  m_spriteram2(*this, "spriteram2") { }

	/* memory pointers */
	UINT16 *  m_pf2_rowscroll;
	UINT16 *  m_pf4_rowscroll;
	UINT16 *  m_ram;
	UINT16 *  m_ram2;
	UINT16 *  m_shared_ram;

	/* devices */
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_audiocpu;
	required_device<cpu_device> m_subcpu;
	required_device<decocomn_device> m_decocomn;
	required_device<deco16ic_device> m_deco_tilegen1;
	required_device<deco16ic_device> m_deco_tilegen2;
	required_device<okim6295_device> m_oki2;
	required_device<buffered_spriteram16_device> m_spriteram;
	required_device<buffered_spriteram16_device> m_spriteram2;
};



/*----------- defined in video/dassault.c -----------*/

VIDEO_START(dassault);
SCREEN_UPDATE_RGB32( dassault );
