# RDL Cleaner

[Español](#español) | [English](#english)

---

## Español

## 1. Propósito

**RDL Cleaner** es una utilidad de consola para Windows desarrollada en C. Su propósito es limpiar el contenido de las carpetas conocidas **Descargas**, **Documentos** y **Escritorio** de la cuenta bajo cuyo contexto se ejecuta el proceso.

La aplicación tiene dos modos:

- **Simulación:** enumera lo que sería eliminado sin modificar el sistema.
- **Ejecución:** elimina permanentemente los elementos identificados.

En el Escritorio conserva los archivos `.lnk` y `.url`, utilizados normalmente como accesos directos.

> **Advertencia:** el modo de ejecución no envía los elementos a la Papelera de reciclaje.

## 2. Alcance

El programa procesa:

1. Descargas, mediante `FOLDERID_Downloads`.
2. Documentos, mediante `FOLDERID_Documents`.
3. Escritorio, mediante `FOLDERID_Desktop`.

No utiliza rutas fijas como `C:\Users\usuario\Desktop`. Obtiene la ubicación vigente mediante `SHGetKnownFolderPath`. Por ello, puede actuar sobre carpetas redirigidas por Windows, OneDrive o políticas del perfil.

### Comportamiento por carpeta

- **Descargas:** elimina recursivamente todos los archivos y subcarpetas, pero conserva la carpeta raíz.
- **Documentos:** elimina recursivamente todos los archivos y subcarpetas, pero conserva la carpeta raíz.
- **Escritorio:** elimina recursivamente el contenido, excepto archivos `.lnk` y `.url`, y conserva la carpeta raíz.

Una subcarpeta del Escritorio que contenga un acceso directo conservado también permanece porque no queda vacía.

## 3. Modos de operación

### Simulación

```powershell
RDL_Cleaner.exe
```

Es el modo predeterminado. No elimina ni cambia atributos. Muestra en consola los archivos y carpetas que serían procesados.

```text
[SIMULACION] Archivo: C:\Users\usuario\Downloads\archivo.zip
[SIMULACION] Carpeta: C:\Users\usuario\Documents\Temporal
[CONSERVADO] Acceso directo: C:\Users\usuario\Desktop\Aplicacion.lnk
```

### Ejecución

```powershell
RDL_Cleaner.exe --execute
```

El borrado se activa únicamente si se recibe exactamente un argumento igual a `--execute`. La comparación no distingue mayúsculas y minúsculas. Cualquier otro argumento mantiene el modo de simulación.

En ejecución:

- Elimina archivos mediante `DeleteFileW`.
- Elimina carpetas vacías mediante `RemoveDirectoryW`.
- No solicita confirmación interactiva.
- No utiliza la Papelera de reciclaje.

## 4. Flujo de funcionamiento

1. `wmain` revisa si se proporcionó `--execute`.
2. Establece el modo de simulación o ejecución.
3. Localiza y procesa Descargas.
4. Localiza y procesa Documentos.
5. Localiza y procesa el Escritorio, preservando `.lnk` y `.url`.
6. Libera con `CoTaskMemFree` la memoria asignada para cada ruta.
7. Muestra que el proceso terminó.

El procesamiento es secuencial. Un error individual no detiene los elementos restantes.

## 5. Descripción de funciones

### `isDesktopShortcut`

Busca la extensión del archivo con `wcsrchr` y utiliza `_wcsicmp` para reconocer `.lnk` y `.url` sin distinguir mayúsculas y minúsculas. La decisión se basa únicamente en el nombre; no valida el contenido interno del acceso directo.

### `joinPath`

Concatena el directorio y el nombre mediante `swprintf`. Comprueba que el resultado quepa en el búfer. Si la ruta excede la capacidad, devuelve `false` y el elemento no se procesa.

### `directoryIsEmpty`

Enumera una carpeta con `FindFirstFileW` y `FindNextFileW`, ignorando `.` y `..`. Se utiliza después del recorrido recursivo para decidir si una subcarpeta puede eliminarse.

### `deleteFileSafely`

En simulación solamente muestra la ruta. En ejecución:

1. Consulta atributos con `GetFileAttributesW`.
2. Retira `READONLY`, `HIDDEN` y `SYSTEM`.
3. Intenta eliminar mediante `DeleteFileW`.
4. Informa éxito o el código de `GetLastError`.

El término "Safely" describe el control de errores. No implica recuperación, borrado criptográfico ni envío a la Papelera.

### `deleteDirectorySafely`

En simulación muestra la ruta. En ejecución cambia los atributos a `FILE_ATTRIBUTE_NORMAL` e intenta eliminar mediante `RemoveDirectoryW`. La operación falla si la carpeta no está vacía o no existen permisos suficientes.

### `cleanDirectory`

Realiza el recorrido recursivo:

1. Omite `.` y `..`.
2. Construye la ruta completa.
3. Detecta puntos de análisis.
4. Procesa recursivamente carpetas normales.
5. Elimina las subcarpetas que queden vacías.
6. Conserva `.lnk` y `.url` cuando `preserveShortcuts` es `true`.
7. Solicita la eliminación de los demás archivos.

`preserveShortcuts` es `false` para Descargas y Documentos, y `true` para el Escritorio.

### `getKnownFolderPath`

Obtiene la ubicación de una carpeta conocida mediante `SHGetKnownFolderPath` y `KF_FLAG_DEFAULT`. Si falla, muestra el `HRESULT` y devuelve `NULL`.

### `cleanKnownFolder`

Resuelve la ruta, muestra la ubicación detectada, llama a `cleanDirectory` y libera la memoria con `CoTaskMemFree`. Si no puede resolver la carpeta, informa el error y continúa.

### `wmain`

Interpreta los argumentos, selecciona el modo, procesa las tres carpetas y muestra la finalización. Actualmente devuelve `0` incluso si hubo errores parciales.

## 6. Puntos de análisis, enlaces y junctions

Antes de entrar en una carpeta, el programa comprueba `FILE_ATTRIBUTE_REPARSE_POINT`.

Cuando lo encuentra:

- No recorre el destino.
- Si aparece como directorio, intenta retirar el enlace con `RemoveDirectoryW`.
- Si aparece como archivo, intenta retirarlo con `DeleteFileW`.

Este control reduce el riesgo de recorrer rutas externas mediante enlaces simbólicos o junctions. El resultado depende del tipo de punto de análisis y sus permisos, por lo que debe probarse en el entorno objetivo.

## 7. Controles incorporados

- **Simulación predeterminada:** el borrado requiere `--execute`.
- **API de carpetas conocidas:** evita asumir rutas fijas.
- **No seguimiento de puntos de análisis:** evita entrar en destinos enlazados.
- **Validación de ruta:** rechaza concatenaciones que excedan el búfer.
- **Conservación de accesos directos:** preserva `.lnk` y `.url` en el Escritorio.
- **Mensajes de operación:** informa simulaciones, conservaciones, eliminaciones y errores.
- **Continuación ante errores:** un fallo no detiene toda la ejecución.

## 8. Privilegios y contexto

El código no solicita elevación ni modifica el token de seguridad. Opera con los permisos de la identidad que inicia el proceso.

Consecuencias:

- Solo puede eliminar objetos para los que tenga permisos.
- Los archivos bloqueados o protegidos pueden fallar.
- Una ejecución elevada aumenta el alcance permitido.
- Una ejecución como `SYSTEM` puede resolver las carpetas del perfil del sistema, no las del empleado conectado.

Antes de automatizarlo debe validarse qué identidad ejecutará el proceso y qué perfil resolverá `SHGetKnownFolderPath`.

## 9. Manejo de errores

Los errores se escriben en `stderr` e incluyen códigos de Windows cuando están disponibles.

Se controlan:

- Rutas que exceden el búfer.
- Carpetas que no pueden enumerarse.
- Carpetas conocidas que no pueden resolverse.
- Archivos que no pueden eliminarse.
- Carpetas que no pueden eliminarse.

El programa continúa después de un error. Sin embargo, no mantiene un contador global y siempre termina con código `0`. Por ello, un sistema externo no puede distinguir por el código de salida entre éxito total, éxito parcial o múltiples errores.

## 10. Cambios realizados en el sistema

En simulación no realiza modificaciones.

En ejecución puede:

- Retirar atributos de solo lectura, oculto y sistema.
- Cambiar atributos de carpetas a normal.
- Eliminar archivos.
- Eliminar carpetas vacías.
- Retirar enlaces o puntos de análisis sin recorrer el destino.

El código no contiene funciones para:

- Comunicarse por red.
- Descargar o cargar información.
- Crear procesos secundarios.
- Ejecutar comandos externos.
- Modificar el Registro.
- Crear servicios o tareas programadas.
- Establecer persistencia.
- Recopilar credenciales.
- Cifrar archivos.
- Sobrescribir físicamente datos.
- Recuperar elementos eliminados.

## 11. Registro y trazabilidad

La actividad se muestra únicamente en consola. El programa no crea archivos de log, eventos en Windows Event Log, marcas de tiempo, identificadores de ejecución ni resúmenes estadísticos.

Para conservar la salida, el proceso que lo invoque puede redirigirla:

```powershell
RDL_Cleaner.exe --execute *> RDL_Cleaner.log
```

La redirección no forma parte del código interno.

## 12. Limitaciones y riesgos

### Eliminación permanente

No existe recuperación integrada y los objetos no se envían a la Papelera.

### Carpetas redirigidas

El programa actúa sobre la ruta devuelta por Windows. Una eliminación local podría propagarse a OneDrive u otro sistema sincronizado según su configuración.

### `MAX_PATH`

Las rutas se almacenan en búferes `MAX_PATH`. Las rutas más largas se omiten y producen error.

### Archivos bloqueados

Los archivos abiertos con bloqueos incompatibles pueden permanecer.

### Preservación por extensión

La conservación de accesos se basa solo en `.lnk` y `.url`; no valida su contenido.

### Modificación previa de atributos

Los atributos pueden cambiar aunque la eliminación posterior falle.

### Código de salida

Siempre devuelve `0`, aunque existan errores parciales.

### Sin confirmación adicional

Una vez iniciado con `--execute`, procede sin intervención del usuario.

### Contexto del proceso

Ejecutarlo con otra identidad puede resolver y afectar un perfil diferente.

### Condiciones de carrera

El contenido puede cambiar entre enumeración, comprobación y eliminación, alterando el resultado.

### Comprobación de carpeta vacía

Si `FindFirstFileW` falla dentro de `directoryIsEmpty`, la función devuelve `true`. Posteriormente se intentará `RemoveDirectoryW`; Windows impedirá eliminarla si realmente contiene elementos, pero se generará un intento de eliminación y posiblemente un error.

## 13. Matriz de comportamiento

| Elemento | Descargas | Documentos | Escritorio |
|---|---:|---:|---:|
| Archivo normal | Eliminar | Eliminar | Eliminar |
| Archivo `.lnk` | Eliminar | Eliminar | Conservar |
| Archivo `.url` | Eliminar | Eliminar | Conservar |
| Subcarpeta | Recorrer | Recorrer | Recorrer |
| Subcarpeta vacía al finalizar | Eliminar | Eliminar | Eliminar |
| Subcarpeta con acceso conservado | No aplica | No aplica | Conservar |
| Punto de análisis | No recorrer; intentar retirar | No recorrer; intentar retirar | No recorrer; intentar retirar |
| Carpeta raíz | Conservar | Conservar | Conservar |

## 14. Pruebas mínimas para auditoría

1. Ejecutar sin argumentos y confirmar que no hay modificaciones.
2. Ejecutar con `--execute` en un perfil de laboratorio.
3. Probar archivos normales en las tres carpetas.
4. Probar archivos de solo lectura, ocultos y de sistema.
5. Probar archivos bloqueados por otro proceso.
6. Probar `.lnk` y `.url` en el Escritorio.
7. Probar extensiones en mayúsculas y minúsculas.
8. Probar accesos dentro de subcarpetas del Escritorio.
9. Probar subcarpetas vacías y no vacías.
10. Probar enlaces simbólicos y junctions.
11. Probar rutas superiores a `MAX_PATH`.
12. Probar carpetas redirigidas o sincronizadas.
13. Probar con permisos insuficientes.
14. Probar como usuario estándar y elevado.
15. Probar bajo otra cuenta y bajo `SYSTEM`.
16. Cambiar el contenido mientras se ejecuta.
17. Confirmar que las tres carpetas raíz permanecen.
18. Confirmar que los errores no detienen los demás elementos.
19. Confirmar que el código de salida actual sigue siendo `0` ante errores parciales.
20. Registrar el hash del binario exacto sometido a aprobación.

## 15. Consideraciones para aprobación

Antes de autorizar el uso deben confirmarse:

- La identidad que ejecutará el proceso.
- Las rutas reales detectadas en los equipos objetivo.
- El efecto sobre OneDrive y carpetas redirigidas.
- La aceptación formal del borrado permanente.
- La conservación correcta de accesos directos.
- El mecanismo para conservar evidencia de ejecución.
- El tratamiento de archivos bloqueados y errores parciales.
- La revisión del código fuente y del hash del binario aprobado.

Debido a que realiza eliminación recursiva de datos, el despliegue debe estar autorizado, documentado y limitado a los equipos y escenarios aprobados.

## 16. Resumen para auditoría

RDL Cleaner es una utilidad local sin conectividad de red ni mecanismos de persistencia. Recorre Descargas, Documentos y Escritorio del perfil asociado al proceso. Su modo predeterminado es de simulación y el borrado requiere `--execute`.

Conserva accesos directos del Escritorio por extensión, no recorre puntos de análisis y continúa después de errores individuales. Sus riesgos principales son el borrado permanente, la actuación sobre carpetas redirigidas, la ausencia de registro persistente, el límite `MAX_PATH`, el código de salida no representativo y la dependencia del contexto de seguridad de ejecución.

---

## English

### 1. Purpose

**RDL Cleaner** is a Windows console utility developed in C. Its purpose is to clean the contents of the known **Downloads**, **Documents**, and **Desktop** folders for the account under whose context the process is running.

The application has two modes:
- **Simulation:** lists what would be deleted without modifying the system.
- **Execution:** permanently deletes the identified items.

On the Desktop, it preserves `.lnk` and `.url` files, which are commonly used as shortcuts.

**Warning:** execution mode does not send items to the Recycle Bin.

### 2. Scope

The program processes:
- Downloads, through `FOLDERID_Downloads`.
- Documents, through `FOLDERID_Documents`.
- Desktop, through `FOLDERID_Desktop`.

It does not use fixed paths such as `C:\Users\user\Desktop`. It obtains the current location through `SHGetKnownFolderPath`. Therefore, it may operate on folders redirected by Windows, OneDrive, or profile policies.

#### Behavior by folder
- **Downloads:** recursively deletes all files and subfolders, but preserves the root folder.
- **Documents:** recursively deletes all files and subfolders, but preserves the root folder.
- **Desktop:** recursively deletes the contents, except for `.lnk` and `.url` files, and preserves the root folder.

A Desktop subfolder containing a preserved shortcut also remains because it is not empty.

### 3. Operating modes

#### Simulation

```text
RDL_Cleaner.exe
```

This is the default mode. It does not delete items or change attributes. It displays in the console the files and folders that would be processed.

```text
[SIMULATION] File: C:\Users\user\Downloads\file.zip
[SIMULATION] Folder: C:\Users\user\Documents\Temporary
[PRESERVED] Shortcut: C:\Users\user\Desktop\Application.lnk
```

#### Execution

```text
RDL_Cleaner.exe --execute
```

Deletion is enabled only if exactly one argument equal to `--execute` is received. The comparison is case-insensitive. Any other argument keeps the program in simulation mode.

In execution mode:
- It deletes files through `DeleteFileW`.
- It deletes empty folders through `RemoveDirectoryW`.
- It does not request interactive confirmation.
- It does not use the Recycle Bin.

### 4. Execution flow

- `wmain` checks whether `--execute` was provided.
- It sets simulation or execution mode.
- It locates and processes Downloads.
- It locates and processes Documents.
- It locates and processes the Desktop while preserving `.lnk` and `.url` files.
- It releases the memory allocated for each path with `CoTaskMemFree`.
- It displays a message indicating that the process has finished.

Processing is sequential. An individual error does not stop the remaining items from being processed.

### 5. Function descriptions

#### `isDesktopShortcut`

Finds the file extension with `wcsrchr` and uses `_wcsicmp` to recognize `.lnk` and `.url` without case sensitivity. The decision is based only on the file name; it does not validate the internal contents of the shortcut.

#### `joinPath`

Concatenates the directory and name using `swprintf`. It checks whether the result fits in the buffer. If the path exceeds the buffer capacity, it returns `false`, and the item is not processed.

#### `directoryIsEmpty`

Enumerates a folder with `FindFirstFileW` and `FindNextFileW`, ignoring `.` and `..`. It is used after recursive traversal to determine whether a subfolder can be deleted.

#### `deleteFileSafely`

In simulation mode, it only displays the path. In execution mode, it:
- Reads attributes with `GetFileAttributesW`.
- Removes the `READONLY`, `HIDDEN`, and `SYSTEM` attributes.
- Attempts deletion through `DeleteFileW`.
- Reports success or the `GetLastError` code.

The term "Safely" refers to error handling. It does not imply recovery, cryptographic erasure, or sending the file to the Recycle Bin.

#### `deleteDirectorySafely`

In simulation mode, it displays the path. In execution mode, it changes the attributes to `FILE_ATTRIBUTE_NORMAL` and attempts to delete the folder through `RemoveDirectoryW`. The operation fails if the folder is not empty or if permissions are insufficient.

#### `cleanDirectory`

Performs recursive traversal:
- Skips `.` and `..`.
- Builds the full path.
- Detects reparse points.
- Recursively processes normal folders.
- Deletes subfolders that are empty after processing.
- Preserves `.lnk` and `.url` files when `preserveShortcuts` is `true`.
- Requests deletion of all other files.

`preserveShortcuts` is `false` for Downloads and Documents, and `true` for the Desktop.

#### `getKnownFolderPath`

Obtains the location of a known folder through `SHGetKnownFolderPath` and `KF_FLAG_DEFAULT`. If the operation fails, it displays the `HRESULT` and returns `NULL`.

#### `cleanKnownFolder`

Resolves the path, displays the detected location, calls `cleanDirectory`, and releases the memory with `CoTaskMemFree`. If it cannot resolve the folder, it reports the error and continues.

#### `wmain`

Interprets the arguments, selects the mode, processes the three folders, and displays the completion message. It currently returns `0` even if partial errors occurred.

### 6. Reparse points, links, and junctions

Before entering a folder, the program checks for `FILE_ATTRIBUTE_REPARSE_POINT`.

When it finds one:
- It does not traverse the target.
- If it appears as a directory, it attempts to remove the link with `RemoveDirectoryW`.
- If it appears as a file, it attempts to remove the link with `DeleteFileW`.

This control reduces the risk of traversing external paths through symbolic links or junctions. The result depends on the reparse point type and its permissions, so it must be tested in the target environment.

### 7. Built-in controls

- **Default simulation:** deletion requires `--execute`.
- **Known Folder API:** avoids assuming fixed paths.
- **No reparse-point traversal:** prevents entering linked targets.
- **Path validation:** rejects concatenations that exceed the buffer.
- **Shortcut preservation:** preserves `.lnk` and `.url` files on the Desktop.
- **Operation messages:** reports simulations, preserved items, deletions, and errors.
- **Continues after errors:** one failure does not stop the entire execution.

### 8. Privileges and context

The code does not request elevation or modify the security token. It operates with the permissions of the identity that starts the process.

Consequences:
- It can delete only objects for which it has permission.
- Locked or protected files may fail.
- Elevated execution increases the permitted scope.
- Execution as `SYSTEM` may resolve the system profile folders rather than those of the signed-in employee.

Before automating it, validate which identity will run the process and which profile `SHGetKnownFolderPath` will resolve.

### 9. Error handling

Errors are written to `stderr` and include Windows error codes when available.

The program handles:
- Paths that exceed the buffer.
- Folders that cannot be enumerated.
- Known folders that cannot be resolved.
- Files that cannot be deleted.
- Folders that cannot be deleted.

The program continues after an error. However, it does not maintain a global counter and always exits with code `0`. Therefore, an external system cannot use the exit code to distinguish between complete success, partial success, or multiple errors.

### 10. Changes made to the system

In simulation mode, it makes no changes.

In execution mode, it may:
- Remove read-only, hidden, and system attributes.
- Change folder attributes to normal.
- Delete files.
- Delete empty folders.
- Remove links or reparse points without traversing the target.

The code does not contain functions to:
- Communicate over a network.
- Download or upload information.
- Create child processes.
- Run external commands.
- Modify the Registry.
- Create services or scheduled tasks.
- Establish persistence.
- Collect credentials.
- Encrypt files.
- Physically overwrite data.
- Recover deleted items.

### 11. Logging and traceability

Activity is displayed only in the console. The program does not create log files, Windows Event Log entries, timestamps, execution identifiers, or statistical summaries.

To preserve the output, the invoking process can redirect it:

```text
RDL_Cleaner.exe --execute *> RDL_Cleaner.log
```

Redirection is not part of the internal code.

### 12. Limitations and risks

#### Permanent deletion

There is no built-in recovery, and objects are not sent to the Recycle Bin.

#### Redirected folders

The program operates on the path returned by Windows. A local deletion could propagate to OneDrive or another synchronized system depending on its configuration.

#### `MAX_PATH`

Paths are stored in `MAX_PATH` buffers. Longer paths are skipped and produce an error.

#### Locked files

Files opened with incompatible locks may remain.

#### Preservation by extension

Shortcut preservation is based only on the `.lnk` and `.url` extensions; the program does not validate their contents.

#### Prior attribute modification

Attributes may be changed even if the subsequent deletion fails.

#### Exit code

The program always returns `0`, even if partial errors occur.

#### No additional confirmation

Once started with `--execute`, it proceeds without user interaction.

#### Process context

Running it under another identity may resolve and affect a different profile.

#### Race conditions

Contents may change between enumeration, verification, and deletion, altering the result.

#### Empty-folder check

If `FindFirstFileW` fails inside `directoryIsEmpty`, the function returns `true`. It will then attempt `RemoveDirectoryW`. Windows will prevent deletion if the folder actually contains items, but an attempted deletion and possibly an error will still occur.

### 13. Behavior matrix

| Item | Downloads | Documents | Desktop |
|---|---|---|---|
| Normal file | Delete | Delete | Delete |
| `.lnk` file | Delete | Delete | Preserve |
| `.url` file | Delete | Delete | Preserve |
| Subfolder | Traverse | Traverse | Traverse |
| Subfolder empty after processing | Delete | Delete | Delete |
| Subfolder containing a preserved shortcut | Not applicable | Not applicable | Preserve |
| Reparse point | Do not traverse; attempt removal | Do not traverse; attempt removal | Do not traverse; attempt removal |
| Root folder | Preserve | Preserve | Preserve |

### 14. Minimum audit tests

- Run without arguments and confirm that no changes are made.
- Run with `--execute` in a test profile.
- Test normal files in all three folders.
- Test read-only, hidden, and system files.
- Test files locked by another process.
- Test `.lnk` and `.url` files on the Desktop.
- Test uppercase and lowercase extensions.
- Test shortcuts inside Desktop subfolders.
- Test empty and non-empty subfolders.
- Test symbolic links and junctions.
- Test paths longer than `MAX_PATH`.
- Test redirected or synchronized folders.
- Test with insufficient permissions.
- Test as a standard user and with elevation.
- Test under another account and under `SYSTEM`.
- Change the contents while the program is running.
- Confirm that all three root folders remain.
- Confirm that errors do not stop the remaining items.
- Confirm that the current exit code remains `0` when partial errors occur.
- Record the hash of the exact binary submitted for approval.

### 15. Approval considerations

Before authorizing use, confirm:
- The identity that will run the process.
- The actual paths detected on the target computers.
- The effect on OneDrive and redirected folders.
- Formal acceptance of permanent deletion.
- Correct preservation of shortcuts.
- The mechanism used to retain execution evidence.
- The handling of locked files and partial errors.
- Review of the source code and the approved binary hash.

Because it performs recursive data deletion, deployment must be authorized, documented, and limited to approved computers and scenarios.

### 16. Audit summary

RDL Cleaner is a local utility with no network connectivity or persistence mechanisms. It traverses the Downloads, Documents, and Desktop folders of the profile associated with the process. Its default mode is simulation, and deletion requires `--execute`.

It preserves Desktop shortcuts by extension, does not traverse reparse points, and continues after individual errors. Its main risks are permanent deletion, operation on redirected folders, the absence of persistent logging, the `MAX_PATH` limit, the non-representative exit code, and dependence on the execution security context.
