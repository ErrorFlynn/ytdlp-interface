# ytdlp-interface
This is a Windows graphical interface for [yt-dlp](https://github.com/yt-dlp/yt-dlp), that is designed as a simple YouTube downloader. Since v1.2, the interface also accepts non-YouTube URLs, so theoretically it can be used to download from any site that `yt-dlp` supports (see [the list](https://github.com/yt-dlp/yt-dlp/blob/master/supportedsites.md)).

To use, unpack the archive in a new folder at a location of your choice, and run `ytdlp-interface.exe`.

Download link for the latest version (64 bit): https://github.com/ErrorFlynn/ytdlp-interface/releases/download/v2.18.0/ytdlp-interface.7z

32 bit build: https://github.com/ErrorFlynn/ytdlp-interface/releases/download/v2.18.0/ytdlp-interface_x86.7z

Windows 7 64 bit: https://github.com/ErrorFlynn/ytdlp-interface/releases/download/v2.18.0/ytdlp-interface_win7.7z

Windows 7 32 bit: https://github.com/ErrorFlynn/ytdlp-interface/releases/download/v2.18.0/ytdlp-interface_x86_win7.7z


---

## Building the source
The project depends on three static libraries: [Nana C++ GUI library](https://github.com/cnjinhao/nana) v1.8 or later (at the time I'm writing this v1.8 is in development, so you must build branch `develop-1.8`), [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo), and [bit7z](https://github.com/rikyoz/bit7z).

The project uses a modified version of the Nana library (the file `nana-develop-1.8 (2024-05-06) ytdlp-interface mod.7z`). You can still link against the original library, but the modified version has features and behaviors not present in the original (as of June 2024). Most importantly, the modified library ensures that all interface elements follow the chosen color scheme, and that most interface elements scale properly with the system scale factor.

The program also uses [JSON for modern C++](https://github.com/nlohmann/json) to get video info from `yt-dlp.exe` and to read/write the settings file, but that's just a header file that's included in the project (you can replace it with its latest version if you really want to).

---

<img width="822" height="682" alt="settings" src="https://github.com/user-attachments/assets/2bf18ef3-e3d7-4e4c-9641-01419e21a6aa" />

---

<img width="1002" height="735" alt="queue" src="https://github.com/user-attachments/assets/215bb101-fa44-4817-8fcb-2fc0e32c883e" />

---

<img width="1002" height="735" alt="output" src="https://github.com/user-attachments/assets/95fd8a51-9c1f-47c1-89fb-b07d6d4dcb60" />
