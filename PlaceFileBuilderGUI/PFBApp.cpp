#include "PFBApp.hpp"
#include "../src/PlaceFileColor.hpp"

#include <string>

#include <Shobjidl.h>

using namespace std;

// Control Values
#define IDB_ADD            1001 // Add Button
#define IDB_ADD_SHAPEFILE  1002 // Add a shapefile
#define IDB_ADD_FILEGDB    1003 // Add a file geo database
#define IDB_ADD_KML        1004 // Add a KML/KMZ file
#define IDB_DELETE         1005 // Delete button
#define IDB_DELETE_ALL     1006 // Delete all button
#define IDC_TREEVIEW       1007 // Treeview for layers
#define IDC_COMBO_LABEL    1008 // Combo box for layer label field
#define IDB_COLOR_BUTTON   1009 // Choose the color of the features
#define IDB_POLYGON_CHECK  1010 // Fill polygons check button
#define IDC_DISP_TRACK_BAR 1011 // Trackbar for setting display threshold
#define IDC_TITLE_EDIT     1012 // Edit control for setting placefile title

PFBApp::PFBApp(HINSTANCE hInstance) : 
  MainWindow{ hInstance, NULL }, appCon_{}, addButton_{ NULL }, 
  deleteButton_{ NULL }, deleteAllButton_{ NULL }, treeView_{ NULL }, 
  labelFieldComboBox_{ NULL }, colorButton_{ NULL }, colorButtonColor_{ NULL },
  fillPolygonsCheck_{ NULL }, displayThreshStatic_{ NULL }, 
  displayThreshTrackBar_{ NULL }, titleEditControl_{ NULL }
{
  // Initialize COM controls
  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  if (FAILED(hr)) HandleFatalComError(__FILEW__, __LINE__, hr);

  // Set the width
  width_ = 600;
  height_ = 700;

  // Get the screen dimensions
  int screen_w = GetSystemMetrics(SM_CXSCREEN);
  int screen_h = GetSystemMetrics(SM_CYSCREEN);

  // Center on screen
  xPos_ = (screen_w - width_) / 2;
  yPos_ = (screen_h - height_) / 2;

  // Load state from the last run of the application
  WCHAR buff1[MAX_PATH];
  WCHAR buff2[MAX_PATH];
  GetModuleFileNameW(NULL, buff1, sizeof(buff1) / sizeof(WCHAR));
  _wsplitpath_s(buff1, NULL, 0, buff2, sizeof(buff2) / sizeof(WCHAR), NULL, 0, NULL, 0);
  pathToAppConSavedState_ = narrow(buff2) + "..\\config\\appState.txt";
  appCon_.loadState(pathToAppConSavedState_);
}

PFBApp::~PFBApp()
{
  appCon_.saveState(pathToAppConSavedState_);
  DeleteObject(colorButtonColor_);
  CoUninitialize();
}

LRESULT PFBApp::WindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_CREATE:
    buildGUI_();
    break;
  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDB_ADD:
      addAction_();
      break;
    case IDB_ADD_SHAPEFILE:
      addFileAction_(FileTypes_::SHP);
      break;
    case IDB_ADD_FILEGDB:
      addFileAction_(FileTypes_::GDB);
      break;
    case IDB_ADD_KML:
      addFileAction_(FileTypes_::KML);
      break;
    case IDB_DELETE:
      deleteAction_();
      break;
    case IDB_DELETE_ALL:
      deleteAllAction_();
      break;
    case IDC_COMBO_LABEL:
      labelFieldCommandAction_(wParam, lParam);
      break;
    case IDB_COLOR_BUTTON:
      colorButtonAction_();
      break;
    case IDB_POLYGON_CHECK:
      fillPolygonsCheckAction_();
      break;
    case IDC_TITLE_EDIT:
      editTitleAction_();
      break;
    }
    break;
  case WM_NOTIFY:
    {
      // Unpack the notification
      LPNMHDR lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
      UINT srcID = static_cast<UINT>(wParam);

      // Check notifications from the treeView_
      if (lpnmhdr->hwndFrom == treeView_)
      {
        switch (lpnmhdr->code)
        {
          // Check to see if we should accept the selection change
        case TVN_SELCHANGINGW:
          return preventSelectionChange_(lParam);
        case TVN_SELCHANGEDW:
          updatePropertyControls_();
          break;
        }
      }
    }
    // End processing for WM_NOTIFY
    break;
  case WM_DRAWITEM:
    {
      switch (wParam)
      {
      case IDB_COLOR_BUTTON:
        updateColorButton_(lParam);
        break;
      }
    }
    break;
  case WM_CTLCOLORBTN:
    return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_3DFACE));
  case WM_HSCROLL:
    if ((HWND)lParam == displayThreshTrackBar_)
    {
      displayThreshAction_();
    }
    break;
  }

  // Always check the default handling too.
  return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

