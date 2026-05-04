/** \file
 * \brief Drag & drop (iOS UIKit, UIDragInteraction / UIDropInteraction)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <string.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_key.h"

#include "iupcocoatouch_drv.h"


/* keys for associated objects holding drag/drop helpers; tied to the UIView's lifetime */
static const void* IUPCOCOATOUCH_DRAG_SOURCE_OBJ_KEY  = "IUPCOCOATOUCH_DRAG_SOURCE_OBJ_KEY";
static const void* IUPCOCOATOUCH_DROP_TARGET_OBJ_KEY  = "IUPCOCOATOUCH_DROP_TARGET_OBJ_KEY";
static const void* IUPCOCOATOUCH_DRAG_INTERACTION_KEY = "IUPCOCOATOUCH_DRAG_INTERACTION_KEY";
static const void* IUPCOCOATOUCH_DROP_INTERACTION_KEY = "IUPCOCOATOUCH_DROP_INTERACTION_KEY";

NSString* iupCocoaTouchDragTypeToUTI(const char* iup_type)
{
	if (!iup_type) return nil;

	if (iupStrEqualNoCase(iup_type, "TEXT") ||
	    iupStrEqualNoCase(iup_type, "UTF8_STRING") ||
	    iupStrEqualNoCase(iup_type, "UNICODETEXT") ||
	    iupStrEqualNoCase(iup_type, "text/plain"))
	{
		return IUPCOCOATOUCH_UTI_PLAIN_TEXT;
	}
	if (iupStrEqualNoCase(iup_type, "text/html"))     return IUPCOCOATOUCH_UTI_HTML;
	if (iupStrEqualNoCase(iup_type, "text/uri-list")) return IUPCOCOATOUCH_UTI_FILE_URL;
	if (iupStrEqualNoCase(iup_type, "image/png"))     return IUPCOCOATOUCH_UTI_PNG;
	if (iupStrEqualNoCase(iup_type, "image/jpeg"))    return IUPCOCOATOUCH_UTI_JPEG;
	if (iupStrEqualNoCase(iup_type, "image/tiff"))    return IUPCOCOATOUCH_UTI_TIFF;
	if (iupStrEqualNoCase(iup_type, "image/bmp"))     return IUPCOCOATOUCH_UTI_BMP;
	if (iupStrEqualNoCase(iup_type, "image/gif"))     return IUPCOCOATOUCH_UTI_GIF;

	return [NSString stringWithUTF8String:iup_type];
}

NSArray<NSString*>* iupCocoaTouchDragParseTypes(const char* value)
{
	if (!value || !*value) return @[];
	NSMutableArray<NSString*>* out = [NSMutableArray array];
	NSArray<NSString*>* parts = [[NSString stringWithUTF8String:value] componentsSeparatedByString:@","];
	for (NSString* part in parts)
	{
		NSString* trimmed = [part stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
		if ([trimmed length] == 0) continue;
		NSString* uti = iupCocoaTouchDragTypeToUTI([trimmed UTF8String]);
		if (uti) [out addObject:uti];
	}
	return out;
}

@interface IupCocoaTouchDragSource : NSObject <UIDragInteractionDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, copy) NSArray<NSString*>* types;
@end


@implementation IupCocoaTouchDragSource

- (NSArray<UIDragItem*>*)dragInteraction:(UIDragInteraction*)interaction itemsForBeginningSession:(id<UIDragSession>)session
{
	if (!_ihandle || !iupObjectCheck(_ihandle) || [_types count] == 0)
	{
		return @[];
	}

	CGPoint pt = [session locationInView:interaction.view];
	IFnii begin_cb = (IFnii)IupGetCallback(_ihandle, "DRAGBEGIN_CB");
	if (begin_cb && begin_cb(_ihandle, (int)pt.x, (int)pt.y) == IUP_IGNORE)
	{
		return @[];
	}

	NSItemProvider* provider = [[[NSItemProvider alloc] init] autorelease];
	Ihandle* ih = _ihandle;

	for (NSString* uti in _types)
	{
		NSString* uti_copy = [[uti copy] autorelease];

		[provider registerDataRepresentationForTypeIdentifier:uti_copy
			visibility:NSItemProviderRepresentationVisibilityAll
			loadHandler:^NSProgress*(void (^completion)(NSData*, NSError*)) {
				if (!iupObjectCheck(ih))
				{
					completion(nil, nil);
					return nil;
				}
				IFns size_cb = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
				IFnsVi data_cb = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");
				if (!size_cb || !data_cb)
				{
					completion(nil, nil);
					return nil;
				}
				char type_cstr[128];
				strlcpy(type_cstr, [uti_copy UTF8String], sizeof(type_cstr));
				int size = size_cb(ih, type_cstr);
				if (size <= 0)
				{
					completion(nil, nil);
					return nil;
				}
				NSMutableData* buf = [NSMutableData dataWithLength:(NSUInteger)size];
				data_cb(ih, type_cstr, [buf mutableBytes], size);
				completion(buf, nil);
				return nil;
			}
		];
	}

	UIDragItem* drag_item = [[[UIDragItem alloc] initWithItemProvider:provider] autorelease];
	/* same-app drops read localObject for DRAGSOURCEMOVE */
	if (iupAttribGetBoolean(_ihandle, "DRAGSOURCEMOVE"))
		drag_item.localObject = @"MOVE";
	return @[drag_item];
}

