// Version file included by the resource file ScaiIlp.rc.
// For documentation what is allowed in resource files, see
// https://docs.microsoft.com/en-us/windows/win32/menurc/about-resource-files.
#define SCAIILP_VERSION_MAJOR 3
#define SCAIILP_VERSION_MINOR 0
#define SCAIILP_VERSION_BUILD 0
#define SCAIILP_VERSION_DEBUG 0

#define SCAIILP_PRODUCT_NAME_STRING  "ScaiIlp"
#define SCAIILP_COPYRIGHT_STRING     "(C) Fraunhofer SCAI, 2016 - 2024, EPL"
#define SCAIILP_COMPANY_NAME_STRING  "Fraunhofer Institute for Algorithms and Scientific Computing SCAI"
#define SCAIILP_FILE_DESC_STRING     "ScaiIlp"
#define SCAIILP_EXE_FILE_NAME_STRING "ScaiIlpExe.exe"
#define SCAIILP_DLL_FILE_NAME_STRING "ScaiIlpDll.dll"

// Helper Macros: STRINGIZE returns the value of the argument in double quotes
#define SCAIILP_STRINGIZE2(s) #s
#define SCAIILP_STRINGIZE(s)  SCAIILP_STRINGIZE2(s)

#define SCAIILP_FILE_VERSION           SCAIILP_VERSION_MAJOR, SCAIILP_VERSION_MINOR, SCAIILP_VERSION_BUILD, SCAIILP_VERSION_DEBUG
#define SCAIILP_FILE_VERSION_STRING    SCAIILP_STRINGIZE(SCAIILP_VERSION_MAJOR) "." SCAIILP_STRINGIZE(SCAIILP_VERSION_MINOR)\
                                       "." SCAIILP_STRINGIZE(SCAIILP_VERSION_BUILD) "." SCAIILP_STRINGIZE(SCAIILP_VERSION_DEBUG)
#define SCAIILP_PRODUCT_VERSION_STRING SCAIILP_STRINGIZE(SCAIILP_VERSION_MAJOR) "." SCAIILP_STRINGIZE(SCAIILP_VERSION_MINOR)\
                                       "." SCAIILP_STRINGIZE(SCAIILP_VERSION_BUILD)
