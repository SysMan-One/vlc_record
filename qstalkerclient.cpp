/*------------------------------ Information ---------------------------*//**
 *
 *  $HeadURL$
 *
 *  @file     qstalkerclient.cpp
 *
 *  @author   Jo2003
 *
 *  @date     06.09.2015
 *
 *  $Id$
 *
 *///------------------------- (c) 2015 by Jo2003  --------------------------
#include "qstalkerclient.h"
#include "qcustparser.h"
#include <stdint.h>
#include <QPair>
#include <QNetworkInterface>

// global customization class ...
extern QCustParser *pCustomization;

// log file functions ...
extern CLogFile VlcLog;

/*-----------------------------------------------------------------------------\
| Function:    QStalkerClient / constructor
|
| Author:      Jo2003
|
| Begin:       Monday, January 04, 2010 16:14:52
|
| Description: constructs a QStalkerClient object to communicate with
|              kartina.tv
| Parameters:  --
|
\-----------------------------------------------------------------------------*/
QStalkerClient::QStalkerClient(QObject *parent) :QIptvCtrlClient(parent)
{
   sUsr           = "";
   sPw            = "";
   sCookie        = "";
   sApiUrl        = "";
   m_Uid          = -1;
   fillErrorMap();

   connect(this, SIGNAL(sigStringResponse(int,QString)), this, SLOT(slotStringResponse(int,QString)));
   connect(this, SIGNAL(sigBinResponse(int,QByteArray)), this, SLOT(slotBinResponse(int,QByteArray)));
   connect(this, SIGNAL(sigErr(int,QString,int)), this, SLOT(slotErr(int,QString,int)));

   setObjectName("QStalkerClient");

   // QTimer::singleShot(120000, this, SLOT(slotPing()));

   eIOps = QStalkerClient::IO_DUNNO;
}

/*-----------------------------------------------------------------------------\
| Function:    ~QStalkerClient / destructor
|
| Author:      Jo2003
|
| Begin:       Thursday, January 07, 2010 11:54:52
|
| Description: clean at destruction ...
|
| Parameters:  --
|
\-----------------------------------------------------------------------------*/
QStalkerClient::~QStalkerClient()
{
}

//---------------------------------------------------------------------------
//
//! \brief   handle inner string response from API server
//
//! \author  Jo2003
//! \date    15.03.2016
//
//! \param   reqId (int) request id
//! \param   resp (const QString&) response string
//
//---------------------------------------------------------------------------
void QStalkerClient::handleInnerOps(int reqId, const QString &resp)
{
    bool        bOk;

    if ((eIOps == QStalkerClient::IO_EPG_CUR)
        || (eIOps == QStalkerClient::IO_EPG_CUR_CHAN))
    {
        int cid = reqId - (int)CIptvDefs::REQ_INNER_OPS;
        mBufMap[cid] = resp;
        mReqsToGo --;

        if (mReqsToGo <= 0)
        {
            // create pseudo response for all channels ...
            mInfo(tr("All EPG values there!"));

            if (eIOps == QStalkerClient::IO_EPG_CUR)
            {
                combineEpgCurrent();
            }
            else if (eIOps == QStalkerClient::IO_EPG_CUR_CHAN)
            {
                combineChannelList();
            }

            // done ...
            eIOps = QStalkerClient::IO_DUNNO;
        }
    }
    else if (eIOps == QStalkerClient::IO_TV_GENRES)
    {
        mTvGenres.clear();
        QVariantMap tmpMap = QtJson::parse(resp, bOk).toMap();

        if (bOk)
        {
            foreach(const QVariant& entry, tmpMap.value("results").toList())
            {
                QVariantMap mEntry = entry.toMap();
                mTvGenres[mEntry.value("id").toString()] = mEntry.value("title").toString();
            }

            // request channels ...
            eIOps = QStalkerClient::IO_TV_CHANNELS;

            QString req = QString("users/%1/tv-channels").arg(m_Uid);
            q_get((int)CIptvDefs::REQ_INNER_OPS, sApiUrl + req);
        }
        else
        {
            mInfo(tr("QtJson parser error in %1 %2():%3")
                  .arg(__FILE__).arg(__FUNCTION__).arg(__LINE__));
        }
    }
    else if (eIOps == QStalkerClient::IO_TV_CHANNELS)
    {
        mTvChannels = QtJson::parse(resp, bOk).toMap();

        if (bOk)
        {
            QStringList chanList;

            foreach (const QVariant& lChans, mTvChannels.value("results").toList())
            {
                chanList << lChans.toMap().value("id").toString();
            }

            if (!chanList.isEmpty())
            {
                epgCurrent(chanList.join(","), true);
            }
        }
        else
        {
            mInfo(tr("QtJson parser error in %1 %2():%3")
                  .arg(__FILE__).arg(__FUNCTION__).arg(__LINE__));
        }
    }
}

