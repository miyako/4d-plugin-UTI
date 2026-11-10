#include "4DPlugin-UTI.h"
#include "4DPlugin-JSON.h"
#include "PrivateTypes.h"
#include "EntryPoints.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>

using Text = std::basic_string<PA_Unichar>;

static std::set<PA_Unichar*> strings;
static std::map<PA_Handle, size_t> handles;
struct Ref {
    int owners = 1;
    std::map<Text, PA_Variable> properties;
};
static std::set<Ref*> references;
static bool nullEmpty = false;
static bool failGet = false;
static size_t clears = 0;

static void require(bool condition, const char* message)
{
    if(!condition) {
        fprintf(stderr, "ownership regression: %s\n", message);
        std::exit(1);
    }
}

static Text text(const PA_Unichar* p)
{
    return p ? Text(p) : Text();
}

static PA_Unistring makeString(const Text& value)
{
    PA_Unistring result = {};
    result.fLength = static_cast<PA_long32>(value.size());
    if(!value.empty() || !nullEmpty) {
        result.fString = static_cast<PA_Unichar*>(malloc((value.size() + 1) * sizeof(PA_Unichar)));
        require(result.fString != nullptr, "host string allocation");
        memcpy(result.fString, value.c_str(), (value.size() + 1) * sizeof(PA_Unichar));
        require(strings.insert(static_cast<PA_Unichar*>(result.fString)).second, "unique string allocation");
    }
    return result;
}

static void disposeString(PA_Unistring value)
{
    if(value.fString) {
        require(strings.erase(static_cast<PA_Unichar*>(value.fString)) == 1, "string disposed once by its owner");
        free(value.fString);
    } else {
        require(value.fLength == 0, "null empty length");
    }
}

static void checkString(PA_Unistring value, const Text& expected)
{
    require(value.fLength == expected.size(), "exact UTF-16 code-unit count");
    if(value.fString) {
        require(strings.count(static_cast<PA_Unichar*>(value.fString)) == 1, "string owner survives");
        require(Text(value.fString, value.fLength) == expected, "exact UTF-16 content");
        require(value.fString[value.fLength] == 0, "terminator outside content length");
    } else {
        require(expected.empty(), "null only represents empty");
    }
}

static Ref* makeRef()
{
    Ref* ref = new Ref;
    references.insert(ref);
    return ref;
}

static void clear(PA_Variable& value);

static void release(void* pointer)
{
    if(!pointer) return;
    Ref* ref = static_cast<Ref*>(pointer);
    require(references.count(ref) == 1 && ref->owners > 0, "live owning reference");
    if(--ref->owners == 0) {
        for(auto& property : ref->properties) clear(property.second);
        references.erase(ref);
        delete ref;
    }
}

static PA_Variable copy(PA_Variable value)
{
    if(value.fType == eVK_Unistring) {
        value.uValue.fString = makeString(text(value.uValue.fString.fString));
    } else if(value.fType == eVK_Object || value.fType == eVK_Collection ||
              value.fType == eVK_Picture) {
        void* pointer = value.fType == eVK_Object ? value.uValue.fObject :
                        value.fType == eVK_Collection ? value.uValue.fCollection :
                        value.uValue.fPicture;
        if(pointer) {
            Ref* ref = static_cast<Ref*>(pointer);
            require(references.count(ref) == 1, "copy retains live reference");
            ++ref->owners;
        }
    }
    return value;
}

static void disposeHandle(PA_Handle handle)
{
    require(handles.erase(handle) == 1, "handle disposed once");
    free(*handle);
    free(handle);
}

