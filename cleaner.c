#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <wchar.h>
#include <stdbool.h>

#ifdef _MSC_VER
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#endif

/*
 * true  = realiza el borrado
 * false = solamente muestra lo que eliminaría
 */
static bool executeDeletion = false;

static bool isDesktopShortcut(const wchar_t *fileName)
{
    const wchar_t *extension = wcsrchr(fileName, L'.');

    if (extension == NULL) {
        return false;
    }

    return _wcsicmp(extension, L".lnk") == 0 ||
           _wcsicmp(extension, L".url") == 0;
}

static bool joinPath(
    wchar_t *destination,
    size_t destinationSize,
    const wchar_t *directory,
    const wchar_t *name)
{
    int result = swprintf(
        destination,
        destinationSize,
        L"%ls\\%ls",
        directory,
        name
    );

    return result >= 0 &&
           (size_t)result < destinationSize;
}

static bool directoryIsEmpty(const wchar_t *directory)
{
    WIN32_FIND_DATAW findData;
    wchar_t searchPath[MAX_PATH];

    if (!joinPath(
            searchPath,
            MAX_PATH,
            directory,
            L"*")) {
        return false;
    }

    HANDLE searchHandle =
        FindFirstFileW(searchPath, &findData);

    if (searchHandle == INVALID_HANDLE_VALUE) {
        return true;
    }

    do {
        if (wcscmp(findData.cFileName, L".") != 0 &&
            wcscmp(findData.cFileName, L"..") != 0) {
            FindClose(searchHandle);
            return false;
        }
    } while (FindNextFileW(searchHandle, &findData));

    FindClose(searchHandle);
    return true;
}

static void deleteFileSafely(const wchar_t *path)
{
    if (!executeDeletion) {
        wprintf(L"[SIMULACION] Archivo: %ls\n", path);
        return;
    }

    /*
     * Quita los atributos de solo lectura y oculto que podrían
     * impedir la eliminación.
     */
    DWORD attributes = GetFileAttributesW(path);

    if (attributes != INVALID_FILE_ATTRIBUTES) {
        SetFileAttributesW(
            path,
            attributes &
            ~(FILE_ATTRIBUTE_READONLY |
              FILE_ATTRIBUTE_HIDDEN |
              FILE_ATTRIBUTE_SYSTEM)
        );
    }

    if (DeleteFileW(path)) {
        wprintf(L"[ELIMINADO] Archivo: %ls\n", path);
    } else {
        fwprintf(
            stderr,
            L"[ERROR %lu] No se pudo eliminar: %ls\n",
            GetLastError(),
            path
        );
    }
}

static void deleteDirectorySafely(const wchar_t *path)
{
    if (!executeDeletion) {
        wprintf(L"[SIMULACION] Carpeta: %ls\n", path);
        return;
    }

    SetFileAttributesW(path, FILE_ATTRIBUTE_NORMAL);

    if (RemoveDirectoryW(path)) {
        wprintf(L"[ELIMINADA] Carpeta: %ls\n", path);
    } else {
        fwprintf(
            stderr,
            L"[ERROR %lu] No se pudo eliminar la carpeta: %ls\n",
            GetLastError(),
            path
        );
    }
}

/*
 * Limpia el contenido de una carpeta.
 *
 * preserveShortcuts:
 *   true  -> conserva archivos .lnk y .url
 *   false -> elimina todos los archivos
 *
 * Esta función no elimina la carpeta raíz.
 */