//---------------------------------------------------------------------------
//
//! \brief   combine data to channel list and send out
//
//! \author  Jo2003
//! \date    18.03.2016
//
//---------------------------------------------------------------------------
void QStalkerClient::combineChannelList()
{
    mGroupMap.clear();

    foreach (const QVariant& lChans, mTvChannels.value("results").toList())
    {
        QVariantMap mChan  = lChans.toMap();
        QString     sGenre = mChan.value("genre_id").toString();
        int         iCid   = mChan.value("id").toInt();

        if (!mGroupMap.contains(sGenre))
        {
            mGroupMap[sGenre] = QList<QVariant>();
        }

        if (mBufMap.contains(iCid))
        {
            QVariantMap epgMap   = QtJson::parse(mBufMap[iCid]).toMap();
            QVariant    epgEntry = epgMap.value("results").toList().at(0);
            mChan.insert("epg", epgEntry);
        }

        mGroupMap[sGenre].append(mChan);
    }

    QList<QVariant> groupList;

    foreach (const QString& key, mGroupMap.keys())
    {
        QVariantMap group;
        group.insert("id", key);
        group.insert("title", mTvGenres[key]);
        group.insert("channels", mGroupMap.value(key));
        groupList.append(group);
    }

    mTvChannels.clear();
    mTvChannels.insert("status", QString("OK"));
    mTvChannels.insert("results", groupList);
    slotStringResponse((int)CIptvDefs::REQ_CHANNELLIST, QtJson::serializeStr(mTvChannels));
}

//---------------------------------------------------------------------------
//
//! \brief   combine epg current data and send out
//
//! \author  Jo2003
//! \date    20.03.2016
//
//---------------------------------------------------------------------------
void QStalkerClient::combineEpgCurrent()
{
    QVariantList epgCurrent;
    foreach(int key, mBufMap.keys())
    {
        QVariantMap epgMap = QtJson::parse(mBufMap[key]).toMap();
        QVariantMap chan;
        chan.insert("cid", key);
        chan.insert("epg", epgMap.value("results").toList());
        epgCurrent.append(chan);
    }

    QVariantMap mCur;
    mCur.insert("results", epgCurrent);
    mCur.insert("status", QString("OK"));

    slotStringResponse((int)CIptvDefs::REQ_EPG_CURRENT, QtJson::serializeStr(mCur));
}

//---------------------------------------------------------------------------
//
//! \brief   handle string response from API server
//
//! \author  Jo2003
//! \date    15.03.2013
//
//! \param   reqId (int) request identifier
//! \param   strResp (QString) response string
//
//! \return  --
//---------------------------------------------------------------------------
void QStalkerClient::slotStringResponse (int reqId, QString strResp)
{
    int     iErr = 0;
    QString sCleanResp;
    bool    bSendResp = true;

#ifdef __TRACE
    mInfo(tr("Response for request '%1':\n ==8<==8<==8<==\n%2\n ==>8==>8==>8==")
        .arg(karTrace.reqValToKey((CIptvDefs::EReq)reqId))
        .arg(strResp));
#endif // __TRACE

    if (reqId == (int)CIptvDefs::REQ_LOGOUT)
    {
        sCookie = "";

        // send response ...
        emit sigHttpResponse ("", reqId);
    }
    else
    {
        mInfo(tr("Request '%2' done!").arg(karTrace.reqValToKey((CIptvDefs::EReq)reqId)));

        // check response ...
        if ((iErr = checkResponse(strResp, sCleanResp)) != 0)
        {
            emit sigError(sCleanResp, reqId, iErr);
        }
        else
        {
            if (reqId == (int)CIptvDefs::REQ_NOOP)
            {
                bSendResp = false;
            }
            else if (reqId >= (int)CIptvDefs::REQ_INNER_OPS)
            {
                // modify response as well as id if needed ...
                handleInnerOps(reqId, sCleanResp);
                bSendResp = false;
            }

            if (bSendResp)
            {
                // send response ...
                emit sigHttpResponse (sCleanResp, reqId);
            }
        }
    }
}

//---------------------------------------------------------------------------
//
//! \brief   handle binary response from API server (maybe icons?)
//
//! \author  Jo2003
//! \date    15.03.2013
//
//! \param   reqId (int) request identifier
//! \param   binResp (QByteArray) binary data
//
//! \return  --
//---------------------------------------------------------------------------
void QStalkerClient::slotBinResponse (int reqId, QByteArray binResp)
{
   switch((CIptvDefs::EReq)reqId)
   {
   case CIptvDefs::REQ_DOWN_IMG:
      emit sigImage(binResp);
      break;

   default:
      break;
   }
}

//---------------------------------------------------------------------------
//
//! \brief   handle error from API server
//
//! \author  Jo2003
//! \date    15.03.2013
//
//! \param   sErr (QString) error string
//! \param   iErr (int) error code
//
//! \return  --
//---------------------------------------------------------------------------
void QStalkerClient::slotErr (int iReqId, QString sErr, int iErr)
{
    emit sigError(sErr, iReqId, iErr);
}

//---------------------------------------------------------------------------
//
//! \brief   tell API server: We're still alive
//
//! \author  Jo2003
//! \date    20.03.2016
//
//---------------------------------------------------------------------------
void QStalkerClient::slotPing()
{
    if (!sCookie.isEmpty())
    {
        // request tv channel list or channel list for settings ...
        q_get((int)CIptvDefs::REQ_NOOP, sApiUrl + "ping");
    }
    QTimer::singleShot(120000, this, SLOT(slotPing()));
}

/////////////////////////////////////////////////////////////////////////////////

