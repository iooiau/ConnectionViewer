/******************************************************************************
*                                                                             *
*    GeoIPManager.cpp                       Copyright(c) 2010-2016 itow,y.    *
*                                                                             *
******************************************************************************/

/*
  Connection Viewer
  Copyright(c) 2010-2016 itow,y.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "ConnectionViewer.h"
#include "GeoIPManager.h"
#include "libGeoIP/GeoIP.h"
#include "libGeoIP/GeoIPCity.h"

#pragma comment(lib, "libGeoIP.lib")


namespace CV
{

GeoIPManager::GeoIPManager()
	: m_pGeoIP(nullptr)
	, m_DatabaseEdition(0)
#ifdef _DEBUG
	, m_QueryCount(0)
	, m_CacheHitCount(0)
#endif
{
}

GeoIPManager::~GeoIPManager()
{
	Close();

#ifdef _DEBUG
	cvDebugTrace(TEXT("GeoIP Query %u / Cahce hit %u\n"),
				 m_QueryCount, m_CacheHitCount);
#endif
}

bool GeoIPManager::Open(LPCTSTR pFileName)
{
	Close();

	cvDebugTrace(TEXT("Open GeoIP database \"%s\"\n"), pFileName);

#ifdef UNICODE
	char szFileName[MAX_PATH];
	if (::WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS,
							  pFileName, -1, szFileName, cvLengthOf(szFileName),
							  nullptr, nullptr) <= 0)
		return false;
#endif
	m_pGeoIP = ::GeoIP_open(
#ifdef UNICODE
		szFileName,
#else
		pFileName,
#endif
		GEOIP_MEMORY_CACHE);
	if (m_pGeoIP == nullptr)
		return false;

	m_DatabaseEdition = GeoIP_database_edition(m_pGeoIP);
	if (m_DatabaseEdition != GEOIP_COUNTRY_EDITION
			&& m_DatabaseEdition != GEOIP_CITY_EDITION_REV0
			&& m_DatabaseEdition != GEOIP_CITY_EDITION_REV1) {
		cvDebugTrace(TEXT("Unsupported GeoIP database edition (%d)\n"), m_DatabaseEdition);
		Close();
		return false;
	}

	cvDebugTrace(TEXT("GeoIP database edition %d\n"), m_DatabaseEdition);

	return true;
}

void GeoIPManager::Close()
{
	if (m_pGeoIP != nullptr) {
		::GeoIP_delete(m_pGeoIP);
		m_pGeoIP = nullptr;
		m_DatabaseEdition = 0;
	}
	m_CityMap.clear();
}

bool GeoIPManager::IsOpen() const
{
	return m_pGeoIP != nullptr;
}

bool GeoIPManager::GetFileName(LPTSTR pFileName, int MaxFileName) const
{
	if (MaxFileName <= 0)
		return false;
	pFileName[0] = _T('\0');
	if (m_pGeoIP == nullptr)
		return false;
#ifdef UNICODE
	int Length = ::MultiByteToWideChar(CP_ACP, 0, m_pGeoIP->file_path, -1,
									   pFileName, MaxFileName);
	if (Length <= 0 || Length > MaxFileName)
		return false;
	::MultiByteToWideChar(CP_ACP, 0, m_pGeoIP->file_path, -1,
						  pFileName, MaxFileName);
#else
	if (::lstrlenA(m_pGeoIP->file_path) >= MaxFileName)
		return false;
	::lstrcpyA(pFileName, m_pGeoIP->file_path);
#endif
	return true;
}

bool GeoIPManager::IsCityInfoAvailable() const
{
	return m_DatabaseEdition == GEOIP_CITY_EDITION_REV0
		|| m_DatabaseEdition == GEOIP_CITY_EDITION_REV1;
}

static void AsciiToTChar(const char *pSrc, LPTSTR pTChar, int Length)
{
	if (pSrc == nullptr) {
		pTChar[0] = _T('\0');
		return;
	}
#ifdef UNICODE
	int i = 0;
	for (const char *p = pSrc; *p != '\0' && i + 1 < Length; p++)
		pTChar[i++] = *p;
	pTChar[i] = '\0';
#else
	::lstrcpynA(pTChar, pSrc, Length);
#endif
}

static void ISO_8859_1ToTChar(const char *pSrc, LPTSTR pTChar, int Length)
{
	if (pSrc == nullptr) {
		pTChar[0] = _T('\0');
		return;
	}
#ifdef UNICODE
	int i = 0;
	for (const char *p = pSrc; *p != '\0' && i + 1 < Length; p++)
		pTChar[i++] = *p;
	pTChar[i] = '\0';
#else
	WCHAR szTemp[MAX_CITY_NAME];
	::MultiByteToWideChar(1252, 0, pSrc, -1, szTemp, cvLengthOf(szTemp));
	::WideCharToMultiByte(CP_ACP, 0, szTemp, -1, pTChar, Length, nullptr, nullptr);
#endif
}

bool GeoIPManager::GetCountryInfo(const IPAddress &Address, CountryInfo *pInfo) const
{
	CityInfo City;

	if (!GetCityInfo(Address, &City))
		return false;
	*pInfo = City.Country;
	return true;
}

bool GeoIPManager::GetCityInfo(const IPAddress &Address, CityInfo *pInfo) const
{
	pInfo->Country.Code2[0] = _T('\0');
	pInfo->Country.Code3[0] = _T('\0');
	pInfo->Country.Name[0] = _T('\0');
	pInfo->Region[0] = _T('\0');
	pInfo->City[0] = _T('\0');
	pInfo->EnableLocation = false;
	pInfo->Latitude = 0.0f;
	pInfo->Longitude = 0.0f;

	if (m_pGeoIP == nullptr)
		return false;

	if (Address.Type != IP_ADDRESS_V4
			|| Address.V4.Address == CV_IP_ADDRESS_V4(0, 0, 0, 0)
			|| Address.V4.Address == CV_IP_ADDRESS_V4(255, 255, 255, 255)
			|| Address.V4.Address == CV_IP_ADDRESS_V4(127, 0, 0, 1))
		return false;

#ifdef _DEBUG
	m_QueryCount++;
#endif

	CityMap::iterator i = m_CityMap.find(Address);
	if (i != m_CityMap.end()) {
		*pInfo = i->second;
#ifdef _DEBUG
		m_CacheHitCount++;
#endif
		return true;
	}

	if (m_DatabaseEdition != GEOIP_CITY_EDITION_REV0
			|| m_DatabaseEdition != GEOIP_CITY_EDITION_REV1) {
		GeoIPRecord *pRecord = ::GeoIP_record_by_ipnum(m_pGeoIP, ::ntohl(Address.V4.Address));
		if (pRecord == nullptr)
			return false;

		AsciiToTChar(pRecord->country_code,
					 pInfo->Country.Code2, cvLengthOf(pInfo->Country.Code2));
		AsciiToTChar(pRecord->country_code3,
					 pInfo->Country.Code3, cvLengthOf(pInfo->Country.Code3));
		AsciiToTChar(pRecord->country_name,
					 pInfo->Country.Name, cvLengthOf(pInfo->Country.Name));
		AsciiToTChar(pRecord->region,
					 pInfo->Region, cvLengthOf(pInfo->Region));
		ISO_8859_1ToTChar(pRecord->city,
						  pInfo->City, cvLengthOf(pInfo->City));
		pInfo->EnableLocation = true;
		pInfo->Latitude = pRecord->latitude;
		pInfo->Longitude = pRecord->longitude;

		::GeoIPRecord_delete(pRecord);
	} else {
		int ID;
		//if (Address.Type == IP_ADDRESS_V4) {
		ID = ::GeoIP_id_by_ipnum(m_pGeoIP, ::ntohl(Address.V4.Address));
		/*} else if (Address.Type == IP_ADDRESS_V6) {
			geoipv6_t Addr;
			::CopyMemory(Addr.u.Byte, Address.V6.Bytes, 16);
			ID = ::GeoIP_id_by_ipnum_v6(m_pGeoIP, &Addr);
		} else {
			return false;
		}*/
		if (ID == 0)
			return false;
		AsciiToTChar(GeoIP_country_code[ID],
					 pInfo->Country.Code2, cvLengthOf(pInfo->Country.Code2));
		AsciiToTChar(GeoIP_country_code3[ID],
					 pInfo->Country.Code3, cvLengthOf(pInfo->Country.Code3));
		AsciiToTChar(GeoIP_country_name[ID],
					 pInfo->Country.Name, cvLengthOf(pInfo->Country.Name));
	}

	m_CityMap.insert(std::pair<IPAddress, CityInfo>(Address, *pInfo));

	return true;
}

