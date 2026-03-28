/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   keyboard_png;
    const int            keyboard_pngSize = 794586;

    extern const char*   icons8arrowdown32_png;
    const int            icons8arrowdown32_pngSize = 91490;

    extern const char*   icons8arrowup32_png;
    const int            icons8arrowup32_pngSize = 813142;

    extern const char*   icons8arrowdown32click_png;
    const int            icons8arrowdown32click_pngSize = 869006;

    extern const char*   icons8arrowup32click_png;
    const int            icons8arrowup32click_pngSize = 1035581;

    extern const char*   help_md;
    const int            help_mdSize = 9974;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 6;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
