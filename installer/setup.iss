#define app_version   "1.2.0"
#define app_name      "VP8 Video For Windows codec"
#define app_copyright "Copyright © Optima SC Inc. 2011"
#define app_webpage   "http://www.optimasc.com/products/vp8vfw/index.html"

;#define x64Build

#if VER < 0x05040200
  #error Update your Inno Setup version
#endif


[Setup]
#ifdef x64Build
AppId={{C67AFB68-825A-4473-AC0A-8DA5BC3D14D3}
UninstallDisplayName={#app_name} {#app_version} (x64)
OutputBaseFilename=vp8vfw-setup-{#app_version}-x64
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
#else
AppId={{0B7FDDF4-23B5-4119-A91A-EC01718DFDC8}
UninstallDisplayName={#app_name} {#app_version}
OutputBaseFilename=vp8vfw-setup-{#app_version}
#endif
AppName={#app_name}
AppVersion={#app_version}
AppVerName={#app_name} {#app_version}
AppCopyright={#app_copyright}
AppContact=info@optimasc.com
AppPublisher=Optima SC, Inc.
AppPublisherURL={#app_webpage}
AppSupportURL={#app_webpage}
AppUpdatesURL={#app_webpage}
VersionInfoCompany=Optima SC Inc.
VersionInfoCopyright={#app_copyright}
VersionInfoDescription={#app_name} {#app_version} Setup
VersionInfoTextVersion={#app_version}
VersionInfoVersion={#app_version}
VersionInfoProductName={#app_name}
VersionInfoProductVersion={#app_version}
VersionInfoProductTextVersion={#app_version}
DefaultDirName={pf}\VP8 VFW
DefaultGroupName=VP8 VFW
InfoBeforeFile=..\bin\readme.txt
OutputDir=.
PrivilegesRequired=admin
AllowNoIcons=yes
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
SolidCompression=yes
Compression=lzma/ultra64
InternalCompressLevel=max
;5.01=XP for MSVC2010 builds, 5.0 for Win2K for MSVC2008 builds
MinVersion=0,5.01


[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"


[Messages]
BeveledLabel={#app_name} v{#app_version}


[Files]
#ifdef x64Build
Source: ..\Release\x64\vp8vfw.dll;   DestDir: {sys}; Flags: sharedfile ignoreversion uninsnosharedfileprompt restartreplace
#else
Source: ..\Release\Win32\vp8vfw.dll; DestDir: {sys}; Flags: sharedfile ignoreversion uninsnosharedfileprompt restartreplace
#endif
Source: ..\bin\readme.txt;           DestDir: {app}; Flags: ignoreversion restartreplace
Source: ..\LICENSE;                  DestDir: {app}; Flags: ignoreversion restartreplace


[INI]
FileName: {win}\system.ini; Section: drivers32; Key: VIDC.VP80; String: vp8vfw.dll; Flags: uninsdeleteentry 


[Registry]
Root: HKLM; SubKey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.VP80; ValueType: string; ValueName: Description;  ValueData: VP8 VFW Video Codec; Flags: uninsdeletevalue
Root: HKLM; SubKey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.VP80; ValueType: string; ValueName: Driver;       ValueData: vp8vfw.dll;          Flags: uninsdeletevalue
Root: HKLM; SubKey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.VP80; ValueType: string; ValueName: FriendlyName; ValueData: VP8;                 Flags: uninsdeletevalue
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc;     ValueType: string; ValueName: vp8vfw.dll;   ValueData: VP8 VFW Video Codec; Flags: uninsdeletevalue
Root: HKLM; SubKey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32;        ValueType: string; ValueName: VIDC.VP80;    ValueData: vp8vfw.dll;          Flags: uninsdeletevalue


;[Icons]
;Name: {group}\Configure VP8 VFW;                 Filename: {syswow64}\rundll32.exe; Parameters: """{syswow64}\vp8vfw.dll"",Configure"; WorkingDir: {syswow64}; Comment: Configure VP8 VFW
;Name: {group}\{cm:UninstallProgram,{#app_name}}; Filename: {uninstallexe}; Comment: {cm:UninstallProgram,{#app_name}}; WorkingDir: {app}
;Name: {group}\{cm:ProgramOnTheWeb,{#app_name}};  Filename: {#app_webpage}


[Run]
#ifdef x64Build
Filename: {sys}\rundll32.exe;      Description: Configure VP8 VFW; Parameters: """{sys}\vp8vfw.dll"",Configure";      WorkingDir: {sys};      Flags: postinstall nowait skipifsilent unchecked
#else
Filename: {syswow64}\rundll32.exe; Description: Configure VP8 VFW; Parameters: """{syswow64}\vp8vfw.dll"",Configure"; WorkingDir: {syswow64}; Flags: postinstall nowait skipifsilent unchecked
#endif
Filename: {#app_webpage};          Description: Visit webpage;     Flags: nowait postinstall skipifsilent shellexec unchecked


[Code]
function IsUpgrade(): Boolean;
var
  sPrevPath: String;
begin
  sPrevPath := WizardForm.PrevAppDir;
  Result := (sPrevPath <> '');
end;


function ShouldSkipPage(PageID: Integer): Boolean;
begin
  if IsUpgrade() then begin
    Case PageID of
      // Hide the license page
      wpInfoBefore: Result := True;
      wpReady: Result := True;
    else
      Result := False;
    end;
  end;
end;


procedure CurPageChanged(CurPageID: Integer);
begin
  if IsUpgrade() and (CurPageID = wpWelcome) then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall)
  else if CurPageID = wpInfoBefore then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall)
  else if CurPageID = wpFinished then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonFinish)
  else if IsUpgrade() then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonNext);
end;


procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  // When uninstalling ask user to delete Lagarith settings
  if CurUninstallStep = usUninstall then begin
    if RegKeyExists(HKEY_CURRENT_USER, 'Software\GNU\vp80') then begin
      if SuppressibleMsgBox('Do you also want to delete VP8 VFW settings? If you plan on reinstalling VP8 VFW you do not have to delete them.',
        mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDYES then begin
         RegDeleteKeyIncludingSubkeys(HKCU, 'Software\GNU\vp80');
         RegDeleteKeyIfEmpty(HKCU, 'Software\GNU');
      end;
    end;
  end;
end;
