![version](https://img.shields.io/badge/version-16%2B-8331AE)
![platform](https://img.shields.io/static/v1?label=platform&message=osx-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-UTI)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-UTI/total)

# 4d-plugin-UTI

This plugin gives 4D methods direct access to macOS's Uniform Type Identifier (UTI) system, LaunchServices, Quick Look, and NSWorkspace/AppKit — letting you convert between UTIs and OS types, MIME types, and file extensions; compare and check conformance between UTIs; look up a type's declaration, description, icon, or default application; inspect and decompose file paths; open a document with a specific application; and generate a Quick Look thumbnail for a file. Text-returning commands give you plain `Text`; image-returning commands give you a `Picture` encoded as TIFF.

---

## Summary table

| Command | Returns | Purpose |
|---|---|---|
| [PATH GET COMPONENTS](#path-get-components) | *(none)* | Splits a path into an array of its individual components |
| [PATH Get name](#path-get-name) | Text | Splits a path's last component into name, base name, and extension |
| [PATH Get directory path](#path-get-directory-path) | Text | Returns the parent directory of a path |
| [UTI To ostype](#uti-to-ostype) | Text | Converts a UTI to a classic OS type (4-character code) |
| [UTI To mime](#uti-to-mime) | Text | Converts a UTI to a MIME type |
| [UTI To extension](#uti-to-extension) | Text | Converts a UTI to a filename extension |
| [UTI From ostype](#uti-from-ostype) | Text | Converts an OS type to a UTI |
| [UTI From mime](#uti-from-mime) | Text | Converts a MIME type to a UTI |
| [UTI From extension](#uti-from-extension) | Text | Converts a filename extension to a UTI |
| [UTI Equal](#uti-equal) | Longint | Tests whether two UTIs are the same type |
| [UTI Conforms to](#uti-conforms-to) | Longint | Tests whether one UTI conforms to (is a subtype of) another |
| [UTI Get declaration](#uti-get-declaration) | Text | Returns a UTI's declaration as an XML property list |
| [UTI Get localized description](#uti-get-localized-description) | Text | Returns NSWorkspace's localized description of a type |
| [UTI Get icon](#uti-get-icon) | Picture | Returns the system icon registered for a UTI |
| [UTI Get application](#uti-get-application) | Text | Returns the path of the default application for a content type |
| [UTI Get description](#uti-get-description) | Text | Returns the UTI declaration's own description string |
| [PATH Get uti](#path-get-uti) | Text | Returns the UTI of an existing file on disk |
| [PATH OPEN WITH APPLICATION](#path-open-with-application) | *(none)* | Opens a document with a specific application bundle |
| [PATH Get thumbnail](#path-get-thumbnail) | Picture | Generates a Quick Look thumbnail for a file |

**Platforms:** macOS only. The source provided contains only Objective-C++/Cocoa code (Core Services `UTType`, LaunchServices, AppKit `NSWorkspace`/`NSImage`, Quick Look) with no `#if VERSIONWIN` branch anywhere in the file — as delivered, every command in this plugin is macOS-only. If a Windows build of this plugin exists, it isn't part of the source reviewed here.

---

## Requirements & platform notes

- **macOS only**, as above — there is no Windows code path in this source to document.
- **Every parameter is mandatory.** No command in this source reads a parameter conditionally or supplies a default for an omitted one — every parameter is read unconditionally, so omitting one in your 4D call is not a supported "leave it out for a default" pattern.
- **Failures return an empty result, not a 4D error.** Every `Text`-returning command falls back to an empty string when the underlying macOS API can't resolve the input (unrecognized UTI, missing declaration, no default app, etc.) — none of them raise a 4D-visible error. Check for `""`, don't wrap these calls expecting an `ON ERR CALL` to fire.
- **Picture results are TIFF.** `UTI Get icon` and `PATH Get thumbnail` both encode their result as TIFF before wrapping it as a 4D `Picture`. 4D itself abstracts over this, but if you re-export the raw bytes elsewhere, it's TIFF, not PNG or JPEG.
- **Declared thread-safe, but calls AppKit.** `manifest.json` marks all 19 commands `threadSafe: true`. Several implementations (`UTI Get icon`, `UTI Get localized description`, `PATH Get uti`, `PATH OPEN WITH APPLICATION`) call `NSWorkspace`, `NSImage`, or `NSBundle` — AppKit APIs that Apple has traditionally documented as most reliable when called from the main thread. If you see intermittent instability calling these from multiple concurrent 4D processes, this is a likely contributing factor, not necessarily a bug in your 4D code.

---

## PATH GET COMPONENTS

### Syntax

```4d
PATH GET COMPONENTS ( path ; components )
```

| Parameter | Type | Description |
|---|---|---|
| `path` | Text | The file or folder path to split into components. |
| `components` | Array Text | Passed by reference; filled with each component of `path`, in order. The leading root separator (`/`) is excluded from the array. |

*(No return value — this command doesn't produce a `Result`.)*

### Description

Splits a path into its individual path components using macOS's own `pathComponents` parsing (not manual string splitting), so it correctly handles the same edge cases Apple's own APIs do.

One behavior worth verifying yourself before relying on it: the array is initialized to size 1 before any components are appended. Depending on the exact semantics of the array-writing helper (not fully visible in the reviewed source), this may leave an empty element ahead of the real components. If your code assumes `components{1}` is the first real path segment, verify that against an actual test call — don't assume index alignment without checking, and prefer iterating the array with `For each` rather than indexing by a fixed offset if you can.

### Example

```4d
ARRAY TEXT($components;0)
PATH GET COMPONENTS("/Users/miyako/Documents/report.pdf";$components)

For each($component;$components)
    ALERT($component)
End for each
```

---

## PATH Get name

### Syntax

```4d
PATH Get name ( path ; baseName ; extension ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `path` | Text | The file or folder path to inspect. |
| `baseName` | Text | Passed by reference; filled with the last path component *without* its extension. |
| `extension` | Text | Passed by reference; filled with the last path component's extension (no leading dot). |
| Result | Text | The full last path component, including its extension. |

### Description

`baseName` and `extension` are output parameters — they're written by the command, not read from. All three values are derived purely from the last path component; no filesystem access occurs, so `path` doesn't need to exist on disk.

### Example

```4d
C_TEXT($base;$ext)
$name:=PATH Get name("/Users/miyako/Documents/report.pdf";$base;$ext)
 // $name = "report.pdf" ; $base = "report" ; $ext = "pdf"
```

---

## PATH Get directory path

### Syntax

```4d
PATH Get directory path ( path ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `path` | Text | The file or folder path to inspect. |
| Result | Text | The parent directory of `path`, or an empty string if `path` has no parent component. |

### Description

Purely lexical — like `PATH Get name`, no filesystem access occurs, so this works on paths that don't exist on disk.

### Example

```4d
$dir:=PATH Get directory path("/Users/miyako/Documents/report.pdf")
 // $dir = "/Users/miyako/Documents"
```

---

## UTI To ostype

### Syntax

```4d
UTI To ostype ( uti ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `uti` | Text | A Uniform Type Identifier, e.g. `"public.text"`. |
| Result | Text | The classic OS type (4-character code) registered for `uti`, or an empty string if none is registered. |

### Description

Many modern UTIs (especially ones without a legacy Mac Classic heritage) simply have no OS type — an empty result is normal and expected, not an error condition.

### Example

```4d
$ostype:=UTI To ostype("public.text")
```

---

## UTI To mime

### Syntax

```4d
UTI To mime ( uti ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `uti` | Text | A Uniform Type Identifier. |
| Result | Text | The MIME type registered for `uti`, or an empty string if none is registered. |

### Example

```4d
$mime:=UTI To mime("public.png")
 // $mime = "image/png"
```

---

## UTI To extension

### Syntax

```4d
UTI To extension ( uti ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `uti` | Text | A Uniform Type Identifier. |
| Result | Text | The filename extension (no leading dot) registered for `uti`, or an empty string if none is registered. |

### Example

```4d
$ext:=UTI To extension("public.jpeg")
 // $ext = "jpeg"
```

---

## UTI From ostype

### Syntax

```4d
UTI From ostype ( ostype ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `ostype` | Text | A classic OS type (4-character code). |
| Result | Text | The preferred UTI for `ostype`, or an empty string if macOS can't derive one. |

### Example

```4d
$uti:=UTI From ostype("TEXT")
```

---

## UTI From mime

### Syntax

```4d
UTI From mime ( mime ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `mime` | Text | A MIME type string, e.g. `"text/html"`. |
| Result | Text | The preferred UTI for `mime`, or an empty string if macOS can't derive one. |

### Example

```4d
$uti:=UTI From mime("text/html")
```

---

## UTI From extension

### Syntax

```4d
UTI From extension ( extension ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `extension` | Text | A filename extension, with or without a leading dot; matched case-insensitively. |
| Result | Text | The preferred UTI for `extension`. |

### Description

Three extensions are special-cased ahead of the general system lookup, returning fixed iWork package UTIs rather than whatever the system's own `UTTypeCreatePreferredIdentifierForTag` would otherwise infer:

| `extension` | Result |
|---|---|
| `key` | `com.apple.iwork.keynote.sffkey` |
| `pages` | `com.apple.iwork.pages.sffpages` |
| `numbers` | `com.apple.iwork.numbers.sffnumbers` |

Any other extension falls through to the standard system lookup, which returns an empty string if the extension is unregistered.

### Example

```4d
 // From the plugin's own test file (TEST.4dm)
$uti:=UTI From extension("xlsx")
```

```4d
$uti:=UTI From extension("key")
 // $uti = "com.apple.iwork.keynote.sffkey"
```

---

## UTI Equal

### Syntax

```4d
UTI Equal ( uti1 ; uti2 ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `uti1` | Text | A Uniform Type Identifier. |
| `uti2` | Text | A Uniform Type Identifier. |
| Result | Longint | `1` if `uti1` and `uti2` identify the same type, otherwise `0`. |

### Example

```4d
$same:=UTI Equal("public.jpeg";"public.png")
 // $same = 0
```

---

## UTI Conforms to

### Syntax

```4d
UTI Conforms to ( uti ; conformsToUTI ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `uti` | Text | The Uniform Type Identifier to test. |
| `conformsToUTI` | Text | The candidate ancestor/base type. |
| Result | Longint | `1` if `uti` conforms to (is the same as, or a subtype of) `conformsToUTI`, otherwise `0`. |

### Example

```4d
 // From the plugin's own test file (TEST.4dm)
$r:=UTI Conforms to ("public.png";"public.image")
 // $r = 1
```

---

## UTI Get declaration

### Syntax

```4d
UTI Get declaration ( uti ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `uti` | Text | A Uniform Type Identifier. |
| Result | Text | The type's full declaration dictionary, serialized as an XML property list, or an empty string if `uti` has no declaration. |

### Description

This is macOS's raw declaration dictionary (the same data an app's `Info.plist` `UTExportedTypeDeclarations`/`UTImportedTypeDeclarations` entries populate), serialized to XML text. Parse it with 4D's XML or property-list handling if you need individual keys out of it rather than the whole document.

### Example

```4d
$xml:=UTI Get declaration("public.png")
```

---

## UTI Get localized description

### Syntax

```4d
UTI Get localized description ( uti ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `uti` | Text | A Uniform Type Identifier. |
| Result | Text | `NSWorkspace`'s localized, user-facing description of the type, in the current user's language, or an empty string if none is available. |

### Description

This queries `NSWorkspace` directly, which is a different underlying path from [`UTI Get description`](#uti-get-description) (which reads the type's own declaration). The two can return different text for the same UTI — `NSWorkspace`'s answer can depend on what's currently registered/installed on the machine, not just the type's static declaration.

### Example

```4d
$description:=UTI Get localized description("public.png")
```

---

## UTI Get icon

### Syntax

```4d
UTI Get icon ( uti ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `uti` | Text | A Uniform Type Identifier. |
| Result | Picture | The system icon registered for `uti`, rendered at up to 1024×1024 and encoded as TIFF. |

### Description

Renders whatever icon macOS has registered for the type via `NSWorkspace`, at up to 1024×1024 pixels (the actual resolution depends on what the source icon natively provides — macOS will scale a smaller icon up to fill the requested size).

**Note on unrecognized types:** in the version of the source reviewed and fixed here, an unrecognized or unregistered `uti` now safely returns an empty/unset `Picture`. This depends on a guard added during code review (checking that both the icon and its rendered image are non-`nil`/non-`NULL` before proceeding) — if you're running an older build of this plugin that predates that fix, an unrecognized `uti` could instead crash the plugin. Confirm you're on a build that includes the fix before relying on the graceful-empty-result behavior.

### Example

```4d
$icon:=UTI Get icon("public.folder")
```

---

## UTI Get application

### Syntax

```4d
UTI Get application ( uti ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `uti` | Text | A content type (UTI). |
| Result | Text | The filesystem path of the application macOS would use by default to open this content type, or an empty string if no default is registered. |

### Example

```4d
 // From the plugin's own test file (TEST.4dm)
$application:=UTI Get application ("public.text")
```

---

## UTI Get description

### Syntax

```4d
UTI Get description ( uti ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `uti` | Text | A Uniform Type Identifier. |
| Result | Text | The description string from the type's own declaration, or an empty string if none is declared. |

### Description

Reads the type's static declaration directly (Core Services `UTType`), as opposed to [`UTI Get localized description`](#uti-get-localized-description), which asks `NSWorkspace`. See that command's description for how the two can diverge.

### Example

```4d
$description:=UTI Get description("public.png")
```

---

## PATH Get uti

### Syntax

```4d
PATH Get uti ( path ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `path` | Text | The path of an existing file. |
| Result | Text | The UTI macOS determines for the file at `path`, or an empty string if the file doesn't exist or its type can't be determined. |

### Description

Unlike the `UTI ...` conversion commands, this one does touch the filesystem — it asks `NSWorkspace` to inspect the actual file. The underlying error reason (file missing vs. permission denied vs. some other failure) is retrieved by the implementation but not passed back to 4D — every failure mode collapses to the same empty-string result, so you can't distinguish "doesn't exist" from "exists but type unknown" from this command's result alone.

### Example

```4d
$uti:=PATH Get uti("/Users/miyako/Documents/report.pdf")
```

---

## PATH OPEN WITH APPLICATION

### Syntax

```4d
PATH OPEN WITH APPLICATION ( path ; applicationPath ; options )
```

| Parameter | Type | Description |
|---|---|---|
| `path` | Text | The document to open. |
| `applicationPath` | Text | The `.app` bundle to open it with. |
| `options` | Longint | A raw `NSWorkspaceLaunchOptions` bitmask, passed straight through to macOS. |

*(No return value.)*

### Description

If `applicationPath` doesn't point to a valid, loadable application bundle, this command does nothing — no document is opened, and no 4D error is raised. There's no feedback path to confirm success from 4D.

`options` is passed through unchanged to `NSWorkspace`'s underlying launch call. Its exact bit values are AppKit constants defined by Apple, not by this plugin — consult Apple's `NSWorkspaceLaunchOptions` documentation for the current set of flags and their numeric values rather than guessing; they aren't reproduced here because they aren't part of this plugin's own source.

### Example

```4d
PATH OPEN WITH APPLICATION("/Users/miyako/Documents/report.pdf";"/Applications/Preview.app";0)
```

---

## PATH Get thumbnail

### Syntax

```4d
PATH Get thumbnail ( path ; width ; height ) -> Result
```

| Parameter | Type | Description |
|---|---|---|
| `path` | Text | The path of the file to generate a thumbnail for. |
| `width` | Real | The requested thumbnail width, in pixels. |
| `height` | Real | The requested thumbnail height, in pixels. |
| Result | Picture | A Quick Look thumbnail for the file, encoded as TIFF, or an empty `Picture` if no Quick Look generator can produce one. |

### Description

Uses macOS's Quick Look thumbnailing system, so results depend on what Quick Look generators are installed and registered for the file's type — the same thumbnail you'd see in Finder. An empty result is the documented outcome when no generator applies; this command already checks for that case and returns cleanly rather than producing a broken image.

### Example

```4d
$thumbnail:=PATH Get thumbnail("/Users/miyako/Documents/report.pdf";256;256)
```

---

## Error handling & troubleshooting

- **Unrecognized or unregistered types return empty text, not a 4D error.** Every `UTI ...`/`PATH Get uti` command falls back to `""` rather than raising an error — check for an empty result explicitly rather than wrapping calls in error handling that will never fire.
- **`UTI Get icon`'s crash-safety on unknown types depends on your build including the review fix.** See [`UTI Get icon`](#uti-get-icon) above — confirm you're on a build that guards against a `nil` icon/`NULL` image before treating an unrecognized `uti` as safe to pass through.
- **`PATH Get uti` discards its own error detail.** You'll get an empty string for a missing file, a permission failure, or any other lookup failure alike — there's no way to distinguish the cause from this command's result.
- **`PATH OPEN WITH APPLICATION` fails silently on a bad `applicationPath`.** If the path isn't a valid, loadable app bundle, nothing happens and nothing is reported back to 4D — verify `applicationPath` yourself beforehand if you need to guarantee the open occurred.
- **`PATH GET COMPONENTS`'s output array may have a leading blank entry.** Verify this against a real test call in your environment before assuming index 1 is always the first real path segment (see the command's own description above for why).
- **Thread-safety is asserted in the manifest but not guaranteed by the underlying AppKit calls.** If you're calling `UTI Get icon`, `UTI Get localized description`, `PATH Get uti`, or `PATH OPEN WITH APPLICATION` from several concurrent 4D processes and see intermittent crashes, this architectural mismatch is a plausible cause worth ruling out.
- **`PATH OPEN WITH APPLICATION`'s `options` parameter isn't validated.** It's passed straight through as a raw bitmask — an out-of-range value is a macOS/AppKit concern, not something this plugin checks.

---

## Quick reference

```4d
 // UTI <-> tag conversions
$uti:=UTI From extension("pdf")
$mime:=UTI To mime($uti)
$ext:=UTI To extension($uti)

 // Comparing types
If (UTI Conforms to($uti;"public.data"))
    ALERT("It's a data type")
End if

 // Type metadata
$description:=UTI Get description($uti)
$icon:=UTI Get icon($uti)
$appPath:=UTI Get application($uti)

 // Path utilities
ARRAY TEXT($components;0)
PATH GET COMPONENTS($path;$components)
C_TEXT($base;$ext2)
$fullName:=PATH Get name($path;$base;$ext2)
$dir:=PATH Get directory path($path)

 // Files on disk
$fileUTI:=PATH Get uti($path)
$thumb:=PATH Get thumbnail($path;256;256)
PATH OPEN WITH APPLICATION($path;"/Applications/Preview.app";0)
```
