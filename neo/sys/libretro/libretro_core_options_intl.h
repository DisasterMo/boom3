#ifndef LIBRETRO_CORE_OPTIONS_INTL_H__
#define LIBRETRO_CORE_OPTIONS_INTL_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1500 && _MSC_VER < 1900)
/* https://support.microsoft.com/en-us/kb/980263 */
#pragma execution_character_set("utf-8")
#pragma warning(disable:4566)
#endif

#include <libretro.h>

/*
 ********************************
 * VERSION: 2.0
 ********************************
 *
 * - 2.0: Add support for core options v2 interface
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/


struct retro_core_option_v2_category option_cats_it[] = {
   { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_it[] = {
	{
      "doom_framerate",
      "Framerate (riavvio)",
      NULL,
      "Modifica il framerate. Richiede un riavvio.",
      NULL,
      NULL,
      {
         { "auto",            "Auto"},
         { "50",              "50fps"},
         { "60",              "60fps"},
         { "72",              "72fps"},
         { "75",              "75fps"},
         { "90",              "90fps"},
         { "100",              "100fps"},
         { "119",              "119fps"},
         { "120",              "120fps"},
         { "144",              "144fps"},
         { "155",              "155fps"},
         { "160",              "160fps"},
         { "165",              "165fps"},
         { "180",              "180fps"},
         { "200",              "200fps"},
         { "240",              "240fps"},
         { "244",              "244fps"},
         { NULL, NULL },
      },
      "auto"
   },
   {
      "doom_resolution",
      "Risoluzione interna (riavvio)",
      NULL,
      "Configura la risoluzione. Richiede un riavvio.",
      NULL,
      NULL,
      {
         { "480x272",   NULL },
         { "640x368",   NULL },
         { "720x408",   NULL },
         { "960x544",   NULL },
		 { "1280x720",   NULL },
		 { "1920x1080",   NULL },
		 { "2560x1440",   NULL },
		 { "3840x2160",   NULL },
         { NULL, NULL },
      },
      "960x544"
   },
   {
      "doom_invert_y_axis",
      "Inverti Asse Y",
      NULL,
      "Inverte l'asse Y dell'analogico destro.",
      NULL,
      NULL,
      {
         { "disabled",  "Disattivato" },
         { "enabled",   "Attivato" },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "doom_fps",
      "Mostra FPS",
      NULL,
      "Mostra il framerate su schermo.",
      NULL,
      NULL,
      {
         { "disabled",  "Disattivato" },
         { "enabled",   "Attivato" },
         { NULL, NULL },
      },
      "disabled"
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};

struct retro_core_options_v2 options_it = {
   option_cats_it,
   option_defs_it
};

#ifdef __cplusplus
}
#endif

#endif