void PFBApp::buildGUI_()
{
  // Handle for checking error status of some window creations.
  HWND temp = NULL;

  // All objects right of the tree can use this position as a reference
  const int middleBorder = 295;
  const int labelFieldsWidth = 100;

  // Create the addButton_
  addButton_ = CreateWindowExW(
    NULL,
    WC_BUTTON,
    L"Add Source",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
    5, 5, 90, 30,
    hwnd_,
    reinterpret_cast<HMENU>(IDB_ADD),
    NULL, NULL);
  if (!addButton_) { HandleFatalError(widen(__FILE__).c_str(), __LINE__); }

  // Create the deleteButton_
  deleteButton_ = CreateWindowExW(
    NULL,
    WC_BUTTON,
    L"Delete Layer",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD,
    100, 5, 95, 30,
    hwnd_,
    reinterpret_cast<HMENU>(IDB_DELETE),
    NULL, NULL);
  if (!deleteButton_) { HandleFatalError(__FILEW__, __LINE__); }

  // Create the deleteButton_
  deleteAllButton_ = CreateWindowExW(
    NULL,
    WC_BUTTON,
    L"Delete All",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD,
    200, 5, 90, 30,
    hwnd_,
    reinterpret_cast<HMENU>(IDB_DELETE_ALL),
    NULL, NULL);
  if (!deleteAllButton_) { HandleFatalError(__FILEW__, __LINE__); }

  // Add the treeview
  treeView_ = CreateWindowExW(
    WS_EX_CLIENTEDGE | TVS_EX_AUTOHSCROLL,
    WC_TREEVIEW,
    L"Layers",
    WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_DISABLEDRAGDROP | TVS_FULLROWSELECT | TVS_SHOWSELALWAYS,
    5, 40, 285, 450,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_TREEVIEW),
    NULL, NULL);
  if (!treeView_) { HandleFatalError(__FILEW__, __LINE__); }

  // Add label for the 'Label Field' controller.
  temp = CreateWindowExW(
    NULL,
    WC_STATICW,
    L"Label Field:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    middleBorder + 5, 40 + 7, labelFieldsWidth, 30,
    hwnd_,
    NULL,
    NULL, NULL);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }

  // Add labelFieldComboBox_
  labelFieldComboBox_ = CreateWindowExW(
    NULL,
    WC_COMBOBOXW,
    L"",
    WS_VISIBLE | WS_CHILD | CBS_AUTOHSCROLL| CBS_DROPDOWNLIST | WS_HSCROLL | WS_VSCROLL,
    middleBorder + 110, 40, 175, 500, // Height also includes dropdown box
    hwnd_,
    reinterpret_cast<HMENU>(IDC_COMBO_LABEL),
    NULL, NULL);
  if (!labelFieldComboBox_) { HandleFatalError(__FILEW__, __LINE__); }

  // Add label for the 'Feature Color' controller.
  temp = CreateWindowExW(
    NULL,
    WC_STATICW,
    L"Color:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    middleBorder + 5, 75 + 6, labelFieldsWidth, 30,
    hwnd_,
    NULL,
    NULL, NULL);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }

  // Create the colorButton_
  colorButton_ = CreateWindowExW(
    NULL,
    WC_BUTTON,
    L"",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD |BS_OWNERDRAW,
    middleBorder + 110, 75, 30, 30,
    hwnd_,
    reinterpret_cast<HMENU>(IDB_COLOR_BUTTON),
    NULL, NULL);
  if (!colorButton_) { HandleFatalError(__FILEW__, __LINE__); }

  // Add label for the 'Line Width' controller.
  temp = CreateWindowExW(
    NULL,
    WC_STATICW,
    L"Line Width:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    middleBorder + 5, 110 + 6, labelFieldsWidth, 30,
    hwnd_,
    NULL,
    NULL, NULL);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }

  // TODO add the line width control

  // Add label for the Fill Polygons checkbox controller.
  temp = CreateWindowExW(
    NULL,
    WC_STATICW,
    L"Fill Polygons:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    middleBorder + 5, 145 + 6, labelFieldsWidth, 30,
    hwnd_,
    NULL,
    NULL, NULL);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }

  // Add the fillPolygonsCheck_
  fillPolygonsCheck_ = CreateWindowExW(
    NULL,
    WC_BUTTON,
    L"",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
    middleBorder + 110, 145, 30, 30,
    hwnd_,
    reinterpret_cast<HMENU>(IDB_POLYGON_CHECK),
    NULL, NULL);
  if (!fillPolygonsCheck_) { HandleFatalError(__FILEW__, __LINE__); }

  // Add label for the displayThreshEdit_ control.
  temp = CreateWindowExW(
    NULL,
    WC_STATICW,
    L"Disp Thresh:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    middleBorder + 5, 180 + 6, labelFieldsWidth, 30,
    hwnd_,
    NULL,
    NULL, NULL);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }

  // Add the displayThreshStatic_
  displayThreshStatic_ = CreateWindowExW(
    NULL,
    WC_STATICW,
    L"999",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | SS_CENTER,
    middleBorder + 110, 180 + 6, labelFieldsWidth, 30,
    hwnd_,
    reinterpret_cast<HMENU>(IDB_POLYGON_CHECK),
    NULL, NULL);
  if (!displayThreshStatic_) { HandleFatalError(__FILEW__, __LINE__); }

  // Add the displayThreshTrackBar_
  displayThreshTrackBar_ = CreateWindowExW(
    NULL,
    TRACKBAR_CLASS,
    L"",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS,
    middleBorder + 5, 215, labelFieldsWidth + 175, 30,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_DISP_TRACK_BAR),
    NULL, NULL);
  if (!fillPolygonsCheck_) { HandleFatalError(__FILEW__, __LINE__); }
  SendMessage(displayThreshTrackBar_, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 100));
  SendMessage(displayThreshTrackBar_, TBM_SETPAGESIZE, 0, 10);
  SendMessage(displayThreshTrackBar_, TBM_SETTICFREQ, 10, 0);

  // Add a label for the titleEditControl_
  temp = CreateWindowExW(
    NULL,
    WC_STATICW,
    L"Title:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    5, 495 + 8, labelFieldsWidth, 20,
    hwnd_,
    NULL,
    NULL, NULL);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }

  // Add the titleEditControl_
  titleEditControl_ = CreateWindowExW(
    WS_EX_CLIENTEDGE,
    WC_EDITW,
    NULL,
    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL ,
    10 + labelFieldsWidth, 500, 180, 20,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_TITLE_EDIT),
    NULL, NULL);
  if (!titleEditControl_) { HandleFatalError(__FILEW__, __LINE__); }

  /****************************************************************************
  * Now that everything is built, initialize the GUI with pre-loaded data.
  ****************************************************************************/
  // Fill the tree
  vector<string> sources = appCon_.getSources();
  for (int i = 0; i < sources.size(); i++)
  {
    HTREEITEM tmp = addSrcToTree_(sources[i]);

    if (i == 0) TreeView_Select(treeView_, tmp, TVGN_CARET);
  }

  // Set the title
  wstring aPFTitle( widen(appCon_.getPFTitle()) );
  unique_ptr<WCHAR> wPFTitle = unique_ptr<WCHAR>(new WCHAR[aPFTitle.length() + 1]);
  wcscpy(wPFTitle.get(), aPFTitle.c_str());
  Edit_SetText(titleEditControl_, wPFTitle.get());
}

