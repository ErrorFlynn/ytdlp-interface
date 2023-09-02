#include <chrono>
#include <nana/gui.hpp>
#include "util.hpp"
#include "icons.hpp"

struct version_t
{
	int year {0}, month {0}, day {0};
	bool empty() { return year == 0; }
	std::string string();
	bool operator > (const version_t &o);
	bool operator == (const version_t &o);
	bool operator != (const version_t &o);
};


struct semver_t
{
	int major {0}, minor {0}, patch {0};
	semver_t() = default;
	semver_t(int maj, int min, int pat) : major {maj}, minor {min}, patch {pat} {}
	semver_t(std::string ver_tag);

	bool operator > (const semver_t &o);
	bool operator < (const semver_t &o);
	bool operator == (const semver_t &o);
};


struct favicon_t
{
	using image = nana::paint::image;
	using callback = std::function<void(image &)>;

	favicon_t() = default;
	~favicon_t();
	void add(std::string favicon_url, callback fn);
	operator const image &() const { return img; }

private:
	image img;
	std::thread thr;
	std::mutex mtx;
	bool working {false};
	std::vector<callback> callbacks;
};