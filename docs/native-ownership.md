# Native ownership audit

## Scope and provenance

Baseline: `6fe46ba4045007c082ce93db2b06aedae2e954da`.
Guidance: `miyako/skills`, `4d-skills/skills/4dplugin/SKILL.md`,
merged revision `e6b492fcb81e5e1a7f7db723594b420e63bf6eb9` (PR #71).

The actual bundled `4D Plugin API/4DPluginAPI.c` has SHA-256
`5bac571c5c4cafd280fa7f23d775458a23701287e8735be27d4a2190002f4e3f`
and Git blob `62f745e91d81ef2c3c901c656ea8e175d3b173e8`. This is also the
SDK implementation blob at `miyako/4d-plugin-poppler` revision
`39a961c8fbe543640fcb3c53d483fb90bbe22586`. UTI history last moved it in
`7292ea470fa0377bcbb8f036cc12d456389a9f6b`; earlier history includes
`d3e99a3392dce04909ac9f951289947977c851b6` ("bump to v18").
Neither that commit title nor the SDK's v13/v14 comments identify an
independently verified upstream SDK release. The SDK remains unchanged.

## Complete tracked native inventory and reachability

| Surface | Reachability and ownership findings |
| --- | --- |
| `4DPlugin-UTI.cpp`, `.h` | Actual `PluginMain` dispatches selectors 1-19. Scalar strings are copied into `C_TEXT`; outputs use the SDK content setter. Cocoa/CF copy/create strings are released after copying; borrowed framework strings are not released. Picture outputs hand their created descriptor to the result. No first-party callback, JSON Parse, pointer, property/collection getter, or ignored owning command-result callsites here. |
| `Classes/ARRAY_TEXT.cpp`, `.h` | Compiled by Xcode; included by the entry header. `PATH_GET_COMPONENTS` (selector 1) is the only live array command. Input descriptors are copied into independent C++ strings. Each output element gets a separate `PA_CreateUnistring` allocation transferred once, without local disposal. Replacement/shrink/clear are owner operations. No Poppler dispose-after-transfer defect found. |
| `Classes/C_TEXT.cpp`, `.h` | Compiled by Xcode and used by scalar commands. Owns an independent C++ UTF-16 string, disallows shallow class copying, and copies input before mutation. Parameter/result setters call `PA_SetUnistring`, not a shallow scalar-variable assignment. UTF-8 conversion uses explicit code-unit/byte lengths. |
| `Classes/C_LONGINT.cpp`, `.h`; `C_REAL.cpp`, `.h` | Compiled by Xcode; primitive scalar wrappers, no native string ownership. |
| `Classes/C_BLOB.cpp`, `.h` | Tracked dormant wrapper, not compiled/included by the production projects. Copies handles to bytes; hex/base64 conversions pass independent C++ text to `C_TEXT`. No `CPicture` class is present anywhere in this repository. Unrelated BLOB allocation-failure behavior is outside this string-ownership fix. |
| `Classes/C_TYPES.h` | Shared types and platform includes; no ownership operations. |
| `support/4DPlugin-JSON.cpp`, `.h` | Tracked dormant helper, neither compiled nor included by either project. Reviewed all setters, getters, and stringify. Four string setter overloads and the picture setter overwrite initially allocated empties. Successful scalar/mismatched property results need clearing; collection success is an owning handoff. These confirmed defects are fixed. `ob_stringify` already clears its independently duplicated input and owning command result after copying. Object/collection/picture setters retain their existing consuming-input semantics; no speculative retain changes. |
| `4DPluginAPI.c`, `.h`; `EntryPoints.h`, `Flags.h`, `PrivateTypes.h`, `PublicTypes.h`, `4DPluginAPI.def` | Bundled SDK boundary, inspected for signatures, layout, allocation, transfer, getter, clear, command and dispatcher contracts; excluded from fixes. Both projects compile this implementation. All SDK files and their internal helpers remain untouched. |
| `include/json/{allocator,assertions,autolink,config,features,forwards,json,reader,value,version,writer}.h` | Third-party jsoncpp headers; not first-party ownership fixes. Static jsoncpp libraries are also excluded. |

`UTI.xcodeproj/project.pbxproj` compiles the entry, full SDK, `ARRAY_TEXT`,
`C_TEXT`, `C_LONGINT`, and `C_REAL`, as Objective-C++. The `.vcxproj` lists
only the entry and SDK as compile items, with wrapper/support include paths
but no wrapper or JSON compile items. The entry unconditionally imports
macOS Quartz: this audit does not certify that historical Windows project.
The solution, filters, workspace and shared scheme add no native translation
units or alternative dispatcher.

Both `manifest.json` and
`test/Plugins/UTI.bundle/Contents/Resources/manifest.json` contain the same 19
commands in dispatcher order. Only `PATH GET COMPONENTS(&T;&Y)` advertises
an array; `&Y` describes array capability, not a transfer contract.
Scalar-only and dormant sources were not excluded from the inventory.
Committed bundles/archives are historical binaries, not additional readable
native source or evidence that these source changes have shipped.

## Verified SDK contracts

Line numbers below refer to the unchanged bundled implementation.

* `PA_CreateVariable` (3920-4067) allocates an empty string or picture for
  those kinds. `PA_SetStringVariable` (4254-4259) and the picture equivalent
  shallow-assign and do not release previous content. Setters now start
  with `eVK_Undefined` and transfer their independently created value to
  that temporary variable; normal `PA_ClearVariable` cleanup is preserved.
* `PA_GetObjectProperty` (636-652) and `PA_GetCollectionElement` (586-601)
  explicitly return owning variables to clear after use. Scalar getters
  now copy first, then clear every successful result including type
  mismatches. `ob_get_c` transfers the successful collection owner to its
  caller, who must `PA_DisposeCollection`; it clears mismatches only.
  No production consumer of this dormant helper exists to update.
* Failed property reads can leave the SDK's local result unspecified.
  Each helper checks `PA_GetLastError` immediately, releases its own key,
  and throws `std::runtime_error` without examining or clearing that output.
  This intentionally makes engine errors explicit; normal absent-property,
  type-mismatch and null-object behavior remains unchanged.
* `PA_GetStringInArray` (4738-4755) returns a borrowed descriptor.
  `PA_SetStringInArray` (4933-4948) accepts Unicode-array type, non-null
  array data, index zero through the inclusive count, and non-null
  descriptor pointer. It disposes the old element and shallow-transfers
  the new allocation. Rejected calls retain caller ownership.
* `PA_SetUnistring` (289-299), also used by `PA_ReturnString`, forwards a
  null-terminated UTF-16 content buffer to the engine setter. It is not
  `PA_SetStringVariable` or the array descriptor-transfer implementation.
* `PA_ExecuteCommandByID` (5778-5815) synchronously passes argument addresses
  and returns a separate variable. `ob_stringify`'s existing argument/result
  cleanup is preserved. No first-party JSON Parse call exists to repair.

## Reproducible non-release regression

On macOS with the existing Xcode command-line tools:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests -p test_ownership.py -v
```

`tests/test_ownership.py` compiles the **actual entry, every first-party
wrapper/helper translation unit, and full unchanged bundled SDK**, including
dormant sources, against `tests/ownership_host.cpp`. It does not substitute
SDK snippets or replace first-party functions. Only the engine callback is
shimmed. ASan and UBSan are enabled; explicit string, handle, and reference
balances replace unavailable macOS LeakSanitizer. Engine string capacities
are `(UTF-16 code units + terminator) * sizeof(PA_Unichar)` bytes, and reported
lengths exclude termination. Both allocated-empty and null-empty engine
representations are exercised. CoreFoundation performs the real macOS
UTF-8/UTF-16 conversions, not a conversion stub.

The model copies property string values, retains object/collection/picture
references, and models the already-string `EX_VARIABLE_TO_STRING` result
as borrowed. It rejects unexpected engine selectors. Failed property output
is poisoned rather than manufactured into a valid owning variable.

The same harness can target an immutable baseline tree without changing
the harness:

```sh
UTI_OWNERSHIP_SOURCE=/absolute/path/to/baseline-tree \
  PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests -p test_ownership.py -v
```

At the baseline above, seven of eight named tests fail: allocated-empty
setter leaks (five lost strings in the repeated setter case), the overwritten
picture owner, four property-getter owner leaks, and failed-output access
detected by UBSan. Null-empty string setters correctly do not reproduce an
allocated-empty leak; the live array/dispatcher test passes at baseline.
The fixed sources pass all eight tests in both empty modes.

Coverage includes exact ASCII, BMP and supplementary text, zero-length
results, setter replacement, owning mismatched strings/references, copied
scalar output surviving owner cleanup, collection handoff surviving source
disposal, synchronous stringify input/result cleanup, immediate getter
failure checks, absent properties, independent array transfers, allocator
churn, borrowed input copies, replacement, shrink, clear, rejected type/index/
data/null-descriptor calls, and the actual selector-1 dispatcher.

## Limits and safety boundary

This is synthetic macOS engine-boundary evidence, not execution in real 4D
or Windows and not reproduction/certification of the original Poppler crash.
Engine-only string conversion/copy/retain behavior is modeled explicitly,
not inferred to certify arbitrary engine internals.

SDK resize-allocation-failure count corruption is not certified by normal
array tests and is deferred; the SDK remains untouched. Application-path/
picture-data output ownership and pointer metadata ownership remain
unresolved SDK-only questions with no relevant first-party callsites here.
Unrelated OOM handling, asynchronous redesign, framework hardening and
third-party changes are excluded.

Both Actions workflows are manual `workflow_dispatch` only. The Xcode
Deployment phase signs/notarizes/packages; it was not run. Validation uses
direct native compilation into temporary storage, never release helpers,
signing, packaging, uploads, workflow dispatch, tags, or tracked binaries.