void PFBApp::updatePropertyControls_()
{
  // Get the currently selected source/layer
  string layer, source;
  bool success = getSourceLayerFromTree_(source, layer);

  if (!success)
  {
    MessageBoxW(hwnd_, L"Failed to delete layer.", L"Error", MB_OK | MB_ICONERROR);
    return;
  }

  //
  // Update the labelFieldComboBox_
  //
  auto fields = appCon_.getFields(source, layer);
  ComboBox_ResetContent(labelFieldComboBox_);
  for (string field: fields)
  {
    ComboBox_AddString(labelFieldComboBox_, widen(field).c_str());
  }
  ComboBox_SelectString(labelFieldComboBox_, -1, widen(appCon_.getLabel(source, layer)).c_str());
  
  //
  // Update the colorButton_
  //
  InvalidateRect(colorButton_, NULL, TRUE);

  //
  // Update the lineWidthSelector_
  //
  // TODO

  //
  // Update the Filled Polygon checkbox
  //
  bool checked = !appCon_.getPolygonDisplayedAsLine(source, layer);
  Button_SetCheck(fillPolygonsCheck_, checked);

  //
  // Update display threshold
  //
  int dispThresh = appCon_.getDisplayThreshold(source, layer);
  WCHAR tempText[8];
  swprintf_s(tempText,  L"%3d", dispThresh);
  Static_SetText(displayThreshStatic_, tempText);
  SendMessage(displayThreshTrackBar_, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(dispThresh / 10));
  
  
  // TODO more
}