/* -----------------------------------------------------------------\
|  Method: queueRequest
|  Begin: 17.03.2013
|  Author: Jo2003
|  Description: queue in new request, check for cookie and abort
|
|  Parameters: req id, param1, param2
|
|  Returns: 0 -> ok, -1 -> unknown request
\----------------------------------------------------------------- */
int QStalkerClient::queueRequest(CIptvDefs::EReq req, const QVariant& par_1, const QVariant& par_2)
{
   int iRet = 0;

   // handle special cases ...
   if (sCookie == "")
   {
      // no ccokie set, only some requests allowed ...
      switch (req)
      {
      case CIptvDefs::REQ_COOKIE:
      case CIptvDefs::REQ_LOGOUT:
      case CIptvDefs::REQ_UPDATE_CHECK:
         break;
      default:
         iRet = -1;
         break;
      }
   }

   if (iRet > -1)
   {
      // handle request ...
      switch (req)
      {
      case CIptvDefs::REQ_CHANNELLIST:
         GetChannelList();
         break;
      case CIptvDefs::REQ_COOKIE:
         GetCookie();
         break;
      case CIptvDefs::REQ_EPG:
         GetEPG(par_1.toInt(), par_2.toInt());
         break;
      case CIptvDefs::REQ_SERVER:
         SetServer(par_1.toString());
         break;
      case CIptvDefs::REQ_HTTPBUFF:
         SetHttpBuffer(par_1.toInt());
         break;
      case CIptvDefs::REQ_STREAM:
      case CIptvDefs::REQ_RADIO_STREAM:
         GetStreamURL(par_1.toInt(), par_2.toString());
         break;
      case CIptvDefs::REQ_TIMERREC:
      case CIptvDefs::REQ_RADIO_TIMERREC:
         GetStreamURL(par_1.toInt(), par_2.toString(), true);
         break;
      case CIptvDefs::REQ_ARCHIV:
         GetArchivURL(par_1.toString(), par_2.toString());
         break;
      case CIptvDefs::REQ_TIMESHIFT:
         SetTimeShift(par_1.toInt());
         break;
      case CIptvDefs::REQ_GETTIMESHIFT:
         GetTimeShift();
         break;
      case CIptvDefs::REQ_GET_SERVER:
         GetServer();
         break;
      case CIptvDefs::REQ_LOGOUT:
         Logout();
         break;
      case CIptvDefs::REQ_GETVODGENRES:
         GetVodGenres();
         break;
      case CIptvDefs::REQ_GETVIDEOS:
         GetVideos(par_1.toString());
         break;
      case CIptvDefs::REQ_GETVIDEOINFO:
         GetVideoInfo(par_1.toInt(), par_2.toString());
         break;
      case CIptvDefs::REQ_GETVODURL:
         GetVodUrl(par_1.toInt(), par_2.toString());
         break;
      case CIptvDefs::REQ_GETBITRATE:
         GetBitRate();
         break;
      case CIptvDefs::REQ_SETBITRATE:
         SetBitRate(par_1.toInt());
         break;
      case CIptvDefs::REQ_SETCHAN_HIDE:
         setChanHide(par_1.toString(), par_2.toString());
         break;
      case CIptvDefs::REQ_SETCHAN_SHOW:
         setChanShow(par_1.toString(), par_2.toString());
         break;
/*
      case CIptvDefs::REQ_CHANLIST_ALL:
         GetChannelList(par_1.toString());
         break;
*/
      case CIptvDefs::REQ_GET_VOD_MANAGER:
         getVodManager(par_1.toString());
         break;
      case CIptvDefs::REQ_SET_VOD_MANAGER:
         setVodManager(par_1.toString(), par_2.toString());
         break;
      case CIptvDefs::REQ_ADD_VOD_FAV:
         addVodFav(par_1.toInt(), par_2.toString());
         break;
      case CIptvDefs::REQ_REM_VOD_FAV:
         remVodFav(par_1.toInt(), par_2.toString());
         break;
      case CIptvDefs::REQ_GET_VOD_FAV:
         getVodFav();
         break;
      case CIptvDefs::REQ_SET_PCODE:
         setParentCode(par_1.toString(), par_2.toString());
         break;
      case CIptvDefs::REQ_EPG_CURRENT:
         epgCurrent(par_1.toString());
         break;
      case CIptvDefs::REQ_UPDATE_CHECK:
         updInfo(par_1.toString());
         break;
      case CIptvDefs::REQ_VOD_LANG:
         getVodLang();
         break;
      case CIptvDefs::REQ_USER:
         userData();
         break;
      default:
         iRet = -1;
         break;
      }
   }

   return iRet;
}

/////////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------------------\
| Function:    SetData
|
| Author:      Jo2003
|
| Begin:       Monday, January 04, 2010 16:14:52
|
| Description: set communication parameter to communicate with
|              kartina.tv
| Parameters:  host, username, password
|
\-----------------------------------------------------------------------------*/
void QStalkerClient::SetData(const QString &host, const QString &usr,
                           const QString &pw, const QString &lang)
{
   Q_UNUSED(lang)
   sUsr           = usr;
   sPw            = pw;
#ifdef _USE_QJSON
   sApiUrl        = QString("http://%1%2").arg(host).arg(pCustomization->strVal("API_JSON_PATH"));
#else
   sApiUrl        = QString("http://%1%2").arg(host).arg(pCustomization->strVal("API_XML_PATH"));
#endif // _USE_QJSON
   sCookie        = "";
}

/*-----------------------------------------------------------------------------\
| Function:    SetCookie
|
| Author:      Jo2003
|
| Begin:       03.03.2010 / 12:18:00
|
| Description: set a former stored cookie
|
| Parameters:  ref. cookie string
|
\-----------------------------------------------------------------------------*/
void QStalkerClient::SetCookie(const QString &cookie)
{
   mInfo(tr("We've got following Cookie: %1").arg(cookie));
   sCookie = cookie;
}

//---------------------------------------------------------------------------
//
//! \brief   set user id
//
//! \author  Jo2003
//
//! \param   id [in] (int) user id
//
//---------------------------------------------------------------------------
void QStalkerClient::setUid(int id)
{
    m_Uid = id;
}

