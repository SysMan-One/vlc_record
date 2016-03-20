/*------------------------------ Information ---------------------------*//**
 *
 *  $HeadURL$
 *
 *  @file     qstalkerparser.h
 *
 *  @author   Jo2003
 *
 *  @date     06.09.2015
 *
 *  $Id$
 *
 *///------------------------- (c) 2015 by Jo2003  --------------------------
#ifndef __20150906_QSTALKERPARSER_H
    #define __20150906_QSTALKERPARSER_H

#include "cstdjsonparser.h"

class QStalkerParser : public CStdJsonParser
{
Q_OBJECT

public:
   QStalkerParser(QObject* parent = 0);
   virtual int parseAuth(const QString &sResp, cparser::SAuth& auth);
   virtual int parseUserSettings(const QString &sResp, QStalkerSettings& stalkSet);
   virtual int parseChannelList (const QString &sResp, QVector<cparser::SChan> &chanList, bool bFixTime);
   virtual int parseEpg (const QString &sResp, QVector<cparser::SEpg> &epgList);
   virtual int parseUrl(const QString &sResp, QString &sUrl);
   virtual int parseEpgCurrent(const QString &sResp, QCurrentMap &currentEpg);
};

#endif // __20150906_QSTALKERPARSER_H