void PFBApp::updateColorButton_(LPARAM lParam)
{
  string source, layer;
  bool success = getSourceLayerFromTree_(source, layer);
  if (!success) 
  {
    MessageBoxW(hwnd_, L"Error updating color button.", L"Error", MB_OK | MB_ICONERROR);
    return;
  }

  PlaceFileColor color = appCon_.getColor(source, layer);
  LPDRAWITEMSTRUCT dis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
  HDC hdc = dis->hDC;
  LPRECT rect = &dis->rcItem;

  // Get the brush color
  DeleteObject(colorButtonColor_);
  colorButtonColor_ = CreateSolidBrush(RGB(color.red,color.green,color.blue));
  HBRUSH old = (HBRUSH)SelectObject(hdc, colorButtonColor_);
  RoundRect(hdc, 0, 0, rect->right, rect->bottom, 0.3*rect->right, 0.3*rect->bottom);
  colorButtonColor_ = (HBRUSH)SelectObject(hdc, old);
}

HTREEITEM PFBApp::addSrcToTree_(const string & src)
{
  TVITEMW tvi = { 0 };
  TVINSERTSTRUCTW tvins = { 0 };
  HTREEITEM hSrc = (HTREEITEM)TVI_LAST;
  HTREEITEM hti;

  // Describe the item
  tvi.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE;
  auto dst = make_unique<wchar_t[]>(src.length() + 1);
  wcscpy(dst.get(), widen(src).c_str());
  tvi.pszText = dst.get();
  tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);
  tvi.cChildren = 1;
  tvi.state = TVIS_BOLD | TVIS_EXPANDED;
  tvi.stateMask = TVIS_BOLD | TVIS_EXPANDED;

  // Describe where to insert it
  tvins.item = tvi;
  tvins.hInsertAfter = hSrc;
  tvins.hParent = TVI_ROOT;

  // Add the item to the tree-view control. 
  hSrc = (HTREEITEM)SendMessage(treeView_, TVM_INSERTITEM,
    0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

  // Check for an error
  if (hSrc == NULL)
  {
    MessageBoxW(hwnd_, L"Error inserting item.", L"Error.", MB_OK | MB_ICONERROR);
    return NULL;
  }

  // Now add children nodes.
  tvi = { 0 };
  tvins = { 0 }; 
  for (const string& lyr : appCon_.getLayers(src))
  {
    tvi.mask = TVIF_TEXT | TVIF_CHILDREN;
    auto dst = make_unique<wchar_t[]>(lyr.length() + 1);
    wcscpy(dst.get(), widen(lyr).c_str());
    tvi.pszText = dst.get();
    tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);
    tvi.cChildren = 0;

    // Describe where to insert it
    tvins.item = tvi;
    tvins.hInsertAfter = TVI_LAST;
    tvins.hParent = hSrc;

    // Add the item to the tree-view control. 
    hti = (HTREEITEM)SendMessage(treeView_, TVM_INSERTITEM,
      0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

    if (hti == NULL)
    {
      MessageBoxW(hwnd_, L"Error inserting item.", L"Error.", MB_OK | MB_ICONERROR);
      return NULL;
    }
  }

  return hti;
}

