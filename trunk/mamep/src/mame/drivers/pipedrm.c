/***************************************************************************

    Pipe Dream

    driver by Bryan McPhail & Aaron Giles

****************************************************************************

    Memory map

****************************************************************************

    ========================================================================
    MAIN CPU
    ========================================================================
    0000-7FFF   R     xxxxxxxx   Program ROM
    8000-9FFF   R/W   xxxxxxxx   Program RAM
    A000-BFFF   R     xxxxxxxx   Banked ROM
    C000-CBFF   R/W   xxxxxxxx   Palette RAM (1536 entries x 2 bytes)
                R/W   ---xxxxx      (0: Blue)
                R/W   xxx-----      (0: Green, 3 LSB)
                R/W   ------xx      (1: Green, 2 MSB)
                R/W   -xxxxx--      (1: Red)
    CC00-CFFF   R/W   xxxxxxxx   Sprite RAM (256 entries x 8 bytes)
                R/W   xxxxxxxx      (0: Y position, 8 LSB)
                R/W   -------x      (1: Y position, 1 MSB)
                R/W   xxxx----      (1: Y zoom factor)
                R/W   xxxxxxxx      (2: X position, 8 LSB)
                R/W   -------x      (3: X position, 1 MSB)
                R/W   xxxx----      (3: X zoom factor)
                R/W   ---x----      (4: Priority)
                R/W   ----xxxx      (4: Palette entry)
                R/W   x-------      (5: Y flip)
                R/W   -xxx----      (5: Number of Y tiles - 1)
                R/W   ----x---      (5: X flip)
                R/W   -----xxx      (5: Number of X tiles - 1)
                R/W   xxxxxxxx      (6: Starting tile index, 8 LSB)
                R/W   ----xxxx      (7: Starting tile index, 4 MSB)
    D000-DFFF   R/W   --xxxxxx   Background tile color
    E000-EFFF   R/W   xxxxxxxx   Background tile index, 8 MSB
    F000-FFFF   R/W   xxxxxxxx   Background tile index, 8 LSB
    ========================================================================
    0020        R     xxxxxxxx   Player 1 controls
                R     --x-----      (Fast button)
                R     ---x----      (Place button)
                R     ----xxxx      (Joystick RLDU)
    0020          W   xxxxxxxx   Sound command
    0021        R     xxxxxxxx   Player 2 controls
                R     --x-----      (Fast button)
                R     ---x----      (Place button)
                R     ----xxxx      (Joystick RLDU)
    0021          W   -xxxxxxx   Bankswitch/video control
                  W   -x------      (Flip screen)
                  W   --x-----      (Background 2 X scroll, 1 MSB)
                  W   ---x----      (Background 1 X scroll, 1 MSB)
                  W   ----x---      (Background videoram select)
                  W   -----xxx      (Bank ROM select)
    0022        R     xxxxxxxx   Coinage DIP switch
                R     xxxx----      (Coin B)
                R     ----xxxx      (Coin A)
    0022          W   xxxxxxxx   Background 1 X scroll, 8 LSB
    0023        R     xxxxxxxx   Game options DIP switch
                R     x-------      (Test switch)
                R     -x------      (Training mode enable)
                R     --x-----      (Flip screen)
                R     ---x----      (Demo sounds)
                R     ----xx--      (Lives)
                R     ------xx      (Difficulty)
    0023          W   xxxxxxxx   Background 1 Y scroll
    0024        R     -x--xxxx   Coinage/start
                R     -x------      (Service coin)
                R     ----x---      (2 player start)
                R     -----x--      (1 player start)
                R     ------x-      (Coin B)
                R     -------x      (Coin A)
    0024          W   xxxxxxxx   Background 2 X scroll, 8 LSB
    0025        R     -------x   Sound command pending
    0025          W   xxxxxxxx   Background 2 Y scroll
    ========================================================================
    Interrupts:
       INT generated by CRTC VBLANK
    ========================================================================


    ========================================================================
    SOUND CPU
    ========================================================================
    0000-77FF   R     xxxxxxxx   Program ROM
    7800-7FFF   R/W   xxxxxxxx   Program RAM
    8000-FFFF   R     xxxxxxxx   Banked ROM
    ========================================================================
    0004          W   -------x   Bank ROM select
    0016        R     xxxxxxxx   Sound command read
    0017          W   --------   Sound command acknowledge
    0018-0019   R/W   xxxxxxxx   YM2610 port A
    001A-001B   R/W   xxxxxxxx   YM2610 port B
    ========================================================================
    Interrupts:
       INT generated by YM2610
       NMI generated by command from main CPU
    ========================================================================

***************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "fromance.h"
#include "sound/2608intf.h"
#include "sound/2610intf.h"
#include <math.h>


static UINT8 pending_command;
static UINT8 sound_command;



/*************************************
 *
 *  Initialization & bankswitching
 *
 *************************************/

