# RDL Cleaner

Herramienta de limpieza para Windows 11 desarrollada en C. El programa elimina el contenido de las carpetas **Descargas**, **Documentos** y **Escritorio** del usuario actual, conservando los accesos directos del Escritorio.

> [!WARNING]
> Este programa puede eliminar archivos de forma permanente. Antes de usar el modo de ejecución real, comprueba cuidadosamente su funcionamiento en modo de simulación y realiza pruebas en un equipo controlado.

## Estado del proyecto

**Versión:** 0.1.0.0  
**Estado:** En fase de pruebas  
**Desarrollador:** Jesus Alberto Lizarraga Rodriguez  
**Plataforma objetivo:** Windows 11

## Características

- Detecta automáticamente las carpetas conocidas de Windows mediante `SHGetKnownFolderPath`.
- Limpia el contenido de **Descargas**.
- Limpia el contenido de **Documentos**.
- Limpia el contenido del **Escritorio**.
- Conserva los accesos directos con extensiones `.lnk` y `.url` del Escritorio.
- No elimina la carpeta raíz de Descargas, Documentos o Escritorio.
- No sigue enlaces simbólicos, junctions ni otros puntos de análisis.
- Incluye un modo de simulación que permite revisar los elementos antes de eliminarlos.
- Puede incorporar metadatos de Windows PE, un icono y un manifiesto de aplicación.
- Puede compilarse con ASLR, DEP/NX y protección reforzada de la pila.
- No requiere Python, GCC ni Visual Studio Code en el equipo donde se ejecutará el archivo final.

## Comportamiento

### Modo de simulación

Al ejecutar el programa sin argumentos, solamente muestra los archivos y carpetas que serían procesados:

```powershell
.\RDL_Cleaner.exe
```

En este modo no se elimina ningún archivo.

### Modo de ejecución

Para realizar la limpieza real:

```powershell
.\RDL_Cleaner.exe --execute
```

El modo `--execute` realiza el borrado sin solicitar confirmación adicional al usuario.

> [!CAUTION]
> La eliminación es permanente. El programa actual utiliza las funciones de eliminación de archivos de Windows y no envía los elementos a la Papelera de reciclaje.

## Elementos conservados en el Escritorio

El programa conserva:

- Accesos directos de Windows: `.lnk`
- Accesos directos de Internet: `.url`
- La Papelera de reciclaje, porque normalmente es un objeto virtual de Windows y no un archivo físico dentro de la carpeta del Escritorio

Si una subcarpeta del Escritorio contiene un acceso directo conservado, esa subcarpeta también puede permanecer porque no estará vacía.

## Consideraciones sobre OneDrive

Windows puede redirigir Escritorio y Documentos a OneDrive. Como el programa usa las carpetas conocidas de Windows, puede detectar y limpiar las ubicaciones redirigidas.

Antes de usarlo en producción:

1. Confirma la ruta mostrada por el modo de simulación.
2. Comprueba si las carpetas están sincronizadas con OneDrive.
3. Prueba el comportamiento con archivos disponibles localmente y archivos disponibles únicamente en la nube.
4. Verifica las políticas de retención y recuperación de la organización.

## Requisitos para compilar

- Windows 11 de 64 bits recomendado.
- GCC mediante MinGW-w64 o WinLibs.
- `windres`, incluido normalmente con MinGW-w64.
- Python 3.8 o posterior, únicamente si se utiliza `build_pe.py`.
- Visual Studio Code es opcional y se utiliza solamente como editor.

Comprueba las herramientas con:

```powershell
gcc --version
windres --version
python --version
```

Comprueba la arquitectura del compilador:

```powershell
gcc -dumpmachine
```

Resultado recomendado:

```text
x86_64-w64-mingw32
```

Si aparece `i686-w64-mingw32`, el compilador genera ejecutables de 32 bits.

## Estructura sugerida

```text
RDL_Cleaner\
├── cleaner.c
├── build_pe.py
├── version.rc
├── version.res
├── app.manifest
├── rdl_cleaner.ico
└── README.md
```

Los archivos `version.rc`, `version.res` y `app.manifest` pueden generarse durante el proceso de compilación.

## Generación de metadatos PE

Ejecuta el generador de metadatos desde PowerShell:

```powershell
python .\build_pe.py `
    --source ".\cleaner.c" `
    --output ".\RDL_Cleaner.exe" `
    --product "RDL Cleaner" `
    --company "Jesus Alberto Lizarraga Rodriguez" `
    --version "0.1.0.0" `
    --description "Herramienta de prueba para limpieza de carpetas de usuario" `
    --copyright "Copyright (C) 2026 Jesus Alberto Lizarraga Rodriguez. Version de prueba." `
    --icon ".\rdl_cleaner.ico"
```

