#include "../Interfaces/Clipboard.h"

#include "CocoaInterop.h"

#include "AppKit/AppKit.h"

const NSString * TypeEngineRuntimeAttributedText = @"com.EngineSoftware.Type.AttributedText";
const NSString * TypeEngineRuntimeBinaryData = @"com.EngineSoftware.Type.BinaryData";

@interface EngineRuntimeAttributedText : NSObject<NSPasteboardWriting, NSPasteboardReading>
{
@public
	Engine::ImmutableString text;
}
- (NSArray<NSPasteboardType> *) writableTypesForPasteboard: (NSPasteboard *) pasteboard;
- (NSPasteboardWritingOptions) writingOptionsForType: (NSPasteboardType) type pasteboard: (NSPasteboard *) pasteboard;
- (id) pasteboardPropertyListForType: (NSPasteboardType) type;
- (id) initWithPasteboardPropertyList: (id) propertyList ofType: (NSPasteboardType) type;
+ (NSArray<NSPasteboardType> *) readableTypesForPasteboard: (NSPasteboard *) pasteboard;
+ (NSPasteboardReadingOptions) readingOptionsForType: (NSPasteboardType) type pasteboard: (NSPasteboard *) pasteboard;
@end
@interface EngineRuntimeBinaryData : NSObject<NSPasteboardWriting, NSPasteboardReading>
{
@public
	Engine::Array<Engine::uint8> * data;
}
- (NSArray<NSPasteboardType> *) writableTypesForPasteboard: (NSPasteboard *) pasteboard;
- (NSPasteboardWritingOptions) writingOptionsForType: (NSPasteboardType) type pasteboard: (NSPasteboard *) pasteboard;
- (id) pasteboardPropertyListForType: (NSPasteboardType) type;
- (id) initWithPasteboardPropertyList: (id) propertyList ofType: (NSPasteboardType) type;
+ (NSArray<NSPasteboardType> *) readableTypesForPasteboard: (NSPasteboard *) pasteboard;
+ (NSPasteboardReadingOptions) readingOptionsForType: (NSPasteboardType) type pasteboard: (NSPasteboard *) pasteboard;
- (void) dealloc;
@end

@implementation EngineRuntimeAttributedText
- (NSArray<NSPasteboardType> *) writableTypesForPasteboard: (NSPasteboard *) pasteboard { return [NSArray arrayWithObject: TypeEngineRuntimeAttributedText]; }
- (NSPasteboardWritingOptions) writingOptionsForType: (NSPasteboardType) type pasteboard: (NSPasteboard *) pasteboard { return 0; }
- (id) pasteboardPropertyListForType: (NSPasteboardType) type
{
    Engine::SafePointer< Engine::Array<Engine::uint8> > data = text.EncodeSequence(Engine::Encoding::UTF8, true);
    return [NSData dataWithBytes: data->GetBuffer() length: data->Length()];
}
- (id) initWithPasteboardPropertyList: (id) propertyList ofType: (NSPasteboardType) type
{
    [super init];
    text = Engine::ImmutableString([propertyList bytes], -1, Engine::Encoding::UTF8);
    return self;
}
+ (NSArray<NSPasteboardType> *) readableTypesForPasteboard: (NSPasteboard *) pasteboard { return [NSArray arrayWithObject: TypeEngineRuntimeAttributedText]; }
+ (NSPasteboardReadingOptions) readingOptionsForType: (NSPasteboardType) type pasteboard: (NSPasteboard *) pasteboard; { return NSPasteboardReadingAsData; }
@end
@implementation EngineRuntimeBinaryData
- (NSArray<NSPasteboardType> *) writableTypesForPasteboard: (NSPasteboard *) pasteboard { return [NSArray arrayWithObject: TypeEngineRuntimeBinaryData]; }
- (NSPasteboardWritingOptions) writingOptionsForType: (NSPasteboardType) type pasteboard: (NSPasteboard *) pasteboard { return 0; }
- (id) pasteboardPropertyListForType: (NSPasteboardType) type { return [NSData dataWithBytes: data->GetBuffer() length: data->Length()]; }
- (id) initWithPasteboardPropertyList: (id) propertyList ofType: (NSPasteboardType) type
{
    [super init];
    data = new Engine::Array<Engine::uint8>(0x100);
    data->SetLength([propertyList length]);
    Engine::MemoryCopy(data->GetBuffer(), [propertyList bytes], [propertyList length]);
    return self;
}
+ (NSArray<NSPasteboardType> *) readableTypesForPasteboard: (NSPasteboard *) pasteboard { return [NSArray arrayWithObject: TypeEngineRuntimeBinaryData]; }
+ (NSPasteboardReadingOptions) readingOptionsForType: (NSPasteboardType) type pasteboard: (NSPasteboard *) pasteboard; { return NSPasteboardReadingAsData; }
- (void) dealloc { if (data) data->Release(); [super dealloc]; }
@end