static void cleanDirectory(
    const wchar_t *directory,
    bool preserveShortcuts)
{
    WIN32_FIND_DATAW findData;
    wchar_t searchPath[MAX_PATH];

    if (!joinPath(
            searchPath,
            MAX_PATH,
            directory,
            L"*")) {
        fwprintf(
            stderr,
            L"[ERROR] Ruta demasiado larga: %ls\n",
            directory
        );
        return;
    }

    HANDLE searchHandle =
        FindFirstFileW(searchPath, &findData);

    if (searchHandle == INVALID_HANDLE_VALUE) {
        fwprintf(
            stderr,
            L"[ERROR %lu] No se pudo abrir: %ls\n",
            GetLastError(),
            directory
        );
        return;
    }

    do {
        const wchar_t *name = findData.cFileName;

        if (wcscmp(name, L".") == 0 ||
            wcscmp(name, L"..") == 0) {
            continue;
        }

        wchar_t fullPath[MAX_PATH];

        if (!joinPath(
                fullPath,
                MAX_PATH,
                directory,
                name)) {
            fwprintf(
                stderr,
                L"[ERROR] Ruta demasiado larga: %ls\\%ls\n",
                directory,
                name
            );
            continue;
        }

        if (findData.dwFileAttributes &
            FILE_ATTRIBUTE_REPARSE_POINT) {

            /*
             * No se entra en enlaces simbólicos ni junctions.
             * Solo se elimina el enlace.
             */
            if (findData.dwFileAttributes &
                FILE_ATTRIBUTE_DIRECTORY) {
                deleteDirectorySafely(fullPath);
            } else {
                deleteFileSafely(fullPath);
            }

            continue;
        }

        if (findData.dwFileAttributes &
            FILE_ATTRIBUTE_DIRECTORY) {

            cleanDirectory(
                fullPath,
                preserveShortcuts
            );

            /*
             * Si había un acceso directo dentro de esta carpeta,
             * la carpeta no quedará vacía y se conservará.
             */
            if (directoryIsEmpty(fullPath)) {
                deleteDirectorySafely(fullPath);
            }

            continue;
        }

        if (preserveShortcuts &&
            isDesktopShortcut(name)) {
            wprintf(
                L"[CONSERVADO] Acceso directo: %ls\n",
                fullPath
            );
            continue;
        }

        deleteFileSafely(fullPath);

    } while (FindNextFileW(searchHandle, &findData));

    FindClose(searchHandle);
}

static wchar_t *getKnownFolderPath(
    REFKNOWNFOLDERID folderId)
{
    wchar_t *path = NULL;

    HRESULT result = SHGetKnownFolderPath(
        folderId,
        KF_FLAG_DEFAULT,
        NULL,
        &path
    );

    if (FAILED(result)) {
        fwprintf(
            stderr,
            L"[ERROR] SHGetKnownFolderPath: 0x%08lX\n",
            (unsigned long)result
        );

        return NULL;
    }

    return path;
}

static void cleanKnownFolder(
    const wchar_t *folderName,
    REFKNOWNFOLDERID folderId,
    bool preserveShortcuts)
{
    wchar_t *path = getKnownFolderPath(folderId);

    if (path == NULL) {
        fwprintf(
            stderr,
            L"No se encontró la carpeta %ls.\n",
            folderName
        );
        return;
    }

    wprintf(
        L"\n========== %ls ==========\n",
        folderName
    );

    wprintf(
        L"Ruta detectada: %ls\n",
        path
    );

    cleanDirectory(path, preserveShortcuts);

    CoTaskMemFree(path);
}

int wmain(int argc, wchar_t *argv[])
{
    if (argc == 2 &&
        _wcsicmp(argv[1], L"--execute") == 0) {
        executeDeletion = true;
    }

    wprintf(
        L"Limpiador de carpetas del usuario\n"
        L"================================\n"
    );

    if (!executeDeletion) {
        wprintf(
            L"\nMODO SIMULACION ACTIVADO.\n"
            L"No se eliminará ningún archivo.\n"
            L"Utiliza --execute para realizar el borrado.\n"
        );
    } else {
        wprintf(
            L"\nADVERTENCIA: MODO DE BORRADO ACTIVADO.\n"
            L"Los archivos serán eliminados permanentemente.\n"
        );
    }

    /*
     * false: elimina todo el contenido.
     */
    cleanKnownFolder(
        L"Descargas",
        &FOLDERID_Downloads,
        false
    );

    cleanKnownFolder(
        L"Documentos",
        &FOLDERID_Documents,
        false
    );

    /*
     * true: conserva accesos directos .lnk y .url.
     */
    cleanKnownFolder(
        L"Escritorio",
        &FOLDERID_Desktop,
        true
    );

    wprintf(L"\nProceso terminado.\n");

    return 0;
}