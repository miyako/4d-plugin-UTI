![version](https://img.shields.io/badge/version-16%2B-8331AE)
![platform](https://img.shields.io/static/v1?label=platform&message=osx-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-UTI)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-UTI/total)

# 4d-plugin-UTI
Utility functions to handle UTI on Mac

To use on v16 or v17, move manifest.json to contents.

**Process path components**
 
```4d
$path:=Get 4D folder(Current resources folder)+"email.key"
ARRAY TEXT($components;0)
PATH GET COMPONENTS ($path;$components)
$name:=PATH Get name ($path;$nameWithoutExtension;$extension)
$directoryPath:=PATH Get directory path ($path)
```

**Inspect UTI**

```4d
$path:=Get 4D folder(Current resources folder)+"email.key"
$uti:=PATH Get uti ($path)
$declaration:=UTI Get declaration ($uti)
$description:=UTI Get description ($uti)
$description_l:=UTI Get localized description ($uti)//same thing
```

**Images**

```4d
$path:=Get 4D folder(Current resources folder)+"email.key"
$uti:=PATH Get uti ($path)
$icon:=UTI Get icon ($uti)
$thumbnail:=PATH Get thumbnail ($path;64;64)
```

**Open with application**

```4d
$path:=Get 4D folder(Current resources folder)+"email.key"
$uti:=PATH Get uti ($path)
$application:=UTI Get application ($uti)//full path
PATH OPEN WITH APPLICATION ($path;$application)
```

**Conversion**

```4d
$uti:=UTI From extension ("key")
$extension:=UTI To extension ($uti)
$uti:=UTI From mime ("image/png")
$mime:=UTI To mime ($uti)
$uti:=UTI From ostype ("TEXT")
$ostype:=UTI To ostype ($uti)
```

**Comparision**

```4d
$result:=UTI Conforms to ("public.png";"public.image")
$result:=UTI Equal ("public.png";"public.image")
```
