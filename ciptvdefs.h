/*=============================================================================\
| $HeadURL$
|
| Author: Jo2003
|
| last changed by: $Author$
|
| Begin: 25.03.2013
|
| $Id$
|
\=============================================================================*/
#ifndef __20130325_CIPTVDEFS_H
   #define __20130325_CIPTVDEFS_H

#include <QObject>
#include <QMetaEnum>

//---------------------------------------------------------------------------
//! \class   CIptvDefs
//! \date    04.06.2012
//! \author  Jo2003
//! \brief   pseudo class for Qt's Meta Data Handling
//!          (replaces Kartina namespace)
//---------------------------------------------------------------------------
class CIptvDefs : public QObject
{
   Q_OBJECT
   Q_ENUMS(EReq)
   Q_ENUMS(EErr)

public:
   //---------------------------------------------------------------------------
   //! request types used in client
   //---------------------------------------------------------------------------
   enum EReq
   {
      REQ_COOKIE,
      REQ_CHANNELLIST,
      REQ_STREAM,
      REQ_TIMESHIFT,
      REQ_EPG,
      REQ_EPG_EXT,
      REQ_SERVER,
      REQ_HTTPBUFF,
      REQ_ARCHIV,
      REQ_TIMERREC,
      REQ_GET_SERVER,
      REQ_LOGOUT,
      REQ_GETTIMESHIFT,
      REQ_GETVODGENRES,
      REQ_GETVIDEOS,
      REQ_GETVIDEOINFO,
      REQ_GETVODURL,
      REQ_GETBITRATE,
      REQ_SETBITRATE,
      REQ_CHANLIST_ALL,
      REQ_SETCHAN_HIDE,
      REQ_SETCHAN_SHOW,
      REQ_GET_VOD_MANAGER,
      REQ_SET_VOD_MANAGER,
      REQ_ADD_VOD_FAV,
      REQ_REM_VOD_FAV,
      REQ_GET_VOD_FAV,
      REQ_SET_PCODE,
      REQ_EPG_CURRENT,
      REQ_UPDATE_CHECK,
      REQ_DOWN_IMG,
      REQ_CHANLIST_RADIO,
      REQ_RADIO_STREAM,
      REQ_RADIO_TIMERREC,
      REQ_SET_LANGUAGE,
      REQ_CL_LANG,
      REQ_GET_ALANG,
      REQ_VOD_LANG,
      REQ_LOGIN_ONLY,
      REQ_STATS_SERVICE,
      REQ_STATS_ONLY,
      REQ_STRSTD,
      REQ_AUTO_STR_SRV,
      REQ_SPEED_TEST_DATA,
      REQ_IVI_INFO,
      REQ_UNKNOWN = 255
   };

   //---------------------------------------------------------------------------
   //! Errors generated by Kartina.TV API
   //---------------------------------------------------------------------------
   enum EErr
   {
      ERR_HTTP = -1,
      ERR_UNKNOWN,
      ERR_INCORRECT_REQUEST,
      ERR_WRONG_LOGIN_DATA,
      ERR_ACCESS_DENIED,
      ERR_LOGIN_INCORRECT,
      ERR_CONTRACT_INACTIVE,
      ERR_CONTRACT_PAUSED,
      ERR_CHANNEL_NOT_FOUND,
      ERR_BAD_PARAM,
      ERR_MISSING_PARAM_DAY,
      ERR_MISSING_PARAM_CID,
      ERR_MULTIPLE_ACCOUNT_USE,
      ERR_AUTHENTICATION,
      ERR_PACKAGE_EXPIRED,
      ERR_UNKNOWN_API_FUNCTION,
      ERR_ARCHIVE_NOT_AVAIL,
      ERR_MISSING_PARAM_PLACE,
      ERR_MISSING_PARAM_NAME,
      ERR_CONFIRMATION_CODE,
      ERR_WRONG_PCODE,
      ERR_NEW_CODE,
      ERR_MISSING_PARAM_VAL,
      ERR_VALUE_NOT_ALLOWED,
      ERR_MISSING_PARAM,
      ERR_MISSING_PARAM_ID,
      ERR_MISSING_PARAM_FILEID,
      ERR_MISSING_PARAM_TYPE,
      ERR_MISSING_PARAM_QUERY,
      ERR_REMOVED_1,
      ERR_BITRATE_NOT_AVAIL,
      ERR_SERVICE_NOT_AVAIL,
      ERR_QUERY_LIMIT_EXCEEDED,
      ERR_RULE_ALREADY_EXISTS,
      ERR_RULE_NEED_CMD,
      ERR_MANAGE_NEED_CMD,
      ERR_MANAGE_BAD_VALUE,
      ERR_MANAGE_FILM_NOT_FOUND,
      ERR_MANAGE_ALREADY_ADDED
   };

   //---------------------------------------------------------------------------
   //
   //! \brief   get ascii text for enum value
   //
   //! \author  Jo2003
   //! \date    05.06.2012
   //
   //! \param   e EReq value
   //
   //! \return  ascii string for enum value
   //---------------------------------------------------------------------------
   const char* reqValToKey(EReq e)
   {
      QMetaEnum eReqEnum = metaObject()->enumerator(metaObject()->indexOfEnumerator("EReq"));
      return eReqEnum.valueToKey(e);
   }

   //---------------------------------------------------------------------------
   //
   //! \brief   get ascii text for enum value
   //
   //! \author  Jo2003
   //! \date    05.06.2012
   //
   //! \param   e EErr value
   //
   //! \return  ascii string for enum value
   //---------------------------------------------------------------------------
   const char* errValToKey(EErr e)
   {
      QMetaEnum eErrEnum = metaObject()->enumerator(metaObject()->indexOfEnumerator("EErr"));
      return eErrEnum.valueToKey(e);
   }
};

typedef QMap<CIptvDefs::EErr, QString> QErrorMap;

#endif // __20130325_CIPTVDEFS_H