static void clear(PA_Variable& value)
{
    require(value.fFiller == 0, "clear only value variables");
    switch(value.fType) {
        case eVK_Unistring: disposeString(value.uValue.fString); break;
        case eVK_Object: release(value.uValue.fObject); break;
        case eVK_Collection: release(value.uValue.fCollection); break;
        case eVK_Picture: release(value.uValue.fPicture); break;
        case eVK_ArrayUnicode:
            if(value.uValue.fArray.fData) {
                auto* data = reinterpret_cast<PA_Unistring*>(*value.uValue.fArray.fData);
                for(int i = 0; i <= value.uValue.fArray.fNbElements; ++i) disposeString(data[i]);
                disposeHandle(value.uValue.fArray.fData);
            }
            break;
        case eVK_Undefined: case eVK_Null: case eVK_Boolean:
        case eVK_Longint: case eVK_Real: break;
        default: require(false, "unexpected variable cleanup kind");
    }
    value = {};
    value.fType = eVK_Undefined;
}

static void host(short selector, EngineBlock* eb)
{
    eb->fError = eER_NoErr;
    switch(selector) {
        case EX_CREATE_UNISTRING:
            eb->fUniString1 = makeString(text(reinterpret_cast<PA_Unichar*>(eb->fHandle)));
            return;
        case EX_DISPOSE_UNISTRING: disposeString(eb->fUniString1); return;
        case EX_SET_UNISTRING: {
            Text content = text(reinterpret_cast<PA_Unichar*>(eb->fHandle));
            disposeString(eb->fUniString1);
            eb->fUniString1 = makeString(content);
            return;
        }
        case EX_VARIABLE_TO_STRING: {
            auto* variable = reinterpret_cast<PA_Variable*>(eb->fHandle);
            require(variable->fType == eVK_Unistring, "only existing string values modeled");
            eb->fUniString1 = variable->uValue.fString;
            return;
        }
        case EX_CREATE_PICTURE: eb->fPicture = makeRef(); return;
        case EX_DISPOSE_PICTURE: release(eb->fPicture); return;
        case EX_CLEAR_VARIABLE:
            ++clears;
            clear(*static_cast<PA_Variable*>(eb->fPtr1));
            return;
        case EX_HANDLE_MANAGER:
            switch(eb->fParam1) {
                case 1: {
                    PA_Handle h = static_cast<PA_Handle>(malloc(sizeof(char*)));
                    *h = static_cast<char*>(calloc(std::max<sLONG_PTR>(1, static_cast<sLONG_PTR>(eb->fParam2)), 1));
                    handles[h] = eb->fParam2;
                    eb->fHandle = h;
                    return;
                }
                case 2: disposeHandle(eb->fHandle); return;
                case 3:
                    require(handles.count(static_cast<PA_Handle>(eb->fHandle)) == 1, "resize live handle");
                    *eb->fHandle = static_cast<char*>(realloc(*eb->fHandle, std::max<sLONG_PTR>(1, static_cast<sLONG_PTR>(eb->fParam2))));
                    handles[static_cast<PA_Handle>(eb->fHandle)] = eb->fParam2;
                    return;
                case 4: eb->fParam2 = handles.at(static_cast<PA_Handle>(eb->fHandle)); return;
                case 5:
                    require(handles.count(static_cast<PA_Handle>(eb->fHandle)) == 1, "lock live handle");
                    eb->fParam3 = reinterpret_cast<sLONG_PTR>(*eb->fHandle);
                    return;
                case 6: return;
            }
            break;
        case EX_SET_OBJ_VALUE: {
            Ref* obj = static_cast<Ref*>(eb->fPtr1);
            Text key = text(eb->fUniString1.fString);
            auto old = obj->properties.find(key);
            if(old != obj->properties.end()) clear(old->second);
            obj->properties[key] = copy(*static_cast<PA_Variable*>(eb->fPtr2));
            return;
        }
        case EX_GET_OBJ_VALUE:
            if(failGet) {
                // Failed SDK outputs are unspecified, not an owning variable.
                memset(eb->fPtr2, 0xa5, sizeof(PA_Variable));
                eb->fError = -1;
            } else {
                Ref* obj = static_cast<Ref*>(eb->fPtr1);
                *static_cast<PA_Variable*>(eb->fPtr2) =
                    copy(obj->properties.at(text(eb->fUniString1.fString)));
            }
            return;
        case EX_EXECUTE_COMMAND_BY_ID: {
            auto** args = static_cast<PA_Variable**>(eb->fPtr1);
            auto* result = static_cast<PA_Variable*>(eb->fPtr2);
            if(eb->fParam1 == 1231) {
                Ref* obj = static_cast<Ref*>(args[0]->uValue.fObject);
                PA_SetBooleanVariable(result, obj->properties.count(text(args[1]->uValue.fString.fString)) != 0);
                return;
            }
            if(eb->fParam1 == 1225) {
                Ref* duplicate = makeRef();
                for(auto& p : static_cast<Ref*>(args[0]->uValue.fObject)->properties)
                    duplicate->properties[p.first] = copy(p.second);
                PA_SetObjectVariable(result, duplicate);
                return;
            }
            if(eb->fParam1 == 1217) {
                PA_Unistring json = makeString(text(reinterpret_cast<const PA_Unichar*>(u"{\"text\":\"\u65e5\U0001f600\"}")));
                PA_SetStringVariable(result, &json);
                return;
            }
            break;
        }
    }
    fprintf(stderr, "unexpected engine selector %d\n", selector);
    std::abort();
}