static MACHINE_RESET( pipedrm )
{
	/* initialize main Z80 bank */
	memory_configure_bank(1, 0, 8, memory_region(REGION_CPU1) + 0x10000, 0x2000);
	memory_set_bank(1, 0);

	/* initialize sound bank */
	memory_configure_bank(2, 0, 2, memory_region(REGION_CPU2) + 0x10000, 0x8000);
	memory_set_bank(2, 0);
	/* state save */
	state_save_register_global(pending_command);
	state_save_register_global(sound_command);

}


static WRITE8_HANDLER( pipedrm_bankswitch_w )
{
	/*
        Bit layout:

        D7 = unknown
        D6 = flip screen
        D5 = background 2 X scroll MSB
        D4 = background 1 X scroll MSB
        D3 = background videoram select
        D2-D0 = program ROM bank select
    */

	/* set the memory bank on the Z80 using the low 3 bits */
	memory_set_bank(1, data & 0x7);

	/* map to the fromance gfx register */
	fromance_gfxreg_w(offset, ((data >> 6) & 0x01) | 	/* flipscreen */
							  ((~data >> 2) & 0x02));	/* videoram select */
}


static WRITE8_HANDLER( sound_bankswitch_w )
{
	memory_set_bank(2, data & 0x01);
}



/*************************************
 *
 *  Sound CPU I/O
 *
 *************************************/

static TIMER_CALLBACK( delayed_command_w	)
{
	sound_command = param & 0xff;
	pending_command = 1;

	/* Hatris polls commands *and* listens to the NMI; this causes it to miss */
	/* sound commands. It's possible the NMI isn't really hooked up on the YM2608 */
	/* sound board. */
	if (param & 0x100)
		cpunum_set_input_line(1, INPUT_LINE_NMI, ASSERT_LINE);
}


static WRITE8_HANDLER( sound_command_w )
{
	timer_call_after_resynch(data | 0x100, delayed_command_w);
}


static WRITE8_HANDLER( sound_command_nonmi_w )
{
	timer_call_after_resynch(data, delayed_command_w);
}


static WRITE8_HANDLER( pending_command_clear_w )
{
	pending_command = 0;
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
}


static READ8_HANDLER( pending_command_r )
{
	return pending_command;
}


