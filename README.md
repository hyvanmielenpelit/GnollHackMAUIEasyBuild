# GnollHackMAUIEasyBuild
Easily buildable version of GnollHack's .NET MAUI version

## Build instructions
1. Create a local copy of this repository
2. Download sound banks zip from Releases (they are too big to be directly in the repository)
3. Unzip the sound bank zip files
- DesktopSoundBanks.zip should be unzipped to:
    - win\win32\xpl\GnollHackM\Platforms\Windows\banks
- MobileSoundsBanks.zip should be unzipped to both of the following two directories:
    - win\win32\xpl\GnollHackM\Platforms\Android\banks
    - win\win32\xpl\GnollHackM\Platforms\iOS\banks
4. Open the solution file at win\win32\xpl\GnollHackM\GnollHackM.sln in Visual Studio 2022
5. Build the solution on your preferred platform (e.g. Windows); building should now succeed
6. Deploy and run the app; it should deploy and run successfully