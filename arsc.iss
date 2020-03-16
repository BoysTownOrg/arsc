[Setup]
AppName=ARSC
AppVerName=BTNRH ARSC_CHK 0.58
AppPublisher=Boys Town Nationial Research Hospital
AppPublisherURL=http://www.boystownhospital.org
AppSupportURL=http://audres.org/rc/arsc/
AppUpdatesURL=http://audres.org/rc/arsc/
DefaultDirName={pf}\BTNRH\ARSC
DefaultGroupName=BTNRH

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; Flags: unchecked

[Files]
Source: "arsc_VS16\Release\arsc_chk.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "no_arsc.reg"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\BTNRH\ARSC_CHK"; Filename: "{app}\arsc_chk.exe"
Name: "{userdesktop}\ARSC_CHK"; Filename: "{app}\arsc_chk.exe"; MinVersion: 4,4; Tasks: desktopicon

[Run]
Filename: "{app}\arsc_chk.exe"; Description: "Launch ARSC_CHK?"; Flags: nowait postinstall skipifsilent