static void balanced(const char* label)
{
    if(!strings.empty() || !handles.empty() || !references.empty()) {
        fprintf(stderr, "%s: strings=%zu handles=%zu refs=%zu\n",
                label, strings.size(), handles.size(), references.size());
        require(false, "all owned allocations released");
    }
}

static Text fixture(const char16_t* value)
{
    return text(reinterpret_cast<const PA_Unichar*>(value));
}

static void setters()
{
    const char* utf8[] = {"ASCII", "\xe6\x97\xa5", "\xf0\x9f\x98\x80", ""};
    const wchar_t* wide[] = {L"ASCII", L"\u65e5", L"\U0001f600", L""};
    const char16_t* utf16[] = {u"ASCII", u"\u65e5", u"\U0001f600", u""};
    for(int i = 0; i != 4; ++i) {
        Ref* obj = makeRef();
        Text expected = fixture(utf16[i]);
        CUTF16String content = expected;
        ob_set_s(obj, "narrow", utf8[i]);
        ob_set_s(obj, L"wide", utf8[i]);
        ob_set_a(obj, L"literal", wide[i]);
        ob_set_a(obj, L"units", &content);
        for(auto& p : obj->properties) checkString(p.second.uValue.fString, expected);
        ob_set_s(obj, "narrow", "replacement");
        PA_DisposeObject(obj);
        balanced("setter-empty-overwrite");
    }
}

static void picture()
{
    Ref* obj = makeRef();
    PA_Picture value = PA_CreatePicture(nullptr, 0);
    // The existing setter consumes its input; the property holds its own retain.
    ob_set_p(obj, L"picture", value);
    require(static_cast<Ref*>(value)->owners == 1, "property survives setter cleanup");
    PA_DisposeObject(obj);
    balanced("picture-empty-overwrite");
}

static void getters(const std::string& kind)
{
    for(const auto* raw : {u"ASCII", u"\u65e5", u"\U0001f600", u""}) {
        Ref* obj = makeRef();
        PA_Variable input = {};
        Text expected = fixture(raw);
        input.fType = eVK_Unistring;
        input.uValue.fString = makeString(expected);
        obj->properties[fixture(u"key")] = input;
        CUTF8String output;
        CUTF8String expectedOutput;
        if(kind == "getter-a") {
            require(ob_get_a(obj, L"key", &output), "existing string property");
            CFStringRef cf = CFStringCreateWithCharacters(nullptr, expected.c_str(), expected.size());
            char bytes[128] = {};
            require(CFStringGetCString(cf, bytes, sizeof(bytes), kCFStringEncodingUTF8), "fixture UTF-8 conversion");
            CFRelease(cf);
            expectedOutput = CUTF8String(reinterpret_cast<const uint8_t*>(bytes));
            require(output == expectedOutput, "exact UTF-8 scalar copy");
        } else if(kind == "getter-b") {
            require(!ob_get_b(obj, L"key"), "boolean mismatch");
        } else if(kind == "getter-n") {
            require(ob_get_n(obj, L"key") == 0, "real mismatch");
        } else {
            require(ob_get_c(obj, L"key") == nullptr, "collection mismatch");
        }
        checkString(input.uValue.fString, expected);
        PA_DisposeObject(obj);
        if(kind == "getter-a")
            require(output == expectedOutput, "scalar output survives owner disposal");
        balanced(kind.c_str());
    }
    Ref* obj = makeRef();
    Ref* nested = makeRef();
    PA_Variable input = {};
    PA_SetObjectVariable(&input, nested);
    obj->properties[fixture(u"key")] = input;
    CUTF8String output;
    if(kind == "getter-a") require(ob_get_a(obj, L"key", &output), "defined object mismatch");
    else if(kind == "getter-b") require(!ob_get_b(obj, L"key"), "object boolean mismatch");
    else if(kind == "getter-n") require(ob_get_n(obj, L"key") == 0, "object real mismatch");
    else require(ob_get_c(obj, L"key") == nullptr, "object collection mismatch");
    require(nested->owners == 1, "mismatched owning reference released");
    PA_DisposeObject(obj);
    balanced("getter-reference-mismatch");
}

