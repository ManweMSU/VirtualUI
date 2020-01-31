#include <EngineRuntime.h>

using namespace Engine;
using namespace Engine::Streaming;
using namespace Engine::Storage;
using namespace Engine::Codec;

struct VersionInfo
{
    bool UseDefines;
    string AppName;
    string CompanyName;
    string Copyright;
    string InternalName;
    uint32 VersionMajor;
    uint32 VersionMinor;
    uint32 Subversion;
    uint32 Build;
    string AppIdent;
    string ComIdent;
    string Description;
};
struct Resource
{
    string SourcePath;
    string Name;
    string Locale;
};
struct FileFormat
{
    string Extension;
    string Description;
    string Icon;
	int IconIndex;
	int UniqueIndex;
    bool CanCreate;
	bool UniqueIcon;
};

handle error_output;
string target_system;
SafePointer<RegistryNode> sys_cfg;
SafePointer<RegistryNode> prj_cfg;
Time prj_time;
VersionInfo prj_ver;
string bundle_path;
Array<Resource> reslist(0x10);
Array<FileFormat> formats(0x10);
bool clean = false;
bool errlog = false;

void print_error(handle from)
{
    FileStream From(from);
    From.Seek(0, Begin);
    TextReader FromReader(&From);
    IO::Console ToWriter(error_output);
    ToWriter.Write(FromReader.ReadAll());
}
void try_create_directory(const string & path)
{
    try { IO::CreateDirectory(path); } catch (...) {}
}
void clear_directory(const string & path)
{
    try {
        SafePointer< Array<string> > files = IO::Search::GetFiles(path + L"/*");
        for (int i = 0; i < files->Length(); i++) IO::RemoveFile(path + L"/" + files->ElementAt(i));
        SafePointer< Array<string> > dirs = IO::Search::GetDirectories(path + L"/*");
        for (int i = 0; i < dirs->Length(); i++) {
            string p = path + L"/" + dirs->ElementAt(i);
            clear_directory(p);
            IO::RemoveDirectory(p);
        }
    }
    catch (...) {}
}
void copy_file(const string & source, const string & dest)
{
    FileStream src(source, AccessRead, OpenExisting);
    FileStream out(dest, AccessWrite, CreateAlways);
    src.CopyTo(&out);
}
bool copy_file_nothrow(const string & source, const string & dest)
{
    try {
        FileStream src(source, AccessRead, OpenExisting);
        FileStream out(dest, AccessWrite, CreateAlways);
        src.CopyTo(&out);
    }
    catch (...) { return false; }
    return true;
}
bool validate_resources(ITextWriter & console)
{
    for (int i = 0; i < reslist.Length(); i++) {
        for (int j = 0; j < reslist[i].Name.Length(); j++) {
            widechar c = reslist[i].Name[j];
            if ((c < L'a' || c > L'z') && (c < L'A' || c > L'Z') && (c < L'0' || c > L'9') && c != L'_') {
                console << L"Invalid resource name: " << reslist[i].Name << IO::NewLineChar;
                return false;
            }
        }
    }
    return true;
}
string make_lexem(const string & str) { return str.Replace(L'\\', L"\\\\").Replace(L'\"', L"\\\""); }
bool compile_resource(const string & rc, const string & res, const string & log, ITextWriter & console)
{
    try {
        Time src_time = 0;
        Time out_time = 0;
        try {
            FileStream src(rc, AccessRead, OpenExisting);
            FileStream out(res, AccessRead, OpenExisting);
            src_time = IO::DateTime::GetFileAlterTime(src.Handle());
            out_time = IO::DateTime::GetFileAlterTime(out.Handle());
        }
        catch (...) {}
        if (src_time < out_time && !clean) return true;
        console << L"Compiling resource script " << IO::Path::GetFileName(rc) << L"...";
        Array<string> rc_args(0x80);
        {
            string out_arg = sys_cfg->GetValueString(L"Compiler/OutputArgument");
            if (out_arg.FindFirst(L'$') == -1) {
                rc_args << out_arg;
                rc_args << res;
            } else rc_args << out_arg.Replace(L'$', res);
        }
        {
            SafePointer<RegistryNode> args_node = sys_cfg->OpenNode(L"Compiler/Arguments");
            if (args_node) {
                auto & args_vals = args_node->GetValues();
                for (int i = 0; i < args_vals.Length(); i++) rc_args << args_node->GetValueString(args_vals[i]);
            }
        }
        rc_args << rc;
        handle rc_log = IO::CreateFile(log, IO::AccessReadWrite, IO::CreateAlways);
        IO::SetStandardOutput(rc_log);
        IO::SetStandardError(rc_log);
        IO::CloseFile(rc_log);
        SafePointer<Process> compiler = CreateCommandProcess(sys_cfg->GetValueString(L"Compiler/Path"), &rc_args);
        if (!compiler) {
            console << L"Failed" << IO::NewLineChar;
            console << L"Failed to launch the compiler (" + sys_cfg->GetValueString(L"Compiler/Path") + L")." << IO::NewLineChar;
            return false;
        }
        compiler->Wait();
        if (compiler->GetExitCode()) {
            console << L"Failed" << IO::NewLineChar;
            if (errlog) print_error(IO::GetStandardError());
            else Shell::OpenFile(log);
            return false;
        }
        console << L"Succeed" << IO::NewLineChar;
    }
    catch (...) { return false; }
    return true;
}
bool asm_icon(const string & source, const string & output, ITextWriter & console)
{
    try {
        Time src_time = 0;
        Time out_time = 0;
        try {
            FileStream src(source, AccessRead, OpenExisting);
            FileStream date(output, AccessRead, OpenExisting);
            src_time = IO::DateTime::GetFileAlterTime(src.Handle());
            out_time = IO::DateTime::GetFileAlterTime(date.Handle());
        }
        catch (...) {}
        if (src_time < out_time && !clean) return true;
        console << L"Converting icon file " << IO::Path::GetFileName(source) << L"...";
        try {
            FileStream src(source, AccessRead, OpenExisting);
            SafePointer<Image> icon = DecodeImage(&src);
            Array<int> sizes(0x10);
            SafePointer<RegistryNode> sizes_node = sys_cfg->OpenNode(L"IconFormats");
            if (sizes_node) {
                auto & names = sizes_node->GetValues();
                for (int i = 0; i < names.Length(); i++) sizes << sizes_node->GetValueInteger(names[i]);
            }
            for (int i = icon->Frames.Length() - 1; i >= 0; i--) {
                bool match = false;
                for (int j = 0; j < sizes.Length(); j++) {
                    if (icon->Frames[i].GetWidth() == sizes[j] && icon->Frames[i].GetHeight() == sizes[j]) {
                        match = true;
                        break;
                    }
                }
                if (!match) icon->Frames.Remove(i);
            }
            if (icon->Frames.Length()) {
                FileStream out(output, AccessReadWrite, CreateAlways);
                EncodeImage(&out, icon, sys_cfg->GetValueString(L"IconCodec"));
            }
        }
        catch (...) { console << L"Failed" << IO::NewLineChar; throw; }
        console << L"Succeed" << IO::NewLineChar;
    }
    catch (...) { return false; }
    return true;
}
bool asm_resscript(const string & manifest, const Array<string> & icons, const string & app_icon, const string & rc, bool is_lib, ITextWriter & console)
{
    try {
        Time max_time = 0;
        Time rc_time = 0;
        string wd = IO::Path::GetDirectory(rc) + L"/";
        try {
            FileStream date(wd + manifest, AccessRead, OpenExisting);
            FileStream rcs(wd + rc, AccessRead, OpenExisting);
            max_time = IO::DateTime::GetFileAlterTime(date.Handle());
            for (int i = 0; i < icons.Length(); i++) {
                FileStream icon(wd + icons[i], AccessRead, OpenExisting);
                Time it = IO::DateTime::GetFileAlterTime(icon.Handle());
                if (it > max_time) max_time = it;
            }
            for (int i = 0; i < reslist.Length(); i++) {
                FileStream res(reslist[i].SourcePath, AccessRead, OpenExisting);
                Time rt = IO::DateTime::GetFileAlterTime(res.Handle());
                if (rt > max_time) max_time = rt;
            }
            if (prj_time > max_time) max_time = prj_time;
            rc_time = IO::DateTime::GetFileAlterTime(rcs.Handle());
        }
        catch (...) {}
        if (max_time < rc_time && !clean) return true;
        console << L"Writing resource script file " << IO::Path::GetFileName(rc) << L"...";
        try {
            FileStream rc_file(rc, AccessWrite, CreateAlways);
            TextWriter script(&rc_file, Encoding::UTF16);
            script.WriteEncodingSignature();
            script << L"#include <Windows.h>" << IO::NewLineChar << IO::NewLineChar;
            script << L"CREATEPROCESS_MANIFEST_RESOURCE_ID RT_MANIFEST \"" + manifest + L"\"" << IO::NewLineChar << IO::NewLineChar;
			if (app_icon.Length()) {
				script << string(1) + L" ICON \"" + IO::Path::GetFileName(app_icon) + L"\"" << IO::NewLineChar;
			}
			for (int i = 0; i < formats.Length(); i++) if (formats[i].UniqueIcon) {
                script << string(formats[i].UniqueIndex + 1) + L" ICON \"" + IO::Path::GetFileName(formats[i].Icon) + L"\"" << IO::NewLineChar;
			}
            if (icons.Length()) script << IO::NewLineChar;
            if (reslist.Length()) {
                for (int i = 0; i < reslist.Length(); i++) {
                    string inner_name = reslist[i].Locale.Length() ? (reslist[i].Name + L"-" + reslist[i].Locale) : reslist[i].Name;
                    script << L"" + inner_name + L" RCDATA \"" + make_lexem(reslist[i].SourcePath) + L"\"" << IO::NewLineChar;
                }
                script << IO::NewLineChar;
            }
            if (prj_ver.AppName.Length()) {
                script << L"1 VERSIONINFO" << IO::NewLineChar;
                script << L"FILEVERSION " + string(prj_ver.VersionMajor) + L", " + string(prj_ver.VersionMinor) + L", " +
                    string(prj_ver.Subversion) + L", " + string(prj_ver.Build) << IO::NewLineChar;
                script << L"PRODUCTVERSION " + string(prj_ver.VersionMajor) + L", " + string(prj_ver.VersionMinor) + L", " +
                    string(prj_ver.Subversion) + L", " + string(prj_ver.Build) << IO::NewLineChar;
                script << L"FILEFLAGSMASK 0x3fL" << IO::NewLineChar;
                script << L"FILEFLAGS 0x0L" << IO::NewLineChar;
                script << L"FILEOS VOS_NT_WINDOWS32" << IO::NewLineChar;
				if (is_lib) {
					script << L"FILETYPE VFT_DLL" << IO::NewLineChar;
				} else {
                	script << L"FILETYPE VFT_APP" << IO::NewLineChar;
				}
                script << L"FILESUBTYPE VFT2_UNKNOWN" << IO::NewLineChar;
                script << L"BEGIN" << IO::NewLineChar;
                script << L"\tBLOCK \"StringFileInfo\"" << IO::NewLineChar;
                script << L"\tBEGIN" << IO::NewLineChar;
                script << L"\t\tBLOCK \"040004b0\"" << IO::NewLineChar;
                script << L"\t\tBEGIN" << IO::NewLineChar;
                if (prj_ver.CompanyName.Length()) script << L"\t\t\tVALUE \"CompanyName\", \"" + make_lexem(prj_ver.CompanyName) + L"\"" << IO::NewLineChar;
                script << L"\t\t\tVALUE \"FileDescription\", \"" + make_lexem(prj_ver.AppName) + L"\"" << IO::NewLineChar;
                script << L"\t\t\tVALUE \"FileVersion\", \"" + string(prj_ver.VersionMajor) + L"." + string(prj_ver.VersionMinor) + L"\"" << IO::NewLineChar;
                script << L"\t\t\tVALUE \"InternalName\", \"" + make_lexem(prj_ver.InternalName) + L"\"" << IO::NewLineChar;
                if (prj_ver.Copyright.Length()) script << L"\t\t\tVALUE \"LegalCopyright\", \"" + make_lexem(prj_ver.Copyright) + L"\"" << IO::NewLineChar;
                script << L"\t\t\tVALUE \"OriginalFilename\", \"" + make_lexem(prj_ver.InternalName) + L".exe\"" << IO::NewLineChar;
                script << L"\t\t\tVALUE \"ProductName\", \"" + make_lexem(prj_ver.AppName) + L"\"" << IO::NewLineChar;
                script << L"\t\t\tVALUE \"ProductVersion\", \"" + string(prj_ver.VersionMajor) + L"." + string(prj_ver.VersionMinor) + L"\"" << IO::NewLineChar;
                script << L"\t\tEND" << IO::NewLineChar;
                script << L"\tEND" << IO::NewLineChar;
                script << L"\tBLOCK \"VarFileInfo\"" << IO::NewLineChar;
                script << L"\tBEGIN" << IO::NewLineChar;
                script << L"\t\tVALUE \"Translation\", 0x400, 1200" << IO::NewLineChar;
                script << L"\tEND" << IO::NewLineChar;
                script << L"END";
            }
        }
        catch (...) { console << L"Failed" << IO::NewLineChar; throw; }
        console << L"Succeed" << IO::NewLineChar;
    }
    catch (...) { return false; }
    return true;
}
bool asm_manifest(const string & output, ITextWriter & console)
{
    try {
        Time man_time = 0;
        try {
            FileStream date(output, AccessRead, OpenExisting);
            man_time = IO::DateTime::GetFileAlterTime(date.Handle());
        }
        catch (...) {}
        if (prj_time < man_time && !clean) return true;
        console << L"Writing manifest file " << IO::Path::GetFileName(output) << L"...";
        try {
            FileStream man_file(output, AccessWrite, CreateAlways);
            TextWriter manifest(&man_file, Encoding::UTF8);
            manifest << L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" << IO::NewLineChar;
            manifest << L"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" << IO::NewLineChar;
            manifest << L"<assemblyIdentity version=\"" +
                string(prj_ver.VersionMajor) + L"." + string(prj_ver.VersionMinor) + L"." + string(prj_ver.Subversion) + L"." + string(prj_ver.Build) +
                L"\" processorArchitecture=\"*\" name=\"" +
                prj_ver.ComIdent + L"." + prj_ver.AppIdent +
                L"\" type=\"win32\"/>" << IO::NewLineChar;
            manifest << L"<description>" + prj_ver.Description + L"</description>" << IO::NewLineChar;
            // Common Controls 6.0 support
            manifest << L"<dependency><dependentAssembly>" << IO::NewLineChar;
            manifest << L"\t<assemblyIdentity type=\"win32\" name=\"Microsoft.Windows.Common-Controls\" version=\"6.0.0.0\" processorArchitecture=\"*\" "
                L"publicKeyToken=\"6595b64144ccf1df\" language=\"*\"/>" << IO::NewLineChar;
            manifest << L"</dependentAssembly></dependency>" << IO::NewLineChar;
            // Make application DPI-aware, enable very long paths
            manifest << L"<application xmlns=\"urn:schemas-microsoft-com:asm.v3\">" << IO::NewLineChar;
            manifest << L"\t<windowsSettings>" << IO::NewLineChar;
            manifest << L"\t\t<dpiAware xmlns=\"http://schemas.microsoft.com/SMI/2005/WindowsSettings\">true</dpiAware>" << IO::NewLineChar;
            manifest << L"\t</windowsSettings>" << IO::NewLineChar;
            manifest << L"\t<windowsSettings xmlns:ws2=\"http://schemas.microsoft.com/SMI/2016/WindowsSettings\">" << IO::NewLineChar;
            manifest << L"\t\t<ws2:longPathAware>true</ws2:longPathAware>" << IO::NewLineChar;
            manifest << L"\t</windowsSettings>" << IO::NewLineChar;
            manifest << L"</application>" << IO::NewLineChar;
            // UAC Execution level
            manifest << L"<trustInfo xmlns=\"urn:schemas-microsoft-com:asm.v2\"><security><requestedPrivileges>" << IO::NewLineChar;
            manifest << L"\t<requestedExecutionLevel level=\"asInvoker\" uiAccess=\"FALSE\"></requestedExecutionLevel>" << IO::NewLineChar;
            manifest << L"</requestedPrivileges></security></trustInfo>" << IO::NewLineChar;
            // Finilize
            manifest << L"</assembly>";
        }
        catch (...) { console << L"Failed" << IO::NewLineChar; throw; }
        console << L"Succeed" << IO::NewLineChar;
    }
    catch (...) { return false; }
    return true;
}
bool asm_plist(const string & output, bool with_icon, ITextWriter & console)
{
    try {
        Time list_time = 0;
        try {
            FileStream date(output, AccessRead, OpenExisting);
            list_time = IO::DateTime::GetFileAlterTime(date.Handle());
        }
        catch (...) {}
        if (prj_time < list_time && !clean) return true;
        console << L"Writing bundle information file " << IO::Path::GetFileName(output) << L"...";
        try {
            FileStream list_file(output, AccessWrite, CreateAlways);
            TextWriter list(&list_file, Encoding::UTF8);
            list << L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << IO::NewLineChar;
            list << L"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">" << IO::NewLineChar;
            list << L"<plist version=\"1.0\">" << IO::NewLineChar;
            list << L"<dict>" << IO::NewLineChar;
            if (prj_ver.AppName.Length()) {
                list << L"\t<key>CFBundleName</key>" << IO::NewLineChar;
                list << L"\t<string>" << prj_ver.AppName << L"</string>" << IO::NewLineChar;
                list << L"\t<key>CFBundleDisplayName</key>" << IO::NewLineChar;
                list << L"\t<string>" << prj_ver.AppName << L"</string>" << IO::NewLineChar;
            } else {
                list << L"\t<key>CFBundleName</key>" << IO::NewLineChar;
                list << L"\t<string>" << prj_ver.InternalName << L"</string>" << IO::NewLineChar;
                list << L"\t<key>CFBundleDisplayName</key>" << IO::NewLineChar;
                list << L"\t<string>" << prj_ver.InternalName << L"</string>" << IO::NewLineChar;
            }
            list << L"\t<key>CFBundleIdentifier</key>" << IO::NewLineChar;
            list << L"\t<string>com." << prj_ver.ComIdent << L"." << prj_ver.AppIdent << L"</string>" << IO::NewLineChar;
            list << L"\t<key>CFBundleVersion</key>" << IO::NewLineChar;
            list << L"\t<string>" << prj_ver.VersionMajor << L"." << prj_ver.VersionMinor << L"." <<
                prj_ver.Subversion << L"." << prj_ver.Build << L"</string>" << IO::NewLineChar;
            list << L"\t<key>CFBundleDevelopmentRegion</key>" << IO::NewLineChar;
            list << L"\t<string>en</string>" << IO::NewLineChar;
            list << L"\t<key>CFBundlePackageType</key>" << IO::NewLineChar;
            list << L"\t<string>APPL</string>" << IO::NewLineChar;
            list << L"\t<key>CFBundleExecutable</key>" << IO::NewLineChar;
            list << L"\t<string>" << prj_ver.InternalName << L"</string>" << IO::NewLineChar;
            list << L"\t<key>CFBundleShortVersionString</key>" << IO::NewLineChar;
            list << L"\t<string>" << prj_ver.VersionMajor << L"." << prj_ver.VersionMinor << L"</string>" << IO::NewLineChar;
            if (with_icon) {
                list << L"\t<key>CFBundleIconFile</key>" << IO::NewLineChar;
                list << L"\t<string>AppIcon</string>" << IO::NewLineChar;
            }
            list << L"\t<key>NSPrincipalClass</key>" << IO::NewLineChar;
            list << L"\t<string>NSApplication</string>" << IO::NewLineChar;
            list << L"\t<key>CFBundleInfoDictionaryVersion</key>" << IO::NewLineChar;
            list << L"\t<string>6.0</string>" << IO::NewLineChar;
            list << L"\t<key>NSHumanReadableCopyright</key>" << IO::NewLineChar;
            list << L"\t<string>" << prj_ver.Copyright << L"</string>" << IO::NewLineChar;
            list << L"\t<key>NSHighResolutionMagnifyAllowed</key>" << IO::NewLineChar;
            list << L"\t<false/>" << IO::NewLineChar;
            list << L"\t<key>LSMinimumSystemVersion</key>" << IO::NewLineChar;
            list << L"\t<string>10.10</string>" << IO::NewLineChar;
            list << L"\t<key>NSHighResolutionCapable</key>" << IO::NewLineChar;
            list << L"\t<true/>" << IO::NewLineChar;
            list << L"\t<key>CFBundleSupportedPlatforms</key>" << IO::NewLineChar;
            list << L"\t<array>" << IO::NewLineChar;
            list << L"\t\t<string>MacOSX</string>" << IO::NewLineChar;
            list << L"\t</array>" << IO::NewLineChar;
            if (formats.Length()) {
                list << L"\t<key>CFBundleDocumentTypes</key>" << IO::NewLineChar;
                list << L"\t<array>" << IO::NewLineChar;
                for (int i = 0; i < formats.Length(); i++) {
					string icon_file;
					if (formats[i].UniqueIndex) icon_file = L"FileIcon" + string(formats[i].UniqueIndex) + L".icns";
					else icon_file = L"AppIcon.icns";
                    list << L"\t\t<dict>" << IO::NewLineChar;
                    list << L"\t\t\t<key>CFBundleTypeExtensions</key>" << IO::NewLineChar;
                    list << L"\t\t\t<array>" << IO::NewLineChar;
                    list << L"\t\t\t\t<string>" << formats[i].Extension << L"</string>" << IO::NewLineChar;
                    list << L"\t\t\t</array>" << IO::NewLineChar;
                    list << L"\t\t\t<key>CFBundleTypeName</key>" << IO::NewLineChar;
                    list << L"\t\t\t<string>" << formats[i].Description << L"</string>" << IO::NewLineChar;
                    list << L"\t\t\t<key>CFBundleTypeIconFile</key>" << IO::NewLineChar;
                    list << L"\t\t\t<string>" << icon_file << L"</string>" << IO::NewLineChar;
                    list << L"\t\t\t<key>CFBundleTypeRole</key>" << IO::NewLineChar;
                    list << L"\t\t\t<string>" << (formats[i].CanCreate ? L"Editor" : L"Viewer") << L"</string>" << IO::NewLineChar;
                    list << L"\t\t</dict>" << IO::NewLineChar;
                }
                list << L"\t</array>" << IO::NewLineChar;
            }
            list << L"</dict>" << IO::NewLineChar;
            list << L"</plist>" << IO::NewLineChar;
        }
        catch (...) { console << L"Failed" << IO::NewLineChar; throw; }
        console << L"Succeed" << IO::NewLineChar;
    }
    catch (...) { return false; }
    return true;
}
bool build_bundle(const string & plist, const string & app_icon, const string & bundle, ITextWriter & console)
{
    console << L"Building Mac OS X Application Bundle " + IO::Path::GetFileName(bundle) + L"...";
    try_create_directory(bundle);
    clear_directory(bundle);
    try_create_directory(bundle + L"/Contents");
    try_create_directory(bundle + L"/Contents/MacOS");
    try_create_directory(bundle + L"/Contents/Resources");
    Array<string> locales(0x10);
    locales << L"en";
    for (int i = 0; i < reslist.Length(); i++) {
        if (!reslist[i].Locale.Length()) continue;
        bool added = false;
        for (int j = 0; j < locales.Length(); j++) if (string::CompareIgnoreCase(reslist[i].Locale, locales[j]) == 0) { added = true; break; }
        if (!added) locales << reslist[i].Locale.LowerCase();
    }
    for (int i = 0; i < locales.Length(); i++) {
        try_create_directory(bundle + L"/Contents/Resources/" + locales[i] + L".lproj");
    }
    try {
        copy_file(plist, bundle + L"/Contents/Info.plist");
        if (app_icon.Length()) copy_file(app_icon, bundle + L"/Contents/Resources/AppIcon.icns");
		for (int i = 0; i < formats.Length(); i++) if (formats[i].UniqueIcon) {
            copy_file(formats[i].Icon, bundle + L"/Contents/Resources/FileIcon" + string(formats[i].UniqueIndex) + L".icns");
        }
        for (int i = 0; i < reslist.Length(); i++) {
            string inner_name = reslist[i].Locale.Length() ? (reslist[i].Name + L"-" + reslist[i].Locale) : reslist[i].Name;
            if (!copy_file_nothrow(reslist[i].SourcePath, bundle + L"/Contents/Resources/" + inner_name)) {
                console << L"Failed" << IO::NewLineChar;
                console << L"Failed to import resource file \"" << reslist[i].SourcePath << L"\"" << IO::NewLineChar;
                return false;
            }
        }
    }
    catch (...) { console << L"Failed" << IO::NewLineChar; return false; }
    console << L"Succeed" << IO::NewLineChar;
    return true;
}
void index_resources(const string & base_path, const string & obj_path, RegistryNode * node, const string & locale)
{
    if (!node) return;
    auto & values = node->GetValues();
    for (int i = 0; i < values.Length(); i++) {
        string src = node->GetValueString(values[i]);
        string path;
        if (string::CompareIgnoreCase(src.Fragment(0, 9), L"<objpath>") == 0) {
            path = IO::ExpandPath(obj_path + src.Fragment(9, -1));
        } else {
            path = IO::ExpandPath(base_path + L"/" + src);
        }
        reslist << Resource{ path, values[i], locale };
    }
    auto & divs = node->GetSubnodes();
    for (int i = 0; i < divs.Length(); i++) {
        auto tags = divs[i].Split(L'-');
        string local_locale;
        bool skip = false;
        for (int j = 0; j < tags.Length(); j++) {
            if (tags[j].Length() == 2) local_locale = tags[j].LowerCase();
            else if (string::CompareIgnoreCase(tags[j], target_system) != 0) skip = true;
        }
        if (skip) continue;
        SafePointer<RegistryNode> resnode = node->OpenNode(divs[i]);
        index_resources(base_path, obj_path, resnode, local_locale);
    }
}
bool index_file_formats(const string & app_icon, const string & app_icon_src, const string & base_path, const string & obj_path, RegistryNode * node, ITextWriter & console)
{
    auto & dirs = node->GetSubnodes();
	Array<string> src_paths(0x10);
    for (int i = 0; i < dirs.Length(); i++) {
        SafePointer<RegistryNode> sub = node->OpenNode(dirs[i]);
        formats << FileFormat();
        formats.LastElement().Extension = sub->GetValueString(L"Extension");
        formats.LastElement().Description = sub->GetValueString(L"Description");
        formats.LastElement().Icon = IO::ExpandPath(base_path + L"/" + sub->GetValueString(L"Icon"));
        formats.LastElement().CanCreate = sub->GetValueBoolean(L"CanCreate");
		src_paths << formats.LastElement().Icon;
		if (formats.LastElement().Icon == app_icon_src) {
			formats.LastElement().Icon = app_icon;
			formats.LastElement().IconIndex = 0;
			formats.LastElement().UniqueIcon = false;
		} else {
			bool found = false;
			for (int i = 0; i < src_paths.Length() - 1; i++) {
				if (formats.LastElement().Icon == src_paths[i]) {
					formats.LastElement().Icon = formats[i].Icon;
					formats.LastElement().IconIndex = i + 1;
					formats.LastElement().UniqueIcon = false;
					found = true;
					break;
				}
			}
			if (!found) {
				string conv = obj_path + L"/" + IO::Path::GetFileNameWithoutExtension(formats.LastElement().Icon) + L"." + sys_cfg->GetValueString(L"IconExtension");
				if (!asm_icon(formats.LastElement().Icon, conv, console)) return false;
				formats.LastElement().Icon = conv;
				formats.LastElement().IconIndex = formats.Length();
				formats.LastElement().UniqueIcon = true;
			}
		}
    }
	int counter = 1;
	for (int i = 0; i < formats.Length(); i++) {
		if (formats[i].UniqueIcon) { formats[i].UniqueIndex = counter; counter++; }
		else { if (formats[i].IconIndex) formats[i].UniqueIndex = formats[formats[i].IconIndex - 1].UniqueIcon; else formats[i].UniqueIndex = 0; }
	}
    return true;
}
bool asm_format_manifest(const string & name, ITextWriter & console)
{
    try {
        Time man_time = 0;
        try {
            FileStream date(name, AccessRead, OpenExisting);
            man_time = IO::DateTime::GetFileAlterTime(date.Handle());
        }
        catch (...) {}
        if (prj_time < man_time && !clean) return true;
        console << L"Writing file formats manifest " << IO::Path::GetFileName(name) << L"...";
        try {
            SafePointer<Registry> man = CreateRegistry();
            for (int i = 0; i < formats.Length(); i++) {
                string ext = formats[i].Extension.LowerCase();
                man->CreateNode(ext);
                SafePointer<RegistryNode> fmt = man->OpenNode(ext);
                fmt->CreateValue(L"Description", RegistryValueType::String);
                fmt->SetValue(L"Description", formats[i].Description);
                fmt->CreateValue(L"IconIndex", RegistryValueType::Integer);
                fmt->SetValue(L"IconIndex", formats[i].UniqueIndex);
                fmt->CreateValue(L"CanCreate", RegistryValueType::Boolean);
                fmt->SetValue(L"CanCreate", formats[i].CanCreate);
            }
            FileStream man_file(name, AccessReadWrite, CreateAlways);
            TextWriter writer(&man_file, Encoding::UTF8);
            writer.WriteEncodingSignature();
            RegistryToText(man, &writer);
        }
        catch (...) { console << L"Failed" << IO::NewLineChar; throw; }
        console << L"Succeed" << IO::NewLineChar;
    }
    catch (...) { return false; }
    return true;
}

int Main(void)
{
    UI::Windows::InitializeCodecCollection();
    handle console_output = IO::CloneHandle(IO::GetStandardOutput());
    error_output = IO::CloneHandle(IO::GetStandardError());
	IO::Console console(console_output);

    SafePointer< Array<string> > args = GetCommandLine();

    console << ENGINE_VI_APPNAME << IO::NewLineChar;
    console << L"Copyright " << string(ENGINE_VI_COPYRIGHT).Replace(L'\xA9', L"(C)") << IO::NewLineChar;
    console << L"Version " << ENGINE_VI_APPVERSION << L", build " << ENGINE_VI_BUILD << IO::NewLineChar << IO::NewLineChar;
    if (args->Length() < 2) {
        console << L"Command line syntax:" << IO::NewLineChar;
        console << L"  " << ENGINE_VI_APPSYSNAME << L" <project config.ini> <object path> :winres|:macres [:clean] [:errlog]" << IO::NewLineChar;
        console << L"      [:bundle <bundle path.app>]" << IO::NewLineChar;
        console << L"Where" << IO::NewLineChar;
        console << L"  project config.ini  - project configuration to take data," << IO::NewLineChar;
        console << L"  object path         - path to folder to store intermediate and final output," << IO::NewLineChar;
        console << L"  :winres             - build .ico, .manifest, .rc and .res resources," << IO::NewLineChar;
        console << L"  :macres             - build .icns and info.plist resources," << IO::NewLineChar;
        console << L"  :clean              - rebuild all outputs," << IO::NewLineChar;
        console << L"  :errlog             - print logs of failed tools into error stream," << IO::NewLineChar;
        console << L"  :bundle             - produce Mac OS X Application Bundle directory (without code)," << IO::NewLineChar;
        console << L"  bundle path.app     - path to .app bundle folder (bundle name)." << IO::NewLineChar;
        console << IO::NewLineChar;
    } else {
        if (args->Length() < 4) {
            console << L"Not enough arguments." << IO::NewLineChar;
            return 1;
        }
        for (int i = 3; i < args->Length(); i++) {
            if (string::CompareIgnoreCase(args->ElementAt(i), L":winres") == 0) target_system = L"windows";
            else if (string::CompareIgnoreCase(args->ElementAt(i), L":macres") == 0) target_system = L"macosx";
            else if (string::CompareIgnoreCase(args->ElementAt(i), L":clean") == 0) clean = true;
            else if (string::CompareIgnoreCase(args->ElementAt(i), L":errlog") == 0) errlog = true;
            else if (string::CompareIgnoreCase(args->ElementAt(i), L":bundle") == 0) {
                bundle_path = (i < args->Length() - 1) ? args->ElementAt(i + 1) : L"";
                if (!bundle_path.Length()) {
                    console << L"Used :bundle without bundle name." << IO::NewLineChar;
                    return 1;
                }
            }
        }
        if (bundle_path.Length() && target_system != L"macosx") {
            console << L"Using of :bundle is allowed only with :macres." << IO::NewLineChar;
            return 1;
        }
        try {
            try {
                args->ElementAt(1) = IO::ExpandPath(args->ElementAt(1));
                args->ElementAt(2) = IO::ExpandPath(args->ElementAt(2));
                FileStream sys_reg_stream(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/" + string(ENGINE_VI_APPSYSNAME) + L".ini", AccessRead, OpenExisting);
                FileStream prj_reg_stream(args->ElementAt(1), AccessRead, OpenExisting);
                SafePointer<RegistryNode> sys_reg = CompileTextRegistry(&sys_reg_stream);
                prj_cfg = CompileTextRegistry(&prj_reg_stream);
                if (sys_reg) sys_cfg = sys_reg->OpenNode(target_system);
                if (!sys_cfg || !prj_cfg) throw Exception();
                prj_time = IO::DateTime::GetFileAlterTime(prj_reg_stream.Handle());
                {
                    prj_ver.UseDefines = true;
                    prj_ver.AppName = prj_cfg->GetValueString(L"VersionInformation/ApplicationName");
                    prj_ver.CompanyName = prj_cfg->GetValueString(L"VersionInformation/CompanyName");
                    prj_ver.Copyright = prj_cfg->GetValueString(L"VersionInformation/Copyright");
                    prj_ver.InternalName = prj_cfg->GetValueString(L"VersionInformation/InternalName");
                    prj_ver.AppIdent = prj_cfg->GetValueString(L"VersionInformation/ApplicationIdentifier");
                    prj_ver.ComIdent = prj_cfg->GetValueString(L"VersionInformation/CompanyIdentifier");
                    prj_ver.Description = prj_cfg->GetValueString(L"VersionInformation/Description");
                    try {
                        auto verind = prj_cfg->GetValueString(L"VersionInformation/Version").Split(L'.');
                        if (verind.Length() > 0 && verind[0].Length()) prj_ver.VersionMajor = verind[0].ToUInt32(); else prj_ver.VersionMajor = 0;
                        if (verind.Length() > 1) prj_ver.VersionMinor = verind[1].ToUInt32(); else prj_ver.VersionMinor = 0;
                        if (verind.Length() > 2) prj_ver.Subversion = verind[2].ToUInt32(); else prj_ver.Subversion = 0;
                        if (verind.Length() > 3) prj_ver.Build = verind[3].ToUInt32(); else prj_ver.Build = 0;
                    }
                    catch (...) {
                        console << L"Invalid application version notation." << IO::NewLineChar;
                        return 1;
                    }
                    if (!prj_ver.AppIdent.Length()) {
                        prj_ver.AppIdent = IO::Path::GetFileNameWithoutExtension(args->ElementAt(1));
                        int dot = prj_ver.AppIdent.FindFirst(L'.');
                        if (dot != -1) prj_ver.AppIdent = prj_ver.AppIdent.Fragment(0, dot);
                    }
                    if (!prj_ver.ComIdent.Length()) prj_ver.ComIdent = L"unknown";
                    if (!prj_ver.InternalName.Length()) {
                        prj_ver.InternalName = IO::Path::GetFileNameWithoutExtension(args->ElementAt(1));
                        int dot = prj_ver.InternalName.FindFirst(L'.');
                        if (dot != -1) prj_ver.InternalName = prj_ver.InternalName.Fragment(0, dot);
                    }
                }
            }
            catch (...) {
                console << L"Failed to load project or tool configuration." << IO::NewLineChar;
                return 1;
            }
            if (target_system == L"windows") {
				Array<string> icons(0x10);
                string app_icon = prj_cfg->GetValueString(L"ApplicationIcon");
				string app_icon_src;
                if (app_icon.Length()) {
                    app_icon_src = app_icon = IO::ExpandPath(IO::Path::GetDirectory(args->ElementAt(1)) + L"/" + app_icon);
                    string conv_icon = IO::Path::GetFileNameWithoutExtension(app_icon) + L"." + sys_cfg->GetValueString(L"IconExtension");
                    conv_icon = args->ElementAt(2) + L"/" + conv_icon;
                    if (!asm_icon(app_icon, conv_icon, console)) return 1;
                    app_icon = conv_icon;
                    icons << IO::Path::GetFileName(app_icon);
                }
                {
                    SafePointer<RegistryNode> resdir = prj_cfg->OpenNode(L"Resources");
                    index_resources(IO::Path::GetDirectory(args->ElementAt(1)), args->ElementAt(2), resdir, L"");
                    if (!validate_resources(console)) return 1;
                }
                {
                    SafePointer<RegistryNode> frmdir = prj_cfg->OpenNode(L"FileFormats");
                    if (frmdir) {
                        if (!index_file_formats(app_icon, app_icon_src, IO::Path::GetDirectory(args->ElementAt(1)), args->ElementAt(2), frmdir, console)) return 1;
                    }
                }
                string manifest = args->ElementAt(2) + L"/" + IO::Path::GetFileNameWithoutExtension(args->ElementAt(1)) + L".manifest";
                if (!asm_manifest(manifest, console)) return 1;
                for (int i = 0; i < formats.Length(); i++) icons << IO::Path::GetFileName(formats[i].Icon);
                string rc = args->ElementAt(2) + L"/" + IO::Path::GetFileNameWithoutExtension(args->ElementAt(1)) + L".rc";
                manifest = IO::Path::GetFileName(manifest);
				bool is_lib = (string::CompareIgnoreCase(prj_cfg->GetValueString(L"Subsystem"), L"library") == 0);
                if (!asm_resscript(manifest, icons, app_icon, rc, is_lib, console)) return 1;
                string res = args->ElementAt(2) + L"/" + IO::Path::GetFileNameWithoutExtension(args->ElementAt(1)) + L".res";
                string res_log = args->ElementAt(2) + L"/" + IO::Path::GetFileNameWithoutExtension(args->ElementAt(1)) + L".log";
                if (!compile_resource(rc, res, res_log, console)) return 1;
                if (formats.Length()) {
                    if (!asm_format_manifest(args->ElementAt(2) + L"/" + IO::Path::GetFileNameWithoutExtension(args->ElementAt(1)) + L".formats.ini", console)) return 1;
                }
            } else if (target_system == L"macosx") {
				string app_icon = prj_cfg->GetValueString(L"ApplicationIcon");
				string app_icon_src;
                if (app_icon.Length()) {
                    app_icon_src = app_icon = IO::ExpandPath(IO::Path::GetDirectory(args->ElementAt(1)) + L"/" + app_icon);
                    string conv_icon = IO::Path::GetFileNameWithoutExtension(app_icon) + L"." + sys_cfg->GetValueString(L"IconExtension");
                    conv_icon = args->ElementAt(2) + L"/" + conv_icon;
                    if (!asm_icon(app_icon, conv_icon, console)) return 1;
                    app_icon = conv_icon;
                }
                {
                    SafePointer<RegistryNode> resdir = prj_cfg->OpenNode(L"Resources");
                    index_resources(IO::Path::GetDirectory(args->ElementAt(1)), args->ElementAt(2), resdir, L"");
                    if (!validate_resources(console)) return 1;
                }
                {
                    SafePointer<RegistryNode> frmdir = prj_cfg->OpenNode(L"FileFormats");
                    if (frmdir) {
                        if (!index_file_formats(app_icon, app_icon_src, IO::Path::GetDirectory(args->ElementAt(1)), args->ElementAt(2), frmdir, console)) return 1;
                    }
                }
                string plist = args->ElementAt(2) + L"/" + IO::Path::GetFileNameWithoutExtension(args->ElementAt(1)) + L".plist";
                if (!asm_plist(plist, app_icon.Length(), console)) return 1;
                if (bundle_path.Length()) {
                    if (!build_bundle(plist, app_icon, bundle_path, console)) return 1;
                }
            } else {
                console << L"Invalid target system." << IO::NewLineChar;
                return 1;
            }
        }
        catch (...) {
            console << L"Some shit occured..." << IO::NewLineChar;
            return 1;
        }
    }
    return 0;
}