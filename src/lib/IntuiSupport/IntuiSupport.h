#ifndef HAVE_INTUITION_SUPPORT_H
#define HAVE_INTUITION_SUPPORT_H

#include <exec/types.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>

#define AE_MENU_DEPLOYED        200
#define AE_MENU_ITEXT_VS_IMAGE  201
#define AE_MENU_NO_WINDOW       202

typedef struct CIntuiText_ CIntuiText;
typedef struct CMenuItem_ CMenuItem;
typedef struct CMenuItemText_ CMenuItemText;
typedef struct CMenu_ CMenu;
typedef struct MenuItemUD_ MenuItemUD;

typedef void (*intuisupport_cb_t)(CMenuItem *item);

struct CIntuiText_
{
    intptr_t          **_vTablePtr;
    struct IntuiText   *_intuiText;
};

struct MenuItemUD_
{
    struct MenuItem     _item;
    CMenuItem          *_wrapper;
};

struct CMenuItem_
{
    intptr_t          **_vTablePtr;
    intuisupport_cb_t   _cb;
    intptr_t            _userData;
    CMenu              *_parent;
    MenuItemUD          _item;
    CMenuItem          *_subItem;
    CMenuItem          *_nextItem;
};

struct CMenuItemText_
{
    intptr_t          **_vTablePtr;
    intuisupport_cb_t   _cb;
    intptr_t            _userData;
    CMenu              *_parent;
    MenuItemUD          _item;
    CMenuItem          *_subItem;
    CMenuItem          *_nextItem;
};

struct CMenu_
{
    intptr_t          **_vTablePtr;
    struct Menu         _menu;
    CMenuItem          *_firstItem;
    CMenuItem          *_lastItem;
    CMenu              *_nextMenu;
    SHORT               _win_id;
};

VOID               _CINTUITEXT_CONSTRUCTOR (CIntuiText *THIS, STRPTR   text);
VOID               _CINTUITEXT_FRONTPEN    (CIntuiText *THIS, UBYTE    B);
UBYTE              _CINTUITEXT_FRONTPEN_   (CIntuiText *THIS);
VOID               _CINTUITEXT_BACKPEN     (CIntuiText *THIS, UBYTE    B);
UBYTE              _CINTUITEXT_BACKPEN_    (CIntuiText *THIS);
VOID               _CINTUITEXT_DRAWMODE    (CIntuiText *THIS, UBYTE    B);
UBYTE              _CINTUITEXT_DRAWMODE_   (CIntuiText *THIS);
VOID               _CINTUITEXT_LEFTEDGE    (CIntuiText *THIS, WORD     B);
WORD               _CINTUITEXT_LEFTEDGE_   (CIntuiText *THIS);
VOID               _CINTUITEXT_TOPEDGE     (CIntuiText *THIS, WORD     B);
WORD               _CINTUITEXT_TOPEDGE_    (CIntuiText *THIS);
VOID               _CINTUITEXT_TEXTFONT    (CIntuiText *THIS, struct TextAttr *B);
struct TextAttr   *_CINTUITEXT_TEXTFONT_   (CIntuiText *THIS);
VOID               _CINTUITEXT_TEXT        (CIntuiText *THIS, STRPTR   B);
STRPTR             _CINTUITEXT_TEXT_       (CIntuiText *THIS);
VOID               _CINTUITEXT_NEXTTEXT    (CIntuiText *THIS, CIntuiText *B);
CIntuiText        *_CINTUITEXT_NEXTTEXT_   (CIntuiText *THIS);
struct IntuiText  *_CINTUITEXT_INTUITEXT_  (CIntuiText *THIS);