static void referencesAndErrors()
{
    Ref* obj = makeRef();
    Ref* collection = makeRef();
    PA_Variable v = {};
    PA_Variable item = {};
    item.fType = eVK_Unistring;
    item.uValue.fString = makeString(fixture(u"\u65e5\U0001f600"));
    collection->properties[fixture(u"item")] = item;
    PA_SetCollectionVariable(&v, collection);
    obj->properties[fixture(u"key")] = v;
    PA_CollectionRef result = ob_get_c(obj, L"key");
    require(result == collection && collection->owners == 2, "owning collection handoff");
    PA_DisposeObject(obj);
    require(collection->owners == 1, "handoff survives source disposal");
    checkString(collection->properties.at(fixture(u"item")).uValue.fString, fixture(u"\u65e5\U0001f600"));
    PA_DisposeCollection(result);
    balanced("collection-handoff");

    for(int getter = 0; getter != 4; ++getter) {
        obj = makeRef();
        PA_SetBooleanVariable(&v, true);
        obj->properties[fixture(u"key")] = v;
        size_t before = clears;
        failGet = true;
        bool caught = false;
        try {
            CUTF8String output;
            if(getter == 0) ob_get_a(obj, L"key", &output);
            if(getter == 1) ob_get_b(obj, L"key");
            if(getter == 2) ob_get_n(obj, L"key");
            if(getter == 3) ob_get_c(obj, L"key");
        } catch(const std::runtime_error& error) {
            caught = std::string(error.what()).find("PA_GetObjectProperty failed") != std::string::npos;
        }
        failGet = false;
        require(caught, "property read failure surfaced explicitly");
        require(clears == before, "failed unspecified output never cleared");
        PA_DisposeObject(obj);
        balanced("getter-error");
    }
    obj = makeRef();
    PA_SetBooleanVariable(&v, true);
    obj->properties[fixture(u"key")] = v;
    require(ob_get_b(obj, L"key"), "matching boolean copied");
    PA_SetRealVariable(&obj->properties[fixture(u"key")], 42.5);
    require(ob_get_n(obj, L"key") == 42.5, "matching real copied");
    CUTF8String json;
    ob_stringify(obj, &json);
    require(json == CUTF8String(reinterpret_cast<const uint8_t*>("{\"text\":\"\xe6\x97\xa5\xf0\x9f\x98\x80\"}")), "owning command result copied");
    PA_DisposeObject(obj);
    balanced("command-result");
    obj = makeRef();
    CUTF8String unchanged(reinterpret_cast<const uint8_t*>("unchanged"));
    size_t before = clears;
    require(!ob_get_a(obj, L"absent", &unchanged), "absent string property");
    require(unchanged == CUTF8String(reinterpret_cast<const uint8_t*>("unchanged")), "absent output unchanged");
    require(!ob_get_b(obj, L"absent") && ob_get_n(obj, L"absent") == 0 &&
            ob_get_c(obj, L"absent") == nullptr, "absent scalar defaults preserved");
    require(clears == before, "no getter result exists for absent property");
    PA_DisposeObject(obj);
    balanced("absent-property");
}

