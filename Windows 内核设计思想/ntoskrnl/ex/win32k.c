/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/win32k.c
 * PURPOSE:         Executive Win32 Object Support (Desktop/WinStation)
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
    #pragma alloc_text(INIT, ExpWin32kInit)
#endif

/* DATA **********************************************************************/

POBJECT_TYPE ExWindowStationObjectType = NULL;
POBJECT_TYPE ExDesktopObjectType = NULL;

GENERIC_MAPPING ExpWindowStationMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_REQUIRED
};

GENERIC_MAPPING ExpDesktopMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_REQUIRED
};

PKWIN32_PARSEMETHOD_CALLOUT ExpWindowStationObjectParse = NULL;
PKWIN32_DELETEMETHOD_CALLOUT ExpWindowStationObjectDelete = NULL;
PKWIN32_OKTOCLOSEMETHOD_CALLOUT ExpWindowStationObjectOkToClose = NULL;
PKWIN32_OKTOCLOSEMETHOD_CALLOUT ExpDesktopObjectOkToClose = NULL;
PKWIN32_DELETEMETHOD_CALLOUT ExpDesktopObjectDelete = NULL;
PKWIN32_OPENMETHOD_CALLOUT ExpDesktopObjectOpen = NULL;
PKWIN32_CLOSEMETHOD_CALLOUT ExpDesktopObjectClose = NULL;

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
ExpDesktopOkToClose(IN PEPROCESS Process OPTIONAL,
                    IN PVOID Object,
                    IN HANDLE Handle,
                    IN KPROCESSOR_MODE AccessMode)
{
    WIN32_OKAYTOCLOSEMETHOD_PARAMETERS Parameters;
    Parameters.Process = Process;
    Parameters.Object = Object;
    Parameters.Handle = Handle;
    Parameters.PreviousMode = AccessMode;
    return NT_SUCCESS(ExpDesktopObjectOkToClose(&Parameters));
}

BOOLEAN
NTAPI
ExpWindowStationOkToClose(IN PEPROCESS Process OPTIONAL,
                          IN PVOID Object,
                          IN HANDLE Handle,
                          IN KPROCESSOR_MODE AccessMode)
{
    WIN32_OKAYTOCLOSEMETHOD_PARAMETERS Parameters;
    Parameters.Process = Process;
    Parameters.Object = Object;
    Parameters.Handle = Handle;
    Parameters.PreviousMode = AccessMode;
    return NT_SUCCESS(ExpWindowStationObjectOkToClose(&Parameters));
}

VOID
NTAPI
ExpWinStaObjectDelete(PVOID DeletedObject)
{
    WIN32_DELETEMETHOD_PARAMETERS Parameters;
    /* Fill out the callback structure */
    Parameters.Object = DeletedObject;
    /* Call the Registered Callback */
    ExpWindowStationObjectDelete(&Parameters);
}

NTSTATUS
NTAPI
ExpWinStaObjectParse(IN PVOID ParseObject,
                     IN PVOID ObjectType,
                     IN OUT PACCESS_STATE AccessState,
                     IN KPROCESSOR_MODE AccessMode,
                     IN ULONG Attributes,
                     IN OUT PUNICODE_STRING CompleteName,
                     IN OUT PUNICODE_STRING RemainingName,
                     IN OUT PVOID Context OPTIONAL,
                     IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
                     OUT PVOID *Object)
{
    WIN32_PARSEMETHOD_PARAMETERS Parameters;
    /* Fill out the callback structure */
    Parameters.ParseObject = ParseObject;
    Parameters.ObjectType = ObjectType;
    Parameters.AccessState = AccessState;
    Parameters.AccessMode = AccessMode;
    Parameters.Attributes = Attributes;
    Parameters.CompleteName = CompleteName;
    Parameters.RemainingName = RemainingName;
    Parameters.Context = Context;
    Parameters.SecurityQos = SecurityQos;
    Parameters.Object = Object;
    /* Call the Registered Callback */
    return ExpWindowStationObjectParse(&Parameters);
}
VOID
NTAPI
ExpDesktopDelete(PVOID DeletedObject)
{
    WIN32_DELETEMETHOD_PARAMETERS Parameters;
    /* Fill out the callback structure */
    Parameters.Object = DeletedObject;
    /* Call the Registered Callback */
    ExpDesktopObjectDelete(&Parameters);
}

NTSTATUS
NTAPI
ExpDesktopOpen(IN OB_OPEN_REASON Reason,
               IN PEPROCESS Process OPTIONAL,
               IN PVOID ObjectBody,
               IN ACCESS_MASK GrantedAccess,
               IN ULONG HandleCount)
{
    WIN32_OPENMETHOD_PARAMETERS Parameters;
    Parameters.OpenReason = Reason;
    Parameters.Process = Process;
    Parameters.Object = ObjectBody;
    Parameters.GrantedAccess = GrantedAccess;
    Parameters.HandleCount = HandleCount;
    return ExpDesktopObjectOpen(&Parameters);
}

VOID
NTAPI
ExpDesktopClose(IN PEPROCESS Process OPTIONAL,
                IN PVOID Object,
                IN ACCESS_MASK GrantedAccess,
                IN ULONG ProcessHandleCount,
                IN ULONG SystemHandleCount)
{
    WIN32_CLOSEMETHOD_PARAMETERS Parameters;
    Parameters.Process = Process;
    Parameters.Object = Object;
    Parameters.AccessMask = GrantedAccess;
    Parameters.ProcessHandleCount = ProcessHandleCount;
    Parameters.SystemHandleCount = SystemHandleCount;
    ExpDesktopObjectClose(&Parameters);
}

BOOLEAN
INIT_FUNCTION
NTAPI
ExpWin32kInit(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    NTSTATUS Status;
    DPRINT("Creating Win32 Object Types\n");
    /* Create the window station Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"WindowStation");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.GenericMapping = ExpWindowStationMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DeleteProcedure = ExpWinStaObjectDelete;
    ObjectTypeInitializer.ParseProcedure = ExpWinStaObjectParse;
    ObjectTypeInitializer.OkayToCloseProcedure = ExpWindowStationOkToClose;
    ObjectTypeInitializer.SecurityRequired = TRUE;
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK |
            OBJ_PERMANENT |
            OBJ_EXCLUSIVE;
    ObjectTypeInitializer.ValidAccessMask = STANDARD_RIGHTS_REQUIRED;
    Status = ObCreateObjectType(&Name,
                                &ObjectTypeInitializer,
                                NULL,
                                &ExWindowStationObjectType);

    if (!NT_SUCCESS(Status)) return FALSE;

    /* Create desktop object type */
    RtlInitUnicodeString(&Name, L"Desktop");
    ObjectTypeInitializer.GenericMapping = ExpDesktopMapping;
    ObjectTypeInitializer.DeleteProcedure = ExpDesktopDelete;
    ObjectTypeInitializer.ParseProcedure = NULL;
    ObjectTypeInitializer.OkayToCloseProcedure = ExpDesktopOkToClose;
    ObjectTypeInitializer.OpenProcedure = ExpDesktopOpen;
    ObjectTypeInitializer.CloseProcedure = ExpDesktopClose;
    ObCreateObjectType(&Name,
                       &ObjectTypeInitializer,
                       NULL,
                       &ExDesktopObjectType);

    if (!NT_SUCCESS(Status)) return FALSE;

    return TRUE;
}

/* EOF */