- (void)dragInteraction:(UIDragInteraction*)interaction session:(id<UIDragSession>)session didEndWithOperation:(UIDropOperation)operation
{
	(void)interaction; (void)session;
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;
	IFni end_cb = (IFni)IupGetCallback(_ihandle, "DRAGEND_CB");
	if (!end_cb) return;

	/* DRAGEND_CB action: 1=move, 0=copy, -1=cancel */
	int action;
	switch (operation)
	{
		case UIDropOperationMove:   action =  1; break;
		case UIDropOperationCopy:   action =  0; break;
		default:                    action = -1; break;
	}
	if (end_cb(_ihandle, action) == IUP_CLOSE) IupExitLoop();
}

@end


@interface IupCocoaTouchDropTarget : NSObject <UIDropInteractionDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, copy) NSArray<NSString*>* types;
@end

@implementation IupCocoaTouchDropTarget

- (NSString*)firstMatchingUTI:(id<UIDropSession>)session
{
	for (NSString* uti in _types)
	{
		if ([session hasItemsConformingToTypeIdentifiers:@[uti]])
		{
			return uti;
		}
	}
	return nil;
}

- (BOOL)dropInteraction:(UIDropInteraction*)interaction canHandleSession:(id<UIDropSession>)session
{
	(void)interaction;
	if (!_ihandle || !iupObjectCheck(_ihandle) || [_types count] == 0) return NO;
	return [self firstMatchingUTI:session] != nil;
}

/* checks if the source marked the drag as move-capable (DRAGSOURCEMOVE) */
static BOOL cocoaTouchDropSessionWantsMove(id<UIDropSession> session)
{
	id<UIDragSession> local = session.localDragSession;
	if (!local || local.items.count == 0) return NO;
	id marker = [local.items[0] localObject];
	return [marker isEqual:@"MOVE"];
}

- (UIDropProposal*)dropInteraction:(UIDropInteraction*)interaction sessionDidUpdate:(id<UIDropSession>)session
{
	UIDropOperation op;
	if ([self firstMatchingUTI:session] == nil)
		op = UIDropOperationForbidden;
	else
		op = cocoaTouchDropSessionWantsMove(session) ? UIDropOperationMove : UIDropOperationCopy;

	if (_ihandle && iupObjectCheck(_ihandle))
	{
		IFniis motion_cb = (IFniis)IupGetCallback(_ihandle, "DROPMOTION_CB");
		if (motion_cb)
		{
			CGPoint pt = [session locationInView:interaction.view];
			char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
			if (motion_cb(_ihandle, (int)pt.x, (int)pt.y, status) == IUP_CLOSE) IupExitLoop();
		}
	}

	return [[[UIDropProposal alloc] initWithDropOperation:op] autorelease];
}

/* dispatches file-URL items through DROPFILES_CB; YES if handled, NO to fall through */
- (BOOL)handleFileDrop:(id<UIDropSession>)session interaction:(UIDropInteraction*)interaction
{
	if (!iupAttribGetBoolean(_ihandle, "DROPFILESTARGET")) return NO;
	IFnsiii files_cb = (IFnsiii)IupGetCallback(_ihandle, "DROPFILES_CB");
	if (!files_cb) return NO;

	NSArray<UIDragItem*>* items = [session items];
	int total = (int)items.count;
	CGPoint pt = [session locationInView:interaction.view];
	int drop_x = (int)pt.x;
	int drop_y = (int)pt.y;
	Ihandle* ih = _ihandle;

	for (int i = 0; i < total; i++)
	{
		UIDragItem* item = items[i];
		NSItemProvider* provider = [item itemProvider];
		if (![provider canLoadObjectOfClass:[NSURL class]]) continue;

		int remaining = total - i - 1;
		[provider loadObjectOfClass:[NSURL class]
			completionHandler:^(id<NSItemProviderReading> obj, NSError* error) {
				if (error || !obj || ![obj isKindOfClass:[NSURL class]]) return;
				NSURL* url = (NSURL*)obj;
				dispatch_async(dispatch_get_main_queue(), ^{
					if (!iupObjectCheck(ih)) return;
					const char* path_cstr = [[url path] UTF8String];
					if (!path_cstr || strlen(path_cstr) >= 2048) return;
					char path[2048];
					strlcpy(path, path_cstr, sizeof(path));
					files_cb(ih, path, remaining, drop_x, drop_y);
				});
			}];
	}
	return YES;
}

