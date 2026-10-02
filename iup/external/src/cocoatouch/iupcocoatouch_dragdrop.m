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

	return iupCocoaTouchStrToNSString(iup_type);
}

NSArray<NSString*>* iupCocoaTouchDragParseTypes(const char* csv)
{
	if (!csv || !*csv) return @[];
	NSMutableArray<NSString*>* out = [NSMutableArray array];
	NSArray<NSString*>* parts = [iupCocoaTouchStrToNSString(csv) componentsSeparatedByString:@","];
	for (NSString* part in parts)
	{
		NSString* trimmed = [part stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
		if ([trimmed length] == 0) continue;
		NSString* uti = iupCocoaTouchDragTypeToUTI([trimmed UTF8String]);
		if (uti) [out addObject:uti];
	}
	return out;
}

@interface IupCocoaTouchDragContext : NSObject
@property(nonatomic, assign) Ihandle* source;
@property(nonatomic, assign) int pending;
@property(nonatomic, assign) BOOL ended;
@property(nonatomic, assign) int action;
@end

@implementation IupCocoaTouchDragContext
@end

static void cocoaTouchDragCallEnd(Ihandle* ih, int action)
{
	if (!iupObjectCheck(ih)) return;
	IFni end_cb = (IFni)IupGetCallback(ih, "DRAGEND_CB");
	if (end_cb && end_cb(ih, action) == IUP_CLOSE) IupExitLoop();
}

IUP_DRV_API void iupCocoaTouchDragBegin(id<UIDragSession> session, Ihandle* ih)
{
	IupCocoaTouchDragContext* ctx = [[IupCocoaTouchDragContext alloc] init];
	ctx.source = ih;
	session.localContext = ctx;
	[ctx release];
}

/* the session ends before a drop in this app has its data, so DRAGEND waits for those drops */
IUP_DRV_API void iupCocoaTouchDragEnd(id<UIDragSession> session, Ihandle* ih, int action)
{
	id local = session.localContext;
	if ([local isKindOfClass:[IupCocoaTouchDragContext class]] && ((IupCocoaTouchDragContext*)local).pending > 0)
	{
		((IupCocoaTouchDragContext*)local).ended = YES;
		((IupCocoaTouchDragContext*)local).action = action;
		return;
	}
	cocoaTouchDragCallEnd(ih, action);
}

IUP_DRV_API id iupCocoaTouchDropBegin(id<UIDropSession> session)
{
	id local = session.localDragSession.localContext;
	if (![local isKindOfClass:[IupCocoaTouchDragContext class]]) return nil;
	((IupCocoaTouchDragContext*)local).pending++;
	return local;
}

IUP_DRV_API void iupCocoaTouchDropDone(id drag_context)
{
	IupCocoaTouchDragContext* ctx = drag_context;
	if (!ctx) return;
	ctx.pending--;
	if (ctx.pending == 0 && ctx.ended)
	{
		ctx.ended = NO;
		cocoaTouchDragCallEnd(ctx.source, ctx.action);
	}
}

/* UIKit loads drag data on a background queue; the IUP callbacks must run on the main thread */
IUP_DRV_API void iupCocoaTouchDragLoadData(Ihandle* ih, NSString* uti, void (^completion)(NSData*, NSError*))
{
	dispatch_async(dispatch_get_main_queue(), ^{
		IFns size_cb = iupObjectCheck(ih) ? (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB") : NULL;
		IFnsVi data_cb = iupObjectCheck(ih) ? (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB") : NULL;
		if (!size_cb || !data_cb)
		{
			completion(nil, nil);
			return;
		}
		char type_cstr[128];
		strlcpy(type_cstr, [uti UTF8String], sizeof(type_cstr));
		int size = size_cb(ih, type_cstr);
		if (size <= 0 || !iupObjectCheck(ih))
		{
			completion(nil, nil);
			return;
		}
		NSMutableData* buf = [NSMutableData dataWithLength:(NSUInteger)size];
		data_cb(ih, type_cstr, [buf mutableBytes], size);
		completion(buf, nil);
	});
}

@interface IupCocoaTouchDragSource : NSObject <UIDragInteractionDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, copy) NSArray<NSString*>* types;
@end


@implementation IupCocoaTouchDragSource

- (void)dealloc
{
	[_types release];
	[super dealloc];
}

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
				iupCocoaTouchDragLoadData(ih, uti_copy, completion);
				return nil;
			}
		];
	}

	iupCocoaTouchDragBegin(session, _ihandle);
	UIDragItem* drag_item = [[[UIDragItem alloc] initWithItemProvider:provider] autorelease];
	/* same-app drops read localObject for DRAGSOURCEMOVE */
	if (iupAttribGetBoolean(_ihandle, "DRAGSOURCEMOVE"))
		drag_item.localObject = @"MOVE";
	return @[drag_item];
}

- (void)dragInteraction:(UIDragInteraction*)interaction session:(id<UIDragSession>)session didEndWithOperation:(UIDropOperation)operation
{
	(void)interaction;
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;

	int action;
	switch (operation)
	{
		case UIDropOperationMove:   action =  1; break;
		case UIDropOperationCopy:   action =  0; break;
		default:                    action = -1; break;
	}
	iupCocoaTouchDragEnd(session, _ihandle, action);
}

@end


@interface IupCocoaTouchDropTarget : NSObject <UIDropInteractionDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, copy) NSArray<NSString*>* types;
@end

@implementation IupCocoaTouchDropTarget

- (void)dealloc
{
	[_types release];
	[super dealloc];
}

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
	BOOL handled = NO;

	for (int i = 0; i < total; i++)
	{
		UIDragItem* item = items[i];
		NSItemProvider* provider = [item itemProvider];
		if (![provider canLoadObjectOfClass:[NSURL class]]) continue;
		handled = YES;

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
	return handled;
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

		id drag_context = iupCocoaTouchDropBegin(session);
		[provider loadDataRepresentationForTypeIdentifier:type_copy
			completionHandler:^(NSData* data, NSError* error) {
				dispatch_async(dispatch_get_main_queue(), ^{
					if (!error && data && [data length] <= (NSUInteger)INT_MAX && iupObjectCheck(ih))
					{
						char type_cstr[128];
						strlcpy(type_cstr, [type_copy UTF8String], sizeof(type_cstr));
						drop_cb(ih, type_cstr, (void*)[data bytes], (int)[data length], drop_x, drop_y);
					}
					iupCocoaTouchDropDone(drag_context);
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
