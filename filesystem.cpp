/*
	MIT License

	Copyright (c) 2018 Christian Fiedler

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#include "filesystem.h"

#include <stdexcept>
#include <vector>
#include <utility>

#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef _WIN32

#include <Windows.h>

#define chdir(PATH) _chdir(PATH)
#define lstat(A,B) stat(A,B)
#define mkdir(PATH,MODE) _mkdir(PATH)

#endif

static auto is_slash(char c) -> bool
{
#ifdef _WIN32
	return c == '/' || c == '\\';
#else
	return c == '/';
#endif
}

static auto last_slash(const std::string& s) -> size_t
{
	size_t i = s.rfind('/');
#ifdef _WIN32
	size_t j = s.rfind('\\');
	if(j != std::string::npos && j > i) i = j;
#endif
	return i;
}

static auto forward_slashes(std::string& s) -> void
{
#ifdef _WIN32
	for(auto& c : s)
		if(c == '\\')
			c = '/';
#endif
}

static auto currentdir(char *buf, size_t len) -> bool
{
#ifdef _WIN32
	auto r = GetCurrentDirectory(len, buf);
	return r != 0 && r < len;
#else
	return getcwd(buf, len) != nullptr;
#endif
}

namespace boostfs{

Path::Path()
{
}

Path::Path(std::string s2) : s(s2)
{
}

Path::Path(const char *s2) : s(s2)
{
}
Path::Path(const Path& p)
{
	s = p.s;
}
Path::Path(Path&& p)
{
	s = std::move(p.s);
}

auto Path::operator+(const Path& p) const -> Path
{
	return s+p.s;
}
auto Path::operator/(const Path& p) const -> Path
{
	if(is_slash(s.back()))
		return s+p.s;
	return s+"/"+p.s;
}
auto Path::operator==(const Path& p) const -> bool
{
	return strcmp(canonical(s).c_str(),canonical(p.s).c_str()) == 0;
}
auto Path::operator!=(const Path& p) const -> bool
{
	return !(*this == p);
}
auto Path::parent_path() const -> Path
{
	size_t i = last_slash(s);
	if(i == std::string::npos){
		return "";
	}
#ifdef _WIN32
	if(isalpha(s[0]) && s[1] == ':' && i == 2){
		return s.substr(0,3);
	}
#else
	if(i == 0){
		return "/";
	}
#endif
	return s.substr(0,i);
}
auto Path::clear() -> void
{
	s.clear();
}
auto Path::empty() const -> bool
{
	return s.empty();
}
auto Path::operator=(const Path& p) -> Path&
{
	s = p.s;
	return *this;
}
auto Path::operator=(Path&& p) -> Path&
{
	s = std::move(p.s);
	return *this;
}
auto Path::operator+=(const Path& p) -> Path&
{
	s += p.s;
	return *this;
}
auto Path::operator/=(const Path& p) -> Path&
{
	if(is_slash(s.back()))
		s += p.s;
	else
		s += "/" +p.s;
	return *this;
}
auto Path::filename() const -> Path
{
	size_t i = last_slash(s);
	if(i == std::string::npos)
		return *this;
	return s.substr(i+1);
}
auto Path::extension() const -> Path
{
	size_t i = s.rfind('.');
	if(i == std::string::npos)
		return "";
	return s.substr(i);
}
auto Path::stem() const -> Path
{
	auto st = filename();
	size_t i = s.rfind('.');
	if(i == std::string::npos)
		return s;
	return s.substr(0, i);
}
auto operator+(const std::string& s, const Path& p) -> Path
{
	return path(s) + p;
}
auto operator/(const std::string& s, const Path& p) -> Path
{
	return path(s) / p;
}
auto operator==(const std::string& s, const Path& p) -> bool
{
	return p == s;
}

auto Path::size() const -> size_t
{
	return s.size();
}
auto Path::string() const -> const std::string&
{
	return s;
}
auto Path::c_str() const -> const char*
{
	return s.c_str();
}

auto Path::replace_extension(const Path& p) -> void
{
	size_t i = s.rfind('.');
	if(i != std::string::npos){
		s.erase(i);
	}
	if(!p.s.empty() && p.s[0] != '.'){
		s += '.';
	}
	s += p.s;
}

auto exists(const Path& p) -> bool
{
	struct stat st;
	return lstat(p.c_str(), &st) == 0;
}
auto remove(const Path& p) -> bool
{
#ifndef FS_DRYRUN
	if(is_directory(p))
		return rmdir(p.c_str()) == 0;
	return unlink(p.c_str()) == 0;
#else
	if(is_directory(p))
		fprintf(stderr, "rmdir %s\n", p.c_str());
	else
		fprintf(stderr, "unlink %s\n", p.c_str());
	return true;
#endif
}
auto remove_all(const Path& p) -> bool
{
	if(!is_directory(p))
		return unlink(p.c_str()) == 0;

	std::vector<std::pair<directory_iterator,Path>> moredirs;
	std::vector<std::pair<directory_iterator,Path>> dirstack;
	dirstack.push_back(std::make_pair(directory_iterator(p), p));

	const auto end_it = directory_iterator();

	while(dirstack.size() > 0){
		// take reference of dirstack, so we must not modify the vector
		auto& dir = dirstack.back();

		for(; dir.first != end_it; ++dir.first){
			auto p2 = (*dir.first).path();
			if(p2.filename() == "." || p2.filename() == "..")
				continue;
			if(is_directory(p2)){
				moredirs.emplace_back(directory_iterator(p2),p2);
			}else{
				if(!remove(p2))
					return false;
			}
		}

		if(moredirs.empty()){
			remove(dir.second);
			dirstack.pop_back();
		}else{
			dirstack.resize(dirstack.size() + moredirs.size());
			std::move(std::begin(moredirs), std::end(moredirs), std::begin(dirstack)+dirstack.size());
		}
	}
	return true;
}
auto extension(const Path& p) -> Path
{
	return p.extension();
}

auto complete(const Path& p) -> Path
{
	constexpr int N = 4096;
	char buf[N];
	auto s = p.string();
#ifdef _WIN32
	if(p.size() >= 3 && isalpha(s[0]) && s[1] == ':' && is_slash(s[2]))
		return p;
#else
	if(p.size() > 0 && is_slash(s[0]))
		return p;
#endif
	if(!currentdir(buf, N))
		throw std::runtime_error("could not obtain current working directory");

	if(p.empty())
		return buf;
	return buf / p;
}
auto canonical(const Path& p)-> Path
{
	auto s = complete(p).string();
	forward_slashes(s);
	size_t pos = 1;
	while((pos = s.find('.', pos)) != std::string::npos){
		if(s[pos-1] == '/' && pos < s.size()-1 && s[pos+1] == '/'){
			auto j = s.rfind('/', pos-2);
			s.erase(j, pos-j+1);
			continue;
		}
		if(s[pos-1] == '/' && pos == s.size()-1){
			s.erase(s.rfind('/', pos-2));
			break;
		}
		if(pos < s.size()-2 && s[pos-1] == '/' && s[pos+1] == '.' && s[pos+2] == '/'){
			auto j = s.rfind('/', pos-2);
			s.erase(j, pos-j+2);
			continue;
		}
		if(pos == s.size()-2 && s[pos-1] == '/' && s[pos+1] == '.'){
			s.erase(s.rfind('/', pos-2));
			break;
		}
		++pos;
	}
	return s;
}
auto is_regular_file(const Path& p) -> bool
{
	struct stat st;
	if(lstat(p.c_str(), &st) != 0){
		return false;
	}
	return S_ISREG(st.st_mode);
}
auto is_directory(const Path& p) -> bool
{
	struct stat st;
	if(lstat(p.c_str(), &st) != 0){
		return false;
	}
	return S_ISDIR(st.st_mode);
}
auto last_write_time(const Path& p) -> std::time_t
{
	struct stat st;
	if(stat(p.c_str(), &st) != 0){
		throw std::runtime_error("cannot stat "+p.string());
	}
	return std::time_t(st.st_mtime);
}
auto create_directory(const Path& p) -> void
{
#ifndef FS_DRYRUN
	mkdir(p.c_str(), 0755);
#else
	fprintf(stderr, "mkdir %s\n", p.c_str());
#endif
}
auto current_path() -> Path
{
	char buf[4096];
	if(!currentdir(buf, 4096)){
		throw std::runtime_error("cannot retrieve current directory");
	}
	return Path(buf);
}
auto current_path(const Path& p) -> void
{
	if(chdir(p.c_str()) != 0){
		throw std::runtime_error("cannot change to directory "+p.string());
	}
}

directory_entry::directory_entry(const Path& p2)
	: p(p2)
{
}
auto directory_entry::path() const -> const Path&
{
	return p;
}

directory_iterator::directory_iterator()
	: dir(nullptr), dp(nullptr), p()
{
}
directory_iterator::directory_iterator(const Path& p2)
	: dir(opendir(p2.string().c_str())), p(p2)
{
	if(dir == nullptr)
		throw std::runtime_error("cannot open directory "+p.string());
	++*this;
}
directory_iterator::directory_iterator(directory_iterator&& di)
	: dir(std::move(di.dir)), dp(std::move(di.dp)), p(std::move(di.p))
{
	di.dir = nullptr;
}
directory_iterator::~directory_iterator()
{
	if(dir != nullptr){
		closedir(dir);
	}
}
auto directory_iterator::operator=(directory_iterator&& di) -> directory_iterator&
{
	dir = std::move(di.dir);
	dp = std::move(di.dp);
	p = std::move(di.p);
	di.dir = nullptr;
	return *this;
}
auto directory_iterator::operator++() -> directory_iterator&
{
	if(dir != nullptr){
		dp = readdir(dir);
		if(dp == nullptr){
			closedir(dir);
			dir = nullptr;	
		}
	}
	return *this;
}
auto directory_iterator::operator*() const -> directory_entry
{
	return directory_entry(p / dp->d_name);
}
auto directory_iterator::operator==(const directory_iterator& rhs) const -> bool
{
	return dir == rhs.dir && dp == rhs.dp && (p == rhs.p || (dir == nullptr && dp == nullptr));
}
auto directory_iterator::operator!=(const directory_iterator& rhs) const -> bool
{
	return ! (*this == rhs);
}

};
