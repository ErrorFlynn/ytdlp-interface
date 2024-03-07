# ytdlp-interface
This is a Windows graphical interface for [yt-dlp](https://github.com/yt-dlp/yt-dlp), that is designed as a simple YouTube downloader. Since v1.2, the interface also accepts non-YouTube URLs, so theoretically it can be used to download from any site that `yt-dlp` supports (see [the list](https://github.com/yt-dlp/yt-dlp/blob/master/supportedsites.md)).

To use, unpack the archive in a new folder at a location of your choice, and run `ytdlp-interface.exe`.

Download link for the latest version (64 bit): https://github.com/ErrorFlynn/ytdlp-interface/releases/download/v2.9.0/ytdlp-interface.7z

32 bit build (works on Win7): https://github.com/ErrorFlynn/ytdlp-interface/releases/download/v2.9.0/ytdlp-interface_x86.7z


---

## Building the source
The project depends on three static libraries: [Nana C++ GUI library](https://github.com/cnjinhao/nana) v1.8 or later (at the time I'm writing this v1.8 is in development, so you must build branch `develop-1.8`), [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo), and [bit7z](https://github.com/rikyoz/bit7z). To build Nana with JPEG support see [this thread](http://nanapro.org/en-us/forum/index.php?u=/topic/1368/ggjpg).

The program also uses [JSON for modern C++](https://github.com/nlohmann/json) to get video info from `yt-dlp.exe` and to read/write the settings file, but that's just a header file that's included in the project (you can replace it with its latest version if you really want to).

The easiest way to build the project is to open the solution file with Visual Studio 2019 or later. If that's not an option, you're a resourceful individual, I'm sure you'll come up with something (that's as far as my support goes, sorry).

---

![ytdlp-interface_settings](https://github.com/ErrorFlynn/ytdlp-interface/assets/20293505/2113ce06-375f-498d-bc3d-555d907db15c)

---

![ytdlp-interface_queue](https://github.com/ErrorFlynn/ytdlp-interface/assets/20293505/bcd25267-e29a-4115-9271-7b3519c49917)

---

![ytdlp-interface_output](https://github.com/ErrorFlynn/ytdlp-interface/assets/20293505/3b9b5b37-2e78-4d25-b20a-76b0a7246167)