Si no tienes un icono `.ico`, omite el argumento:

```text
--icon ".\rdl_cleaner.ico"
```

Los metadatos son informativos. No equivalen a una firma digital y no convierten al desarrollador en un editor verificado por Windows.

## Compilación de recursos

Después de generar `version.rc` y `app.manifest`, compila los recursos:

```powershell
windres .\version.rc -O coff -o .\version.res
```

El archivo `version.rc` debe incluir el manifiesto. Por ejemplo:

```rc
1 RT_MANIFEST "app.manifest"
```

Si `RT_MANIFEST` no es reconocido, puede utilizarse su identificador numérico:

```rc
1 24 "app.manifest"
```

## Compilación con GCC

### Compilación de 32 bits

Utiliza este comando si `gcc -dumpmachine` devuelve `i686-w64-mingw32`:

```powershell
gcc cleaner.c version.res -o RDL_Cleaner.exe -O2 -s -Wall -Wextra -municode -fstack-protector-strong "-Wl,--dynamicbase" "-Wl,--nxcompat" -lshell32 -lole32 -luuid
```

### Compilación recomendada de 64 bits

Utiliza este comando si `gcc -dumpmachine` devuelve `x86_64-w64-mingw32`:

```powershell
gcc cleaner.c version.res -o RDL_Cleaner.exe -O2 -s -Wall -Wextra -municode -fstack-protector-strong "-Wl,--dynamicbase" "-Wl,--nxcompat" "-Wl,--high-entropy-va" -lshell32 -lole32 -luuid
```

Las opciones `-Wl,...` están entre comillas porque PowerShell puede interpretar las comas como separadores de argumentos.

## Opciones de compilación

- `-O2`: habilita optimizaciones del compilador.
- `-s`: elimina símbolos innecesarios del ejecutable final.
- `-Wall -Wextra`: habilita advertencias adicionales.
- `-municode`: utiliza `wmain` y argumentos Unicode.
- `-fstack-protector-strong`: agrega protección reforzada de la pila.
- `--dynamicbase`: habilita ASLR.
- `--nxcompat`: declara compatibilidad con DEP/NX.
- `--high-entropy-va`: habilita ASLR de alta entropía para ejecutables de 64 bits.
- `-lshell32`: enlaza las funciones de Windows Shell.
- `-lole32`: enlaza las funciones OLE y COM utilizadas.
- `-luuid`: enlaza los identificadores de las carpetas conocidas de Windows.

## Verificación del resultado

### Comprobar que existe el ejecutable

```powershell
Get-Item .\RDL_Cleaner.exe
```

### Consultar los metadatos

```powershell
(Get-Item .\RDL_Cleaner.exe).VersionInfo | Format-List
```

### Calcular el hash SHA-256

```powershell
Get-FileHash .\RDL_Cleaner.exe -Algorithm SHA256
```

Guarda el hash de cada versión aprobada para poder verificar que el archivo no fue modificado.

### Consultar dependencias

```powershell
objdump -p .\RDL_Cleaner.exe | Select-String "DLL Name"
```

Si aparecen DLL como `libgcc_s_*.dll`, comprueba que el ejecutable pueda funcionar en un equipo limpio. Si fuera necesario, prueba la opción `-static-libgcc`, pero vuelve a revisar el tamaño, las dependencias y el resultado del análisis de seguridad.

### Consultar mitigaciones PE

```powershell
objdump -p .\RDL_Cleaner.exe | Select-String "DllCharacteristics|HIGH_ENTROPY_VA|DYNAMIC_BASE|NX_COMPAT"
```

## Errores comunes

### `undefined reference to FOLDERID_Downloads`

Agrega `-luuid` al comando de compilación:

```text
-lshell32 -lole32 -luuid
```

### `Falta un argumento en la lista de parámetros`

PowerShell está interpretando las comas de las opciones del enlazador. Usa comillas:

```powershell
"-Wl,--dynamicbase" "-Wl,--nxcompat"
```

### `stray '\302' in program`

El archivo contiene un espacio Unicode invisible `U+00A0`, normalmente introducido al copiar código desde una página web o un chat.

Puedes reemplazarlo con PowerShell:

```powershell
$content = Get-Content .\cleaner.c -Raw
$content = $content.Replace([char]0x00A0, ' ')
Set-Content .\cleaner.c -Value $content -Encoding utf8
```

