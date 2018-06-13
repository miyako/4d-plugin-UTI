# 4d-plugin-uti-tools
Utility functions to handle UTI on OS X.

### Platform

| carbon | cocoa | win32 | win64 |
|:------:|:-----:|:---------:|:---------:|
|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|||

### Version

<img src="https://cloud.githubusercontent.com/assets/1725068/18940649/21945000-8645-11e6-86ed-4a0f800e5a73.png" width="32" height="32" /> <img src="https://cloud.githubusercontent.com/assets/1725068/18940648/2192ddba-8645-11e6-864d-6d5692d55717.png" width="32" height="32" /> <img src="https://user-images.githubusercontent.com/1725068/41266195-ddf767b2-6e30-11e8-9d6b-2adf6a9f57a5.png" width="32" height="32" />

### Releases

[1.2](https://github.com/miyako/4d-plugin-uti-tools/releases/tag/1.2)

![preemption xx](https://user-images.githubusercontent.com/1725068/41327179-4e839948-6efd-11e8-982b-a670d511e04f.png)

except 

* ``UTI Get icon`` (``PA_CreatePicture``)

* ``PATH Get thumbnail`` (``PA_CreatePicture``)

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
$path:=Get 4D folder(Current resources folder)+"email.key"
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
