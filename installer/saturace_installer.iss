[Setup]
AppName=SATURACE
AppVersion=1.0
DefaultDirName={commonpf64}\Common Files\VST3
DefaultGroupName=SATURACE
OutputBaseFilename=SATURACE_Setup
Compression=lzma
SolidCompression=yes
OutputDir=Output

[Files]
Source: "SATURACE.vst3\*"; DestDir: "{commonpf64}\Common Files\VST3\SATURACE.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Uninstall SATURACE"; Filename: "{uninstallexe}"
