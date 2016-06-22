#pragma once

#include <Windows.h>
#include <tchar.h>
#include "resource.h"

#include <map>

/*
Base class for developing a main window for an application.

To use this class (or its subclasses):
 - Initialize the class by creating a variable
 - Call create() to register the window class (may be done at end of subclass 
   constructor)
 - Call run() to start the message loop.

To create your WndProc procedure, in a subclass override the protected virtual 
method WindowProc.

To use a custom icon for the application, make sure and create a resource with 
id = IDI_PRIMARY_ICON. The constructor (via the pre-processor) checks for this
value to be defined, if it is, it loads it as the icon. In order for this to
also be the compiled application's icon, it must be the icon with the lowest
value.
*/
class MainWindow
{
public:
  /*
  Constructor. When subclassing, call this constructor in your constructor,
  then only override the aspects of wc that you need to. Also add style 
  information via the protected member values dwExStyle_ and dwStyle_ below.

  Use the hMenu argument to assign a menu from a resource file, or create a 
  menu on the fly when creating other GUI elements.

  Arguments:
  hInstance - the application instance.
  hMenu     - the value from a resource file corresponding to the default menu
              for this window class.
  */
  MainWindow(HINSTANCE hInstance, int hMenu = NULL);
  MainWindow(const MainWindow& other);
  MainWindow(MainWindow && other);

  /*
  Once the wc_ struct and any styles are added in the subclass constructor, 
  call this method to register the class and show the window. 
  
  Since this is a base class, this method can be called in the constructor of a
  sub-class, but it is probably better to explicitly call it to be consistent 
  and clear.
  */
  void create(int nCmdShow, LPTSTR title);

  /*
  Just implements the message loop. Call when you are ready to execute.
  */
  int run();

  /*
  Virtual destructor, it is likely that sub-classes will allocate resources and
  want to destroy them.
  */
  virtual ~MainWindow();

protected:
  // Window class definition
  WNDCLASSEX wc_;

  // Window style flags
  // dwExStyle_ defaults to 0 (use all Win32 API defaults)
  // dwStyle_ defaults to :
  //   WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX
  DWORD dwExStyle_; // Extended window style flags
  DWORD dwStyle_;   // Window style flags 

  // Window position and size
  int xPos_;
  int yPos_;
  int width_;
  int height_;

  // Override this function to define your own message handler!
  virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

private:
  HWND hwnd_;
  HINSTANCE hInstance_;
  bool created_;

  static std::map<HWND, MainWindow*> map_;

  static LRESULT CALLBACK internal_WndProc(HWND, UINT, WPARAM, LPARAM);

  static void HandleFatalError(LPCTSTR file, UINT line);
  
};

