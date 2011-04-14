#include "main.h"
#include "settings.h"
#include "display.h"
#include "firmware.h"

#include "custom.h"

type_CAMERA_MODE customCameraMode;

void get_filename(char *filename, int id);

void custom_read(int id) {
	int  version = 0;
	int  file   = -1;
	char filename[16];

	get_filename(filename, id);
	FIO_OpenFile(SETTINGS_FILE, O_RDONLY, 644);

	if (file != -1) {
		FIO_ReadFile(file, &version, sizeof(version));

		if (version == CUSTOM_VERSION) {
			FIO_ReadFile(file, &settings,         sizeof(settings));
			FIO_ReadFile(file, &customCameraMode, sizeof(customCameraMode));
		}

		FIO_CloseFile(file);
	}
}

void custom_write(int id) {
	const int version = CUSTOM_VERSION;

	int  file;
	char filename[16];

	get_filename(filename, id);
	file = FIO_OpenFile(filename, O_CREAT | O_WRONLY , 644);

	if (file != -1) {
		FIO_WriteFile(file, (void*)&version, sizeof(version));
		FIO_WriteFile(file, &settings,   sizeof(settings));
		FIO_WriteFile(file, &cameraMode, sizeof(cameraMode));
		FIO_CloseFile(file);
	}
}

extern void custom_apply() {
	settings_apply();

	SendToIntercom(EVENT_SET_AEMODE,    1, customCameraMode.AEMode);
	SendToIntercom(EVENT_SET_METMODE,   1, customCameraMode.MeteringMode);
	SendToIntercom(EVENT_SET_FLASHCOMP, 1, customCameraMode.FlashExComp);
	SendToIntercom(EVENT_SET_WBMODE,    1, customCameraMode.WB);
	SendToIntercom(EVENT_SET_AFPOINT,   2, customCameraMode.AfPoint);
	SendToIntercom(EVENT_SET_TV,        1, customCameraMode.TvVal);
	SendToIntercom(EVENT_SET_AV,        1, customCameraMode.AvVal);
	SendToIntercom(EVENT_SET_EVCOMP,    1, customCameraMode.AvComp);
	SendToIntercom(EVENT_SET_ISO,       2, customCameraMode.ISO);
	SendToIntercom(EVENT_SET_AEB,       1, customCameraMode.AEB);
	SendToIntercom(EVENT_SET_COLORTEMP, 2, customCameraMode.ColorTemp);

	SendToIntercom(EVENT_SET_CFN_AFFLASH,     1, customCameraMode.CfAfAssistBeam);
	SendToIntercom(EVENT_SET_CFN_NOFLASH,     1, customCameraMode.CfNotEmitFlash);
	SendToIntercom(EVENT_SET_CFN_MLU,         1, customCameraMode.CfMLU);
	SendToIntercom(EVENT_SET_CFN_FLASHSYNCR,  1, customCameraMode.CfFlashSyncRear);
	SendToIntercom(EVENT_SET_CFN_SAFETYSHIFT, 1, customCameraMode.CfSafetyShift);

	display_refresh();
}

void get_filename(char *filename, int id) {
	sprintf(filename, CUSTOM_FILE, id);
}