- (void)dropInteraction:(UIDropInteraction*)interaction performDrop:(id<UIDropSession>)session
{
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;

	if ([self handleFileDrop:session interaction:interaction]) return;

	IFnsViii drop_cb = (IFnsViii)IupGetCallback(_ihandle, "DROPDATA_CB");
	if (!drop_cb) return;

	NSString* match = [self firstMatchingUTI:session];
	if (!match) return;

	CGPoint pt = [session locationInView:interaction.view];
	int drop_x = (int)pt.x;
	int drop_y = (int)pt.y;
	Ihandle* ih = _ihandle;
	NSString* type_copy = [[match copy] autorelease];

	for (UIDragItem* item in [session items])
	{
		NSItemProvider* provider = [item itemProvider];
		if (![provider hasItemConformingToTypeIdentifier:type_copy]) continue;

		[provider loadDataRepresentationForTypeIdentifier:type_copy
			completionHandler:^(NSData* data, NSError* error) {
				if (error || !data) return;
				NSUInteger dlen = [data length];
				if (dlen > (NSUInteger)INT_MAX) return;
				dispatch_async(dispatch_get_main_queue(), ^{
					if (!iupObjectCheck(ih)) return;
					char type_cstr[128];
					strlcpy(type_cstr, [type_copy UTF8String], sizeof(type_cstr));
					drop_cb(ih,
					        type_cstr,
					        (void*)[data bytes],
					        (int)dlen,
					        drop_x,
					        drop_y);
				});
			}];
	}
}

@end


static UIView* cocoaTouchDragGetView(Ihandle* ih)
{
	if ([(id)ih->handle isKindOfClass:[UIView class]])
	{
		return (UIView*)ih->handle;
	}
	return nil;
}

static IupCocoaTouchDragSource* cocoaTouchDragEnsureSource(UIView* view)
{
	IupCocoaTouchDragSource* src = objc_getAssociatedObject(view, IUPCOCOATOUCH_DRAG_SOURCE_OBJ_KEY);
	if (src) return src;
	src = [[IupCocoaTouchDragSource alloc] init];
	objc_setAssociatedObject(view, IUPCOCOATOUCH_DRAG_SOURCE_OBJ_KEY, src, OBJC_ASSOCIATION_RETAIN);
	[src release];
	return src;
}

static IupCocoaTouchDropTarget* cocoaTouchDragEnsureTarget(UIView* view)
{
	IupCocoaTouchDropTarget* tgt = objc_getAssociatedObject(view, IUPCOCOATOUCH_DROP_TARGET_OBJ_KEY);
	if (tgt) return tgt;
	tgt = [[IupCocoaTouchDropTarget alloc] init];
	objc_setAssociatedObject(view, IUPCOCOATOUCH_DROP_TARGET_OBJ_KEY, tgt, OBJC_ASSOCIATION_RETAIN);
	[tgt release];
	return tgt;
}

static int cocoaTouchDragSetDragSourceAttrib(Ihandle* ih, const char* value)
{
	UIView* view = cocoaTouchDragGetView(ih);
	if (!view) return 1;

	UIDragInteraction* existing = objc_getAssociatedObject(view, IUPCOCOATOUCH_DRAG_INTERACTION_KEY);

	if (iupStrBoolean(value))
	{
		IupCocoaTouchDragSource* src = cocoaTouchDragEnsureSource(view);
		src.ihandle = ih;
		if (!src.types)
		{
			const char* types = iupAttribGet(ih, "DRAGTYPES");
			src.types = iupCocoaTouchDragParseTypes(types);
		}
		if (!existing)
		{
			UIDragInteraction* drag = [[[UIDragInteraction alloc] initWithDelegate:src] autorelease];
			[drag setEnabled:YES];
			[view addInteraction:drag];
			objc_setAssociatedObject(view, IUPCOCOATOUCH_DRAG_INTERACTION_KEY, drag, OBJC_ASSOCIATION_ASSIGN);
		}
		else
		{
			[existing setEnabled:YES];
		}
	}
	else if (existing)
	{
		[existing setEnabled:NO];
	}
	return 1;
}

