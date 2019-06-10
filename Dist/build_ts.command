cd Documents/GitHub/VirtualUI/Dist
../Tools/tools/ertbuild ../Tools/ertbuild.project.ini
../Tools/tools/ertbuild ../Tools/ertres.project.ini
../Tools/tools/ertbuild ../Utilities/earcmgr.project.ini
../Tools/tools/ertbuild ../Utilities/eimgconv.project.ini
../Tools/tools/ertbuild ../Utilities/eregconv.project.ini
../Tools/tools/ertbuild ../Utilities/estrtab.project.ini
../Tools/tools/ertbuild bundle_manager/ertbndl.project.ini
mkdir tools_macosx
cp ../Tools/tools/bootstrapper.cpp tools_macosx/bootstrapper.cpp
cp ../Tools/_build/macosx/ertbuild tools_macosx/ertbuild
cp ../Tools/_build/macosx/ertres tools_macosx/ertres
cp ../Utilities/_build/macosx/earcmgr tools_macosx/earcmgr
cp ../Utilities/_build/macosx/eimgconv tools_macosx/eimgconv
cp ../Utilities/_build/macosx/eregconv tools_macosx/eregconv
cp ../Utilities/_build/macosx/estrtab tools_macosx/estrtab
cp bundle_manager/_build/macosx/ertbndl tools_macosx/ertbndl
cp bundle.ecs tools_macosx/ertbndl.ecs
tools_macosx/earcmgr mac_al.ecsal :create :cs mac_as.txt
tools_macosx/earcmgr mac_al.ecsal :arc