..\Tools\tools\ertbuild.exe ..\Tools\ertbuild.project.ini
..\Tools\tools\ertbuild.exe ..\Tools\ertres.project.ini
..\Tools\tools\ertbuild.exe ..\Utilities\earcmgr.project.ini
..\Tools\tools\ertbuild.exe ..\Utilities\eimgconv.project.ini
..\Tools\tools\ertbuild.exe ..\Utilities\eregconv.project.ini
..\Tools\tools\ertbuild.exe ..\Utilities\estrtab.project.ini
..\Tools\tools\ertbuild.exe bundle_manager\ertbndl.project.ini
mkdir tools_windows
copy ..\Tools\tools\bootstrapper.cpp tools_windows\bootstrapper.cpp
copy ..\Tools\_build\win32\ertbuild.exe tools_windows\ertbuild.exe
copy ..\Tools\_build\win32\ertres.exe tools_windows\ertres.exe
copy ..\Utilities\_build\win32\earcmgr.exe tools_windows\earcmgr.exe
copy ..\Utilities\_build\win32\eimgconv.exe tools_windows\eimgconv.exe
copy ..\Utilities\_build\win32\eregconv.exe tools_windows\eregconv.exe
copy ..\Utilities\_build\win32\estrtab.exe tools_windows\estrtab.exe
copy bundle_manager\_build\win32\ertbndl.exe tools_windows\ertbndl.exe
copy bundle.ecs tools_windows\ertbndl.ecs
tools_windows\earcmgr.exe win_al.ecsal :create :cs win_as.txt
tools_windows\earcmgr.exe win_al.ecsal :arc