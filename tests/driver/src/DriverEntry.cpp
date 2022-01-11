#include <ntifs.h>

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registry) {
    return STATUS_UNSUCCESSFUL;
}
