#include <string.h>

#include "firmware.h"

#include "main.h"
#include "macros.h"

#include "autoiso.h"
#include "display.h"
#include "persist.h"
#include "settings.h"
#include "scripts.h"
#include "utils.h"
#include "intercom.h"

#include "shortcuts.h"

const char *label_iso     = SHORTCUT_LABEL_ISO;
const char *label_mlu     = SHORTCUT_LABEL_MLU;
const char *label_aeb     = SHORTCUT_LABEL_AEB;
const char *label_flash   = SHORTCUT_LABEL_FLASH;
const char *label_display = SHORTCUT_LABEL_DISPLAY;

void repeat_last_script     (void);

void shortcut_start(shortcut_t action);

void shortcut_iso_toggle (void);
void shortcut_aeb_toggle (void);

void shortcut_iso_set    (iso_t iso);
void shortcut_mlu_set    (int status);
void shortcut_efl_set    (int status);
void shortcut_f2c_set    (int status);
void shortcut_aeb_set    (int aeb);
void shortcut_disp_set   (int brightness);

void shortcut_info_iso     (void);
void shortcut_info_mlu     (void);
void shortcut_info_aeb     (void);
void shortcut_info_flash   (void);
void shortcut_info_display (void);

void shortcut_info_str(const char *label, const char *value);
void shortcut_info_int(const char *label, const int   value);
void shortcut_info_end(void);

void shortcut_jump() {
	shortcut_start(settings.shortcut_jump);
}

void shortcut_trash() {
	shortcut_start(settings.shortcut_trash);
}

void shortcut_disp() {
	if (settings.button_disp) {
		if (FLAG_GUI_MODE == GUIMODE_OFF) {
			press_button(IC_BUTTON_DISP);
		} else {
			status.shortcut_running = SHORTCUT_DISPLAY;
			shortcut_info_display();
		}
	} else {
		press_button(IC_BUTTON_DISP);
	}
}

void shortcut_start(shortcut_t action) {
	char *msg = NULL;

	status.shortcut_running = action;

	switch (action) {
	case SHORTCUT_ISO:
		shortcut_info_iso();
		break;
	case SHORTCUT_SCRIPT:
		repeat_last_script();
		break;
	case SHORTCUT_MLU:
		shortcut_info_mlu();
		break;
	case SHORTCUT_AEB:
		shortcut_info_aeb();
		break;
	case SHORTCUT_HACK_MENU:
		menu_main_start();
		break;
	case SHORTCUT_TOGGLE_FLASH:
		shortcut_info_flash();
		break;
	case SHORTCUT_DISPLAY:
		shortcut_info_display();
		break;
#ifdef DEV_BTN_ACTION
	case SHORTCUT_DEV_BTN:
		dev_btn_action();
		break;
#endif
	default:
		break;
	}

	if (msg) {
		dialog_item_set_str(hMainDialog, 0x26, msg);
		display_refresh();
	}
}

void shortcut_event_disp() {
	press_button(IC_BUTTON_DISP);
	enqueue_action(beep);
	shortcut_event_end();
}

void shortcut_event_end() {
	switch (status.shortcut_running) {
	case SHORTCUT_AEB:
		enqueue_action(persist_write);
		break;
	default:
		break;
	}

	status.shortcut_running = SHORTCUT_NONE;

	shortcut_info_end();
}

void shortcut_event_set(void) {
	switch (status.shortcut_running) {
	case SHORTCUT_ISO:
		shortcut_iso_toggle();
		break;
	case SHORTCUT_AEB:
		shortcut_aeb_toggle();
		break;
	case SHORTCUT_DISPLAY:
		enqueue_action(beep);
		shortcut_event_end();
		break;
	default:
		break;
	}
}

void shortcut_event_up(void) {
	switch (status.shortcut_running) {
	case SHORTCUT_ISO:
		shortcut_iso_set(iso_next(DPData.iso));
		break;
	case SHORTCUT_MLU:
		shortcut_mlu_set(TRUE);
		break;
	case SHORTCUT_AEB:
		shortcut_aeb_set(MIN((EV_TRUNC(DPData.ae_bkt) + EV_CODE(1, 0)), EC_MAX));
		break;
	case SHORTCUT_TOGGLE_FLASH:
		shortcut_efl_set(TRUE);
		break;
	case SHORTCUT_DISPLAY:
		shortcut_disp_set(7);
		break;
	default:
		break;
	}
}

void shortcut_event_down(void) {
	switch (status.shortcut_running) {
	case SHORTCUT_ISO:
		shortcut_iso_set(iso_prev(DPData.iso));
		break;
	case SHORTCUT_MLU:
		shortcut_mlu_set(FALSE);
		break;
	case SHORTCUT_AEB:
		shortcut_aeb_set(MAX((EV_TRUNC(DPData.ae_bkt) - EV_CODE(1, 0)), EC_ZERO));
		break;
	case SHORTCUT_TOGGLE_FLASH:
		shortcut_efl_set(FALSE);
		break;
	case SHORTCUT_DISPLAY:
		shortcut_disp_set(1);
		break;
	default:
		break;
	}
}