static READ8_HANDLER( sound_command_r )
{
	return sound_command;
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xcfff) AM_READWRITE(MRA8_RAM, paletteram_xRRRRRGGGGGBBBBB_le_w) AM_BASE(&paletteram)
	AM_RANGE(0xd000, 0xffff) AM_READWRITE(fromance_videoram_r, fromance_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
ADDRESS_MAP_END


static ADDRESS_MAP_START( main_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x10) AM_WRITE(fromance_crtc_data_w)
	AM_RANGE(0x11, 0x11) AM_WRITE(fromance_crtc_register_w)
	AM_RANGE(0x20, 0x20) AM_READWRITE(input_port_0_r, sound_command_w)
	AM_RANGE(0x21, 0x21) AM_READWRITE(input_port_1_r, pipedrm_bankswitch_w)
	AM_RANGE(0x22, 0x25) AM_WRITE(fromance_scroll_w)
	AM_RANGE(0x22, 0x22) AM_READ(input_port_2_r)
	AM_RANGE(0x23, 0x23) AM_READ(input_port_3_r)
	AM_RANGE(0x24, 0x24) AM_READ(input_port_4_r)
	AM_RANGE(0x25, 0x25) AM_READ(pending_command_r)
ADDRESS_MAP_END



/*************************************
 *
 *  Sound CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x77ff) AM_ROM
	AM_RANGE(0x7800, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0xffff) AM_ROMBANK(2)
ADDRESS_MAP_END


static ADDRESS_MAP_START( sound_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x04, 0x04) AM_WRITE(sound_bankswitch_w)
	AM_RANGE(0x16, 0x16) AM_READ(sound_command_r)
	AM_RANGE(0x17, 0x17) AM_WRITE(pending_command_clear_w)
	AM_RANGE(0x18, 0x18) AM_READWRITE(YM2610_status_port_0_A_r, YM2610_control_port_0_A_w)
	AM_RANGE(0x19, 0x19) AM_WRITE(YM2610_data_port_0_A_w)
	AM_RANGE(0x1a, 0x1a) AM_READWRITE(YM2610_status_port_0_B_r, YM2610_control_port_0_B_w)
	AM_RANGE(0x1b, 0x1b) AM_WRITE(YM2610_data_port_0_B_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( hatris_sound_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x02, 0x02) AM_WRITE(YM2608_control_port_0_B_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(YM2608_data_port_0_B_w)
	AM_RANGE(0x04, 0x04) AM_READ(sound_command_r)
	AM_RANGE(0x05, 0x05) AM_READWRITE(pending_command_r, pending_command_clear_w)
	AM_RANGE(0x08, 0x08) AM_READWRITE(YM2608_status_port_0_A_r, YM2608_control_port_0_A_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(YM2608_data_port_0_A_w)
	AM_RANGE(0x0a, 0x0a) AM_READWRITE(YM2608_status_port_0_B_r, YM2608_control_port_0_B_w)
	AM_RANGE(0x0b, 0x0b) AM_WRITE(YM2608_data_port_0_B_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

static INPUT_PORTS_START( pipedrm )
	PORT_START	/* $20 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* $21 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* $22 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ))
	PORT_DIPSETTING(    0x06, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x04, "6 Coins/4 Credits" )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_3C ))
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x02, "5 Coins/6 Credits" )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_5C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ))
//  PORT_DIPSETTING(    0x05, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ))
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ))
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ))
	PORT_DIPSETTING(    0x60, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x40, "6 Coins/4 Credits" )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_3C ))
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x20, "5 Coins/6 Credits" )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_5C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ))
//  PORT_DIPSETTING(    0x50, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ))
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ))

	PORT_START	/* $23 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ))
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, "Super" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x10, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ))
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, "Training Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x40, DEF_STR( On ))
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* $24 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static INPUT_PORTS_START( hatris )
	PORT_START	/* $20 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* $21 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* $22 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ))
	PORT_DIPSETTING(    0x09, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x0b, "6 Coins/4 Credits" )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_3C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x0d, "5 Coins/6 Credits" )
	PORT_DIPSETTING(    0x0e, DEF_STR( 4C_5C ))
	PORT_DIPSETTING(    0x0f, DEF_STR( 2C_3C ))
//  PORT_DIPSETTING(    0x0a, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ))
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ))
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ))
	PORT_DIPSETTING(    0x90, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0xb0, "6 Coins/4 Credits" )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_3C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0xd0, "5 Coins/6 Credits" )
	PORT_DIPSETTING(    0xe0, DEF_STR( 4C_5C ))
	PORT_DIPSETTING(    0xf0, DEF_STR( 2C_3C ))
//  PORT_DIPSETTING(    0xa0, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ))
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ))

	PORT_START	/* $23 */
	PORT_DIPNAME( 0x03, 0x00, "Difficulty 1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x03, "Super" )
	PORT_DIPNAME( 0x0c, 0x00, "Difficulty 2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x0c, "Super" )
	PORT_SERVICE( 0x10, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x20, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* $24 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

static const gfx_layout bglayout =
{
	8,4,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24 },
	{ 0*32, 1*32, 2*32, 3*32 },
	8*16
};


static const gfx_layout splayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 12, 8, 28, 24, 4, 0, 20, 16, 44, 40, 60, 56, 36, 32, 52, 48 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	8*128
};


static GFXDECODE_START( pipedrm )
	GFXDECODE_ENTRY( REGION_GFX1, 0, bglayout,    0, 64 )
	GFXDECODE_ENTRY( REGION_GFX2, 0, bglayout,    0, 64 )
	GFXDECODE_ENTRY( REGION_GFX3, 0, splayout, 1024, 32 )
GFXDECODE_END


static GFXDECODE_START( hatris )
	GFXDECODE_ENTRY( REGION_GFX1, 0, bglayout,    0, 128 )
	GFXDECODE_ENTRY( REGION_GFX2, 0, bglayout,    0, 128 )
GFXDECODE_END



/*************************************
 *
 *  Sound definitions
 *
 *************************************/

static void irqhandler(int irq)
{
	cpunum_set_input_line(1, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}


static struct YM2608interface ym2608_interface =
{
	0,0,0,0,irqhandler,
	REGION_SOUND1
};


static struct YM2610interface ym2610_interface =
{
	irqhandler,
	REGION_SOUND1,
	REGION_SOUND2
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( pipedrm )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/2)
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_IO_MAP(main_portmap,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,14318000/4)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_map,0)
	MDRV_CPU_IO_MAP(sound_portmap,0)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(pipedrm)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(44*8, 30*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 44*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(pipedrm)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(pipedrm)
	MDRV_VIDEO_UPDATE(pipedrm)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2610, 8000000)
	MDRV_SOUND_CONFIG(ym2610_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 1.0)
	MDRV_SOUND_ROUTE(2, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hatris )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/2)
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_IO_MAP(main_portmap,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,14318000/4)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_map,0)
	MDRV_CPU_IO_MAP(hatris_sound_portmap,0)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(pipedrm)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(44*8, 30*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 44*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(hatris)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(hatris)
	MDRV_VIDEO_UPDATE(fromance)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2608, 8000000)
	MDRV_SOUND_CONFIG(ym2608_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 1.0)
	MDRV_SOUND_ROUTE(2, "mono", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( pipedrm )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "01.u12",	0x00000, 0x08000, CRC(9fe261fb) SHA1(57beeeade8809be0a71086f55b14b1676c0b3759) )
	ROM_LOAD( "02.u11",	0x10000, 0x10000, CRC(c8209b67) SHA1(cca7356d75e8091b07e3328aef523ff452abbcd8) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "4",	0x00000, 0x08000, CRC(497fad4c) SHA1(f151543a0c4a1d6d5d2de5e1dc12fd59dabcf1a8) )
	ROM_LOAD( "3",	0x10000, 0x10000, CRC(4800322a) SHA1(a616c497ac18351b68b8307050a2a62c717a7873) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "s73",    0x000000, 0x80000, CRC(63f4e10c) SHA1(ba935490578887080d8b16508fa6191236a8fea6) )
	ROM_LOAD( "s72",    0x080000, 0x80000, CRC(4e669e97) SHA1(1de8a8cd8f8f69fa86b8fe2c73c6997e7a89c706) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "s71",    0x000000, 0x80000, CRC(431485ee) SHA1(70a2ba5338598db9fcd9ef2be46e5cc2fd9510ee) )
	ROM_COPY( REGION_GFX1, 0x080000, 0x080000, 0x80000 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "a30", 0x00000, 0x40000, CRC(50bc5e98) SHA1(b351af780d04e67a560935a9eeaedf597ac5bb1f) )
	ROM_LOAD16_BYTE( "a29", 0x00001, 0x40000, CRC(a240a448) SHA1(d64169258e91eb09e8685bcdd96b16bf56e82ef1) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "g72",     0x00000, 0x80000, CRC(dc3d14be) SHA1(4220f3fd13487dd861ac84b1b0d3e92125b3cc19) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "g71",     0x00000, 0x80000, CRC(488e2fd1) SHA1(8ef8ceb2bd36a245138802f51babf62f17c30942) )

	ROM_REGION( 0x0800, REGION_PLDS, ROMREGION_DISPOSE )
	ROM_LOAD( "palce16v8h.114", 0x0000, 0x0117, CRC(1f3a3816) SHA1(2b4d84ab98036b8861961f610b1b1ec23a653ef7) )
	ROM_LOAD( "gal16v8a.115",   0x0200, 0x0117, CRC(2b32e239) SHA1(a3b9e45a1ce15ea4cc5754b2bf89cbaa416e814a) )
	ROM_LOAD( "gal16v8a.116",   0x0400, 0x0117, CRC(3674f043) SHA1(06c88f65877a6575149bdd4f7cea64cd310227bd) )
	ROM_LOAD( "gal16v8a.127",   0x0600, 0x0117, CRC(7115d95c) SHA1(23044039373b5a2face63d72c3fc6bf7f0c8a475) )