### Advertencia `UNICODE redefined`

Utiliza esta protección al principio de `cleaner.c`:

```c
#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif
```

### Advertencia sobre `#pragma comment`

Los pragmas de enlace son propios de MSVC. Para mantener compatibilidad:

```c
#ifdef _MSC_VER
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#endif
```

GCC recibe esas librerías mediante los argumentos `-lshell32`, `-lole32` y `-luuid`.

## Seguridad y pruebas

Antes de distribuir una versión:

1. Revisa el código fuente.
2. Compila desde un entorno confiable.
3. Ejecuta primero el modo de simulación.
4. Prueba en una máquina virtual o equipo de laboratorio.
5. Comprueba exactamente qué rutas detecta.
6. Verifica el resultado con una cuenta de usuario de prueba.
7. Calcula y registra el hash SHA-256.
8. Envía cualquier falso positivo al proveedor de seguridad correspondiente.
9. Firma digitalmente la versión definitiva cuando exista un certificado autorizado.
10. Distribuye únicamente el binario validado.

No se recomienda:

- Desactivar Microsoft Defender.
- Crear exclusiones amplias para carpetas de usuario.
- Ofuscar el ejecutable para evitar el análisis.
- Comprimirlo con herramientas que dificulten su inspección.
- Ejecutarlo en producción sin una prueba previa.

## Firma digital

Los metadatos PE no sustituyen una firma digital. Para mostrar un editor verificado se necesita un certificado válido de firma de código y `SignTool`.

Ejemplo general:

```powershell
signtool sign /a /fd SHA256 /tr "URL_DEL_SERVIDOR_DE_TIMESTAMP" /td SHA256 .\RDL_Cleaner.exe
```

Verificación:

```powershell
signtool verify /pa /v .\RDL_Cleaner.exe
```

La firma debe aplicarse después de completar la compilación y las pruebas. Cualquier modificación posterior cambia el archivo y puede invalidar la firma.

## Empaquetado para Microsoft Intune

El ejecutable puede empaquetarse como aplicación Win32 en formato `.intunewin` mediante `IntuneWinAppUtil.exe`.

Ejemplo:

```powershell
.\IntuneWinAppUtil.exe `
    -c "C:\Intune\RDL_Cleaner\Source" `
    -s "RDL_Cleaner.exe" `
    -o "C:\Intune\RDL_Cleaner\Output" `
    -q
```

Comando de ejecución sugerido en Intune:

```text
RDL_Cleaner.exe --execute
```

Para un despliegue administrado, se recomienda:

- Ejecutar en contexto de sistema solamente después de validar qué perfil de usuario será limpiado.
- Crear un archivo de registro o marcador de finalización.
- Configurar una regla de detección en Intune.
- Asignar primero el paquete a un grupo pequeño de pruebas.
- Documentar y aprobar el comportamiento con el equipo de seguridad.

> [!IMPORTANT]
> Una aplicación ejecutada como `SYSTEM` puede resolver las carpetas conocidas del perfil de sistema y no necesariamente las del empleado conectado. Antes del despliegue mediante Intune, valida el contexto de ejecución y adapta el programa para identificar explícitamente el perfil objetivo.

## Flujo de publicación recomendado

```text
cleaner.c
   +
build_pe.py
   |
   v
version.rc + app.manifest
   |
   v
windres -> version.res
   |
   v
gcc -> RDL_Cleaner.exe
   |
   v
Pruebas y revisión
   |
   v
Firma digital
   |
   v
Empaquetado .intunewin
   |
   v
Despliegue de prueba
```

## Limitaciones conocidas

- Los archivos abiertos o bloqueados por otro proceso pueden no eliminarse.
- El programa puede afectar carpetas redirigidas o sincronizadas con OneDrive.
- El borrado actual no utiliza la Papelera de reciclaje.
- Los accesos directos se conservan según su extensión, no mediante validación del contenido interno.
- Una carpeta del Escritorio que contenga un acceso directo conservado puede permanecer.
- El programa no sustituye una solución institucional de borrado seguro, retención o administración de datos.
- La ejecución como `SYSTEM` requiere tratamiento especial para seleccionar correctamente el perfil del usuario.

## Licencia y responsabilidad

Proyecto en fase de pruebas para fines de desarrollo y evaluación. Úsalo únicamente en equipos y cuentas para los cuales tengas autorización. El desarrollador y los operadores deben validar el alcance de la limpieza antes de cualquier despliegue.
