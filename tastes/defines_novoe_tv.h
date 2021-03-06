/*********************** Information *************************\
| $HeadURL$
|
| Author: Jo2003
|
| Begin: 18.01.2010 / 09:19:48
|
| Last edited by: $Author$
|
| $Id$
\*************************************************************/
#ifndef __20130626__DEFINES_NOVOE_TV_H
   #define __20130626__DEFINES_NOVOE_TV_H

#include <QtGlobal>

#define APP_NAME      "Novoe.TV"
#define UPD_CHECK_URL "http://rt.coujo.de/novoe_tv_ver.xml"
#define BIN_NAME      "novoe_tv"
#define API_SERVER    "ott.new-rus.tv"
#define COMPANY_NAME  "Novoe.TV"
#define COMPANY_LINK  "<a href='http://www.novoe.tv'>" COMPANY_NAME "</a>"
#define VERSION_APPENDIX

#define API_JSON_PATH  "/api/json2/"

// define classes of api client ...
#define ApiClient             CNovoeClient
#define ApiParser             CNovoeParser

#endif // __20130626__DEFINES_NOVOE_TV_H