ROM_END


ROM_START( pipedrmj )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "1",	0x00000, 0x08000, CRC(dbfac46b) SHA1(98ddfaed61de28b238964445572eb398b9dd03c7) )
	ROM_LOAD( "2",	0x10000, 0x10000, CRC(b7adb99a) SHA1(fdab2b99e86aa0b6b17ec95556222e5211ba55e9) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "4",	0x00000, 0x08000, CRC(497fad4c) SHA1(f151543a0c4a1d6d5d2de5e1dc12fd59dabcf1a8) )
	ROM_LOAD( "3",	0x10000, 0x10000, CRC(4800322a) SHA1(a616c497ac18351b68b8307050a2a62c717a7873) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "s73",    0x000000, 0x80000, CRC(63f4e10c) SHA1(ba935490578887080d8b16508fa6191236a8fea6) )
	ROM_LOAD( "s72",    0x080000, 0x80000, CRC(4e669e97) SHA1(1de8a8cd8f8f69fa86b8fe2c73c6997e7a89c706) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "s71",    0x000000, 0x80000, CRC(431485ee) SHA1(70a2ba5338598db9fcd9ef2be46e5cc2fd9510ee) )
	ROM_COPY( REGION_GFX1, 0x080000, 0x080000, 0x80000 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "a30", 0x00000, 0x40000, CRC(50bc5e98) SHA1(b351af780d04e67a560935a9eeaedf597ac5bb1f) )
	ROM_LOAD16_BYTE( "a29", 0x00001, 0x40000, CRC(a240a448) SHA1(d64169258e91eb09e8685bcdd96b16bf56e82ef1) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "g72",     0x00000, 0x80000, CRC(dc3d14be) SHA1(4220f3fd13487dd861ac84b1b0d3e92125b3cc19) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "g71",     0x00000, 0x80000, CRC(488e2fd1) SHA1(8ef8ceb2bd36a245138802f51babf62f17c30942) )

	ROM_REGION( 0x0800, REGION_PLDS, ROMREGION_DISPOSE )
	ROM_LOAD( "palce16v8h.114", 0x0000, 0x0117, CRC(1f3a3816) SHA1(2b4d84ab98036b8861961f610b1b1ec23a653ef7) )
	ROM_LOAD( "gal16v8a.115",   0x0200, 0x0117, CRC(2b32e239) SHA1(a3b9e45a1ce15ea4cc5754b2bf89cbaa416e814a) )
	ROM_LOAD( "gal16v8a.116",   0x0400, 0x0117, CRC(3674f043) SHA1(06c88f65877a6575149bdd4f7cea64cd310227bd) )
	ROM_LOAD( "gal16v8a.127",   0x0600, 0x0117, CRC(7115d95c) SHA1(23044039373b5a2face63d72c3fc6bf7f0c8a475) )
