/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2004 by the KFTPGrabber developers
 * Copyright (C) 2003-2004 Jernej Kos <kostko@jweb-network.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "ftpdirectoryparser.h"
#include "ftpsocket.h"

#include <QVector>
#include <QStringList>

#include <time.h>
#include <sys/stat.h>

namespace KFTPEngine {

class DToken {
public:
    enum TokenTypeInfo {
      Unknown,
      Yes,
      No
    };
    
    DToken()
      : m_token(QString::null),
        m_valid(false)
    {
    }
    
    DToken(const QString &token, int start = 0)
      : m_token(token),
        m_length(token.length()),
        m_start(start),
        m_valid(true),
        m_numeric(Unknown),
        m_leftNumeric(Unknown),
        m_rightNumeric(Unknown)
    {
    }
    
    int getStart()
    {
      return m_start;
    }
    
    QString getToken()
    {
      return m_token;
    }
    
    int getLength()
    {
      return m_length;
    }
    
    QString getString(int type = 0)
    {
      switch (type) {
        case 0: return m_token; break;
        case 1: {
          if (!isRightNumeric() || isNumeric())
            return QString::null;
    
          int pos = m_length - 1;
          while (m_token[pos] >= '0' && m_token[pos] <= '9')
            pos--;
          
          return m_token.mid(0, pos + 1);
          break;
        }
        case 2: {
          if (!isLeftNumeric() || isNumeric())
            return QString::null;
    
          int len = 0;
          while (m_token[len] >= '0' && m_token[len] <= '9')
            len++;
            
          return m_token.mid(0, len);
          break;
        }
      }
      
      return QString::null;
    }
    
    int find(const char *chr, unsigned int start = 0) const
    {
      if (!chr)
        return -1;
      
      for (unsigned int i = start; i < m_length; i++) {
        for (int c = 0; chr[c]; c++) {
          if (m_token[i] == chr[c])
            return i;
        }
      }
      
      return -1;
    }
    
    unsigned long long getInteger()
    {
      return m_token.toULongLong();
    }
    
    unsigned long long getInteger(unsigned int start, int len)
    {
      return m_token.mid(start, len).toULongLong();
    }
    
    bool isValid()
    {
      return m_valid;
    }
    
    bool isNumeric()
    {
      if (m_numeric == Unknown) {
        bool ok;
        (void) m_token.toInt(&ok);
        
        m_numeric = ok ? Yes : No;
      }
      
      return m_numeric == Yes;
    }
    
    
    bool isNumeric(unsigned int start, unsigned int len)
    {
      len = start + len < m_length ? start + len : m_length;
        
      for (unsigned int i = start; i < len; i++) {
        if (m_token[i] < '0' || m_token[i] > '9')
          return false;
      }
          
      return true;
    }
    
    bool isLeftNumeric()
    {
      if (m_leftNumeric == Unknown) {
        if (m_length < 2)
          m_leftNumeric = No;
        else if (m_token[0] < '0' || m_token[0] > '9')
          m_leftNumeric = No;
        else
          m_leftNumeric = Yes;
      }
      
      return m_leftNumeric == Yes;
    }
    
    bool isRightNumeric()
    {
      if (m_rightNumeric == Unknown) {
        if (m_length < 2)
          m_rightNumeric = No;
        else if (m_token[m_length - 1] < '0' || m_token[m_length - 1] > '9')
          m_rightNumeric = No;
        else
          m_rightNumeric = Yes;
      }
      
      return m_rightNumeric == Yes;
    }
    
    char operator[](unsigned int n) const
    {
      return m_token[n].toAscii();
    }
private:
    QString m_token;
    unsigned int m_length;
    int m_start;
    bool m_valid;
    
    TokenTypeInfo m_numeric;
    TokenTypeInfo m_leftNumeric;
    TokenTypeInfo m_rightNumeric;
};

class DLine {
public:
    DLine(const QString &line)
      : m_line(line.trimmed()),
        m_parsePos(0)
    {
    }
    
