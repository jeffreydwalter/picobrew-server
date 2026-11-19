/* Reconstructed from Pico Pro v0.1.34 action-name and localized-label tables.
 * Identifiers below preserve the firmware string spellings. This is reconstructed
 * declaration material, not a recovered original source file.
 * Values 0..49 are actual actions; value50 is the count sentinel.
 */
#ifndef PICO_RECOVERED_ACTION_ID_H
#define PICO_RECOVERED_ACTION_ID_H

typedef enum PicoActionId {
    PICO_ACTION_None = 0,
    PICO_ACTION_Continue = 1,
    PICO_ACTION_StillFan = 2,
    PICO_ACTION_NextStep = 3,
    PICO_ACTION_EndStep = 4,
    PICO_ACTION_PauseSystem = 5,
    PICO_ACTION_DrainToKeg = 6,
    PICO_ACTION_BasicStatus = 7,
    PICO_ACTION_DetailedStatus = 8,
    PICO_ACTION_ToggleStatus = 9,
    PICO_ACTION_ChangeTemp = 10,
    PICO_ACTION_Stop = 11,
    PICO_ACTION_StartBrewing = 12,
    PICO_ACTION_StartSousVide = 13,
    PICO_ACTION_StartColdBrew = 14,
    PICO_ACTION_StartManualBrew = 15,
    PICO_ACTION_UtilitiesMenu = 16,
    PICO_ACTION_SettingsMenu = 17,
    PICO_ACTION_SetTemperature = 18,
    PICO_ACTION_SetTargetTemperature = 19,
    PICO_ACTION_SetDuration = 20,
    PICO_ACTION_SetHoldDuration = 21,
    PICO_ACTION_SetLocation = 22,
    PICO_ACTION_Next = 23,
    PICO_ACTION_NextAsDone = 24,
    PICO_ACTION_AdjustTempAndTime = 25,
    PICO_ACTION_StartCooking = 26,
    PICO_ACTION_StartStep = 27,
    PICO_ACTION_HoldTemp = 28,
    PICO_ACTION_DrainStart = 29,
    PICO_ACTION_DrainStop = 30,
    PICO_ACTION_DrainPause = 31,
    PICO_ACTION_DrainResume = 32,
    PICO_ACTION_Pause = 33,
    PICO_ACTION_Exit = 34,
    PICO_ACTION_ExitAsEndManualBrew = 35,
    PICO_ACTION_MoveTo = 36,
    PICO_ACTION_HeatTo = 37,
    PICO_ACTION_HoldFor = 38,
    PICO_ACTION_Back = 39,
    PICO_ACTION_FirstRinse = 40,
    PICO_ACTION_Rinse = 41,
    PICO_ACTION_DeepClean = 42,
    PICO_ACTION_RackBeer = 43,
    PICO_ACTION_DrainFilter = 44,
    PICO_ACTION_DrainReserve = 45,
    PICO_ACTION_Descale = 46,
    PICO_ACTION_PicoStill = 47,
    PICO_ACTION_Circulate = 48,
    PICO_ACTION_SilenceAlarm = 49,
    PICO_ACTION__EnumCount = 50
} PicoActionId;

#endif
