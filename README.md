# 4d-plugin-uti-tools
Utility functions to handle UTI on Mac

### Platform

| carbon | cocoa | win32 | win64 |
|:------:|:-----:|:---------:|:---------:|
||<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|

### Version

<img width="32" height="32" src="https://user-images.githubusercontent.com/1725068/73986501-15964580-4981-11ea-9ac1-73c5cee50aae.png"> <img src="https://user-images.githubusercontent.com/1725068/73987971-db2ea780-4984-11ea-8ada-e25fb9c3cf4e.png" width="32" height="32" />

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
