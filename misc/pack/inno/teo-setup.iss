; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define OUTPUTDIR  'misc\pack'
#define SOURCEDIR  '..\..\..'
#define APPVERSION  '1.8.1'

[Setup]
AppName=Teow
AppVersion={#APPVERSION}
AppPublisherURL=http://nostalgies.thomsonistes.org/
AppSupportURL=http://nostalgies.thomsonistes.org/
AppUpdatesURL=http://nostalgies.thomsonistes.org/
DefaultDirName={pf}\Teow
DefaultGroupName=Teow
SourceDir={#SOURCEDIR}
UsePreviousAppDir=no
OutputDir={#OUTPUTDIR}
OutputBaseFilename=teo-{#APPVERSION}-setup
Compression=lzma
InternalCompressLevel=ultra
WizardImageFile={#OUTPUTDIR}\inno\teo-big-img.bmp
WizardSmallImageFile={#OUTPUTDIR}\inno\teo-small-img.bmp

[Languages]
Name: en; MessagesFile: compiler:Default.isl
Name: fr; MessagesFile: compiler:Languages\French.isl

[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}
Name: quicklaunchicon; Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}

[Files]
Source: {#OUTPUTDIR}\inno\teow-en.exe; DestDir: {app}; DestName: teow.exe; Languages: en
Source: {#OUTPUTDIR}\inno\teow-fr.exe; DestDir: {app}; DestName: teow.exe; Languages: fr
Source: readme-en.txt; DestDir: {app}; Languages: en; Flags: isreadme
Source: readme-fr.txt; DestDir: {app}; Languages: fr; Flags: isreadme
Source: change-en.log; DestDir: {app}; Languages: en
Source: change-fr.log; DestDir: {app}; Languages: fr
Source: licence-en.txt; DestDir: {app}; Languages: en
Source: licence-fr.txt; DestDir: {app}; Languages: fr
Source: teo.cfg; DestDir: {app}
Source: language.dat; DestDir: {app}
Source: keyboard.dat; DestDir: {app}
Source: alleg40.dll; DestDir: {app}
Source: libpng3.dll; DestDir: {app}
Source: zlib1.dll; DestDir: {app}
Source: b512_b0.rom; DestDir: {app}
Source: b512_b1.rom; DestDir: {app}
Source: basic1.rom; DestDir: {app}
Source: fichier.rom; DestDir: {app}
Source: to8mon1.rom; DestDir: {app}
Source: to8mon2.rom; DestDir: {app}
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: doc\images\home.gif; DestDir: {app}\doc\images
Source: doc\images\logo.jpg; DestDir: {app}\doc\images
Source: doc\images\logoblank.jpg; DestDir: {app}\doc\images
Source: doc\images\teo_w0fr.gif; DestDir: {app}\doc\images; Languages: fr
Source: doc\images\teo_w0en.gif; DestDir: {app}\doc\images; Languages: en
Source: doc\images\teo_w1fr.gif; DestDir: {app}\doc\images; Languages: fr
Source: doc\images\teo_w1en.gif; DestDir: {app}\doc\images; Languages: en
Source: doc\images\teo_w2fr.gif; DestDir: {app}\doc\images; Languages: fr
Source: doc\images\teo_w2en.gif; DestDir: {app}\doc\images; Languages: en
Source: doc\images\teo_w3fr.gif; DestDir: {app}\doc\images; Languages: fr
Source: doc\images\teo_w3en.gif; DestDir: {app}\doc\images; Languages: en
Source: doc\images\teo_w4fr.gif; DestDir: {app}\doc\images; Languages: fr
Source: doc\images\teo_w4en.gif; DestDir: {app}\doc\images; Languages: en
Source: doc\images\teo_w5fr.gif; DestDir: {app}\doc\images; Languages: fr
Source: doc\images\teo_w5en.gif; DestDir: {app}\doc\images; Languages: en
Source: doc\images\teo_v1fr.gif; DestDir: {app}\doc\images; Languages: fr
Source: doc\images\teo_v1en.gif; DestDir: {app}\doc\images; Languages: en
Source: doc\images\teo_v2fr.gif; DestDir: {app}\doc\images; Languages: fr
Source: doc\images\teo_v2en.gif; DestDir: {app}\doc\images; Languages: en
Source: doc\images\teo_v3fr.gif; DestDir: {app}\doc\images; Languages: fr
Source: doc\images\teo_v3en.gif; DestDir: {app}\doc\images; Languages: en
Source: doc\images\teo_v4fr.gif; DestDir: {app}\doc\images; Languages: fr
Source: doc\images\teo_v4en.gif; DestDir: {app}\doc\images; Languages: en
Source: doc\images\teo_v5fr.gif; DestDir: {app}\doc\images; Languages: fr
Source: doc\images\teo_v5en.gif; DestDir: {app}\doc\images; Languages: en
Source: doc\images\teo_v6fr.gif; DestDir: {app}\doc\images; Languages: fr
Source: doc\images\teo_v6en.gif; DestDir: {app}\doc\images; Languages: en
Source: doc\teo_win.htm; DestDir: {app}\doc; Languages: fr
Source: doc\teo_win_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\doc.css; DestDir: {app}\doc

[Icons]
Name: {group}\Teow; Filename: {app}\teow.exe; IconIndex: 0; Parameters: " -window -loadstate"
Name: {group}\Manuel de Teow; Filename: {app}\doc\teo_win.htm; IconIndex: 0; Languages: fr
Name: {group}\Teow's manual; Filename: {app}\doc\teo_win_en.htm; IconIndex: 0; Languages: en
Name: {group}\Licence de Teow; Filename: {app}\licence-fr.txt; IconIndex: 0; Languages: fr
Name: {group}\Teow's licence; Filename: {app}\licence-en.txt; IconIndex: 0; Languages: en
Name: {group}\{cm:ProgramOnTheWeb,Teow}; Filename: http://nostalgies.thomsonistes.org/teo_home.html; IconIndex: 0
Name: {group}\{cm:UninstallProgram,Teow}; Filename: {uninstallexe}; IconIndex: 0
Name: {commondesktop}\Teow {#APPVERSION}; Filename: {app}\teow.exe; Tasks: desktopicon; IconIndex: 0; Parameters: " -window -loadstate"
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\Teow {#APPVERSION}; Filename: {app}\teow.exe; Tasks: quicklaunchicon; IconIndex: 0; Parameters: " -window -loadstate"

[UninstallDelete]
Name: {app}; Type: filesandordirs

[Dirs]
Name: {app}\doc

[InstallDelete]
Name: {group}; Type: filesandordirs
Name: {app}; Type: filesandordirs
Name: {commondesktop}\Teow {#APPVERSION}; Type: files
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\Teow {#APPVERSION}; Type: files