BOOL PFBApp::preventSelectionChange_(LPARAM lparam)
{
  LPNMTREEVIEWW lpnmTv = reinterpret_cast<LPNMTREEVIEWW>(lparam);

  /* 
  //  Code used to debug, turns out the itemNew struct only has valid values in
  //  the lParam, hItem, and state fields.

  if (lpnmTv->itemNew.cChildren == 1) return TRUE;

  std::cerr << "FALSE : mask   : TVIF_CHILDREN   : " << (lpnmTv->itemNew.mask & TVIF_CHILDREN) << std::endl;
  std::cerr << "FALSE : mask   : TVIF_DI_SETITEM : " << (lpnmTv->itemNew.mask & TVIF_DI_SETITEM) << std::endl;
  std::cerr << "FALSE : mask   : TVIF_HANDLE     : " << (lpnmTv->itemNew.mask & TVIF_HANDLE) << std::endl;
  std::cerr << "FALSE : mask   : TVIF_PARAM      : " << (lpnmTv->itemNew.mask & TVIF_PARAM) << std::endl;
  std::cerr << "FALSE : lParam : " << (lpnmTv->itemNew.lParam) << std::endl;
  std::cerr << "FALSE : mask   : TVIF_STATE      : " << (lpnmTv->itemNew.mask & TVIF_STATE) << std::endl;
  std::cerr << "FALSE : mask   : TVIF_TEXT       : " << (lpnmTv->itemNew.mask & TVIF_TEXT) << std::endl << std::endl;
  */

  // If the new item has children, it cannot be selected.
  if (TreeView_GetParent(treeView_, lpnmTv->itemNew.hItem) == NULL) return TRUE;

  return FALSE;
}

bool PFBApp::getTreeItemText_(HTREEITEM hti, string& src)
{
  WCHAR itemChars[MAX_PATH];

  HREFTYPE hResult;
  TVITEMW tvi{ 0 };
  tvi.mask = TVIF_HANDLE | TVIF_TEXT;
  tvi.hItem = hti;
  tvi.pszText = itemChars;
  tvi.cchTextMax = MAX_PATH;
  hResult = SendMessageW(treeView_, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&tvi));
  if (FAILED(hResult))
  {
    MessageBoxW(hwnd_, L"Failed to get item text.", L"Error", MB_OK | MB_ICONERROR);
    return false;
  }

  src = narrow(itemChars);

  return true;
}

bool PFBApp::getSourceLayerFromTree_(string & source, string & layer)
{
  // Get the currently selected item from the tree, and its parent
  HTREEITEM hSelect = TreeView_GetSelection(treeView_);
  HTREEITEM hParent = TreeView_GetParent(treeView_, hSelect);
  if (hSelect == NULL || hParent == NULL) // Invalid selection
  {
    //MessageBoxW(hwnd_, L"Invalid Selection in tree.", L"Error", MB_OK | MB_ICONERROR);
    return false;
  }

  bool success = getTreeItemText_(hSelect, layer);
  success &= getTreeItemText_(hParent, source);

  if (!success)
  {
    MessageBoxW(hwnd_, L"Failed to get tree text.", L"Error", MB_OK | MB_ICONERROR);
  }

  return success;
}

void PFBApp::addAction_()
{
  // Popup a menu to decide what type to add

  // Create the menu items
  HMENU popUpMenu = CreatePopupMenu();
  InsertMenuW(popUpMenu, -1, MF_BYPOSITION | MF_STRING, IDB_ADD_SHAPEFILE, L"Add Shapefile"       );
  InsertMenuW(popUpMenu, -1, MF_BYPOSITION | MF_STRING, IDB_ADD_FILEGDB,   L"Add File GeoDatabase");
  InsertMenuW(popUpMenu, -1, MF_BYPOSITION | MF_STRING, IDB_ADD_KML,       L"Add KML/KMZ"         );

  // Get the screen coordinates of the button
  RECT r;
  HWND h = GetDlgItem(hwnd_, IDB_ADD);
  GetWindowRect(h, &r);

  SetForegroundWindow(hwnd_);
  TrackPopupMenu(popUpMenu, TPM_TOPALIGN | TPM_LEFTALIGN, r.left, r.bottom, 0, hwnd_, NULL);
}