VOID               _CMENUITEM_CONSTRUCTOR   (CMenuItem *THIS, CMenu *parent, intptr_t userData);
VOID               _CMENUITEM_CHECKIT       (CMenuItem *THIS, BOOL     B);
BOOL               _CMENUITEM_CHECKIT_      (CMenuItem *THIS);
VOID               _CMENUITEM_CHECKED       (CMenuItem *THIS, BOOL     B);
BOOL               _CMENUITEM_CHECKED_      (CMenuItem *THIS);
VOID               _CMENUITEM_TOGGLE        (CMenuItem *THIS, BOOL     B);
BOOL               _CMENUITEM_TOGGLE_       (CMenuItem *THIS);
VOID               _CMENUITEM_ENABLED       (CMenuItem *THIS, BOOL     B);
BOOL               _CMENUITEM_ENABLED_      (CMenuItem *THIS);
VOID               _CMENUITEM_COMMAND       (CMenuItem *THIS, BYTE     B);
BYTE               _CMENUITEM_COMMAND_      (CMenuItem *THIS);
VOID               _CMENUITEM_MUTUALEXCLUDE (CMenuItem *THIS, LONG     B);
LONG               _CMENUITEM_MUTUALEXCLUDE_(CMenuItem *THIS);
VOID               _CMENUITEM_HIGHFLAGS     (CMenuItem *THIS, UWORD    B);
UWORD              _CMENUITEM_HIGHFLAGS_    (CMenuItem *THIS);
VOID               _CMENUITEM_ADDSUBITEM    (CMenuItem *THIS, CMenuItem *SubItem);
VOID               _CMENUITEM_REMOVESUBITEMS(CMenuItem *THIS);
VOID               _CMENUITEM_NEXTITEM      (CMenuItem *THIS, CMenuItem *i);
CMenuItem         *_CMENUITEM_NEXTITEM_     (CMenuItem *THIS);
VOID               _CMENUITEM_BBOX          (CMenuItem *THIS, WORD *x1, WORD *y1, WORD *x2, WORD *y2);

VOID               _CMENUITEMTEXT___init         (CMenuItemText *THIS);
VOID               _CMENUITEMTEXT_CONSTRUCTOR    (CMenuItemText *THIS, STRPTR text, CMenu *parent, intptr_t userData);
VOID               _CMENUITEMTEXT_TEXT           (CMenuItemText *THIS, STRPTR s);
STRPTR             _CMENUITEMTEXT_TEXT_          (CMenuItemText *THIS);
VOID               _CMENUITEMTEXT_TEXTSELECTED   (CMenuItemText *THIS, STRPTR s);
STRPTR             _CMENUITEMTEXT_TEXTSELECTED_  (CMenuItemText *THIS);
VOID               _CMENUITEMTEXT_ITEXT          (CMenuItemText *THIS, struct IntuiText *s);
struct IntuiText  *_CMENUITEMTEXT_ITEXT_         (CMenuItemText *THIS);
VOID               _CMENUITEMTEXT_ITEXTSELECTED  (CMenuItemText *THIS, struct IntuiText *s);
struct IntuiText  *_CMENUITEMTEXT_ITEXTSELECTED_ (CMenuItemText *THIS);

VOID               _CMENU_CONSTRUCTOR      (CMenu *THIS, STRPTR Name, CMenu *prevMenu);
VOID               _CMENU_NEXTMENU         (CMenu *THIS, CMenu *x);
CMenu             *_CMENU_NEXTMENU_        (CMenu *THIS);
VOID               _CMENU_FIRSTITEM        (CMenu *THIS, CMenuItem *i);
CMenuItem         *_CMENU_FIRSTITEM_       (CMenu *THIS);
VOID               _CMENU_ENABLED          (CMenu *THIS, BOOL B);
BOOL               _CMENU_ENABLED_         (CMenu *THIS);
VOID               _CMENU_ADDITEM          (CMenu *THIS, CMenuItem *item);
VOID               _CMENU_REMOVEALLITEMS   (CMenu *THIS);
VOID               _CMENU_DEPLOY           (CMenu *THIS);
VOID               _CMENU_UNDEPLOY         (CMenu *THIS);

typedef struct
{
    struct Screen    *screen;
    struct TextFont  *screen_font;
    struct DrawInfo  *draw_info;
    CMenu            *deployedMenu;
    BOOL              close_cb_installed;
    BOOL              menu_msg_cb_installed;
} intuis_win_ext_t;

intuis_win_ext_t *_IntuiSupport_get_ext(short win_id);

#endif

