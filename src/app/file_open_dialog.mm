// file_open_dialog.mm — NSOpenPanel wrapper (Objective-C++, ARC enabled)

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#include "fastscope/file_open_dialog.hpp"

namespace fastscope {

OpenDialogResult show_open_dialog(const char*              title,
                                  bool                     allow_multiple,
                                  std::vector<std::string> allowed_types)
{
    OpenDialogResult result;

    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];

        panel.title                    = @(title);
        panel.canChooseFiles           = YES;
        panel.canChooseDirectories     = NO;
        panel.allowsMultipleSelection  = allow_multiple ? YES : NO;
        panel.canCreateDirectories     = NO;

        // File-type filter
        if (!allowed_types.empty()) {
            NSMutableArray<UTType*>* types = [NSMutableArray array];
            for (const auto& ext : allowed_types) {
                UTType* ut = [UTType typeWithFilenameExtension:@(ext.c_str())];
                if (ut) [types addObject:ut];
            }
            if (types.count > 0)
                panel.allowedContentTypes = types;
        }

        // Run modal on the main thread
        NSModalResponse response = [panel runModal];
        if (response == NSModalResponseOK) {
            result.confirmed = true;
            for (NSURL* url in panel.URLs) {
                if (url.isFileURL)
                    result.paths.emplace_back(url.fileSystemRepresentation);
            }
        }
    }

    return result;
}

SaveDialogResult show_save_dialog(const std::string& title,
                                   const std::vector<std::string>& extensions,
                                   const std::string& default_name)
{
    SaveDialogResult result;

    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        panel.title = [NSString stringWithUTF8String:title.c_str()];
        panel.nameFieldStringValue =
            [NSString stringWithUTF8String:default_name.c_str()];
        panel.canCreateDirectories = YES;

        if (!extensions.empty()) {
            NSMutableArray<UTType*>* types = [NSMutableArray array];
            for (const auto& ext : extensions) {
                UTType* t = [UTType typeWithFilenameExtension:
                             [NSString stringWithUTF8String:ext.c_str()]];
                if (t) [types addObject:t];
            }
            if (types.count > 0)
                panel.allowedContentTypes = types;
        }

        if ([panel runModal] == NSModalResponseOK && panel.URL != nil) {
            result.confirmed = true;
            result.path = std::filesystem::path([panel.URL.path UTF8String]);
        }
    }

    return result;
}

} // namespace fastscope
