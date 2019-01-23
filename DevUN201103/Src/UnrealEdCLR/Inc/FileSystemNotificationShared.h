/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#ifndef __FILE_SYSTEM_NOTIFICATION_SHARED_H__
#define __FILE_SYSTEM_NOTIFICATION_SHARED_H__


/** Called from UUnrealEngine::Tick to send file notifications to avoid multiple events per file change */
void TickFileSystemNotifications();

/**
 * Ensures that the file system notifications are active
 * @param InExtension - The extension of file to watch for
 * @param InStart - TRUE if this is a "turn on" event, FALSE if this is a "turn off" event
 */
void SetFileSystemNotification(const FString& InExtension, UBOOL InStart);

/**
 * Shuts down global pointers before shut down
 */
void CloseFileSystemNotification();

#endif // __FILE_SYSTEM_NOTIFICATION_SHARED_H__