//---------------------------------------------------------------------------
//
//! \brief   add some more header information for Stalker API v2
//
//! \author  Jo2003
//! \date    28.01.2014
//
//! \param   req [in/out] (QNetworkRequest &) request to be altered
//! \param   url [in] (const QString&) url as string
//! \param   iSize [in] (int) request size
//
//! \return  altered QNetworkRequest
//---------------------------------------------------------------------------
QNetworkRequest &QStalkerClient::prepareRequest(QNetworkRequest &req, const QString &url, int iSize)
{
    QIptvCtrlClient::prepareRequest(req, url, iSize);

    /// \todo We might need to use the accept and language header
    ///       outside of authentication.
    if (!sCookie.isEmpty())
    {
        req.setRawHeader("Authorization", sCookie.toUtf8());
        req.setRawHeader("Accept", "application/json");
        req.setRawHeader("MAC", getFirstMAC().toAscii());
        // req.setRawHeader("Accept-Language", "ru-Ru");
    }

#ifdef __TRACE
    mInfo(tr("URL: %1").arg(req.url().toString()));
    QList<QByteArray> headerKeys = req.rawHeaderList();
    for (int i = 0; i < headerKeys.count(); i++)
    {
        mInfo(tr("Raw Header #%1: %2: %3").arg(i).arg(QString(headerKeys[i])).arg(QString(req.rawHeader(headerKeys[i]))));
    }
#endif // __TRACE
    return req;
}

/*-----------------------------------------------------------------------------\
| Function:    Logout
|
| Author:      Jo2003
|
| Begin:       28.07.2010 / 12:18:00
|
| Description: logout from kartina
|
| Parameters:  --
|
\-----------------------------------------------------------------------------*/
void QStalkerClient::Logout ()
{
   mInfo(tr("Logout ..."));
   q_get((int)CIptvDefs::REQ_LOGOUT, sApiUrl + "logout", Iptv::Logout);
}

