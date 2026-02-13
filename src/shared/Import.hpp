#ifndef IMPORT_HPP
#define IMPORT_HPP

inline const char* requestImport(const char* path) {
#ifdef __EMSCRIPTEN__
    /*extern "C" const char* onImportRequested(const char* path);
    return onImportRequested(path);*/
#else
    /*(void)path;
    return nullptr;*/
#endif
    return nullptr;
}

#endif
