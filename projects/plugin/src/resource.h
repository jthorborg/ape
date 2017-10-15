#define PLUG_MFR "Janus"
#define PLUG_NAME "Audio Programming Environment"

#define PLUG_CLASS_NAME AudioProgrammingEnvironment

#define BUNDLE_MFR "Janus"
#define BUNDLE_NAME "AudioProgrammingEnvironment"

#define PLUG_ENTRY AudioProgrammingEnvironment_Entry
#define PLUG_VIEW_ENTRY AudioProgrammingEnvironment_ViewEntry

#define PLUG_ENTRY_STR "AudioProgrammingEnvironment_Entry"
#define PLUG_VIEW_ENTRY_STR "AudioProgrammingEnvironment_ViewEntry"

#define VIEW_CLASS AudioProgrammingEnvironment_View
#define VIEW_CLASS_STR "AudioProgrammingEnvironment_View"

// Format        0xMAJR.MN.BG - in HEX! so version 10.1.5 would be 0x000A0105
#define PLUG_VER 0x00010000
#define VST3_VER_STR "1.0.0"

// http://service.steinberg.de/databases/plugin.nsf/plugIn?openForm
// 4 chars, single quotes. At least one capital letter
#define PLUG_UNIQUE_ID 'Ipef'
// make sure this is not the same as BUNDLE_MFR
#define PLUG_MFR_ID 'Acme'

// ProTools stuff

#if (defined(AAX_API) || defined(RTAS_API)) && !defined(_PIDS_)
  #define _PIDS_
  const int PLUG_TYPE_IDS[2] = {'EFN1', 'EFN2'};
  const int PLUG_TYPE_IDS_AS[2] = {'EFA1', 'EFA2'}; // AudioSuite
#endif

#define PLUG_MFR_PT "Janus\nJanus\nAcme"
#define PLUG_NAME_PT "AudioProgrammingEnvironment\nIPEF"
#define PLUG_TYPE_PT "Effect"
#define PLUG_DOES_AUDIOSUITE 1

/* PLUG_TYPE_PT can be "None", "EQ", "Dynamics", "PitchShift", "Reverb", "Delay", "Modulation", 
"Harmonic" "NoiseReduction" "Dither" "SoundField" "Effect" 
instrument determined by PLUG _IS _INST
*/

#define PLUG_CHANNEL_IO "1-1 2-2"

#define PLUG_LATENCY 0
#define PLUG_IS_INST 0

// if this is 0 RTAS can't get tempo info
#define PLUG_DOES_MIDI 0

#define PLUG_DOES_STATE_CHUNKS 0

// Unique IDs for each image resource.
#define KNOB_ID 101

// Image resource locations for this plug.
#define KNOB_FN "resources/img/knob.png"

// GUI default dimensions
#define GUI_WIDTH 300
#define GUI_HEIGHT 300

// on MSVC, you must define SA_API in the resource editor preprocessor macros as well as the c++ ones
#if defined(SA_API) && !defined(OS_IOS)
#include "app_wrapper/app_resource.h"
#endif

// vst3 stuff
#define MFR_URL "www.olilarkin.co.uk"
#define MFR_EMAIL "spam@me.com"
#define EFFECT_TYPE_VST3 "Fx"


#define IDM_MYMENURESOURCE   3

#define IDR_MENU1                       101
#define IDR_ACCELERATOR1                102
#define IDM_FILE_NEW                     40001
#define IDM_FILE_OPEN                    40002
#define IDM_FILE_SAVE                    40003
#define IDM_FILE_SAVEAS                  40004
#define IDM_FILE_EXIT                    40005
#define IDM_EDIT_CUT                     40006
#define IDM_EDIT_COPY                    40007
#define IDM_EDIT_UNDO                    40008
#define IDM_EDIT_REDO                    40009
#define IDM_EDIT_PASTE                   40010
#define IDM_EDIT_DELETE                  40011
#define IDM_EDIT_SELECTALL               40012

  // Next default values for new objects
  // 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        103
#define _APS_NEXT_COMMAND_VALUE         40013
#define _APS_NEXT_CONTROL_VALUE         1000
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif
