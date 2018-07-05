int Main(void);
#ifdef ENGINE_WINDOWS
#ifdef ENGINE_SUBSYSTEM_CONSOLE
int wmain(void) { return Main(); }
#endif
#ifdef ENGINE_SUBSYSTEM_GUI

#endif
#endif
#ifdef ENGINE_MACOSX
#ifdef ENGINE_SUBSYSTEM_CONSOLE
int main(void) { return Main(); }
#endif
#ifdef ENGINE_SUBSYSTEM_GUI

#endif
#endif