void shortcut_event_right(void) {
	switch (status.shortcut_running) {
	case SHORTCUT_ISO:
		shortcut_iso_set(iso_inc(DPData.iso));
		break;
	case SHORTCUT_AEB:
		shortcut_aeb_set(MIN(ec_inc(DPData.ae_bkt), EC_MAX));
		break;
	case SHORTCUT_TOGGLE_FLASH:
		shortcut_f2c_set(TRUE);
		break;
	case SHORTCUT_DISPLAY:
		shortcut_disp_set(MIN(DPData.lcd_brightness + 1, 7));
		break;
	default:
		break;
	}
}

void shortcut_event_left(void) {
	switch (status.shortcut_running) {
	case SHORTCUT_ISO:
		shortcut_iso_set(iso_dec(DPData.iso));
		break;
	case SHORTCUT_AEB:
		shortcut_aeb_set(MAX(ec_dec(DPData.ae_bkt), EV_ZERO));
		break;
	case SHORTCUT_TOGGLE_FLASH:
		shortcut_f2c_set(FALSE);
		break;
	case SHORTCUT_DISPLAY:
		shortcut_disp_set(MAX(DPData.lcd_brightness - 1, 1));
		break;
	default:
		break;
	}
}

void shortcut_iso_toggle() {
	settings.autoiso_enable = ! settings.autoiso_enable;
	enqueue_action(settings_write);

	shortcut_info_iso();
}

void shortcut_aeb_toggle() {
	shortcut_aeb_set(DPData.ae_bkt ? EC_ZERO : persist.last_aeb);
}

void shortcut_iso_set(iso_t iso) {
	if (settings.autoiso_enable) {
		settings.autoiso_enable = FALSE;
		enqueue_action(settings_write);
		enqueue_action(beep);
	}

	send_to_intercom(IC_SET_ISO, iso);
	shortcut_info_iso();
}

void shortcut_mlu_set(int status) {
	send_to_intercom(IC_SET_CF_MIRROR_UP_LOCK, status);
	shortcut_info_mlu();
}

void shortcut_efl_set(int status) {
	send_to_intercom(IC_SET_CF_EMIT_FLASH, status);
	shortcut_info_flash();
}

void shortcut_f2c_set(int status) {
	send_to_intercom(IC_SET_CF_FLASH_SYNC_REAR, status);
	shortcut_info_flash();
}

void shortcut_aeb_set(int aeb) {
	send_to_intercom(IC_SET_AE_BKT, aeb);

	persist.aeb = aeb;

	if (persist.aeb)
		persist.last_aeb = persist.aeb;

	shortcut_info_aeb();
}

void shortcut_disp_set(int brightness) {
	send_to_intercom(IC_SET_LCD_BRIGHTNESS, brightness);
	shortcut_info_display();
}

#ifdef DEV_BTN_ACTION
void dev_btn_action() {
	// quick shortcut for developers to test stuff
	beep();
	ptp_dump_info();
}
#endif

void repeat_last_script(void) {
	switch (persist.last_script) {
	case SCRIPT_EXT_AEB:
		script_ext_aeb();
		break;
	case SCRIPT_EFL_AEB:
		script_efl_aeb();
		break;
	case SCRIPT_ISO_AEB:
		script_iso_aeb();
		break;
	case SCRIPT_INTERVAL:
		script_interval();
		break;
	case SCRIPT_WAVE:
		script_wave();
		break;
	case SCRIPT_TIMER:
		script_self_timer();
		break;
	default:
		break;
	}
}

void shortcut_info_iso() {
	char buffer[8] = "AUTO";

	if (!settings.autoiso_enable)
		iso_print(buffer, DPData.iso);

	shortcut_info_str(label_iso, buffer);
}

void shortcut_info_mlu() {
	shortcut_info_str(label_mlu, DPData.cf_mirror_up_lock ? " On" : " Off");
}

void shortcut_info_aeb() {
	char buffer[8] = "", *sub;

	switch (EV_SUB(DPData.ae_bkt)) {
	case 0x03:
		sub = "+";
		break;
	case 0x04:
		sub = "*";
		break;
	case 0x05:
		sub = "++";
		break;
	default:
		sub = "";
	}

	sprintf(buffer, "%i %s", EV_VAL(DPData.ae_bkt), sub);
	shortcut_info_str(label_aeb, buffer);

}

void shortcut_info_flash() {
	char buffer[8] = "";

	sprintf(buffer, "%s%s", DPData.cf_emit_flash ? "On " : "Off", DPData.cf_flash_sync_rear ? "2" : "1");
	shortcut_info_str(label_flash, buffer);
}

void shortcut_info_display() {
	shortcut_info_int(label_display, DPData.lcd_brightness);
}

void shortcut_info_str(const char *label, const char *value) {
	dialog_item_set_label(hMainDialog, 0x08, label, 1 + strlen(label), 0x26);
	dialog_item_set_label(hMainDialog, 0x08, value, 1 + strlen(value), 0x04);

	display_refresh();
}

void shortcut_info_int(const char *label, const int value) {
	char buffer[8];

	sprintf(buffer, "%4i", value);
	shortcut_info_str(label, buffer);
}

void shortcut_info_end() {
	char label[8], value[8];

	sprintf(label, "%i", DPData.avail_shot);
	iso_print(value, DPData.iso);

	shortcut_info_str(label, value);
}
