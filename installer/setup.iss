#define app_version   "1.2.0"
#define app_name      "VP8 Video For Windows codec"
#define app_copyright "Copyright © Optima SC Inc. 2011"
#define app_webpage   "http://www.optimasc.com/products/vp8vfw/index.html"

#if VER < EncodeVer(5,5,1)
  #error Update your Inno Setup version (5.5.1 or newer)
#endif


[Setup]
AppId=vp8vfw
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
UninstallDisplayName={#app_name} {#app_version}
DefaultDirName={pf}\VP8 VFW
DefaultGroupName=VP8 VFW
InfoBeforeFile=..\bin\readme.txt
OutputDir=.
OutputBaseFilename=vp8vfw-setup-{#app_version}
PrivilegesRequired=admin
AllowNoIcons=yes
DisableDirPage=yes
DisableProgramGroupPage=auto
DisableReadyPage=yes
SolidCompression=yes
Compression=lzma/ultra64
InternalCompressLevel=max
MinVersion=5.01sp3
ArchitecturesAllowed=x86 x64
ArchitecturesInstallIn64BitMode=x64


[Languages]
Name: en; MessagesFile: compiler:Default.isl


[Messages]
BeveledLabel={#app_name} v{#app_version}


[Files]
Source: ..\Release\Win32\vp8vfw.dll; DestDir: {sys}; Flags: sharedfile ignoreversion uninsnosharedfileprompt restartreplace 32bit
Source: ..\Release\x64\vp8vfw.dll;   DestDir: {sys}; Flags: sharedfile ignoreversion uninsnosharedfileprompt restartreplace 64bit; Check: Is64BitInstallMode()
Source: ..\bin\readme.txt;           DestDir: {app}; Flags: ignoreversion restartreplace
Source: ..\LICENSE;                  DestDir: {app}; Flags: ignoreversion restartreplace


[INI]
FileName: {win}\system.ini; Section: drivers32; Key: VIDC.VP80; String: vp8vfw.dll; Flags: uninsdeleteentry 


[Registry]
Root: HKLM;   SubKey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.VP80; ValueType: string; ValueName: Description;  ValueData: VP8 VFW Video Codec; Flags: uninsdeletevalue
Root: HKLM;   SubKey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.VP80; ValueType: string; ValueName: Driver;       ValueData: vp8vfw.dll;          Flags: uninsdeletevalue
Root: HKLM;   SubKey: SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.VP80; ValueType: string; ValueName: FriendlyName; ValueData: VP8;                 Flags: uninsdeletevalue
Root: HKLM;   SubKey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc;     ValueType: string; ValueName: vp8vfw.dll;   ValueData: VP8 VFW Video Codec; Flags: uninsdeletevalue
Root: HKLM;   SubKey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32;        ValueType: string; ValueName: VIDC.VP80;    ValueData: vp8vfw.dll;          Flags: uninsdeletevalue
Root: HKLM64; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\drivers.desc;     ValueType: string; ValueName: vp8vfw.dll;   ValueData: VP8 VFW Video Codec; Flags: uninsdeletevalue; Check: Is64BitInstallMode()
Root: HKLM64; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32;        ValueType: string; ValueName: VIDC.VP80;    ValueData: vp8vfw.dll;          Flags: uninsdeletevalue; Check: Is64BitInstallMode()


[Icons]
Name: {group}\Configure VP8 VFW;                 Filename: {sys}\rundll32.exe; Parameters: """{sys}\vp8vfw.dll"",Configure"; WorkingDir: {sys}; Comment: Configure VP8 VFW
Name: {group}\{cm:ProgramOnTheWeb,{#app_name}};  Filename: {#app_webpage}
Name: {group}\{cm:UninstallProgram,{#app_name}}; Filename: {uninstallexe}; Comment: {cm:UninstallProgram,{#app_name}}; WorkingDir: {app}


[Run]
Filename: {sys}\rundll32.exe; Description: Configure VP8 VFW; Parameters: """{sys}\vp8vfw.dll"",Configure"; WorkingDir: {sys}; Flags: postinstall nowait skipifsilent unchecked
Filename: {#app_webpage};     Description: Visit webpage;     Flags: nowait postinstall skipifsilent shellexec unchecked


[UninstallDelete]
Type: dirifempty; Name: {app}


[Code]
function IsUpgrade(): Boolean;
var
  sPrevPath: String;
begin
  sPrevPath := WizardForm.PrevAppDir;
  Result := (sPrevPath <> '');
end;


function IsOldBuildInstalled(iRootKey: Integer; sUnisKey: String): Boolean;
begin
  if RegKeyExists(iRootKey, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\' + sUnisKey) then begin
    Log('Custom Code: The old build is installed');
    Result := True;
  end
  else begin
    Log('Custom Code: The old build is NOT installed');
    Result := False;
  end;
end;


function UninstallOldVersion(iRootKey: Integer; sUnisKey: String): Integer;
var
  iResultCode: Integer;
  sUnInstallString: String;
begin
// Return Values:
// 0 - no idea
// 1 - can't find the registry key (probably no previous version installed)
// 2 - uninstall string is empty
// 3 - error executing the UnInstallString
// 4 - successfully executed the UnInstallString

  // default return value
  Result := 0;

  sUnInstallString := '';

  // Get the uninstall string of the old build
  if RegQueryStringValue(iRootKey, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\' + sUnisKey, 'UninstallString', sUnInstallString) then begin
    if sUnInstallString <> '' then begin
      sUnInstallString := RemoveQuotes(sUnInstallString);
      if Exec(sUnInstallString, '/SILENT /VERYSILENT /NORESTART /SUPPRESSMSGBOXES','', SW_HIDE, ewWaitUntilTerminated, iResultCode) then begin
        Result := 4;
        Sleep(200);
      end else
        Result := 3;
    end else
      Result := 2;
  end else
    Result := 1;
end;


function ShouldSkipPage(PageID: Integer): Boolean;
begin
  // Hide the InfoBefore page if IsUpgrade()
  if IsUpgrade() and (PageID = wpInfoBefore) then
    Result := True;
end;


procedure CurPageChanged(CurPageID: Integer);
begin
  if IsUpgrade() and (CurPageID = wpWelcome) then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall)
  else if CurPageID = wpInfoBefore then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonNext)
  else if CurPageID = wpSelectProgramGroup then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall)
  else if CurPageID = wpFinished then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonFinish)
  else if IsUpgrade() then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonNext);
end;


procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssInstall) then begin
    if IsWin64 then begin
      if IsOldBuildInstalled(HKLM64, '{C67AFB68-825A-4473-AC0A-8DA5BC3D14D3}_is1') then
        UninstallOldVersion(HKLM64, '{C67AFB68-825A-4473-AC0A-8DA5BC3D14D3}_is1');
    end;
    if IsOldBuildInstalled(HKLM32, '{0B7FDDF4-23B5-4119-A91A-EC01718DFDC8}_is1') then
      UninstallOldVersion(HKLM32, '{0B7FDDF4-23B5-4119-A91A-EC01718DFDC8}_is1');
  end;
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
