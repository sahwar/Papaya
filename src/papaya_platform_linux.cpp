
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#ifdef USE_GTK
#include <gtk/gtk.h>
#endif

#include "papaya_platform.h"
#include "papaya_core.h"

#include "libs/glew/glew.h"
#include "libs/glew/glxew.h"
#include <X11/Xlib.h>
#include <GL/glx.h>
#include "libs/imgui/imgui.h"
#include "libs/easytab.h"

#define GLEW_NO_GLU

#define PAPAYA_DEFAULT_IMAGE "/home/apoorvaj/Pictures/Linux.png"


global_variable Display* XlibDisplay;
global_variable Window XlibWindow;
// =================================================================================================

void platform::print(char* Message)
{
    printf("%s", Message);
}

void platform::start_mouse_capture()
{
    //
}

void platform::stop_mouse_capture()
{
    //
}

void platform::set_mouse_position(int32 x, int32 y)
{
    XWarpPointer(XlibDisplay, None, XlibWindow, 0, 0, 0, 0, x, y);
    XFlush(XlibDisplay);
}

void platform::set_cursor_visibility(bool Visible)
{
    if (!Visible)
    {
        // Make a blank cursor
        char Empty[1] = {0};
        Pixmap Blank = XCreateBitmapFromData (XlibDisplay, XlibWindow, Empty, 1, 1);
        if(Blank == None) fprintf(stderr, "error: out of memory.\n");
        XColor Dummy;
        Cursor InvisCursor = XCreatePixmapCursor(XlibDisplay, Blank, Blank, &Dummy, &Dummy, 0, 0);
        XFreePixmap (XlibDisplay, Blank);
        XDefineCursor(XlibDisplay, XlibWindow, InvisCursor);
    }
    else
    {
        XUndefineCursor(XlibDisplay, XlibWindow); // TODO: Test what happens if cursor is attempted to be shown when it is not hidden in the first place.
    }
}

char* platform::open_file_dialog()
{
#ifdef USE_GTK
    GtkWidget *Dialog = gtk_file_chooser_dialog_new(
        "Open File",
        NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Open", GTK_RESPONSE_ACCEPT,
        NULL
    );
    GtkFileChooser *Chooser = GTK_FILE_CHOOSER(Dialog);
    GtkFileFilter *Filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(Filter, "*.[pP][nN][gG]");
    gtk_file_filter_add_pattern(Filter, "*.[jJ][pP][gG]");
    gtk_file_filter_add_pattern(Filter, "*.[jJ][pP][eE][gG]");
    gtk_file_chooser_add_filter(Chooser, Filter);

    char *OutFilename = 0;
    if (gtk_dialog_run(GTK_DIALOG(Dialog)) == GTK_RESPONSE_ACCEPT)
    {
        gchar *GTKFilename = gtk_file_chooser_get_filename(Chooser);
        int FilenameLen = strlen(GTKFilename) + 1; // +1 for terminator
        OutFilename = (char*)malloc(FilenameLen);
        strcpy(OutFilename, GTKFilename);
        g_free(GTKFilename);
    }

    gtk_widget_destroy(Dialog);

    return OutFilename;
#else
    return 0;
#endif
}

char* platform::save_file_dialog()
{
#if USE_GTK
    GtkWidget *Dialog = gtk_file_chooser_dialog_new(
        "Save File",
        NULL,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Save", GTK_RESPONSE_ACCEPT,
        NULL
    );
    GtkFileChooser *Chooser = GTK_FILE_CHOOSER(Dialog);
    GtkFileFilter *Filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(Filter, "*.[pP][nN][gG]");
    gtk_file_chooser_add_filter(Chooser, Filter);
    gtk_file_chooser_set_do_overwrite_confirmation(Chooser, TRUE);
    gtk_file_chooser_set_filename(Chooser, "untitled.png"); // what if the user saves a file that already has a name?

    char *OutFilename = 0;
    if (gtk_dialog_run(GTK_DIALOG(Dialog)) == GTK_RESPONSE_ACCEPT)
    {
        gchar *GTKFilename = gtk_file_chooser_get_filename(Chooser);
        int FilenameLen = strlen(GTKFilename) + 1; // +1 for terminator
        OutFilename = (char*)malloc(FilenameLen);
        strcpy(OutFilename, GTKFilename);
        g_free(GTKFilename);
    }

    gtk_widget_destroy(Dialog);

    return OutFilename;
#else
    return 0;
#endif
}