bool GeoIPManager::FindDatabaseFile(LPCTSTR pDirectory, LPTSTR pFileName, int MaxFileName)
{
	const int DirectoryLength = ::lstrlen(pDirectory);
	TCHAR szMask[MAX_PATH];
	if (DirectoryLength + 6 >= cvLengthOf(szMask))
		return false;
	::PathCombine(szMask, pDirectory, TEXT("*.dat"));

	HANDLE hFind;
	WIN32_FIND_DATA fd;
	hFind = ::FindFirstFile(szMask, &fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return false;

	struct {
		LPCTSTR pFileName;
		bool Found;
	} FileList[] = {
		{TEXT("GeoCity.dat"),		false},
		{TEXT("GeoLiteCity.dat"),	false},
		{TEXT("GeoIP.dat"),			false},
	};

	bool Found = false;
	do {
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			if (DirectoryLength + 1 +::lstrlen(fd.cFileName) < MaxFileName) {
				for (int i = 0; i < cvLengthOf(FileList); i++) {
					if (::lstrcmpi(FileList[i].pFileName, fd.cFileName) == 0) {
						cvDebugTrace(TEXT("GeoIP database found \"%s\"\n"), fd.cFileName);
						FileList[i].Found = true;
						break;
					}
				}
			}
		}
	} while (::FindNextFile(hFind, &fd));
	::FindClose(hFind);

	for (int i = 0; i < cvLengthOf(FileList); i++) {
		if (FileList[i].Found) {
			::PathCombine(pFileName, pDirectory, FileList[i].pFileName);
			return true;
		}
	}

	return false;
}

}	// namespace CV
