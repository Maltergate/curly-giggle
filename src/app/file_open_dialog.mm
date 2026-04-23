// file_open_dialog.mm — NSOpenPanel wrapper (Objective-C++, ARC enabled)

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#include "gnc_viz/file_open_dialog.hpp"

namespace gnc_viz {

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

} // namespace gnc_viz
