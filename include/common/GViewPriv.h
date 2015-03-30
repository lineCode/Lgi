#ifndef _GVIEW_PRIV_
#define _GVIEW_PRIV_

#if WINNATIVE
	#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x501
		#undef _WIN32_WINNT
		#define _WIN32_WINNT 0x501
	#endif

	#include "commctrl.h"
	#include "Uxtheme.h"
#endif

#define PAINT_VIRTUAL_CHILDREN	1

extern bool In_SetWindowPos;
extern GMouse &lgi_adjust_click(GMouse &Info, GViewI *Wnd, bool Debug = false);

class GViewIter : public GViewIterator
{
	GView *v;
	List<GViewI>::I i;

public:
	GViewIter(GView *view);
	~GViewIter() {}
	GViewI *First();
	GViewI *Last();
	GViewI *Next();
	GViewI *Prev();
	int Length();
	int IndexOf(GViewI *view);
	GViewI *operator [](int Idx);
};

#if !WINNATIVE
class GPulseThread : public GThread
{
	GView *View;
	int Length;

public:
	bool Loop;

	GPulseThread(GView *view, int len) : GThread("GPulseThread")
	{
		LgiAssert(view);
		
		Loop = true;
		View = view;
		Length = len;
		
		Run();
	}
	
	~GPulseThread()
	{
		LgiAssert(Loop == false);
	}

	void Delete()
	{
		DeleteOnExit = true;
		Loop = false;
	}

	int Main()
	{
		while (Loop)
		{
			LgiSleep(Length);
			if (Loop && View)
			{
				if (!View->PostEvent(M_PULSE))
				{
					printf("%s:%i - Pulse PostEvent failed. Exiting loop.\n", _FL);
					Loop = false;
				}
			}
		}
		
		return 0;
	}
};
#endif

class GViewPrivate
{
public:
	// General
	int				CtrlId;
	GDragDropSource	*DropSource;
	GDragDropTarget	*DropTarget;
	bool			IsThemed;

	// Heirarchy
	GViewI			*ParentI;
	GView			*Parent;
	GViewI			*Notify;

	// Size
	GdcPt2			MinimumSize;
	
	// Font
	GFont			*Font;
	bool			FontOwn;
	
	// Style
	GAutoPtr<GCss>  Css;

	// OS Specific
	#if WINNATIVE

		static OsView	hPrevCapture;
		int				WndStyle;				// Win32 hWnd Style
		int				WndExStyle;				// Win32 hWnd ExStyle
		int				WndDlgCode;				// Win32 Dialog Code (WM_GETDLGCODE)
		const char		*WndClass;
		UINT			TimerId;
		HTHEME			hTheme;

	#else

		// Cursor
		GPulseThread	*Pulse;
		GView			*Popup;
		bool			TabStop;

		#if defined __GTK_H__
		#elif defined(MAC) && !defined(COCOA)
		static HIObjectClassRef BaseClass;
		#endif

	#endif
	
	#ifdef MAC
	EventHandlerRef DndHandler;
	GAutoString AcceptedDropFormat;
	#endif
	
	GViewPrivate();
	~GViewPrivate();
	
	GView *GetParent()
	{
		if (Parent)
			return Parent;
		
		if (ParentI)
			return ParentI->GetGView();

		return 0;
	}
};

#endif