/*-----------------------------------------------------------------------------\
| Function:    GetCookie
|
| Author:      Jo2003
|
| Begin:       Monday, January 04, 2010 16:14:52
|
| Description: request authentication
|
| Parameters:  --
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::GetCookie ()
{
   mInfo(tr("Request Authentication ..."));
   QUrl url(sApiUrl);
   q_post((int)CIptvDefs::REQ_COOKIE,
        QString("%1://%2/stalker_portal/auth/token").arg(url.scheme()).arg(url.host()),
        QString("grant_type=password&username=%1&password=%2")
            .arg(sUsr).arg(sPw),
        Iptv::Login);
}

/*-----------------------------------------------------------------------------\
| Function:    GetChannelList
|
| Author:      Jo2003
|
| Begin:       Monday, January 04, 2010 16:14:52
|
| Description: request channel list from kartina.tv
|
| Parameters:  --
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::GetChannelList ()
{
    // stalker works very different ...
    // 1) get tv ganres
    // 2) get tv channels
    // 3) get epg current
    // 4) combine the stuff
    mInfo(tr("Request Channel List ..."));
    eIOps = QStalkerClient::IO_TV_GENRES;

    QString req = "tv-genres";

    // request tv channel list or channel list for settings ...
    q_get((int)CIptvDefs::REQ_INNER_OPS, sApiUrl + req);
}

//---------------------------------------------------------------------------
//
//! \brief   get available VOD languages
//
//! \author  Jo2003
//! \date    28.01.2014
//
//! \param   --
//
//! \return  --
//---------------------------------------------------------------------------
void QStalkerClient::getVodLang()
{
   mInfo(tr("Request available VOD languages ..."));

   q_get((int)CIptvDefs::REQ_VOD_LANG, sApiUrl + "getinfo?info=langs");
}

/*-----------------------------------------------------------------------------\
| Function:    GetServer
|
| Author:      Jo2003
|
| Begin:       17.02.2010, 18:30:52
|
| Description: request stream server list from kartina
|
| Parameters:  --
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::GetServer()
{
   mInfo(tr("Request Stream Server List ..."));

   q_get((int)CIptvDefs::REQ_GET_SERVER, sApiUrl + "settings?var=stream_server");
}

/*-----------------------------------------------------------------------------\
| Function:    GetTimeShift
|
| Author:      Jo2003
|
| Begin:       29.07.2010, 13:50:52
|
| Description: request timeshift from kartina server
|
| Parameters:  --
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::GetTimeShift()
{
   mInfo(tr("Request Time Shift ..."));

   q_get((int)CIptvDefs::REQ_GETTIMESHIFT, sApiUrl + "settings?var=timeshift");
}

/*-----------------------------------------------------------------------------\
| Function:    SetTimeShift
|
| Author:      Jo2003
|
| Begin:       Monday, January 04, 2010 16:14:52
|
| Description: set timeshift
|
| Parameters:  int value in hours (allowed: 0, 1, 2, 3, 4, 8, 9, 10, 11)
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::SetTimeShift (int iHours)
{
   mInfo(tr("Set TimeShift to %1 hour(s) ...").arg(iHours));

   q_post((int)CIptvDefs::REQ_TIMESHIFT, sApiUrl + "settings_set",
               QString("var=timeshift&val=%1").arg(iHours));
}

/*-----------------------------------------------------------------------------\
| Function:    GetBitRate
|
| Author:      Jo2003
|
| Begin:       14.01.2011, 13:50:52
|
| Description: request bitrate from kartina server
|
| Parameters:  --
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::GetBitRate()
{
   mInfo(tr("Request Bit Rate ..."));

   q_get((int)CIptvDefs::REQ_GETBITRATE, sApiUrl + "settings?var=bitrate");
}

/*-----------------------------------------------------------------------------\
| Function:    SetBitRate
|
| Author:      Jo2003
|
| Begin:       14.01.2011 / 14:05
|
| Description: set bit rate
|
| Parameters:  int value kbit/sec. (allowed: 900, 1500)
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::SetBitRate(int iRate)
{
   mInfo(tr("Set BitRate to %1 kbit/s ...").arg(iRate));

   q_post((int)CIptvDefs::REQ_SETBITRATE, sApiUrl + "settings_set",
               QString("var=bitrate&val=%1").arg(iRate));
}

/*-----------------------------------------------------------------------------\
| Function:    GetStreamURL
|
| Author:      Jo2003
|
| Begin:       Monday, January 04, 2010 16:14:52
|
| Description: request stream URL for given channel id
|
| Parameters:  channel id, security code, [flag for timer record]
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::GetStreamURL(int iChanID, const QString &secCode, bool bTimerRec)
{
   mInfo(tr("Request URL for channel %1 ...").arg(iChanID));

   QString req = QString("users/%1/tv-channels/%2/link").arg(m_Uid).arg(iChanID);

   if (secCode != "")
   {
      req += QString("&protect_code=%1").arg(secCode);
   }

   q_get((bTimerRec) ? (int)CIptvDefs::REQ_TIMERREC : (int)CIptvDefs::REQ_STREAM,
               sApiUrl + req);
}

/*-----------------------------------------------------------------------------\
| Function:    SetServer
|
| Author:      Jo2003
|
| Begin:       Monday, January 04, 2010 16:14:52
|
| Description: set streaming server
|
| Parameters:  server number (1 or 3)
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::SetServer (const QString &sIp)
{
   mInfo(tr("Set Streaming Server to %1 ...").arg(sIp));

   q_post((int)CIptvDefs::REQ_SERVER, sApiUrl + "settings_set",
               QString("var=stream_server&val=%1").arg(sIp));
}

/*-----------------------------------------------------------------------------\
| Function:    SetHttpBuffer
|
| Author:      Jo2003
|
| Begin:       Thursday, January 21, 2010 11:49:52
|
| Description: set http buffer time
|
| Parameters:  time in msec. (1500, 3000, 5000, 8000, 15000)
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::SetHttpBuffer(int iTime)
{
   mInfo(tr("Set Http Buffer to %1 msec. ...").arg(iTime));

   q_post((int)CIptvDefs::REQ_HTTPBUFF, sApiUrl + "settings_set",
               QString("var=http_caching&val=%1").arg(iTime));
}

/*-----------------------------------------------------------------------------\
| Function:    GetEPG
|
| Author:      Jo2003
|
| Begin:       Monday, January 04, 2010 16:14:52
|
| Description: request todays EPG for given channel id
|
| Parameters:  channel index
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::GetEPG(int iChanID, int iOffset)
{
    mInfo(tr("Request EPG for Channel %1 ...").arg(iChanID));

    QDateTime dt;
    dt.setDate(QDate::currentDate().addDays(iOffset));
    time_t from = dt.toTime_t();
    dt = dt.addDays(1);
    time_t to = dt.toTime_t();

    q_get((int)CIptvDefs::REQ_EPG, sApiUrl + QString("users/%1/tv-channels/%2/epg?from=%3&to=%4")
          .arg(m_Uid).arg(iChanID).arg(from).arg(to));
}

/*-----------------------------------------------------------------------------\
| Function:    GetArchivURL
|
| Author:      Jo2003
|
| Begin:       Monday, January 18, 2010 14:57:52
|
| Description: request archiv URL for prepared request
|
| Parameters:  prepared request
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::GetArchivURL (const QString &prepared, const QString &secCode)
{
    mInfo(tr("Request Archiv URL (%1) ...").arg(prepared));

    QUrl    url;
    url.setEncodedQuery(prepared.toUtf8());
    QString req;

    // we have 2 possibilities:
    // 1) get the link through epg id ...
    // 2) get the link through start time ...

    // 1)
    if (prepared.contains("epg_id"))
    {
        req = QString("epg/%1/link").arg(url.queryItemValue("epg_id"));
    }
    else // 2)
    {
        int      cid     = url.queryItemValue("cid").toInt();
        uint32_t uiStart = url.queryItemValue("gmt").toUInt();

        req = QString("users/%1/tv-channels/%2/link?start=%3").arg(m_Uid).arg(cid).arg(uiStart);
    }

    if (secCode != "")
    {
       req += QString("&protect_code=%1").arg(secCode);
    }

    q_get((int)CIptvDefs::REQ_ARCHIV, sApiUrl + req);
}

/*-----------------------------------------------------------------------------\
| Function:    GetVodGenres
|
| Author:      Jo2003
|
| Begin:       09.12.2010 / 13:18
|
| Description: request VOD genres (for now answer will be html code)
|
| Parameters:  --
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::GetVodGenres()
{
   mInfo(tr("Request VOD Genres ..."));

   q_get((int)CIptvDefs::REQ_GETVODGENRES, sApiUrl + "vod_genres");
}

/*-----------------------------------------------------------------------------\
| Function:    GetVideos
|
| Author:      Jo2003
|
| Begin:       09.12.2010 / 13:18
|
| Description: get vidoes matching to prepared search string
|
| Parameters:  prepared search string
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::GetVideos(const QString &sPrepared)
{
   mInfo(tr("Request Videos ..."));

   q_get((int)CIptvDefs::REQ_GETVIDEOS, sApiUrl + "vod_list?" + QUrl::fromPercentEncoding(sPrepared.toUtf8()));
}

/*-----------------------------------------------------------------------------\
| Function:    GetVideoInfo
|
| Author:      Jo2003
|
| Begin:       21.12.2010 / 16:18
|
| Description: get video info for video id (VOD)
|
| Parameters:  video id
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::GetVideoInfo(int iVodID, const QString &secCode)
{
   mInfo(tr("Request Video info for video %1...").arg(iVodID));

   QString req = QString("vod_info?id=%1").arg(iVodID);

   if (secCode != "")
   {
      req += QString("&protect_code=%1").arg(secCode);
   }

   q_get((int)CIptvDefs::REQ_GETVIDEOINFO, sApiUrl + req);
}

/*-----------------------------------------------------------------------------\
| Function:    GetVodUrl
|
| Author:      Jo2003
|
| Begin:       22.12.2010 / 16:18
|
| Description: get video url for video id (VOD)
|
| Parameters:  video id
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::GetVodUrl(int iVidId, const QString &secCode)
{
   mInfo(tr("Request Video Url for video %1...").arg(iVidId));

   QString req = QString("vod_geturl?fileid=%1&ad=1")
         .arg(iVidId);

   if (secCode != "")
   {
      req += QString("&protect_code=%1").arg(secCode);
   }

   q_get((int)CIptvDefs::REQ_GETVODURL, sApiUrl + req);
}

/*-----------------------------------------------------------------------------\
| Function:    setChanHide
|
| Author:      Jo2003
|
| Begin:       14.05.20012
|
| Description: hide channel from channel list
|
| Parameters:  channel id(s), security code
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::setChanHide(const QString &cids, const QString &secCode)
{
   mInfo(tr("Hide channel(s) %1 from channel list ...").arg(cids));

   QString req = QString("cmd=hide_channel&cid=%1&protect_code=%2")
           .arg(cids).arg(secCode);

   q_post((int)CIptvDefs::REQ_SETCHAN_HIDE, sApiUrl + "rule", req);
}

/*-----------------------------------------------------------------------------\
| Function:    setChanShow
|
| Author:      Jo2003
|
| Begin:       14.05.20012
|
| Description: show channel in channel list
|
| Parameters:  channel id(s), security code
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::setChanShow(const QString &cids, const QString &secCode)
{
   mInfo(tr("Show channel(s) %1 in channel list ...").arg(cids));

   QString req = QString("cmd=show_channel&cid=%1&protect_code=%2")
           .arg(cids).arg(secCode);

   q_post((int)CIptvDefs::REQ_SETCHAN_SHOW, sApiUrl + "rule", req);
}

/*-----------------------------------------------------------------------------\
| Function:    getVodManager
|
| Author:      Jo2003
|
| Begin:       23.05.2012
|
| Description: request VOD manager data
|
| Parameters:  security code
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::getVodManager(const QString &secCode)
{
   mInfo(tr("Request VOD manager data ..."));

   QString req = QString("cmd=get_user_rates&protect_code=%1")
           .arg(secCode);

   q_post((int)CIptvDefs::REQ_GET_VOD_MANAGER, sApiUrl + "vod_manage", req);
}

/*-----------------------------------------------------------------------------\
| Function:    setVodManager
|
| Author:      Jo2003
|
| Begin:       23.05.2012
|
| Description: set VOD manager data
|
| Parameters:  rules in form: "violence=hide&porn=pass", security code
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::setVodManager(const QString &rules, const QString &secCode)
{
   mInfo(tr("Set VOD manager data (%1) ...").arg(rules));

   QString req = QString("cmd=set_user_rates%1&protect_code=%2")
           .arg(rules).arg(secCode);

   q_post((int)CIptvDefs::REQ_SET_VOD_MANAGER, sApiUrl + "vod_manage", req);
}

/*-----------------------------------------------------------------------------\
| Function:    addVodFav
|
| Author:      Jo2003
|
| Begin:       29.05.2012
|
| Description: add one video to favourites
|
| Parameters:  video id
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::addVodFav(int iVidID, const QString &secCode)
{
   mInfo(tr("Add VOD favourite (%1) ...").arg(iVidID));

   QString req = QString("id=%1").arg(iVidID);

   if (secCode != "")
   {
      req += QString("&protect_code=%1").arg(secCode);
   }

   q_post((int)CIptvDefs::REQ_ADD_VOD_FAV, sApiUrl + "vod_favadd", req);
}

/*-----------------------------------------------------------------------------\
| Function:    remVodFav
|
| Author:      Jo2003
|
| Begin:       29.05.2012
|
| Description: remove one video from favourites
|
| Parameters:  video id
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::remVodFav(int iVidID, const QString &secCode)
{
   mInfo(tr("Remove VOD favourite (%1) ...").arg(iVidID));

   QString req = QString("id=%1").arg(iVidID);

   if (secCode != "")
   {
      req += QString("&protect_code=%1").arg(secCode);
   }

   q_post((int)CIptvDefs::REQ_REM_VOD_FAV, sApiUrl + "vod_favsub", req);
}

/*-----------------------------------------------------------------------------\
| Function:    getVodFav
|
| Author:      Jo2003
|
| Begin:       29.05.2012
|
| Description: request vod favourites
|
| Parameters:  --
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::getVodFav()
{
   mInfo(tr("Get VOD favourites (%1) ..."));
   q_get((int)CIptvDefs::REQ_GET_VOD_FAV, sApiUrl + "vod_favlist");
}

/*-----------------------------------------------------------------------------\
| Function:    setParentCode
|
| Author:      Jo2003
|
| Begin:       31.05.2012
|
| Description: set new parent code
|
| Parameters:  old code, new code
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::setParentCode(const QString &oldCode, const QString &newCode)
{
   mInfo(tr("Change parent code ..."));

   QString req = QString("var=pcode&old_code=%1&new_code=%2&confirm_code=%2")
         .arg(oldCode).arg(newCode);

   q_post((int)CIptvDefs::REQ_SET_PCODE, sApiUrl + "settings_set", req);
}

/*-----------------------------------------------------------------------------\
| Function:    epgCurrent
|
| Author:      Jo2003
|
| Begin:       06.12.2012
|
| Description: request current epg for given channels
|
| Parameters:  comma separated list of channel ids
|              flag when used for channel list
|
| Returns:     --
\-----------------------------------------------------------------------------*/
void QStalkerClient::epgCurrent(const QString &cids, bool bChanList)
{
    mInfo(tr("EPG current for Channels: %1 ...").arg(cids));

    // stalker API doesn't support epg current!
    // We have to split the channel ids and ask for every channel ...
    mBufMap.clear();

    QStringList sl = cids.split(",", QString::SkipEmptyParts);

    int toks  = sl.count();
    mReqsToGo = toks;

    for (int i = 0; i < toks; i++)
    {
        int cid = sl[i].toInt();
        eIOps   = bChanList ? QStalkerClient::IO_EPG_CUR_CHAN : QStalkerClient::IO_EPG_CUR;
        q_get((int)CIptvDefs::REQ_INNER_OPS + cid, sApiUrl + QString("users/%1/tv-channels/%2/epg?next=%3")
                    .arg(m_Uid).arg(cid).arg(bChanList ? 1 : 3));
    }
}

