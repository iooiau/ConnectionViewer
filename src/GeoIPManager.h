/******************************************************************************
*                                                                             *
*    GeoIPManager.h                         Copyright(c) 2010-2016 itow,y.    *
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


#ifndef CV_GEOIP_MANAGER_H
#define CV_GEOIP_MANAGER_H


#include <map>


struct GeoIPTag;


namespace CV
{

class GeoIPManager
{
public:
	enum
	{
		MAX_COUNTRY_NAME	= 64,
		MAX_CITY_NAME		= 64
	};

	struct CountryInfo
	{
		TCHAR Code2[3];
		TCHAR Code3[4];
		TCHAR Name[MAX_COUNTRY_NAME];
	};

	struct CityInfo
	{
		CountryInfo Country;
		TCHAR Region[8];
		TCHAR City[MAX_CITY_NAME];
		bool EnableLocation;
		float Latitude;
		float Longitude;
	};

	GeoIPManager();
	~GeoIPManager();
	bool Open(LPCTSTR pFileName);
	void Close();
	bool IsOpen() const;
	bool GetFileName(LPTSTR pFileName, int MaxFileName) const;
	bool IsCityInfoAvailable() const;
	bool GetCountryInfo(const IPAddress &Address, CountryInfo *pInfo) const;
	bool GetCityInfo(const IPAddress &Address, CityInfo *pInfo) const;

	static bool FindDatabaseFile(LPCTSTR pDirectory, LPTSTR pFileName, int MaxFileName);

private:
	typedef std::map<IPAddress, CityInfo> CityMap;

	GeoIPTag *m_pGeoIP;
	int m_DatabaseEdition;
	mutable CityMap m_CityMap;
#ifdef _DEBUG
	mutable UINT m_QueryCount;
	mutable UINT m_CacheHitCount;
#endif
};

}	// namespace CV


#endif	// ndef CV_GEOIP_MANAGER_H