ROM_END


ROM_START( hatris )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "2-ic79.bin",	0x00000, 0x08000, CRC(bbcaddbf) SHA1(7f01493dadfed87112644a8ef77ae58fa273980d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "1-ic81.bin",	0x00000, 0x08000, CRC(db25e166) SHA1(3538963d092967311d0a216b1e33ea39389b0d87) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b0-ic56.bin", 0x00000, 0x20000, CRC(34f337a4) SHA1(ad74bb3fbfd16c9e92daa1cf5c5e522d11ba7dfb) )
	ROM_FILL(                0x20000, 0x20000, 0 )
	ROM_LOAD( "b1-ic73.bin", 0x40000, 0x08000, CRC(6351d0ba) SHA1(6d6b2e23f0569e625414de11803955df60bbbd48) )
	ROM_FILL(                0x48000, 0x18000, 0 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a0-ic55.bin", 0x00000, 0x20000, CRC(7b7bc619) SHA1(b661c772e33aa7352dcdc20c4a9a84ed25ff89d7) )
	ROM_LOAD( "a1-ic60.bin", 0x20000, 0x20000, CRC(f74d4168) SHA1(9ac433c4ce61fe402334aa97d32a51cfac634c46) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "pc-ic53.bin", 0x00000, 0x20000, CRC(07147712) SHA1(97692186e85f3a4a19dbd1bd95ed882e903a3c4a) )
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( pipedrm )
{
	/* sprite RAM lives at the end of palette RAM */
	spriteram = &paletteram[0xc00];
	spriteram_size = 0x400;
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xcc00, 0xcfff, 0, 0, MRA8_BANK3);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xcc00, 0xcfff, 0, 0, MWA8_BANK3);
	memory_set_bankptr(3, spriteram);
}


static DRIVER_INIT( hatris )
{
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x20, 0x20, 0, 0, sound_command_nonmi_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x21, 0x21, 0, 0, fromance_gfxreg_w);
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1990, pipedrm,  0,       pipedrm, pipedrm, pipedrm, ROT0, "Video System Co.", "Pipe Dream (US)", GAME_SUPPORTS_SAVE )
GAME( 1990, pipedrmj, pipedrm, pipedrm, pipedrm, pipedrm, ROT0, "Video System Co.", "Pipe Dream (Japan)", GAME_SUPPORTS_SAVE )
GAME( 1990, hatris,   0,       hatris,  hatris,  hatris,  ROT0, "Video System Co.", "Hatris (Japan)", GAME_SUPPORTS_SAVE )
