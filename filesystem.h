#pragma once

#include <ctime>
#include <dirent.h>
#include <sys/types.h>

#include <string>

namespace boostfs{

class Path{
	std::string s;

public:
	Path();
	Path(std::string);
	Path(const char *);
	Path(const Path&);
	Path(Path&&);
	
	auto operator+(const Path& p) const -> Path;
	auto operator/(const Path& p) const -> Path;
	auto operator==(const Path& p) const -> bool;
	auto filename() const -> Path;
	auto extension() const -> Path;
	auto stem() const -> Path;
	auto parent_path() const -> Path;
	auto clear() -> void;

	auto empty() const -> bool;
	auto size() const -> size_t;
	auto string() const -> const std::string&;
	auto c_str() const -> const char*;

	auto operator=(const Path& p) -> Path&;
	auto operator=(Path&& p) -> Path&;
	auto operator+=(const Path& p) -> Path&;
	auto operator/=(const Path& p) -> Path&;
	auto replace_extension(const Path& p = Path()) -> void;
};

auto operator+(const std::string& s, const Path& p) -> Path;
auto operator/(const std::string& s, const Path& p) -> Path;
auto operator==(const std::string& s, const Path& p) -> bool;



auto exists(const Path&) -> bool;
auto remove(const Path&) -> bool;
auto remove_all(const Path&) -> bool;
auto extension(const Path&) -> Path;
auto complete(const Path&) -> Path;
auto canonical(const Path&) -> Path;
auto is_regular_file(const Path&) -> bool;
auto is_directory(const Path&) -> bool;
auto last_write_time(const Path&) -> std::time_t;
auto create_directory(const Path&) -> void;
auto current_path() -> Path;
auto current_path(const Path&) -> void;

class directory_entry{
	Path p;
public:
	explicit directory_entry(const Path&);
	auto path() const -> const Path&;
};

class directory_iterator{
	DIR *dir;
	dirent *dp;
	Path p;
public:
	directory_iterator();
	directory_iterator(const Path&);
	directory_iterator(const directory_iterator&) = delete;
	directory_iterator(directory_iterator&&);
	~directory_iterator();
	auto operator=(const directory_iterator&) -> directory_iterator& = delete;
	auto operator=(directory_iterator&&) -> directory_iterator&;

	auto operator++() -> directory_iterator&;
	auto operator*() const -> directory_entry;
	auto operator==(const directory_iterator&) const -> bool;
	auto operator!=(const directory_iterator&) const -> bool;

};

typedef Path path;
};