    bool getToken(int index, DToken &token, bool toEnd = false)
    {
      if (!toEnd) {
        if (m_tokens.count() > index) {
          token = m_tokens[index];
          return true;
        }
        
        int start = m_parsePos;
        while (m_parsePos < m_line.length()) {
          if (m_line[m_parsePos] == ' ') {
            m_tokens.append(DToken(m_line.mid(start, m_parsePos - start), start));
            
            while (m_line[m_parsePos] == ' ' && m_parsePos < m_line.length())
              m_parsePos++;
              
            if (m_tokens.count() > index) {
              token = m_tokens[index];
              return true;
            }
            
            start = m_parsePos;
          }
          
          m_parsePos++;
        }
        
        if (m_parsePos != start) {
          m_tokens.append(DToken(m_line.mid(start, m_parsePos - start), start));
        }
        
        if (m_tokens.count() > index) {
          token = m_tokens[index];
          return true;
        }
        
        return false;
      } else {
        if (m_endLineTokens.count() > index) {
          token = m_endLineTokens[index];
          return true;
        }
        
        if (m_tokens.count() <= index && !getToken(index, token))
          return false;
          
        for (int i = m_endLineTokens.count(); i <= index; i++) {
          m_endLineTokens.append(DToken(m_line.mid(m_tokens[i].getStart())));
        }
        
        token = m_endLineTokens[index];
        return true;
      }
    }
private:
    QStringList m_stringList;
    QVector<DToken> m_tokens;
    QVector<DToken> m_endLineTokens;
    QString m_line;
    int m_parsePos;
};

FtpDirectoryParser::FtpDirectoryParser(FtpSocket *socket)
  : m_socket(socket),
    m_listing(DirectoryListing(socket->getCurrentDirectory()))
{
  // Populate month names as they appear in the listing
  m_monthNameMap["jan"] = 1;
  m_monthNameMap["feb"] = 2;
  m_monthNameMap["mar"] = 3;
  m_monthNameMap["apr"] = 4;
  m_monthNameMap["may"] = 5;
  m_monthNameMap["jun"] = 6;
  m_monthNameMap["june"] = 6;
  m_monthNameMap["jul"] = 7;
  m_monthNameMap["july"] = 7;
  m_monthNameMap["aug"] = 8;
  m_monthNameMap["sep"] = 9;
  m_monthNameMap["sept"] = 9;
  m_monthNameMap["oct"] = 10;
  m_monthNameMap["nov"] = 11;
  m_monthNameMap["dec"] = 12;
  
  m_monthNameMap["1"] = 1;
  m_monthNameMap["01"] = 1;
  m_monthNameMap["2"] = 2;
  m_monthNameMap["02"] = 2;
  m_monthNameMap["3"] = 3;
  m_monthNameMap["03"] = 3;
  m_monthNameMap["4"] = 4;
  m_monthNameMap["04"] = 4;
  m_monthNameMap["5"] = 5;
  m_monthNameMap["05"] = 5;
  m_monthNameMap["6"] = 6;
  m_monthNameMap["06"] = 6;
  m_monthNameMap["7"] = 7;
  m_monthNameMap["07"] = 7;
  m_monthNameMap["8"] = 8;
  m_monthNameMap["08"] = 8;
  m_monthNameMap["9"] = 9;
  m_monthNameMap["09"] = 9;
  m_monthNameMap["10"] = 10;
  m_monthNameMap["11"] = 11;
  m_monthNameMap["12"] = 12;
}

void FtpDirectoryParser::addDataLine(const QString &line)
{
  QString tmp(line);
  tmp.append("\n");
  addData(tmp.toAscii(), tmp.length());
}

void FtpDirectoryParser::addData(const char *data, int len)
{
  // Append new data to the buffer and check for any new lines
  m_buffer.append(QString::fromAscii(data, len));
  
  int pos;
  while ((pos = m_buffer.indexOf('\n')) > -1) {
    DirectoryEntry entry;
    QString line = m_buffer.mid(0, pos).trimmed();
    line = m_socket->remoteEncoding()->decode(line.toAscii());
    
    if (parseLine(line, entry) && !entry.filename().isEmpty()) {
      if (entry.type() == '-')
        entry.setType('f');

      m_listing.addEntry(entry);
    }
    
    // Remove what we just parsed
    m_buffer.remove(0, pos + 1);
  }
}

bool FtpDirectoryParser::parseMlsd(const QString &line, DirectoryEntry &entry)
{
  QStringList facts = line.split(';');
  QStringList::Iterator end = facts.end();
  
  for (QStringList::Iterator i = facts.begin(); i != end; ++i) {
    if ((*i).contains('=')) {
      QString key = (*i).section('=', 0, 0).toLower();
      QString value = (*i).section('=', 1, 1);
      
      if (key == "type") {
        if (value == "file")
          entry.setType('f');
        else if (value == "dir")
          entry.setType('d');
      } else if (key == "size") {
        entry.setSize(value.toULongLong());
      } else if (key == "modify") {
        struct tm dt;
  
        dt.tm_year = value.left(4).toInt() - 1900;
        dt.tm_mon = value.mid(4, 2).toInt() - 1;
        dt.tm_mday = value.mid(6, 2).toInt();
        dt.tm_hour = value.mid(8, 2).toInt();
        dt.tm_min = value.mid(10, 2).toInt();
        dt.tm_sec = value.mid(12, 2).toInt();
        entry.setTime(mktime(&dt));
      } else if (key == "unix.mode") {
        entry.setPermissions(value.toInt(0, 8));
      } else if (key == "unix.uid") {
        entry.setOwner(value);
      } else if (key == "unix.gid") {
        entry.setGroup(value);
      }
    } else {
      entry.setFilename((*i).trimmed());
    }
  }
  
  return true;
}

bool FtpDirectoryParser::parseUnixPermissions(const QString &permissions, DirectoryEntry &entry)
{
  int p = 0;
  
  if (permissions[1] == 'r') p |= S_IRUSR;
  if (permissions[2] == 'w') p |= S_IWUSR;
  if (permissions[3] == 'x' || permissions[3] == 's') p |= S_IXUSR;
  
  if (permissions[4] == 'r') p |= S_IRGRP;
  if (permissions[5] == 'w') p |= S_IWGRP;
  if (permissions[6] == 'x' || permissions[6] == 's') p |= S_IXGRP;
  
  if (permissions[7] == 'r') p |= S_IROTH;
  if (permissions[8] == 'w') p |= S_IWOTH;
  if (permissions[9] == 'x' || permissions[9] == 't') p |= S_IXOTH;
  
  if (permissions[3] == 's' || permissions[3] == 'S') p |= S_ISUID;
  if (permissions[6] == 's' || permissions[6] == 'S') p |= S_ISGID;
  if (permissions[9] == 't' || permissions[9] == 'T') p |= S_ISVTX;
   
  entry.setPermissions(p);
  
  return true;
}

bool FtpDirectoryParser::parseLine(const QString &line, DirectoryEntry &entry)
{
  DLine *tLine = new DLine(line);
  bool done = false;
  
  // Invalidate timestamp
  entry.setTime(-1);
  entry.timeStruct.tm_year = 0;
  entry.timeStruct.tm_mon = 0;
  entry.timeStruct.tm_hour = 0;
  entry.timeStruct.tm_mday = 0;
  entry.timeStruct.tm_min = 0;
  entry.timeStruct.tm_sec = 0;
  entry.timeStruct.tm_wday = 0;
  entry.timeStruct.tm_yday = 0;
  entry.timeStruct.tm_isdst = 0;
  
  // Attempt machine friendly format first, when socket supports MLSD
  if (m_socket->getConfig<bool>("feat.mlsd"))
    done = parseMlsd(line, entry);
  
  if (!done)
    done = parseUnix(tLine, entry);
  if (!done)
    done = parseDos(tLine, entry);
  if (!done)
    done = parseVms(tLine, entry);
  
  if (done) {
    // Convert datetime to UNIX epoch
    if (entry.time() == -1) {
      // Correct format for mktime
      entry.timeStruct.tm_year -= 1900;
      entry.timeStruct.tm_mon -= 1;
      entry.setTime(mktime(&entry.timeStruct));
    }
    
    // Add symlink if any
    if (entry.filename().contains(" -> ")) {
      int pos = entry.filename().lastIndexOf(" -> ");
      
      entry.setLink(entry.filename().mid(pos + 4));
      entry.setFilename(entry.filename().mid(0, pos));
    }
    
    // Parse owner into group/owner
    if (entry.owner().contains(" ")) {
      int pos = entry.owner().indexOf(" ");
      
      entry.setGroup(entry.owner().mid(pos + 1));
      entry.setOwner(entry.owner().mid(0, pos));
    }
    
    // Remove unwanted names
    if (entry.filename() == "." || entry.filename() == "..") {
      entry.setFilename(QString::null);
    }
  }

  delete tLine;
  return done;
}

bool FtpDirectoryParser::parseUnix(DLine *line, DirectoryEntry &entry)
{
  int index = 0;
  DToken token;
  
  if (!line->getToken(index, token))
    return false;
    
  
  char chr = token[0];
  if (chr != 'b' &&
      chr != 'c' &&
      chr != 'd' &&
      chr != 'l' &&
      chr != 'p' &&
      chr != 's' &&
      chr != '-')
      return false;
  
  QString permissions = token.getString();
  entry.setType(chr);

  // Check for netware servers, which split the permissions into two parts
  bool netware = false;
  if (token.getLength() == 1) {
    if (!line->getToken(++index, token))
      return false;
      
    permissions += " " + token.getString();
    netware = true;
  }
  
  parseUnixPermissions(permissions, entry);
  
  int numOwnerGroup = 3;
  if (!netware) {
    // Filter out groupid, we don't need it
    if (!line->getToken(++index, token))
      return false;

    if (!token.isNumeric())
      index--;
  }
  
  // Repeat until numOwnerGroup is 0 since not all servers send every possible field
  int startindex = index;
  do {
    // Reset index
    index = startindex;

    entry.setOwner(QString::null);
    for (int i = 0; i < numOwnerGroup; i++) {
      if (!line->getToken(++index, token))
        return false;
        
      if (i)
        entry.setOwner(entry.owner() + " ");
        
      entry.setOwner(entry.owner() + token.getString());
    }

    if (!line->getToken(++index, token))
      return false;

    
    // Check for concatenated groupname and size fields
    filesize_t size;
    if (!parseComplexFileSize(token, size)) {
      if (!token.isRightNumeric())
        continue;
        
      entry.setSize(token.getInteger());
    } else {
      entry.setSize(size);
    }

    // Append missing group to ownerGroup
    if (!token.isNumeric() && token.isRightNumeric()) {
      if (!entry.owner().isEmpty())
        entry.setOwner(entry.owner() + " ");
        
      entry.setOwner(entry.owner() + token.getString(1));
    }

    if (!parseUnixDateTime(line, index, entry))
      continue;

    // Get the filename
    if (!line->getToken(++index, token, true))
      continue;

    entry.setFilename(token.getString());

    // Filter out cpecial chars at the end of the filenames
    chr = token[token.getLength() - 1];
    if (chr == '/' ||
        chr == '|' ||
        chr == '*')
        entry.setFilename(entry.filename().mid(0, entry.filename().length() - 1));

    return true;
  } while (--numOwnerGroup);
      
  return false;
}

bool FtpDirectoryParser::parseUnixDateTime(DLine *line, int &index, DirectoryEntry &entry)
{
  DToken token;
  
  // Get the month date field
  QString dateMonth;
  if (!line->getToken(++index, token))
    return false;
    
  // Some servers use the following date formats:
  // 26-05 2002, 2002-10-14, 01-jun-99
  // slashes instead of dashes are also possible
  int pos = token.find("-/");
    
  if (pos != -1) {
    int pos2 = token.find("-/", pos + 1);

    if (pos2 == -1) {
      // something like 26-05 2002
      int day = token.getInteger(pos + 1, token.getLength() - pos - 1);
      
      if (day < 1 || day > 31)
        return false;
        
      entry.timeStruct.tm_mday = day;
      dateMonth = token.getString().left(pos);
    } else if (!parseShortDate(token, entry)) {
      return false;
    }
  } else {
    dateMonth = token.getString();
  }
  
  bool bHasYearAndTime = false;
  if (!entry.timeStruct.tm_mday) {
    // Get day field
    if (!line->getToken(++index, token))
      return false;
  
    int dateDay;
  
    // Check for non-numeric day
    if (!token.isNumeric() && !token.isLeftNumeric()) {
      if (dateMonth.right(1) == ".")
        dateMonth.remove(dateMonth.length() - 1, 1);
        
      bool tmp;
      dateDay = dateMonth.toInt(&tmp);
      if (!tmp)
        return false;
        
      dateMonth = token.getString();
    } else {
      dateDay = token.getInteger();
      
      if (token[token.getLength() - 1] == ',')
        bHasYearAndTime = true;
    }

    if (dateDay < 1 || dateDay > 31)
      return false;
      
    entry.timeStruct.tm_mday = dateDay;
  }
  
  if (!entry.timeStruct.tm_mon) {
    // Check month name
    if (dateMonth.right(1) == "," || dateMonth.right(1) == ".")
      dateMonth.remove(dateMonth.length() - 1, 1);
      
    dateMonth = dateMonth.toLower();
    
    QMap<QString, int>::iterator iter = m_monthNameMap.find(dateMonth);
    if (iter == m_monthNameMap.end())
      return false;
      
    entry.timeStruct.tm_mon = iter.value();
  }
  
  // Get time/year field
  if (!line->getToken(++index, token))
    return false;
    
  pos = token.find(":.-");
  if (pos != -1) {
    // Token is a time
    if (!pos || pos == (token.getLength() - 1))
      return false;

    QString str = token.getString();
    bool tmp;
    int hour = str.left(pos).toInt(&tmp);
    if (!tmp)
      return false;
      
    int minute = str.mid(pos + 1).toInt(&tmp);
    if (!tmp)
      return false;

    if (hour < 0 || hour > 23)
      return false;
      
    if (minute < 0 || minute > 59)
      return false;

    entry.timeStruct.tm_hour = hour;
    entry.timeStruct.tm_min = minute;

    // Some servers use times only for files nweer than 6 months,
    int year = QDate::currentDate().year();
    int now = QDate::currentDate().day() + 31 * QDate::currentDate().month();
    int file = entry.timeStruct.tm_mon * 31 + entry.timeStruct.tm_mday;

    if (now >= file)
      entry.timeStruct.tm_year = year;
    else
      entry.timeStruct.tm_year = year - 1;
  } else if (!entry.timeStruct.tm_year) {
    // token is a year
    if (!token.isNumeric() && !token.isLeftNumeric())
      return false;

    int year = token.getInteger();
    if (year > 3000)
      return false;
      
    if (year < 1000)
      year += 1900;

    entry.timeStruct.tm_year = year;

    if (bHasYearAndTime) {
      if (!line->getToken(++index, token))
        return false;

      if (token.find(":") == 2 && token.getLength() == 5 && token.isLeftNumeric() && token.isRightNumeric()) {
        int pos = token.find(":");
        
        // Token is a time
        if (!pos || pos == (token.getLength() - 1))
          return false;

        QString str = token.getString();
        bool tmp;
        long hour = str.left(pos).toInt(&tmp);
        if (!tmp)
          return false;
          
        long minute = str.mid(pos + 1).toInt(&tmp);
        if (!tmp)
          return false;

        if (hour < 0 || hour > 23)
          return false;
          
        if (minute < 0 || minute > 59)
          return false;

        entry.timeStruct.tm_hour = hour;
        entry.timeStruct.tm_min = minute;
      } else {
        index--;
      }
    }
  } else {
    index--;
  }
  
  return true;
}

bool FtpDirectoryParser::parseShortDate(DToken &token, DirectoryEntry &entry)
{
  if (token.getLength() < 1)
    return false;

  bool gotYear = false;
  bool gotMonth = false;
  bool gotDay = false;
  bool gotMonthName = false;

  int value = 0;

  int pos = token.find("-./");
  if (pos < 1)
    return false;
    
  if (!token.isNumeric(0, pos)) {
    // Seems to be monthname-dd-yy
    
    // Check month name
    QString dateMonth = token.getString().mid(0, pos);
    dateMonth = dateMonth.toLower();
    
    QMap<QString, int>::iterator iter = m_monthNameMap.find(dateMonth);
    if (iter == m_monthNameMap.end())
      return false;
      
    entry.timeStruct.tm_mon = iter.value();
    gotMonth = true;
    gotMonthName = true;
  } else if (pos == 4) {
    // Seems to be yyyy-mm-dd
    int year = token.getInteger(0, pos);
    
    if (year < 1900 || year > 3000)
      return false;
      
    entry.timeStruct.tm_year = year;
    gotYear = true;
  } else if (pos <= 2) {
    int value = token.getInteger(0, pos);
    
    if (token[pos] == '.') {
      // Maybe dd.mm.yyyy
      if (value < 1900 || value > 3000)
        return false;
        
      entry.timeStruct.tm_mday = value;
      gotDay = true;
    } else {
      // Detect mm-dd-yyyy or mm/dd/yyyy and
      // dd-mm-yyyy or dd/mm/yyyy
      if (value < 1)
        return false;
        
      if (value > 12) {
        if (value > 31)
          return false;

        entry.timeStruct.tm_mday = value;
        gotDay = true;
      } else {
        entry.timeStruct.tm_mon = value;
        gotMonth = true;
      }
    }
  } else {
    return false;
  }
  
  
  int pos2 = token.find("-./", pos + 1);
  
  if (pos2 == -1 || (pos2 - pos) == 1)
    return false;
    
  if (pos2 == (token.getLength() - 1))
    return false;
    
  // If we already got the month and the second field is not numeric, 
  // change old month into day and use new token as month
  if (!token.isNumeric(pos + 1, pos2 - pos - 1) && gotMonth) {
    if (gotMonthName)
      return false;

    if (gotDay)
      return false;

    gotDay = true;
    gotMonth = false;
    entry.timeStruct.tm_mday = entry.timeStruct.tm_mon;
  }
  
  if (gotYear || gotDay) {
    // Month field in yyyy-mm-dd or dd-mm-yyyy
    // Check month name
    QString dateMonth = token.getString().mid(pos + 1, pos2 - pos - 1);
    dateMonth = dateMonth.toLower();
    
    QMap<QString, int>::iterator iter = m_monthNameMap.find(dateMonth);
    if (iter == m_monthNameMap.end())
      return false;
      
    entry.timeStruct.tm_mon = iter.value();
    gotMonth = true;
  } else {
    int value = token.getInteger(pos + 1, pos2 - pos - 1);
    
    // Day field in mm-dd-yyyy
    if (value < 1 || value > 31)
      return false;
    
    entry.timeStruct.tm_mday = value;
    gotDay = true;
  }
  
  value = token.getInteger(pos2 + 1, token.getLength() - pos2 - 1);
  if (gotYear) {
    // Day field in yyy-mm-dd
    if (!value || value > 31)
      return false;
      
    entry.timeStruct.tm_mday = value;
    gotDay = true;
  } else {
    if (value < 0)
      return false;

    if (value < 50) {
      value += 2000;
    } else if (value < 1000) {
      value += 1900;
    }
    
    entry.timeStruct.tm_year = value;
    gotYear = true;
  }

  if (!gotMonth || !gotDay || !gotYear)
    return false;
    
  return true;
}

bool FtpDirectoryParser::parseDos(DLine *line, DirectoryEntry &entry)
{
  int index = 0;
  DToken token;

  // Get first token, has to be a valid date
  if (!line->getToken(index, token))
    return false;

  if (!parseShortDate(token, entry))
    return false;

  // Extract time
  if (!line->getToken(++index, token))
    return false;

  if (!parseTime(token, entry))
    return false;

  // If next token is <DIR>, entry is a directory
  // else, it should be the filesize.
  if (!line->getToken(++index, token))
    return false;

  if (token.getString() == "<DIR>") {
    entry.setType('d');
    entry.setSize(0);
  } else if (token.isNumeric() || token.isLeftNumeric()) {
    // Convert size, filter out separators
    unsigned long size = 0;
    int len = token.getLength();
    
    for (int i = 0; i < len; i++) {
      char chr = token[i];
      
      if (chr == ',' || chr == '.')
        continue;
        
      if (chr < '0' || chr > '9')
        return false;

      size *= 10;
      size += chr - '0';
    }
    
    entry.setSize(size);
    entry.setType('f');
  } else {
    return false;
  }

  // Extract filename
  if (!line->getToken(++index, token, true))
    return false;
    
  entry.setFilename(token.getString());
  
  return true;
}


bool FtpDirectoryParser::parseTime(DToken &token, DirectoryEntry &entry)
{
  int pos = token.find(":");
  if (pos < 1 || pos >= (token.getLength() - 1))
    return false;

  int hour = token.getInteger(0, pos);
  if (hour < 0 || hour > 23)
    return false;

  int minute = token.getInteger(pos + 1, 2);
  if (minute < 0 || minute > 59)
    return false;

  // Convert to 24h format
  if (!token.isRightNumeric()) {
    if (token[token.getLength() - 2] == 'P') {
      if (hour < 12) {
        hour += 12;
      }
    } else if (hour == 12) {
      hour = 0;
    }
  }

  entry.timeStruct.tm_hour = hour;
  entry.timeStruct.tm_min = minute;

  return true;
}

bool FtpDirectoryParser::parseVms(DLine *line, DirectoryEntry &entry)
{
  DToken token;
  int index = 0;

  if (!line->getToken(index, token))
    return false;

  int pos = token.find(";");
  
  if (pos == -1)
    return false;

  if (pos > 4 && token.getString().mid(pos - 4, 4) == ".DIR") {
    entry.setType('d');
    entry.setFilename(token.getString().left(pos - 4) + token.getString().mid(pos));
  } else {
    entry.setType('f');
    entry.setFilename(token.getString());
  }

  // Get size
  if (!line->getToken(++index, token))
    return false;

  if (!token.isNumeric() && !token.isLeftNumeric())
    return false;

  entry.setSize(token.getInteger());

  // Get date
  if (!line->getToken(++index, token))
    return false;

  if (!parseShortDate(token, entry))
    return false;

  // Get time
  if (!line->getToken(++index, token))
    return true;

  if (!parseTime(token, entry)) {
    int len = token.getLength();
    
    if (token[0] == '[' && token[len] != ']')
      return false;
    if (token[0] == '(' && token[len] != ')')
      return false;
    if (token[0] != '[' && token[len] == ']')
      return false;
    if (token[0] != '(' && token[len] == ')')
      return false;

    index--;
  }

  // Owner / group
  while (line->getToken(++index, token)) {
    int len = token.getLength();
    
    if (len > 2 && token[0] == '(' && token[len - 1] == ')')
      entry.setPermissions(0);
    else if (len > 2 && token[0] == '[' && token[len - 1] == ']')
      entry.setOwner(token.getString().mid(1, len - 2));
    else
      entry.setPermissions(0);
  }

  return true;
}

bool FtpDirectoryParser::parseComplexFileSize(DToken &token, filesize_t &size)
{
  if (token.isNumeric()) {
    size = token.getInteger();
    return true;
  }

  int len = token.getLength() - 1;

  char last = token[len];
  if (last == 'B' || last == 'b') {
    char c = token[len];
    
    if (c < '0' || c > '9') {
      last = token[len];
      len--;
    }
  }

  size = 0;

  int dot = -1;
  for (int i = 0; i < len; i++) {
    char c = token[i];
    
    if (c >= '0' && c <= '9') {
      size *= 10;
      size += c - '0';
    } else if (c == '.') {
      if (dot != -1)
        return false;

      dot = len - i - 1;
    } else {
      return false;
    }
  }
  
  switch (last) {
    case 'k':
    case 'K': {
      size *= 1000;
      break;
    }
    case 'm':
    case 'M': {
      size *= 1000 * 1000;
      break;
    }
    case 'g':
    case 'G': {
      size *= 1000 * 1000 * 1000;
      break;
    }
    case 't':
    case 'T': {
      size *= 1000 * 1000;
      size *= 1000 * 1000;
      break;
    }
    case 'b':
    case 'B': break;
    default: return false;
  }
  
  while (dot-- > 0)
    size /= 10;

  return true;
}

}