static void arrays()
{
    PA_Variable array = PA_CreateVariable(eVK_ArrayUnicode);
    BytePtr params[] = {reinterpret_cast<BytePtr>(&array)};
    {
        ARRAY_TEXT output;
        output.setSize(1);
        for(const auto* value : {u"ASCII", u"\u65e5", u"\U0001f600", u""})
            output.appendUTF16String(reinterpret_cast<const PA_Unichar*>(value));
        output.toParamAtIndex(params, 1);
    }
    for(int i = 0; i != 32; ++i) {
        PA_Unistring churn = makeString(fixture(u"allocator churn \u65e5\U0001f600"));
        PA_DisposeUnistring(&churn);
    }
    for(int i = 0; i != 5; ++i) {
        Text expected[] = {Text(), fixture(u"ASCII"), fixture(u"\u65e5"), fixture(u"\U0001f600"), Text()};
        checkString(PA_GetStringInArray(array, i), expected[i]);
    }
    ARRAY_TEXT copied;
    copied.fromParamAtIndex(params, 1);
    PA_Unistring replacement = makeString(fixture(u"replacement"));
    PA_SetStringInArray(array, 1, &replacement);
    CUTF16String independent;
    copied.copyUTF16StringAtIndex(&independent, 1);
    require(independent == fixture(u"ASCII"), "borrowed input copied before owner mutation");
    PA_Unistring separate = makeString(fixture(u"replacement"));
    PA_SetStringInArray(array, 2, &separate);
    require(separate.fString != replacement.fString, "independent accepted transfers");
    PA_ResizeArray(&array, 2);
    for(int rejected : {-1, 3}) {
        PA_Unistring input = makeString(fixture(u"retained"));
        PA_SetStringInArray(array, rejected, &input);
        checkString(input, fixture(u"retained"));
        PA_DisposeUnistring(&input);
    }
    PA_Unistring input = makeString(fixture(u"retained"));
    PA_Variable wrong = PA_CreateVariable(eVK_Longint);
    PA_SetStringInArray(wrong, 0, &input);
    PA_Variable noData = {};
    noData.fType = eVK_ArrayUnicode;
    PA_SetStringInArray(noData, 0, &input);
    PA_SetStringInArray(array, 0, nullptr);
    checkString(input, fixture(u"retained"));
    PA_DisposeUnistring(&input);
    PA_ResizeArray(&array, 0);
    PA_ClearVariable(&array);
    balanced("array-transfers");

    PA_Unistring path = makeString(fixture(u"Disk:folder:file.txt"));
    array = PA_CreateVariable(eVK_ArrayUnicode);
    BytePtr dispatchParams[] = {reinterpret_cast<BytePtr>(&path), reinterpret_cast<BytePtr>(&array)};
    PluginBlock block = {};
    block.fParameters = dispatchParams;
    PluginMain(1, &block);
    require(array.uValue.fArray.fNbElements > 0, "actual PATH GET COMPONENTS dispatcher");
    for(int i = 1; i <= array.uValue.fArray.fNbElements; ++i)
        require(PA_GetStringInArray(array, i).fLength > 0, "dispatcher output readable after wrapper destruction");
    PA_DisposeUnistring(&path);
    PA_ClearVariable(&array);
    balanced("dispatcher");
}

int main(int argc, char** argv)
{
    require(argc >= 2, "named test required");
    nullEmpty = argc == 3;
    gCall4D = host;
    @autoreleasepool {
        std::string name = argv[1];
        if(name == "setters") setters();
        else if(name == "picture") picture();
        else if(name.find("getter-") == 0) getters(name);
        else if(name == "references-errors") referencesAndErrors();
        else if(name == "arrays") arrays();
        else require(false, "unknown named test");
    }
    balanced(argv[1]);
    printf("%s (%s empty): PASS\n", argv[1], nullEmpty ? "null" : "allocated");
}
