..\Tools\tools\ertbuild.exe ..\Tools\ertbuild.project.ini
..\Tools\tools\ertbuild.exe ..\Tools\ertres.project.ini
..\Tools\tools\ertbuild.exe ..\Utilities\earcmgr.project.ini
..\Tools\tools\ertbuild.exe ..\Utilities\eimgconv.project.ini
..\Tools\tools\ertbuild.exe ..\Utilities\eregconv.project.ini
..\Tools\tools\ertbuild.exe ..\Utilities\estrtab.project.ini
..\Tools\tools\ertbuild.exe bundle_manager\ertbndl.project.ini
..\Tools\tools\ertbuild.exe auto_config\ertaconf.project.ini
..\Tools\tools\ertbuild.exe ..\Tools\ertbuild.project.ini :x64
..\Tools\tools\ertbuild.exe ..\Tools\ertres.project.ini :x64
..\Tools\tools\ertbuild.exe ..\Utilities\earcmgr.project.ini :x64
..\Tools\tools\ertbuild.exe ..\Utilities\eimgconv.project.ini :x64
..\Tools\tools\ertbuild.exe ..\Utilities\eregconv.project.ini :x64
..\Tools\tools\ertbuild.exe ..\Utilities\estrtab.project.ini :x64
..\Tools\tools\ertbuild.exe bundle_manager\ertbndl.project.ini :x64
..\Tools\tools\ertbuild.exe auto_config\ertaconf.project.ini :x64
mkdir tools_windows
mkdir tools_windows_64
copy ..\Tools\tools\bootstrapper.cpp tools_windows\bootstrapper.cpp
copy ..\Tools\_build\win32\ertbuild.exe tools_windows\ertbuild.exe
copy ..\Tools\_build\win32\ertres.exe tools_windows\ertres.exe
copy ..\Utilities\_build\win32\earcmgr.exe tools_windows\earcmgr.exe
copy ..\Utilities\_build\win32\eimgconv.exe tools_windows\eimgconv.exe
copy ..\Utilities\_build\win32\eregconv.exe tools_windows\eregconv.exe
copy ..\Utilities\_build\win32\estrtab.exe tools_windows\estrtab.exe
copy bundle_manager\_build\win32\ertbndl.exe tools_windows\ertbndl.exe
copy auto_config\_build\win32\ertaconf.exe tools_windows\ertaconf.exe
copy bundle.ecs tools_windows\ertbndl.ecs
copy ..\Tools\tools\bootstrapper.cpp tools_windows_64\bootstrapper.cpp
copy ..\Tools\_build\win64\ertbuild.exe tools_windows_64\ertbuild.exe
copy ..\Tools\_build\win64\ertres.exe tools_windows_64\ertres.exe
copy ..\Utilities\_build\win64\earcmgr.exe tools_windows_64\earcmgr.exe
copy ..\Utilities\_build\win64\eimgconv.exe tools_windows_64\eimgconv.exe
copy ..\Utilities\_build\win64\eregconv.exe tools_windows_64\eregconv.exe
copy ..\Utilities\_build\win64\estrtab.exe tools_windows_64\estrtab.exe
copy bundle_manager\_build\win64\ertbndl.exe tools_windows_64\ertbndl.exe
copy auto_config\_build\win64\ertaconf.exe tools_windows_64\ertaconf.exe
copy bundle.ecs tools_windows_64\ertbndl.ecs
tools_windows\earcmgr.exe win_al.ecsal :create :cs win_as.txt
tools_windows\earcmgr.exe win_al.ecsal :arc
tools_windows_64\earcmgr.exe win_al_64.ecsal :create :cs win_as_64.txt
tools_windows_64\earcmgr.exe win_al_64.ecsal :arc