namespace Engine
{
	namespace Clipboard
	{
		bool IsFormatAvailable(Format format)
        {
            @autoreleasepool {
                if (format == Format::Text) {
                    bool result = false;
                    NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                    NSArray<NSPasteboardType> * types = [pasteboard types];
                    for (int i = 0; i < [types count]; i++) {
                        if ([types[i] isEqualToString: NSPasteboardTypeString] && format == Format::Text) result = true;
                    }
                    return result;
                } else if (format == Format::Image) {
                    NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                    NSArray<Class> * clss = [NSArray arrayWithObject: [NSImage class]];
                    NSDictionary<NSPasteboardReadingOptionKey, id> * dict = [NSDictionary dictionary];
                    if ([pasteboard canReadObjectForClasses: clss options: dict]) return true; else return false;
                } else if (format == Format::RichText) {
                    NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                    NSArray<Class> * clss = [NSArray arrayWithObject: [EngineRuntimeAttributedText class]];
                    NSDictionary<NSPasteboardReadingOptionKey, id> * dict = [NSDictionary dictionary];
                    if ([pasteboard canReadObjectForClasses: clss options: dict]) return true; else return false;
                } else if (format == Format::Custom) {
                    NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                    NSArray<Class> * clss = [NSArray arrayWithObject: [EngineRuntimeBinaryData class]];
                    NSDictionary<NSPasteboardReadingOptionKey, id> * dict = [NSDictionary dictionary];
                    if ([pasteboard canReadObjectForClasses: clss options: dict]) return true; else return false;
                } else return false;
            }
        }
		bool GetData(string & value)
        {
            NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
            NSString * text = [pasteboard stringForType: NSPasteboardTypeString];
            if (text) {
                value = Cocoa::EngineString(text);
                return true;
            } else return false;
        }
        bool GetData(Codec::Frame ** value)
        {
            @autoreleasepool {
                NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                NSArray<Class> * clss = [NSArray arrayWithObject: [NSImage class]];
                NSDictionary<NSPasteboardReadingOptionKey, id> * dict = [NSDictionary dictionary];
                NSArray<NSImage *> * images = [pasteboard readObjectsForClasses: clss options: dict];
                if (!images || ![images count]) return false;
                *value = Cocoa::EngineImage([images firstObject]);
                return true;
            }
        }
        bool GetData(string & value, bool attributed)
		{
            if (attributed) {
                @autoreleasepool {
                    NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                    NSArray<Class> * clss = [NSArray arrayWithObject: [EngineRuntimeAttributedText class]];
                    NSDictionary<NSPasteboardReadingOptionKey, id> * dict = [NSDictionary dictionary];
                    NSArray<EngineRuntimeAttributedText *> * texts = [pasteboard readObjectsForClasses: clss options: dict];
                    if (!texts || ![texts count]) return false;
                    value = [texts firstObject]->text;
                    return true;
                }
            } else return GetData(value);
		}
		bool GetData(const string & subclass, Array<uint8> ** value)
		{
            @autoreleasepool {
                NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                NSArray<Class> * clss = [NSArray arrayWithObject: [EngineRuntimeBinaryData class]];
                NSDictionary<NSPasteboardReadingOptionKey, id> * dict = [NSDictionary dictionary];
                NSArray<EngineRuntimeBinaryData *> * data = [pasteboard readObjectsForClasses: clss options: dict];
                if (!data || ![data count]) return false;
                auto cls = string([data firstObject]->data->GetBuffer(), -1, Encoding::UTF8);
                if (cls == subclass) {
                    int len_skip = cls.GetEncodedLength(Encoding::UTF8) + 1;
                    int len = [data firstObject]->data->Length() - len_skip;
                    SafePointer< Array<uint8> > result = new Array<uint8>(len);
                    result->SetLength(len);
                    MemoryCopy(result->GetBuffer(), [data firstObject]->data->GetBuffer() + len_skip, len);
                    result->Retain();
                    *value = result;
                    return true;
                } else return false;
            }
		}
		bool SetData(const string & value)
        {
            NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
            NSString * text = Cocoa::CocoaString(value);
            [pasteboard clearContents];
            bool result = [pasteboard setString: text forType: NSPasteboardTypeString];
            [text release];
            return result;
        }
        bool SetData(Codec::Frame * value)
        {
            @autoreleasepool {
                NSImage * image = Cocoa::CocoaImage(value);
                NSArray<NSImage *> * objs = [NSArray arrayWithObject: image];
                NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                [pasteboard clearContents];
                BOOL result = [pasteboard writeObjects: objs];
                [image release];
                return result ? true : false;
            }
        }
        bool SetData(const string & plain, const string & attributed)
		{
            @autoreleasepool {
                EngineRuntimeAttributedText * text = [[EngineRuntimeAttributedText alloc] init];
                NSString * plain_text = Cocoa::CocoaString(plain);
                text->text = attributed;
                NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                [pasteboard clearContents];
                NSArray * objs = [NSArray arrayWithObjects: plain_text, text, nil];
                BOOL result = [pasteboard writeObjects: objs];
                [text release];
                [plain_text release];
                return result ? true : false;
            }
		}
		bool SetData(const string & subclass, const void * data, int size)
		{
            @autoreleasepool {
                EngineRuntimeBinaryData * obj = [[EngineRuntimeBinaryData alloc] init];
                int len = subclass.GetEncodedLength(Encoding::UTF8) + 1;
                obj->data = new Array<uint8>(0x100);
                obj->data->SetLength(len + size);
                subclass.Encode(obj->data->GetBuffer(), Encoding::UTF8, true);
                MemoryCopy(obj->data->GetBuffer() + len, data, size);
                NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                [pasteboard clearContents];
                NSArray * objs = [NSArray arrayWithObject: obj];
                BOOL result = [pasteboard writeObjects: objs];
                [obj release];
                return result ? true : false;
            }
		}
		string GetCustomSubclass(void)
		{
            @autoreleasepool {
                NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                NSArray<Class> * clss = [NSArray arrayWithObject: [EngineRuntimeBinaryData class]];
                NSDictionary<NSPasteboardReadingOptionKey, id> * dict = [NSDictionary dictionary];
                NSArray<EngineRuntimeBinaryData *> * data = [pasteboard readObjectsForClasses: clss options: dict];
                if (!data || ![data count]) return L"";
                return string([data firstObject]->data->GetBuffer(), -1, Encoding::UTF8);
            }
		}
	}
}