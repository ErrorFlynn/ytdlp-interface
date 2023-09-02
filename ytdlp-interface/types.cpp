#include "types.hpp"


std::string version_t::string()
{
	auto fmt = [](int i)
	{
		auto str {std::to_string(i)};
		return str.size() == 1 ? '0' + str : str;
	};
	return std::to_string(year) + '.' + fmt(month) + '.' + fmt(day);
}


bool version_t::operator > (const version_t &o)
{
	auto self {std::chrono::days{day} + std::chrono::months{month} + std::chrono::years{year}};
	auto other {std::chrono::days{o.day} + std::chrono::months{o.month} + std::chrono::years{o.year}};
	return self > other;
}


bool version_t::operator == (const version_t &o)
{
	auto self {std::chrono::days{day} + std::chrono::months{month} + std::chrono::years{year}};
	auto other {std::chrono::days{o.day} + std::chrono::months{o.month} + std::chrono::years{o.year}};
	return self == other;
}


bool version_t::operator != (const version_t &o)
{
	auto self {std::chrono::days{day} + std::chrono::months{month} + std::chrono::years{year}};
	auto other {std::chrono::days{o.day} + std::chrono::months{o.month} + std::chrono::years{o.year}};
	return self != other;
}


semver_t::semver_t(std::string ver_tag)
{
	if(!ver_tag.empty())
	{
		std::string strmajor, strminor, strpatch;
		if(ver_tag[0] == 'v')
			ver_tag.erase(0, 1);
		auto pos {ver_tag.find('.')};
		if(pos != -1)
		{
			strmajor = ver_tag.substr(0, pos++);
			auto pos2 {ver_tag.find('.', pos)};
			if(pos2 != -1)
			{
				strminor = ver_tag.substr(pos, pos2++ - pos);
				strpatch = ver_tag.substr(pos2);
			}
		}

		try
		{
			major = std::stoi(strmajor);
			minor = std::stoi(strminor);
			patch = std::stoi(strpatch);
		}
		catch(...) {}
	}
}


bool semver_t::operator > (const semver_t &o)
{
	return major > o.major || major == o.major && minor > o.minor || major == o.major && minor == o.minor && patch > o.patch;
}


bool semver_t::operator < (const semver_t &o)
{
	return major < o.major || major == o.major && minor < o.minor || major == o.major && minor == o.minor && patch < o.patch;
}


bool semver_t::operator == (const semver_t &o)
{
	return major == o.major && minor == o.minor && patch == o.patch;
}


void favicon_t::add(std::string favicon_url, callback fn)
{
	if(img.empty())
	{
		std::lock_guard<std::mutex> lock {mtx};
		callbacks.push_back(std::move(fn));
		if(!thr.joinable())
			thr = std::thread {[favicon_url, this]
			{
				working = true;
				if(!favicon_url.empty())
				{
					std::string res, error;
					res = util::get_inet_res(favicon_url, &error);
					if(working)
					{
						std::lock_guard<std::mutex> lock {mtx};
						if(!img.open(res.data(), res.size()))
							img.open(arr_url22_png, sizeof arr_url22_png);
						while(!callbacks.empty())
						{
							callbacks.back()(img);
							callbacks.pop_back();
						}
						working = false;
						if(thr.joinable())
							thr.detach();
					}
				}
			}};
	}
	else fn(img);
}


favicon_t::~favicon_t()
{
	if(thr.joinable())
	{
		working = false;
		thr.join();
	}
}