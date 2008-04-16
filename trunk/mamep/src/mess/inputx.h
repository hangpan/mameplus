/*********************************************************************

	inputx.h

	Secondary input related functions for MESS specific functionality

*********************************************************************/

#ifndef __INPUTX_H__
#define __INPUTX_H__

#include "mame.h"
#include "driver.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* input classes */
enum
{
	INPUT_CLASS_INTERNAL,
	INPUT_CLASS_KEYBOARD,
	INPUT_CLASS_CONTROLLER,
	INPUT_CLASS_CONFIG,
	INPUT_CLASS_DIPSWITCH,
	INPUT_CLASS_CATEGORIZED,
	INPUT_CLASS_MISC
};

/* MESS uses Supplementary private use B to represent code points
 * corresponding to MAME keycodes and shift keys.  The nice thing about
 * supplemental private use B is that it is higher than any possible code
 * points */
#define UCHAR_PRIVATE		(0x100000)
#define UCHAR_SHIFT_1		(UCHAR_PRIVATE + 0)
#define UCHAR_SHIFT_2		(UCHAR_PRIVATE + 1)
#define UCHAR_MAMEKEY_BEGIN	(UCHAR_PRIVATE + 2)
#define UCHAR_MAMEKEY_END	(UCHAR_MAMEKEY_BEGIN + __code_key_last)
#define UCHAR_MAMEKEY(code)	(UCHAR_MAMEKEY_BEGIN + KEYCODE_##code)

#define UCHAR_SHIFT_BEGIN	(UCHAR_SHIFT_1)
#define UCHAR_SHIFT_END		(UCHAR_SHIFT_2)



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* these are called by the core; they should not be called from FEs */
void inputx_init(running_machine *machine);
void mess_input_port_update_hook(int portnum, UINT32 *digital);
void inputx_handle_mess_extensions(input_port_entry *ipt);

/* called by drivers to setup natural keyboard support */
void inputx_setup_natural_keyboard(
	int (*queue_chars)(const unicode_char *text, size_t text_len),
	int (*accept_char)(unicode_char ch),
	int (*charqueue_empty)(void));

/* run the validity checks */
int inputx_validitycheck(const game_driver *gamedrv, input_port_entry **memory);

/* these can be called from FEs */
int inputx_can_post(void);
int inputx_can_post_key(unicode_char ch);
int inputx_is_posting(void);
const char *inputx_key_name(unicode_char ch);

/* various posting functions; can be called from FEs */
void inputx_post(const unicode_char *text);
void inputx_post_rate(const unicode_char *text, attotime rate);
void inputx_postc(unicode_char ch);
void inputx_postc_rate(unicode_char ch, attotime rate);
void inputx_postn(const unicode_char *text, size_t text_len);
void inputx_postn_rate(const unicode_char *text, size_t text_len, attotime rate);
void inputx_post_utf16(const utf16_char *text);
void inputx_post_utf16_rate(const utf16_char *text, attotime rate);
void inputx_postn_utf16(const utf16_char *text, size_t text_len);
void inputx_postn_utf16_rate(const utf16_char *text, size_t text_len, attotime rate);
void inputx_post_utf8(const char *text);
void inputx_post_utf8_rate(const char *text, attotime rate);
void inputx_postn_utf8(const char *text, size_t text_len);
void inputx_postn_utf8_rate(const char *text, size_t text_len, attotime rate);
void inputx_post_coded(const char *text);
void inputx_post_coded_rate(const char *text, attotime rate);
void inputx_postn_coded(const char *text, size_t text_len);
void inputx_postn_coded_rate(const char *text, size_t text_len, attotime rate);

/* miscellaneous functions */
int input_classify_port(const input_port_entry *in);
int input_has_input_class(running_machine *machine, int inputclass);
int input_player_number(const input_port_entry *in);
int input_count_players(running_machine *machine);
int input_category_active(running_machine *machine, int category);

#endif /* INPUTX_H */