//---------------------------------------------------------------------------
//
//! \brief   check for program updates
//
//! \author  Jo2003
//! \date    17.03.2013
//
//! \param   url (QString) url with update information
//
//! \return  --
//---------------------------------------------------------------------------
void QStalkerClient::updInfo (const QString& url)
{
   mInfo(tr("Check for available updates ..."));

   q_get((int)CIptvDefs::REQ_UPDATE_CHECK, url);
}

//---------------------------------------------------------------------------
//
//! \brief   download image given by url
//
//! \author  Jo2003
//! \date    17.03.2013
//
//! \param   url (QString) url image data
//
//! \return  --
//---------------------------------------------------------------------------
void QStalkerClient::slotDownImg(const QString &url)
{
   mInfo(tr("Download image ..."));

   q_get((int)CIptvDefs::REQ_DOWN_IMG, url, Iptv::Binary);
}

/* -----------------------------------------------------------------\
|  Method: cookieSet
|  Begin: 24.01.2011 / 10:40
|  Author: Jo2003
|  Description: is cookie set?
|
|  Parameters: --
|
|  Returns: true ==> set
|          false ==> not set
\----------------------------------------------------------------- */
bool QStalkerClient::cookieSet()
{
   return (sCookie != "") ? true : false;
}

/* -----------------------------------------------------------------\
|  Method: checkResponse
|  Begin: 28.07.2010 / 18:42:54
|  Author: Jo2003
|  Description: format kartina error string
|
|  Parameters: sResp -> string to heck
|              sCleanReasp -> cleaned response
|
|  Returns: error code
\----------------------------------------------------------------- */
int QStalkerClient::checkResponse (const QString &sResp, QString &sCleanResp)
{
   int iRV = 0;
#ifdef _USE_QJSON
   sCleanResp = sResp;

   if (sCleanResp.contains("\"error\""))
   {
      iRV = -1;
   }
#else
   // clean response ... (delete content which may come
   // after / before the xml code ...
   int iStartPos   = sResp.indexOf("<?xml");       // xml start tag
   int iEndPos     = sResp.lastIndexOf('>') + 1;   // end of last tag

   // store clean string in private variable ...
   sCleanResp      = sResp.mid(iStartPos, iEndPos - iStartPos);

   QRegExp rx("<message>(.*)</message>[ \t\n\r]*"
              "<code>(.*)</code>");

   // quick'n'dirty error check ...
   if (sCleanResp.contains("<error>"))
   {
      if (rx.indexIn(sCleanResp) > -1)
      {
         iRV = rx.cap(2).toInt();

         sCleanResp = errMap.contains((CIptvDefs::EErr)iRV) ? errMap[(CIptvDefs::EErr)iRV] : rx.cap(1);
      }
   }
#endif // _USE_QJSON
   return iRV;
}