void PFBApp::addFileAction_(FileTypes_ tp)
{
  IFileOpenDialog *pFileOpen = NULL;
  IShellItem *pItem = NULL;
  LPWSTR lpszFilePath = nullptr;
  bool foundFile = false;
  string finalPath;

  HRESULT hr = CoCreateInstance(__uuidof(FileOpenDialog), NULL,
    CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpen));

  if(SUCCEEDED(hr))
  {
    // Set up the filter types
    COMDLG_FILTERSPEC fltr = { 0 };
    switch (tp)
    {
    case FileTypes_::SHP:
      fltr.pszName = L"Shapefile";
      fltr.pszSpec = L"*.shp;*.SHP";
      hr = pFileOpen->SetFileTypes(1, &fltr);
      break;
    case FileTypes_::KML:
      fltr.pszName = L"KML/KMZ";
      fltr.pszSpec = L"*.KML;*.KMZ;*.kmz;*.kml";
      hr = pFileOpen->SetFileTypes(1, &fltr);
      break;
    case FileTypes_::GDB:
      {
        DWORD dwOptions = NULL;
        hr = pFileOpen->GetOptions(&dwOptions);
        if (SUCCEEDED(hr))
        {
          hr = pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
        }
      }
      break;
    }
  }
  if(SUCCEEDED(hr))
  {
    hr = pFileOpen->Show(NULL);
  }
  if(SUCCEEDED(hr))
  {
    hr = pFileOpen->GetResult(&pItem);
  }
  if(SUCCEEDED(hr))
  {
    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &lpszFilePath);
  }
  if(SUCCEEDED(hr))
  {
    finalPath = narrow(lpszFilePath);
    foundFile = true;
  }

  // Clean up.
  SafeRelease(&pItem);
  SafeRelease(&pFileOpen);
  CoTaskMemFree(lpszFilePath);

  if(foundFile)
  {
    try
    {
      string addedSrc = appCon_.addSource(finalPath);
      addSrcToTree_(addedSrc);
    }
    catch (const runtime_error& e)
    {
      MessageBoxW(hwnd_, widen(e.what()).c_str(), L"ERROR!", MB_OK | MB_ICONEXCLAMATION);
    }
  }
  else // Check if this was an error or canceled.
  {
    DWORD errCode = CommDlgExtendedError();
    if (errCode != 0)
    {
      WCHAR errMsg[64];
      _snwprintf_s(errMsg, sizeof(errMsg) / sizeof(WCHAR), L"Error getting file name from system: code %#06X.", errCode);
      MessageBoxW(hwnd_, errMsg, L"ERROR", MB_OK | MB_ICONERROR);
    }
  }
}

void PFBApp::deleteAction_()
{
  // Get the currently selected item from the tree, and its parent
  HTREEITEM hSelect = TreeView_GetSelection(treeView_);
  HTREEITEM hParent = TreeView_GetParent(treeView_, hSelect);
  if (hSelect == NULL || hParent == NULL) return; // Invalid selection

  string layer;
  string source;
  bool success = getTreeItemText_(hSelect, layer);
  success &= getTreeItemText_(hParent, source);

  if (!success)
  {
    MessageBoxW(hwnd_, L"Failed to delete layer.", L"Error", MB_OK | MB_ICONERROR);
    return;
  }

  // Delete from the app controller
  bool deletedSource = appCon_.hideLayer(source, layer);

  HTREEITEM hDelete = hSelect;
  if (deletedSource)
  {
    hDelete = hParent;
  }
  if (FAILED(TreeView_DeleteItem(treeView_, hDelete)))
  {
    MessageBoxW(hwnd_, L"Failed to delete item from view...", L"Error", MB_OK | MB_ICONERROR);
  }
}

void PFBApp::deleteAllAction_()
{
  auto srcs = appCon_.getSources();

  for (string src : srcs)
  {
    appCon_.deleteSource(src);
  }

  TreeView_DeleteAllItems(treeView_);
}

