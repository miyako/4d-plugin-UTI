# 4d-plugin-uti-tools
Utility functions to handle UTI on OS X

**Process path components**
 
```
$path:=Get 4D folder(Current resources folder)+"email.key"
ARRAY TEXT($components;0)
PATH GET COMPONENTS ($path;$components)
$name:=PATH Get name ($path;$nameWithoutExtension;$extension)
$directoryPath:=PATH Get directory path ($path)
```

**Inspect UTI**

```
$uti:=PATH Get uti ($path)
$declaration:=UTI Get declaration ($uti)
$description:=UTI Get description ($uti)
$description_l:=UTI Get localized description ($uti)//same thing
```

**Images**

```
$path:=Get 4D folder(Current resources folder)+"email.key"
$uti:=PATH Get uti ($path)
$icon:=UTI Get icon ($uti)
$thumbnail:=PATH Get thumbnail ($path;64;64)
```

**Open with application**

```
$path:=Get 4D folder(Current resources folder)+"email.key"
$uti:=PATH Get uti ($path)
$application:=UTI Get application ($uti)//full path
PATH OPEN WITH APPLICATION ($path;$application)
```

**Conversion**

```
$uti:=UTI From extension ("key")
$extension:=UTI To extension ($uti)
$uti:=UTI From mime ("image/png")
$mime:=UTI To mime ($uti)
$uti:=UTI From ostype ("TEXT")
$ostype:=UTI To ostype ($uti)
```

**Comparision**

```
$result:=UTI Conforms to ("public.png";"public.image")
$result:=UTI Equal ("public.png";"public.image")
```