/* -----------------------------------------------------------------\
|  Method: apiUrl
|  Begin: 12.12.2013
|  Author: Jo2003
|  Description: get API url
|
|  Parameters: --
|
|  Returns: api url
\----------------------------------------------------------------- */
const QString& QStalkerClient::apiUrl()
{
    return sApiUrl;
}

//---------------------------------------------------------------------------
//
//! \brief   request user settings
//
//! \author  Jo2003
//! \date    21.09.2015
//
//! \param   id [in] (int) user id
//
//---------------------------------------------------------------------------
void QStalkerClient::userData()
{
    mInfo(tr("Get user settings ..."));
    QString req = QString("users/%1/settings").arg(m_Uid);
    q_get((int)CIptvDefs::REQ_USER, sApiUrl + req);
}

//---------------------------------------------------------------------------
//
//! \brief   get first mac address
//
//! \author  Jo2003
//! \date    20.03.2016
//
//! \return  MAC address
//---------------------------------------------------------------------------
QString QStalkerClient::getFirstMAC()
{
    if (sMac.isEmpty())
    {
        /// 1. get all interfaces, but loopback
        /// 2. check if hardware address is a valid MAC address
        foreach (QNetworkInterface interface, QNetworkInterface::allInterfaces())
        {
            if (!interface.flags().testFlag(QNetworkInterface::IsLoopBack))
            {
                sMac = interface.hardwareAddress();

                if (sMac.split(":").count() == 6)
                {
                    mInfo(tr("Interface: '%1'; MAC: %2")
                        .arg(interface.name())
                        .arg(interface.hardwareAddress()));

                    break;
                }
            }
        }
    }

    return sMac;
}