static int cocoaTouchDragSetDragTypesAttrib(Ihandle* ih, const char* value)
{
	UIView* view = cocoaTouchDragGetView(ih);
	if (!view) return 1;
	IupCocoaTouchDragSource* src = cocoaTouchDragEnsureSource(view);
	src.ihandle = ih;
	src.types = iupCocoaTouchDragParseTypes(value);
	return 1;
}

static int cocoaTouchDragSetDropTargetAttrib(Ihandle* ih, const char* value)
{
	UIView* view = cocoaTouchDragGetView(ih);
	if (!view) return 1;

	UIDropInteraction* existing = objc_getAssociatedObject(view, IUPCOCOATOUCH_DROP_INTERACTION_KEY);

	if (iupStrBoolean(value))
	{
		IupCocoaTouchDropTarget* tgt = cocoaTouchDragEnsureTarget(view);
		tgt.ihandle = ih;
		if (!tgt.types)
		{
			const char* types = iupAttribGet(ih, "DROPTYPES");
			tgt.types = iupCocoaTouchDragParseTypes(types);
		}
		if (!existing)
		{
			UIDropInteraction* drop = [[[UIDropInteraction alloc] initWithDelegate:tgt] autorelease];
			[view addInteraction:drop];
			objc_setAssociatedObject(view, IUPCOCOATOUCH_DROP_INTERACTION_KEY, drop, OBJC_ASSOCIATION_ASSIGN);
		}
		else
		{
			[view addInteraction:existing];
		}
	}
	else if (existing)
	{
		[view removeInteraction:existing];
	}
	return 1;
}

static int cocoaTouchDragSetDropTypesAttrib(Ihandle* ih, const char* value)
{
	UIView* view = cocoaTouchDragGetView(ih);
	if (!view) return 1;
	IupCocoaTouchDropTarget* tgt = cocoaTouchDragEnsureTarget(view);
	tgt.ihandle = ih;
	tgt.types = iupCocoaTouchDragParseTypes(value);
	return 1;
}

/* DRAGDROP and DROPFILESTARGET both enable a file-URL drop target firing DROPFILES_CB */
static int cocoaTouchDragSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
	UIView* view = cocoaTouchDragGetView(ih);
	if (!view) return 1;

	BOOL enable = iupStrBoolean(value) ? YES : NO;
	iupAttribSetStr(ih, "DROPFILESTARGET", enable ? "YES" : "NO");

	if (!enable)
	{
		UIDropInteraction* existing = objc_getAssociatedObject(view, IUPCOCOATOUCH_DROP_INTERACTION_KEY);
		if (existing) [view removeInteraction:existing];
		return 1;
	}

	IupCocoaTouchDropTarget* tgt = cocoaTouchDragEnsureTarget(view);
	tgt.ihandle = ih;
	/* accept plain + security-scoped file URLs */
	NSMutableArray<NSString*>* types = [NSMutableArray arrayWithArray:(tgt.types ?: @[])];
	NSString* file_uti = IUPCOCOATOUCH_UTI_FILE_URL;
	if (![types containsObject:file_uti]) [types addObject:file_uti];
	tgt.types = types;

	UIDropInteraction* existing = objc_getAssociatedObject(view, IUPCOCOATOUCH_DROP_INTERACTION_KEY);
	if (!existing)
	{
		UIDropInteraction* drop = [[[UIDropInteraction alloc] initWithDelegate:tgt] autorelease];
		[view addInteraction:drop];
		objc_setAssociatedObject(view, IUPCOCOATOUCH_DROP_INTERACTION_KEY, drop, OBJC_ASSOCIATION_ASSIGN);
	}
	return 1;
}

IUP_SDK_API void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
	iupClassRegisterCallback(ic, "DRAGBEGIN_CB", "ii");
	iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
	iupClassRegisterCallback(ic, "DRAGDATA_CB", "sVi");
	iupClassRegisterCallback(ic, "DRAGEND_CB", "i");
	iupClassRegisterCallback(ic, "DROPDATA_CB", "sViii");
	iupClassRegisterCallback(ic, "DROPMOTION_CB", "iis");
	iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");

	iupClassRegisterAttribute(ic, "DRAGSOURCE", NULL, cocoaTouchDragSetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DRAGTYPES", NULL, cocoaTouchDragSetDragTypesAttrib, NULL, NULL, IUPAF_NO_INHERIT);

	/* drag cursor follows UIDropOperation; no public override */
	iupClassRegisterAttribute(ic, "DRAGCURSOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DRAGCURSORCOPY", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "DROPTARGET", NULL, cocoaTouchDragSetDropTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DROPTYPES", NULL, cocoaTouchDragSetDropTypesAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, cocoaTouchDragSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DRAGDROP", NULL, cocoaTouchDragSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