double platform::get_milliseconds()
{
    timespec Time;
    clock_gettime(CLOCK_MONOTONIC, &Time);
    return (double)(Time.tv_sec * 1000 + Time.tv_nsec / 1000000);
}

// =================================================================================================

int main(int argc, char **argv)
{
    PapayaMemory Mem = {0};

    timer::init(1000.0); // TODO: Check linux timer manual. Is this correct?
    timer::start(&Mem.profile.timers[Timer_Startup]);

    XVisualInfo* VisualInfo;
    Atom WmDeleteMessage;

    // Initialize GTK for Open/Save file dialogs
#ifdef USE_GTK
    gtk_init(&argc, &argv);
#endif

    // Set path to executable path
    {
        char PathBuffer[PATH_MAX];
        size_t PathLength = readlink("/proc/self/exe", PathBuffer, sizeof(PathBuffer)-1);
        if (PathLength != -1)
        {
            char *LastSlash = strrchr(PathBuffer, '/');
            if (LastSlash != NULL) { *LastSlash = '\0'; }
            chdir(PathBuffer);
        }
    }

    // Create window
    {
         XlibDisplay = XOpenDisplay(0);
         if (!XlibDisplay)
         {
            // TODO: Log: Error opening connection to the X server
            exit(1);
         }

        int32 Attributes[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
         VisualInfo = glXChooseVisual(XlibDisplay, 0, Attributes);
        if(VisualInfo == NULL)
        {
            // TODO: Log: No appropriate visual found
            exit(1);
        }

        XSetWindowAttributes SetWindowAttributes = {0};
        SetWindowAttributes.colormap = XCreateColormap(XlibDisplay, DefaultRootWindow(XlibDisplay), VisualInfo->visual, AllocNone);
        SetWindowAttributes.event_mask = ExposureMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;

        XlibWindow = XCreateWindow(XlibDisplay, DefaultRootWindow(XlibDisplay), 0, 0, 600, 600, 0, VisualInfo->depth, InputOutput, VisualInfo->visual, CWColormap | CWEventMask, &SetWindowAttributes);
        Mem.window.Width = 600;
        Mem.window.Height = 600;

        XMapWindow(XlibDisplay, XlibWindow);
        XStoreName(XlibDisplay, XlibWindow, "Papaya");

        WmDeleteMessage = XInternAtom(XlibDisplay, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(XlibDisplay, XlibWindow, &WmDeleteMessage, 1);

        // Window icon
        {
            /*
               Utility to print out icon data
               ==============================
               Linux apparently needs pixel data to be in a 64-bit-per-pixel buffer in ARGB format.
               stb_image gives a 32-bit-per-pixel ABGR output. This function is used to pre-calculate
               and print the swizzled and expanded buffer in the correct format to minimize startup time.

               TODO: The 48x48 icon is rendered blurry in the dock (at least on Elementary OS). Investigate why.
            */
            #if 0
            {
                int32 ImageWidth, ImageHeight, ComponentsPerPixel;
                uint8* Image = stbi_load("../../img/icon48.png", &ImageWidth, &ImageHeight, &ComponentsPerPixel, 4);
                printf("{%d,%d",ImageWidth, ImageHeight);
                for (int32 i = 0; i < ImageWidth * ImageHeight; i++)
                {
                    uint32 Pixel = ((uint32*)Image)[i];
                    uint32 ARGB = 	((Pixel & 0xff000000) >> 00) |
                                    ((Pixel & 0x000000ff) << 16) |
                                    ((Pixel & 0x0000ff00) >> 00) |
                                    ((Pixel & 0x00ff0000) >> 16);

                    printf(",%u", ARGB);
                }
                printf("};\n");
            }
            #endif

            uint64 buffer[] = {48,48,0,0,0,0,0,0,0,0,0,0,0,2960687,0,0,2960687,19737903,0,422391087,1445801263,2402102575,3140300079,3710725423,4063046959,4247596335,4247596335,4063046959,3710725423,3140300079,2402102575,1445801263,422391087,0,19737903,2960687,0,0,2960687,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2960687,0,0,2960687,36515119,372059439,1714236719,3375181103,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3375181103,1714236719,372059439,36515119,2960687,0,0,2960687,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2960687,0,0,36515119,556608815,2267884847,4046269743,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4046269743,2267884847,556608815,36515119,0,0,2960687,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2960687,2960687,0,2960687,321727791,1848454447,3928829231,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3928829231,1848454447,321727791,2960687,0,2960687,2960687,0,0,0,0,0,0,0,0,0,0,0,2960687,0,0,19737903,892153135,3089968431,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3089968431,892153135,19737903,0,0,2960687,0,0,0,0,0,0,0,0,0,2960687,0,0,86846767,1445801263,3794611503,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3794611503,1445801263,86846767,0,0,2960687,0,0,0,0,0,0,0,2960687,0,0,137178415,1764568367,4113378607,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4113378607,1764568367,137178415,0,0,2960687,0,0,0,0,0,0,2960687,0,86846767,1764568367,4214041903,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4214041903,1764568367,86846767,0,2960687,0,0,0,0,0,2960687,0,19737903,1445801263,4113378607,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4113378607,1445801263,19737903,0,2960687,0,0,0,2960687,0,2960687,892153135,3794611503,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4280756526,4280756526,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3794611503,892153135,2960687,0,2960687,0,0,0,0,321727791,3089968431,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4280822062,4281282096,4284436027,4284436027,4281282352,4280822062,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3089968431,321727791,0,0,0,2960687,0,36515119,1848454447,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4280887854,4280756526,4283516472,4287327302,4291401300,4291401300,4287327558,4283516472,4280756526,4280887854,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,1848454447,36515119,0,2960687,0,2960687,556608815,3928829231,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281019439,4280493613,4282530868,4286473283,4290284369,4293044058,4292846938,4292846938,4293044058,4290284369,4286473283,4282530868,4280493613,4281019439,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3928829231,556608815,2960687,0,0,36515119,2267884847,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281085231,4280624941,4281545009,4285487423,4289364557,4292518489,4292912730,4292649817,4292649817,4292649817,4292649817,4292912730,4292518489,4289364557,4285487423,4281545009,4280624941,4281085231,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,2267884847,36515119,0,2960687,372059439,4046269743,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4280822318,4280822318,4284304698,4288641611,4291795798,4292912730,4292715609,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292715609,4292912730,4291795798,4288641611,4284304698,4280822318,4280822318,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4046269743,372059439,2960687,19737903,1714236719,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281019182,4280493613,4282990646,4287721799,4291138387,4292781402,4292846938,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292846938,4292781402,4291138387,4287721799,4282990646,4280493613,4281019182,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,1714236719,19737903,0,3375181103,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4280953646,4282070834,4286538819,4290481489,4292452697,4292846938,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292846938,4292452697,4290481489,4286538818,4282070577,4280953646,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3375181103,0,422391087,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281150767,4286867524,4292452696,4292846938,4292715609,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292715353,4292781145,4292584798,4287065420,4281151023,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,422391087,1445801263,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281216560,4288050249,4293306715,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649560,4292650331,4292979814,4293834866,4288380246,4281216560,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,1445801263,2402102575,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281216560,4287984456,4293175643,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649816,4292649817,4292913763,4293177709,4293243760,4293835124,4288314711,4281216560,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,2402102575,3140300079,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281216560,4287984456,4293175643,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292781919,4293111659,4293243760,4293243761,4293243761,4293835124,4288314711,4281216560,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3140300079,3710725423,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281216560,4287984456,4293175643,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292715867,4292980073,4293243761,4293243761,4293243761,4293243761,4293243761,4293835124,4288314711,4281216560,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3710725423,4063046959,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281216560,4287984456,4293175643,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649818,4292913764,4293243504,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293835124,4288314711,4281216560,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4063046959,4247596335,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281216560,4287984456,4293175643,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292847712,4293177710,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293835124,4288314711,4281216560,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4247596335,4247596335,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281216560,4287984456,4293175643,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293835124,4288314711,4281216560,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4247596335,4063046959,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281216560,4287984456,4293175643,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293835124,4288314711,4281216560,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4063046959,3710725423,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281216560,4287984456,4293175643,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293835124,4288314711,4281216560,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3710725423,3140300079,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281216560,4287984456,4293175643,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293835124,4288314711,4281216560,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3140300079,2402102575,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281216560,4287984456,4293175643,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293835124,4288314711,4281216560,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,2402102575,1445801263,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281216560,4288050249,4293306715,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293900917,4288446039,4281216560,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,1445801263,422391087,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4281150767,4286933060,4292452696,4292846938,4292715609,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293309553,4293440882,4293046640,4287197264,4281150767,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,422391087,0,3375181103,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4280953646,4282136370,4286538819,4290481489,4292452697,4292846938,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4292649817,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293243761,4293441138,4293046640,4291009125,4286803022,4282136628,4280953646,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3375181103,0,19737903,1714236719,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281019182,4280493613,4282990646,4287721799,4291138387,4292781402,4292846938,4292649817,4292649817,4292649817,4292649817,4292649817,4293243761,4293243761,4293243761,4293243761,4293243761,4293440882,4293440882,4291666280,4288117333,4283056697,4280493611,4280953646,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,1714236719,19737903,2960687,372059439,4046269743,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4280822318,4280822318,4284304698,4288641611,4291795798,4292912730,4292715609,4292649817,4292649817,4292649817,4293243761,4293243761,4293243761,4293309553,4293572211,4292389228,4289037402,4284437057,4280822061,4280822061,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4046269743,372059439,2960687,0,36515119,2267884847,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281085231,4280624941,4281545009,4285487423,4289430093,4292518489,4292912730,4292649817,4292649817,4293243761,4293243761,4293572211,4293112176,4289826143,4285751368,4281545265,4280559404,4281085231,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,2267884847,36515119,0,0,2960687,556608815,3928829231,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281019439,4280493613,4282530868,4286473283,4290284369,4293044058,4292846937,4293440882,4293638003,4290812004,4286737230,4282596663,4280493611,4281019438,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3928829231,556608815,2960687,0,2960687,0,36515119,1848454447,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4280887854,4280756526,4283516472,4287327558,4291401300,4291929450,4287591762,4283648317,4280756525,4280887854,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,1848454447,36515119,0,2960687,0,0,0,321727791,3089968431,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4280822062,4281282352,4284436027,4284568386,4281282352,4280756525,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3089968431,321727791,0,0,0,0,2960687,0,2960687,892153135,3794611503,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281084975,4280756526,4280756269,4281084975,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3794611503,892153135,2960687,0,2960687,0,0,0,2960687,0,19737903,1445801263,4113378607,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4113378607,1445801263,19737903,0,2960687,0,0,0,0,0,2960687,0,86846767,1764568367,4214041903,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4214041903,1764568367,86846767,0,2960687,0,0,0,0,0,0,2960687,0,0,137178415,1764568367,4113378607,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4113378607,1764568367,137178415,0,0,2960687,0,0,0,0,0,0,0,2960687,0,0,86846767,1445801263,3794611503,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3794611503,1445801263,86846767,0,0,2960687,0,0,0,0,0,0,0,0,0,2960687,0,0,19737903,892153135,3089968431,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3089968431,892153135,19737903,0,0,2960687,0,0,0,0,0,0,0,0,0,0,0,2960687,2960687,0,2960687,321727791,1848454447,3928829231,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3928829231,1848454447,321727791,2960687,0,2960687,2960687,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2960687,0,0,36515119,556608815,2267884847,4046269743,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4046269743,2267884847,556608815,36515119,0,0,2960687,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2960687,0,0,2960687,36515119,372059439,1714236719,3375181103,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,4281150767,3375181103,1714236719,372059439,36515119,2960687,0,0,2960687,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2960687,0,0,2960687,19737903,0,422391087,1445801263,2402102575,3140300079,3710725423,4063046959,4247596335,4247596335,4063046959,3710725423,3140300079,2402102575,1445801263,422391087,0,19737903,2960687,0,0,2960687,0,0,0,0,0,0,0,0,0,0,0};

            //printf("%d\n", sizeof(buffer));
            //uint64 buffer[] = {2,2,4294901760,4278255360,4292649817,4294967295};
            Atom net_wm_icon = XInternAtom(XlibDisplay, "_NET_WM_ICON", False);
            Atom cardinal = XInternAtom(XlibDisplay, "CARDINAL", False);
            int32 length = sizeof(buffer)/8;
            XChangeProperty(XlibDisplay, XlibWindow, net_wm_icon, cardinal, 32, PropModeReplace, (uint8*) buffer, length);


        }
    }

    // Create OpenGL context
    {
        GLXContext GraphicsContext = glXCreateContext(XlibDisplay, VisualInfo, 0, GL_TRUE);
        glXMakeCurrent(XlibDisplay, XlibWindow, GraphicsContext);

        if (!GL::InitAndValidate()) { exit(1); }

        glGetIntegerv(GL_MAJOR_VERSION, &Mem.system.OpenGLVersion[0]);
        glGetIntegerv(GL_MINOR_VERSION, &Mem.system.OpenGLVersion[1]);

        // Disable vsync
        if (glxewIsSupported("GLX_EXT_swap_control"))
        {
            glXSwapIntervalEXT(XlibDisplay, XlibWindow, 0);
        }
    }

    EasyTab_Load(XlibDisplay, XlibWindow);

    core::init(&Mem);

    // Initialize ImGui
    {
        // TODO: Profiler timer setup

        ImGuiIO& io = ImGui::GetIO();
        io.RenderDrawListsFn = core::render_imgui;

        // Keyboard mappings
        {
            io.KeyMap[ImGuiKey_Tab]        = 0;
            io.KeyMap[ImGuiKey_LeftArrow]  = 1;
            io.KeyMap[ImGuiKey_RightArrow] = 2;
            io.KeyMap[ImGuiKey_UpArrow]    = 3;
            io.KeyMap[ImGuiKey_DownArrow]  = 4;
            io.KeyMap[ImGuiKey_PageUp]     = 5;
            io.KeyMap[ImGuiKey_PageDown]   = 6;
            io.KeyMap[ImGuiKey_Home]       = 7;
            io.KeyMap[ImGuiKey_End]        = 8;
            io.KeyMap[ImGuiKey_Delete]     = 9;
            io.KeyMap[ImGuiKey_Backspace]  = 10;
            io.KeyMap[ImGuiKey_Enter]      = 11;
            io.KeyMap[ImGuiKey_Escape]     = 12;
            io.KeyMap[ImGuiKey_A]          = XK_a;
            io.KeyMap[ImGuiKey_C]          = XK_c;
            io.KeyMap[ImGuiKey_V]          = XK_v;
            io.KeyMap[ImGuiKey_X]          = XK_x;
            io.KeyMap[ImGuiKey_Y]          = XK_y;
            io.KeyMap[ImGuiKey_Z]          = XK_z;
        }
    }

    timer::stop(&Mem.profile.timers[Timer_Startup]);

#ifdef PAPAYA_DEFAULT_IMAGE
    core::open_doc(PAPAYA_DEFAULT_IMAGE, &Mem);
#endif

    Mem.is_running = true;

    while (Mem.is_running)
    {
        timer::start(&Mem.profile.timers[Timer_Frame]);

        // Event handling
        while (XPending(XlibDisplay))
        {
            XEvent Event;
            XNextEvent(XlibDisplay, &Event);

            if (EasyTab_HandleEvent(&Event) == EASYTAB_OK) { continue; }

            switch (Event.type)
            {
                case Expose:
                {
                    XWindowAttributes WindowAttributes;
                    XGetWindowAttributes(XlibDisplay, XlibWindow, &WindowAttributes);
                    core::resize(&Mem, WindowAttributes.width,WindowAttributes.height);
                } break;

                case ClientMessage:
                {
                    if (Event.xclient.data.l[0] == WmDeleteMessage) { Mem.is_running = false; }
                } break;

                case MotionNotify:
                {
                    ImGui::GetIO().MousePos.x = Event.xmotion.x;
                    ImGui::GetIO().MousePos.y = Event.xmotion.y;
                } break;

                case ButtonPress:
                case ButtonRelease:
                {
                    int32 Button = Event.xbutton.button;
                    if 		(Button == 1) { Button = 0; } // This section maps Xlib's button indices (Left = 1, Middle = 2, Right = 3)
                    else if (Button == 2) { Button = 2; } // to Papaya's button indices              (Left = 0, Right = 1, Middle = 2)
                    else if (Button == 3) { Button = 1; } //

                    if 		(Button < 3)	{ ImGui::GetIO().MouseDown[Button] = (Event.type == ButtonPress); }
                    else
                    {
                        if (Event.type == ButtonPress) { ImGui::GetIO().MouseWheel += (Button == 4) ? +1.0f : -1.0f; }
                    }
                } break;

                case KeyPress:
                case KeyRelease:
                {
                    if (Event.type == KeyRelease && XEventsQueued(XlibDisplay, QueuedAfterReading))
                    {
                        XEvent NextEvent;
                        XPeekEvent(XlibDisplay, &NextEvent);
                        if (NextEvent.type == KeyPress &&
                            NextEvent.xkey.time == Event.xkey.time &&
                            NextEvent.xkey.keycode == Event.xkey.keycode)
                        {
                            break; // Key wasn’t actually released
                        }
                    }

                    // List of keysym's can be found in keysymdef.h or here
                    // [http://www.cl.cam.ac.uk/~mgk25/ucs/keysymdef.h]

                    ImGuiIO& io = ImGui::GetIO();
                    bool down   = (Event.type == KeyPress);

                    KeySym keysym;
                    XComposeStatus status;
                    char input; // TODO: Do we need an array here?
                    XLookupString(&Event.xkey, &input, 1, &keysym, &status);
                    if (down) { io.AddInputCharacter(input); }

                    switch (keysym)
                    {
                        case XK_Control_L:
                        case XK_Control_R: { io.KeyCtrl = down; } break;
                        case XK_Shift_L:
                        case XK_Shift_R:   { io.KeyShift = down; } break;
                        case XK_Alt_L:
                        case XK_Alt_R:     { io.KeyAlt = down; } break;

                        case XK_Tab:        { io.KeysDown[0]  = down; } break;
                        case XK_Left:       { io.KeysDown[1]  = down; } break;
                        case XK_Right:      { io.KeysDown[2]  = down; } break;
                        case XK_Up:         { io.KeysDown[3]  = down; } break;
                        case XK_Down:       { io.KeysDown[4]  = down; } break;
                        case XK_Page_Up:    { io.KeysDown[5]  = down; } break;
                        case XK_Page_Down:  { io.KeysDown[6]  = down; } break;
                        case XK_Home:       { io.KeysDown[7]  = down; } break;
                        case XK_End:        { io.KeysDown[8]  = down; } break;
                        case XK_Delete:     { io.KeysDown[9]  = down; } break;
                        case XK_BackSpace:  { io.KeysDown[10] = down; } break;
                        case XK_Return:     { io.KeysDown[11] = down; } break;
                        case XK_Escape:     { io.KeysDown[12] = down; } break;

                        case XK_a:     
                        case XK_c:     
                        case XK_v:     
                        case XK_x:     
                        case XK_y:     
                        case XK_z:     
                            io.KeysDown[keysym] = down;
                            break;
                    }
                } break;
            }
        }

        // Tablet input // TODO: Put this in papaya.cpp
        {
            Mem.tablet.Pressure = EasyTab->Pressure;
            Mem.tablet.PosX = EasyTab->PosX;
            Mem.tablet.PosY = EasyTab->PosY;
        }

        // Start new ImGui Frame
        {
            timespec Time;
            clock_gettime(CLOCK_MONOTONIC, &Time);
            float CurTime = Time.tv_sec + (float)Time.tv_nsec / 1000000000.0f;
            ImGui::GetIO().DeltaTime = (float)(CurTime - Mem.profile.last_frame_time);
            Mem.profile.last_frame_time = CurTime;

            ImGui::NewFrame();
        }

        // Update and render
        {
            core::update(&Mem);
            glXSwapBuffers(XlibDisplay, XlibWindow);
        }

#ifdef USE_GTK
        // Run a GTK+ loop, and *don't* block if there are no events pending
        gtk_main_iteration_do(FALSE);
#endif

        // End Of Frame
        timer::stop(&Mem.profile.timers[Timer_Frame]);
        double FrameRate = (Mem.current_tool == PapayaTool_Brush && Mem.mouse.IsDown[0]) ?
                           500.0 : 60.0;
        double FrameTime = 1000.0 / FrameRate;
        double SleepTime = FrameTime - Mem.profile.timers[Timer_Frame].elapsed_ms;
        Mem.profile.timers[Timer_Sleep].elapsed_ms = SleepTime;
        if (SleepTime > 0) { usleep((uint32)SleepTime * 1000); }
    }

    //core::destroy(&Mem);
    EasyTab_Unload();

    return 0;
}
