; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define OUTPUTDIR  'misc\pack'
#define SOURCEDIR  '..\..\..'
#define TEOVERSION  '1.8.2'
#define WEBSITE "https://sourceforge.net/projects/teoemulator/"

[Setup]
AppName=Teow
AppVersion={#TEOVERSION}
AppPublisherURL={#WEBSITE}
AppSupportURL={#WEBSITE}
AppUpdatesURL={#WEBSITE}
DefaultDirName={pf}\Teow
DefaultGroupName=Teow
SourceDir={#SOURCEDIR}
UsePreviousAppDir=no
OutputDir={#OUTPUTDIR}
OutputBaseFilename=teo-{#TEOVERSION}-setup
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
; ------------------------- Executables -------------------------------
Source: {#OUTPUTDIR}\mingw\teow-en.exe; DestDir: {app}; DestName: teow.exe; Languages: en
Source: {#OUTPUTDIR}\mingw\teow-fr.exe; DestDir: {app}; DestName: teow.exe; Languages: fr
Source: {#OUTPUTDIR}\mingw\cc90hfe-en.exe; DestDir: {app}; DestName: cc90hfe.exe; Languages: en
Source: {#OUTPUTDIR}\mingw\cc90hfe-fr.exe; DestDir: {app}; DestName: cc90hfe.exe; Languages: fr
Source: {#OUTPUTDIR}\mingw\cc90hfe-com-en.exe; DestDir: {app}\bin; DestName: cc90hfe.exe; Languages: en
Source: {#OUTPUTDIR}\mingw\cc90hfe-com-fr.exe; DestDir: {app}\bin; DestName: cc90hfe.exe; Languages: fr
Source: {#OUTPUTDIR}\msdos\wav2k7-en.exe; DestDir: {app}; DestName: wav2k7.exe; Languages: en
Source: {#OUTPUTDIR}\msdos\wav2k7-fr.exe; DestDir: {app}; DestName: wav2k7.exe; Languages: fr
Source: {#OUTPUTDIR}\msdos\sap2-en.exe; DestDir: {app}; DestName: sap2.exe; Languages: en
Source: {#OUTPUTDIR}\msdos\sap2-fr.exe; DestDir: {app}; DestName: sap2.exe; Languages: fr
Source: {#OUTPUTDIR}\msdos\sapfs-en.exe; DestDir: {app}; DestName: sapfs.exe; Languages: en
Source: {#OUTPUTDIR}\msdos\sapfs-fr.exe; DestDir: {app}; DestName: sapfs.exe; Languages: fr
; ------------------------- Root files -------------------------------
Source: readme-en.txt; DestDir: {app}; DestName: readme.txt; Languages: en; Flags: isreadme
Source: readme-fr.txt; DestDir: {app}; DestName: readme.txt; Languages: fr; Flags: isreadme
Source: change-en.log; DestDir: {app}; DestName: change.log; Languages: en
Source: change-fr.log; DestDir: {app}; DestName: change.log; Languages: fr
Source: licence-en.txt; DestDir: {app}; DestName: license.txt; Languages: en
Source: licence-fr.txt; DestDir: {app}; DestName: license.txt; Languages: fr
Source: allegro.cfg; DestDir: {app};
Source: language.dat; DestDir: {app}
Source: keyboard.dat; DestDir: {app}
Source: alleg40.dll; DestDir: {app}
Source: libpng3.dll; DestDir: {app}
Source: zlib1.dll; DestDir: {app}
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
; ------------------------- ROMs -------------------------------
Source: system\rom\basic512.rom; DestDir: {app}\system\rom;
Source: system\rom\extramon.rom; DestDir: {app}\system\rom;
Source: system\rom\basic1.rom; DestDir: {app}\system\rom;
Source: system\rom\expl.rom; DestDir: {app}\system\rom;
Source: system\rom\monitor1.rom; DestDir: {app}\system\rom;
Source: system\rom\monitor2.rom; DestDir: {app}\system\rom;
; ------------------------- Printer -------------------------------
Source: system\printer\042\picas.txt; DestDir: {app}\system\printer\042;
Source: system\printer\055\picas.txt; DestDir: {app}\system\printer\055;
Source: system\printer\582\conds.txt; DestDir: {app}\system\printer\582;
Source: system\printer\582\italc.txt; DestDir: {app}\system\printer\582;
Source: system\printer\582\picac.txt; DestDir: {app}\system\printer\582;
Source: system\printer\582\picas.txt; DestDir: {app}\system\printer\582;
Source: system\printer\582\scrpc.txt; DestDir: {app}\system\printer\582;
Source: system\printer\582\table.txt; DestDir: {app}\system\printer\582;
Source: system\printer\600\elitc.txt; DestDir: {app}\system\printer\600;
Source: system\printer\600\italc.txt; DestDir: {app}\system\printer\600;
Source: system\printer\600\itals.txt; DestDir: {app}\system\printer\600;
Source: system\printer\600\picac.txt; DestDir: {app}\system\printer\600;
Source: system\printer\600\picas.txt; DestDir: {app}\system\printer\600;
Source: system\printer\600\scrpc.txt; DestDir: {app}\system\printer\600;
Source: system\printer\612\elitc.txt; DestDir: {app}\system\printer\612;
Source: system\printer\612\italc.txt; DestDir: {app}\system\printer\612;
Source: system\printer\612\itals.txt; DestDir: {app}\system\printer\612;
Source: system\printer\612\picac.txt; DestDir: {app}\system\printer\612;
Source: system\printer\612\picas.txt; DestDir: {app}\system\printer\612;
Source: system\printer\612\scrpc.txt; DestDir: {app}\system\printer\612;
Source: system\printer\612\table.txt; DestDir: {app}\system\printer\612;
; --------------------- Documentation images ----------------------------
Source: doc\images\adapt1.jpg; DestDir: {app}\doc\images;
Source: doc\images\adapt2.jpg; DestDir: {app}\doc\images;
Source: doc\images\adapt3.jpg; DestDir: {app}\doc\images;
Source: doc\images\index1.jpg; DestDir: {app}\doc\images;
Source: doc\images\index2.jpg; DestDir: {app}\doc\images;
Source: doc\images\index3.jpg; DestDir: {app}\doc\images;
Source: doc\images\index4.jpg; DestDir: {app}\doc\images;
Source: doc\images\modif1.jpg; DestDir: {app}\doc\images;
Source: doc\images\modif2.jpg; DestDir: {app}\doc\images;
Source: doc\images\modif3.jpg; DestDir: {app}\doc\images;
Source: doc\images\modif3a.gif; DestDir: {app}\doc\images;
Source: doc\images\modif3b.gif; DestDir: {app}\doc\images;
Source: doc\images\modif4.gif; DestDir: {app}\doc\images;
Source: doc\images\modif5.jpg; DestDir: {app}\doc\images;
Source: doc\images\modif6.jpg; DestDir: {app}\doc\images;
Source: doc\images\modif7.jpg; DestDir: {app}\doc\images;
Source: doc\images\modif8.jpg; DestDir: {app}\doc\images;
Source: doc\images\modif9.gif; DestDir: {app}\doc\images;
Source: doc\images\modif10.jpg; DestDir: {app}\doc\images;
Source: doc\images\modif11.jpg; DestDir: {app}\doc\images;
Source: doc\images\modif12.jpg; DestDir: {app}\doc\images;
Source: doc\images\modif13.jpg; DestDir: {app}\doc\images;
Source: doc\images\thomson1.gif; DestDir: {app}\doc\images;
; --------------------- Documentation html ----------------------------
Source: doc\cc90hfe_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\cc90hfe_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\cc90_thomson_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\cc90_thomson_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\cc90232_adapt_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\cc90232_adapt_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\cc90232_faq_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\cc90232_faq_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\cc90232_modif_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\cc90232_modif_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\doc.css; DestDir: {app}\doc;
Source: doc\libsap_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\libsap_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\licence_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\licence_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\sap2_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\sap2_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\sapfs_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\sapfs_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\teo_changes_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\teo_changes_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\teo_dos_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\teo_dos_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\teo_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\teo_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\teo_linux_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\teo_linux_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\teo_windows_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\teo_windows_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\wav2k7_en.htm; DestDir: {app}\doc; Languages: en
Source: doc\wav2k7_fr.htm; DestDir: {app}\doc; Languages: fr
Source: doc\welcome_fr.htm; DestDir: {app}\doc; Languages: fr; DestName: index.htm;
Source: doc\welcome_en.htm; DestDir: {app}\doc; Languages: en; DestName: index.htm;

[Icons]
Name: {group}\Teow; Filename: {app}\teow.exe; IconIndex: 0; Parameters:
Name: {group}\Cc90hfe; Filename: {app}\cc90hfe.exe; IconIndex: 0; Parameters:
Name: {group}\Teow + reset; Filename: {app}\teow.exe; IconIndex: 0; Parameters: --reset
Name: {group}\Manuel de Teow; Filename: {app}\doc\index.htm; IconIndex: 0; Languages: fr
Name: {group}\Teow's manual; Filename: {app}\doc\index.htm; IconIndex: 0; Languages: en
Name: {group}\Licence de Teow; Filename: {app}\license.txt; IconIndex: 0; Languages: fr
Name: {group}\Teow's licence; Filename: {app}\license.txt; IconIndex: 0; Languages: en
Name: {group}\{cm:ProgramOnTheWeb,Teow}; Filename: {#WEBSITE}; IconIndex: 0
Name: {group}\{cm:UninstallProgram,Teow}; Filename: {uninstallexe}; IconIndex: 0
Name: {commondesktop}\Teow {#TEOVERSION}; Filename: {app}\teow.exe; Tasks: desktopicon; IconIndex: 0; Parameters:
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\Teow {#TEOVERSION}; Filename: {app}\teow.exe; Tasks: quicklaunchicon; IconIndex: 0; Parameters:

[UninstallDelete]
Name: {app}; Type: filesandordirs

[Dirs]
Name: {app}\doc

[InstallDelete]
Name: {group}; Type: filesandordirs
Name: {app}; Type: filesandordirs
Name: {commondesktop}\Teow {#TEOVERSION}; Type: files
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\Teow {#TEOVERSION}; Type: files