void PFBApp::labelFieldCommandAction_(WPARAM wParam, LPARAM lParam)
{
  // Process selection change
  if (HIWORD(wParam) == CBN_SELCHANGE)
  {
    // Get the new selected text.
    int selectedIndex = ComboBox_GetCurSel(labelFieldComboBox_);
    if (selectedIndex == CB_ERR)
    {
      MessageBoxW(hwnd_, L"Error getting selected label index.", L"Error.", MB_OK | MB_ICONERROR);
      return;
    }
    size_t sz = ComboBox_GetLBTextLen(labelFieldComboBox_, selectedIndex);
    unique_ptr<WCHAR> selectedText (new WCHAR[sz + 1]); // RAII to recover memory
    sz = ComboBox_GetLBText(labelFieldComboBox_, selectedIndex, selectedText.get());
    if (selectedIndex == CB_ERR)
    {
      MessageBoxW(hwnd_, L"Error getting selected label index.", L"Error.", MB_OK | MB_ICONERROR);
      return;
    }
    string label = narrow(selectedText.get());

    // Get the currently selected source/layer
    string layer, source;
    bool success = getSourceLayerFromTree_(source, layer);
    if (!success)
    {
      MessageBoxW(hwnd_, L"Failed to get info from tree.", L"Error", MB_OK | MB_ICONERROR);
      return;
    }

    appCon_.setLabel(source, layer, label);
  }
}

void PFBApp::colorButtonAction_()
{
  CHOOSECOLOR cc{ 0 };            // common dialog box structure 
  static COLORREF acrCustClr[16]; // array of custom colors 

  // Get the current color
  LOGBRUSH lb{ 0 };
  GetObject(colorButtonColor_, sizeof(LOGBRUSH), &lb);
  COLORREF rgbCurrent = lb.lbColor;

  // Initialize cc struct
  cc.lStructSize = sizeof(cc);
  cc.hwndOwner = hwnd_;
  cc.lpCustColors = (LPDWORD)acrCustClr;
  cc.rgbResult = rgbCurrent;
  cc.Flags = CC_FULLOPEN | CC_RGBINIT;

  // Open the dialog
  BOOL success = ChooseColor(&cc);
  if (success != TRUE)
  {
    MessageBoxW(hwnd_, L"Error choosing color.", L"Error", MB_OK | MB_ICONERROR);
    return;
  }
  
  // Set the new colors
  string source, layer;
  bool success2 = getSourceLayerFromTree_(source, layer);
  if (!success2)
  {
    MessageBoxW(hwnd_, L"Error getting source/layer information from the tree.", L"Error", MB_OK | MB_ICONERROR);
    return;
  }
  appCon_.setColor(source, layer, PlaceFileColor(GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult)));
  InvalidateRect(colorButton_, NULL, TRUE);
}

void PFBApp::fillPolygonsCheckAction_()
{
  // Get the status of the box
  bool checked = Button_GetCheck(fillPolygonsCheck_) == BST_CHECKED;

  // Get the layer info to change
  string source, layer;
  bool success = getSourceLayerFromTree_(source, layer);
  if (!success)
  {
    MessageBoxW(hwnd_, L"Error getting info from tree.", L"Error", MB_OK | MB_ICONERROR);
    return;
  }

  appCon_.setPolygonDisplayedAsLine(source, layer, !checked);
}

void PFBApp::displayThreshAction_()
{
  // Get the value
  int pos = SendMessage(displayThreshTrackBar_, TBM_GETPOS, 0, 0);
  pos *= 10;
  if (pos > 999) pos = 999;

  // Set the value in the static control
  WCHAR tempText[8];
  swprintf_s(tempText, L"%3d", pos);
  Static_SetText(displayThreshStatic_, tempText);

  // Set the value in the controller
  string source, layer;
  bool success = getSourceLayerFromTree_(source, layer);
  if (!success)
  {
    MessageBoxW(hwnd_, L"Error updating display threshold.", L"Error.", MB_ICONERROR | MB_OK);
    return;
  }
  
  appCon_.setDisplayThreshold(source, layer, pos);
}

void PFBApp::editTitleAction_()
{
  int sizeOfText = Edit_GetTextLength(titleEditControl_) + 1;
  unique_ptr<WCHAR> newTitle(new WCHAR[sizeOfText]);
  Edit_GetText(titleEditControl_, newTitle.get(), sizeOfText);

  appCon_.setPFTitle(narrow(newTitle.get()));
}