/* -----------------------------------------------------------------\
|  Method: fillErrorMap
|  Begin: 21.07.2011 / 12:30
|  Author: Jo2003
|  Description: fill error translation map
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void QStalkerClient::fillErrorMap()
{
   errMap.clear();
   errMap.insert(CIptvDefs::ERR_UNKNOWN,                 tr("Unknown error"));
   errMap.insert(CIptvDefs::ERR_INCORRECT_REQUEST,       tr("Incorrect request"));
   errMap.insert(CIptvDefs::ERR_WRONG_LOGIN_DATA,        tr("Wrong login or password"));
   errMap.insert(CIptvDefs::ERR_ACCESS_DENIED,           tr("Access denied"));
   errMap.insert(CIptvDefs::ERR_LOGIN_INCORRECT,         tr("Login incorrect"));
   errMap.insert(CIptvDefs::ERR_CONTRACT_INACTIVE,       tr("Your contract is inactive"));
   errMap.insert(CIptvDefs::ERR_CONTRACT_PAUSED,         tr("Your contract is paused"));
   errMap.insert(CIptvDefs::ERR_CHANNEL_NOT_FOUND,       tr("Channel not found or not allowed"));
   errMap.insert(CIptvDefs::ERR_BAD_PARAM,               tr("Error in request: Bad parameters"));
   errMap.insert(CIptvDefs::ERR_MISSING_PARAM_DAY,       tr("Missing parameter (day) in format <DDMMYY>"));
   errMap.insert(CIptvDefs::ERR_MISSING_PARAM_CID,       tr("Missing parameter (cid)"));
   errMap.insert(CIptvDefs::ERR_MULTIPLE_ACCOUNT_USE,    tr("Another client with your data logged in"));
   errMap.insert(CIptvDefs::ERR_AUTHENTICATION,          tr("Authentication error"));
   errMap.insert(CIptvDefs::ERR_PACKAGE_EXPIRED,         tr("Your package expired"));
   errMap.insert(CIptvDefs::ERR_UNKNOWN_API_FUNCTION,    tr("Unknown API function"));
   errMap.insert(CIptvDefs::ERR_ARCHIVE_NOT_AVAIL,       tr("Archive not available"));
   errMap.insert(CIptvDefs::ERR_MISSING_PARAM_PLACE,     tr("Missing parameter (place)"));
   errMap.insert(CIptvDefs::ERR_MISSING_PARAM_NAME,      tr("Missing parameter (name)"));
   errMap.insert(CIptvDefs::ERR_CONFIRMATION_CODE,       tr("Incorrect confirmation code"));
   errMap.insert(CIptvDefs::ERR_WRONG_PCODE,             tr("Current code is wrong"));
   errMap.insert(CIptvDefs::ERR_NEW_CODE,                tr("New code is wrong"));
   errMap.insert(CIptvDefs::ERR_MISSING_PARAM_VAL,       tr("Missing parameter (val)"));
   errMap.insert(CIptvDefs::ERR_VALUE_NOT_ALLOWED,       tr("Value not allowed"));
   errMap.insert(CIptvDefs::ERR_MISSING_PARAM,           tr("Missing parameter"));
   errMap.insert(CIptvDefs::ERR_MISSING_PARAM_ID,        tr("Missing parameter (id)"));
   errMap.insert(CIptvDefs::ERR_MISSING_PARAM_FILEID,    tr("Missing parameter (fileid)"));
   errMap.insert(CIptvDefs::ERR_MISSING_PARAM_TYPE,      tr("Missing parameter (type)"));
   errMap.insert(CIptvDefs::ERR_MISSING_PARAM_QUERY,     tr("Missing parameter (query)"));
   errMap.insert(CIptvDefs::ERR_BITRATE_NOT_AVAIL,       tr("Bitrate not available"));
   errMap.insert(CIptvDefs::ERR_SERVICE_NOT_AVAIL,       tr("Service not available"));
   errMap.insert(CIptvDefs::ERR_QUERY_LIMIT_EXCEEDED,    tr("Query limit exceeded"));
   errMap.insert(CIptvDefs::ERR_RULE_ALREADY_EXISTS,     tr("Rule already exists"));
   errMap.insert(CIptvDefs::ERR_RULE_NEED_CMD,           tr("Missing parameter (cmd)"));
   errMap.insert(CIptvDefs::ERR_MANAGE_NEED_CMD,         tr("Missing parameter (cmd)"));
   errMap.insert(CIptvDefs::ERR_MANAGE_BAD_VALUE,        tr("Bad value (rate)"));
   errMap.insert(CIptvDefs::ERR_MANAGE_FILM_NOT_FOUND,   tr("Can't find film"));
   errMap.insert(CIptvDefs::ERR_MANAGE_ALREADY_ADDED,    tr("Film already added"));
}

/*=============================================================================\
|                                    History:
| ---------------------------------------------------------------------------
| 04.Jan.2010 - communication API for kartina.tv (inspired by conros)
\=============================================================================*/
