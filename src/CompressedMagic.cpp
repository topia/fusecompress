/*
    (C) Copyright Milan Svoboda 2009.
    
    This file is part of FuseCompress.

    FuseCompress is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <rlog/rlog.h>
#include <stdlib.h>
#include <ostream>
#include <iostream>
#include <iomanip>
#include <string>
#include <sys/ioctl.h>
#include <stdio.h>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

#include "Lock.hpp"
#include "CompressedMagic.hpp"

void CompressedMagic::PopulateTable()
{
	m_table.insert("audio/mp4");
	m_table.insert("audio/mpeg");
	m_table.insert("audio/x-pn-realaudio");
	m_table.insert("audio/x-mod");
	m_table.insert("audio/x-flac");
	m_table.insert("audio/x-musepack");
	m_table.insert("audio/x-ape");
	m_table.insert("audio/x-wavpack");
	m_table.insert("audio/x-wavpack-correction");
	m_table.insert("application/x-7z-compressed");
	m_table.insert("application/ogg");
	m_table.insert("application/pdf");
	m_table.insert("application/vnd.rn-realmedia");
	m_table.insert("application/x-arc");
	m_table.insert("application/x-arj");
	m_table.insert("application/x-bzip2");
	m_table.insert("application/x-bzip");
	m_table.insert("application/x-bzip-compressed-tar");
	m_table.insert("application/x-compress");
	m_table.insert("application/x-compress-tar");
	m_table.insert("application/x-cpio");
	m_table.insert("application/x-debian-package");
	m_table.insert("application/x-deb");
	m_table.insert("application/x-gzip");
	m_table.insert("application/x-lharc");
	m_table.insert("application/x-lzma");
	m_table.insert("application/x-lzma-compressed-tar");
	m_table.insert("application/x-quicktime");
	m_table.insert("application/x-rar");
	m_table.insert("application/x-rpm");
	m_table.insert("application/x-shockwave-flash");
	m_table.insert("application/x-xz");
	m_table.insert("application/x-zip");
	m_table.insert("application/x-zoo");
	m_table.insert("image/gif");
	m_table.insert("image/jpeg");
	m_table.insert("image/jp2");
	m_table.insert("image/png");
	m_table.insert("image/x-quicktime");
	m_table.insert("video/3gpp");
	m_table.insert("video/mp4");
	m_table.insert("video/mp4v-es");
	m_table.insert("video/mpeg");
	m_table.insert("video/mp2t");
	m_table.insert("video/mpv");
	m_table.insert("video/quicktime");
	m_table.insert("video/x-msvideo");
	m_table.insert("video/x-ms-asf");
	m_table.insert("video/x-ms-wmv");
	m_table.insert("video/x-matroska");
	m_table.insert("video/x-flv");
}

CompressedMagic::CompressedMagic()
{
	// In newer versions of libmagic MAGIC_MIME is declared as MAGIC_MIME_TYPE | MAGIC_MIME_ENCODING.
	// Older versions don't know MAGIC_MIME_TYPE, though -- the old MAGIC_MIME is the new MAGIC_MIME_TYPE,
	// and the new MAGIC_MIME has been redefined.
	// However, I recommend you to upgrade to the newest version of libmagic because at least
	// one bug (memory leak) is fixed there.

#ifdef MAGIC_MIME_TYPE
	m_magic = magic_open(MAGIC_MIME_TYPE | MAGIC_PRESERVE_ATIME);
#else
	m_magic = magic_open(MAGIC_MIME      | MAGIC_PRESERVE_ATIME);
#endif
	if (!m_magic)
	{
		rError("CompressedMagic::CompressedMagic magic_open failed with: %s",
				magic_error(m_magic));
		abort();
	}
	magic_load(m_magic, NULL);

	PopulateTable();
}

CompressedMagic::~CompressedMagic()
{
	m_table.clear();

	magic_close(m_magic);
}

std::ostream &operator<<(std::ostream &os, const CompressedMagic &rObj)
{
	CompressedMagic::con_t::const_iterator it;
	unsigned int len = 0;

	// Get length of the longest name.
	//
	for (it  = rObj.m_table.begin(); it != rObj.m_table.end(); ++it)
	{
		if (len < (*it).length())
			len = (*it).length();
	}

	// Compute how many colums fit to a terminal.
	//
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);

	unsigned int cols = (w.ws_col - 2) / (len + 5);
	cols = cols > 0 ? cols : 1;

	unsigned int col;

	os << "  ";

	for (it  = rObj.m_table.begin(), col = 1; it != rObj.m_table.end(); ++it, col++)
	{
		os << std::setw(len + 5) << std::left << *it;
		if ((col % cols)== 0)
			os << std::endl << "  ";
	}
	return os;
}

void CompressedMagic::Add(const std::string &rMimes)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(";");
	tokenizer tokens(rMimes, sep);
	for (tokenizer::iterator tok_it = tokens.begin(); tok_it != tokens.end(); ++tok_it)
	{
		m_table.insert(boost::algorithm::to_lower_copy(*tok_it));
	}
}

void CompressedMagic::Remove(const std::string &rMimes)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(";");
	tokenizer tokens(rMimes, sep);
	for (tokenizer::iterator tok_it = tokens.begin(); tok_it != tokens.end(); ++tok_it)
	{
		con_t::iterator it = m_table.find(boost::algorithm::to_lower_copy(*tok_it));
		if (it != m_table.end())
			m_table.erase(it);
	}
}

bool CompressedMagic::isNativelyCompressed(const char *buf, int len)
{
	const char *mime;

	Lock lock(m_Mutex);

	mime = magic_buffer(m_magic, buf, len);

	if (mime != NULL)
	{
		if (m_table.find(mime) != m_table.end())
		{
			rDebug("Data identified as already compressed (%s)", mime);
			return true;
		}
	}
	rDebug("Data identified as not compressed (%s)", mime);
	return false;
}

bool CompressedMagic::isNativelyCompressed(const char *name)
{
	const char *mime;

	Lock lock(m_Mutex);

	mime = magic_file(m_magic, name);

	if (mime != NULL)
	{
		if (m_table.find(mime) != m_table.end())
		{
			rDebug("Data identified as already compressed (%s)", mime);
			return true;
		}
	}
	rDebug("Data identified as not compressed (%s)", mime);
	return false;
}

